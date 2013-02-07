#include "base/OpticSplitter.h"

#include "base/ProcMgr.h"


base::OpticSplitter::OpticSplitter(unsigned brdid) :
   base::StreamProc("Splitter"),
   fMap()
{
   mgr()->RegisterProc(this, base::proc_RawData, brdid);
}

base::OpticSplitter::~OpticSplitter()
{
}

void base::OpticSplitter::AddSub(SysCoreProc* procs, unsigned id)
{

}

bool base::OpticSplitter::FirstBufferScan(const base::Buffer& buf)
{
   return true;
}

