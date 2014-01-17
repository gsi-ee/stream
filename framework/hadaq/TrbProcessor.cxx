#include "hadaq/TrbProcessor.h"

#include <string.h>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbIterator.h"
#include "hadaq/TdcProcessor.h"

#define RAWPRINT( args ...) if(IsPrintRawData()) printf( args )

hadaq::TrbProcessor::TrbProcessor(unsigned brdid) :
   base::StreamProc("TRB%u", brdid, false),
   fMap()
{
   mgr()->RegisterProc(this, base::proc_TRBEvent, brdid);

   printf("Create TrbProcessor %s\n", GetName());

   fMsgPerBrd = MakeH1("MsgPerTDC", "Number of messages per TDC", TdcProcessor::fMaxBrdId, 0, TdcProcessor::fMaxBrdId, "tdc");
   fErrPerBrd = MakeH1("ErrPerTDC", "Number of errors per TDC", TdcProcessor::fMaxBrdId, 0, TdcProcessor::fMaxBrdId, "tdc");
   fHitsPerBrd = MakeH1("HitsPerTDC", "Number of data hits per TDC", TdcProcessor::fMaxBrdId, 0, TdcProcessor::fMaxBrdId, "tdc");

   fEvSize = MakeH1("EvSize", "Event size", 500, 0, 5000, "bytes");
   fSubevSize = MakeH1("SubevSize", "Subevent size", 500, 0, 5000, "bytes");

   fLostRate = MakeH1("LostRate", "Relative number of lost packets", 1000, 0, 1., "data lost");

   fTdcDistr = MakeH1("TdcDistr", "Data distribution over TDCs", 64, 0, 64, "tdc");

   fHadaqCTSId = 0x8000;
   fHadaqHUBId = 0x9000;
   fHadaqTDCId = 0xC000;

   fLastTriggerId = 0;
   fLostTriggerCnt = 0;
   fTakenTriggerCnt = 0;

   fSyncTrigMask = 0;
   fSyncTrigValue = 0;

   fUseTriggerAsSync = false;
   fCompensateEpochReset = false;

   fPrintRawData = false;
   fCrossProcess = false;
   fPrintErrCnt = 1000;

   // this is raw-scan processor, therefore no synchronization is required for it
   SetSynchronisationKind(sync_None);

   // only raw scan, data can be immediately removed
   SetRawScanOnly();
}

hadaq::TrbProcessor::~TrbProcessor()
{
}

bool hadaq::TrbProcessor::CheckPrintError()
{
   if (fPrintErrCnt<=0) return false;

   if (--fPrintErrCnt==0) {
      printf("%5s Suppress all other errors ...\n", GetName());
      return false;
   }

   return true;
}

void hadaq::TrbProcessor::CreateTDC(unsigned id1, unsigned id2, unsigned id3, unsigned id4)
{
   // overwrite default value in the beginning
   if ((id1!=0) && (fHadaqTDCId == 0xC000)) fHadaqTDCId = (id1 & 0xff00);

   for (unsigned cnt=0;cnt<4;cnt++) {
      unsigned tdcid = id1;
      switch (cnt) {
         case 1: tdcid = id2; break;
         case 2: tdcid = id3; break;
         case 3: tdcid = id4; break;
         default: tdcid = id1; break;
      }
      if (tdcid==0) continue;

      if (fHadaqTDCId == 0) fHadaqTDCId = (tdcid & 0xff00);

      if ((tdcid & 0xff00) != fHadaqTDCId) {
         printf("TDC id 0x%04x do not match with expected mask 0x%04x\n", tdcid, fHadaqTDCId);
      } else {
         new hadaq::TdcProcessor(this, tdcid, 65, 0x1);
      }
   }
}

void hadaq::TrbProcessor::LoadCalibrations(const char* fileprefix)
{
   char fname[1024];

   for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {

      snprintf(fname, sizeof(fname), "%s%04x.cal", fileprefix, iter->second->GetID());

      iter->second->LoadCalibration(fname);
   }
}

void hadaq::TrbProcessor::AddSub(TdcProcessor* tdc, unsigned id)
{
   fMap[id] = tdc;
}


void hadaq::TrbProcessor::SetTriggerWindow(double left, double right)
{
   base::StreamProc::SetTriggerWindow(left, right);

   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      iter->second->CreateTriggerHist(16, 3000, -2e-6, 2e-6);
      iter->second->SetTriggerWindow(left, right);
   }
}


void hadaq::TrbProcessor::AccountTriggerId(unsigned subid)
{
   if (fLastTriggerId==0) {
      fLostTriggerCnt = 0;
      fTakenTriggerCnt = 0;
   } else {
      unsigned diff = (subid - fLastTriggerId) & 0xffff;
      if ((diff > 0x8000) || (diff==0)) {
         // ignore this, most probable it is some mistake
      } else
      if (diff==1) {
         fTakenTriggerCnt++;
      } else {
         fLostTriggerCnt+=(diff-1);
      }
   }

   fLastTriggerId = subid;

   if (fTakenTriggerCnt + fLostTriggerCnt > 1000) {
      double lostratio = 1.*fLostTriggerCnt / (fTakenTriggerCnt + fLostTriggerCnt + 0.);
      FillH1(fLostRate, lostratio);
      fTakenTriggerCnt = 0;
      fLostTriggerCnt = 0;
   }
}


bool hadaq::TrbProcessor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

//   RAWPRINT("TRB3 - first scan of buffer %u\n", buf().datalen);

//   printf("Scan TRB buffer size %u\n", buf().datalen);

   hadaq::TrbIterator iter(buf().buf, buf().datalen);

   hadaq::RawEvent* ev = 0;

   while ((ev = iter.nextEvent()) != 0) {

      if (IsPrintRawData()) ev->Dump();

      FillH1(fEvSize, ev->GetSize());

      hadaq::RawSubevent* sub = 0;
      unsigned subcnt(0);

      while ((sub = iter.nextSubevent()) != 0) {

         if (IsPrintRawData()) sub->Dump(true);

         FillH1(fSubevSize, sub->GetSize());

         // use only 16-bit in trigger number while CTS make a lot of errors in higher 8 bits
         AccountTriggerId((sub->GetTrigNr() >> 8) & 0xffff);

         ScanSubEvent(sub, ev->GetSeqNr());

         subcnt++;
      }

      // we scan in all TDC processors newly add buffers
      // this is required to
      if (IsCrossProcess()) {
         for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
            iter->second->ScanNewBuffers();

         for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
            iter->second->AfterFill(&fMap);
      }

   }

   return true;
}

void hadaq::TrbProcessor::ScanSubEvent(hadaq::RawSubevent* sub, unsigned trb3eventid)
{
   // this is first scan of subevent from TRB3 data
   // our task is statistic over all messages we will found
   // also for trigger-type 1 we should add SYNC message to each processor


   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetNewDataFlag(false);

   unsigned ix = 0;           // cursor

   unsigned trbSubEvSize = sub->GetSize() / 4 - 4;

   unsigned syncNumber(0);
   bool findSync(false);

//   RAWPRINT("Scan TRB3 raw event 4-bytes size %u\n", trbSubEvSize);
//   printf("Scan TRB3 raw data\n");

   while (ix < trbSubEvSize) {
      //! Extract data portion from the whole packet (in a loop)
      uint32_t data = sub->Data(ix++);

      unsigned datalen = (data >> 16) & 0xFFFF;

//      RAWPRINT("Subevent id 0x%04x len %u\n", (data & 0xFFFF), datalen);

      // ===========  this is header for TDC, build inside the TRB3 =================
//      if ((data & 0xFF00) == 0xB000) {
//         // do that ever you want
//         unsigned brdid = data & 0xFF;
//         RAWPRINT("    TRB3-TDC header: 0x%08x, TRB3-buildin TDC-FPGA brd=%u, size=%u  IGNORED\n", (unsigned) data, brdid, datalen);
//         ix+=datalen;
//         continue;
//      }

      if ((data & 0xFF00) == fHadaqHUBId) {
         RAWPRINT ("   HUB header: 0x%08x, hub=%u, size=%u (ignore)\n", (unsigned) data, (unsigned) data & 0xFF, datalen);

         // ix+=datalen;  // WORKAROUND !!!

         // TODO: formally we should analyze HUB subevent as real subevent but
         // we just skip header and continue to analyze data
         continue;
      }


      //! ================= FPGA TDC header ========================
      if ((data & 0xFF00) == fHadaqTDCId) {
         unsigned tdcid = data & 0xFFFF;

         FillH1(fTdcDistr, tdcid & 0xff);

         RAWPRINT ("   FPGA-TDC header: 0x%08x, tdcid=0x%04x, size=%u\n", (unsigned) data, tdcid, datalen);

         if (IsPrintRawData()) {
            TdcIterator iter;
            iter.assign(sub, ix, datalen);
            while (iter.next()) iter.printmsg();
         }

         TdcProcessor* subproc = GetTDC(tdcid);

         if (subproc != 0) {
            base::Buffer buf;
            buf.makenew((datalen+1)*4);

            memset(buf.ptr(), 0xff, 4); // fill dummy sync id in the begin
            sub->CopyDataTo(buf.ptr(4), ix, datalen);

            buf().kind = 0;
            buf().boardid = tdcid;
            buf().format = 0;

            subproc->AddNextBuffer(buf);
            subproc->SetNewDataFlag(true);
         } else {
            RAWPRINT("Did not find processor for TDC 0x%04x - skip data %u\n", tdcid, datalen);
         }

         ix+=datalen;

         continue; // go to next block
      }  // end of if TDC header

      //! ==================== CTS header and inside ================
      if ((data & 0xFFFF) == fHadaqCTSId) {
         RAWPRINT("   CTS header: 0x%x, size=%d\n", (unsigned) data, datalen);
         //hTrbTriggerCount->Fill(5);          //! TRB - CTS
         //hTrbTriggerCount->Fill(0);          //! TRB TOTAL

         data = sub->Data(ix++); datalen--;
         unsigned trigtype = (data & 0xFFFF);
         unsigned trignum = (data >> 16) & 0xFFFF;
         RAWPRINT("     CTS trigtype: 0x%04x, trignum=0x%04x\n", trigtype, trignum);

         while (datalen-- > 0) {
            //! Look only for the CTS data
            data = sub->Data(ix++);
            RAWPRINT("     CTS word: 0x%08x\n", (unsigned) data);

            if ((datalen==0) && (fSyncTrigMask!=0) && ((trigtype & fSyncTrigMask) == fSyncTrigValue)) {
               //! Last CTS word - SYNC number
               syncNumber = (data & 0xFFFFFF);
               findSync = true;
               RAWPRINT("     Find SYNC %u\n", (unsigned) syncNumber);
            }
         }

         continue;
      }

      //! ==================  Dummy header and inside ==========================
      if ((data & 0xFFFF) == 0x5555) {
         RAWPRINT("   Dummy header: 0x%x, size=%d\n", (unsigned) data, datalen);
         //hTrbTriggerCount->Fill(4);          //! TRB - DUMMY
         //hTrbTriggerCount->Fill(0);          //! TRB TOTAL
         while (datalen-- > 0) {
            //! In theory here must be only one word - termination package with the status
            data = sub->Data(ix++);
            RAWPRINT("      word: 0x%08x\n", (unsigned) data);
            //uint32_t fSubeventStatus = data;
            //if (fSubeventStatus != 0x00000001) { bad events}
         }

         continue;
      }


//      if ((data & 0xFFFF) == 0x8001) {
//         // no idea that this, but appeared in beam-time data with different sizes
//         ix+=datalen;
//         continue;
//      }
//
//      if ((data & 0xFFFF) == 0x8002) {
//         // no idea that this, but appeared in beam-time data with size 0
//         if (datalen!=0) printf("Strange length %u, expected 0 with 8002 subtype\n", datalen);
//         ix+=datalen;
//         continue;
//      }
//
      RAWPRINT("Unknown header %x length %u in TDC subevent\n", data & 0xFFFF, datalen);
      ix+=datalen;
   }

   if (fUseTriggerAsSync) {
      findSync = true;
      syncNumber = trb3eventid;
   }


   if (findSync) {

//      printf("Append new SYNC %u\n", syncNumber);

      for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
         if (iter->second->IsNewDataFlag())
            iter->second->AppendTrbSync(syncNumber);
      }
   }
}


