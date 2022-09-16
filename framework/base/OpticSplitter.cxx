#include "base/OpticSplitter.h"

#include <cstring>

#include "base/ProcMgr.h"

///////////////////////////////////////////////////////////////////////////
/// constructor

base::OpticSplitter::OpticSplitter(unsigned brdid) :
   base::StreamProc("Splitter", DummyBrdId, false),
   fMap()
{
   mgr()->RegisterProc(this, base::proc_RawData, brdid);

   // this is raw-scan processor, therefore no synchronization is required for it
   SetSynchronisationKind(sync_None);

   // only raw scan, data can be immediately removed
   SetRawScanOnly();
}

///////////////////////////////////////////////////////////////////////////
/// destructor

base::OpticSplitter::~OpticSplitter()
{
}

///////////////////////////////////////////////////////////////////////////
/// Way to register sub-processor

void base::OpticSplitter::AddSub(SysCoreProc* proc, unsigned id)
{
   fMap[id] = proc;
}

///////////////////////////////////////////////////////////////////////////
/// Scan all messages, find reference signals

bool base::OpticSplitter::FirstBufferScan(const base::Buffer& buf)
{
   // TODO: one can treat special case when buffer data only from single board
   // in this case one could deliver buffer as is to the SysCoreProc

   uint64_t *ptr = (uint64_t*) buf.ptr();

   unsigned len = buf.datalen();

   while(len > 0) {
      unsigned brdid = (*ptr & 0xffff);

      auto iter = fMap.find(brdid);

      if (iter != fMap.end()) {
         SysCoreProc* proc = iter->second;

         if (!proc->fSplitPtr) {
            proc->fSplitBuf.makenew(buf.datalen());
            proc->fSplitPtr = (uint64_t*) proc->fSplitBuf.ptr();
         }

         memcpy(proc->fSplitPtr, ptr, 8);
         proc->fSplitPtr++;
      }

      len-=8;
      ptr++;
   }

   for (auto &entry : fMap) {
      SysCoreProc* proc = entry.second;
      if (proc->fSplitPtr) {
         proc->fSplitBuf.setdatalen((proc->fSplitPtr - (uint64_t*) proc->fSplitBuf.ptr())*8);

         proc->fSplitBuf.rec().kind = buf.rec().kind;
         proc->fSplitBuf.rec().format = buf.rec().format;
         proc->fSplitBuf.rec().boardid = entry.first;

         proc->AddNextBuffer(proc->fSplitBuf);

         proc->fSplitPtr = nullptr;
         proc->fSplitBuf.reset();
      }
   }

   return true;
}
