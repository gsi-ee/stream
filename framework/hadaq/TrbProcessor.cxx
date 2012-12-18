#include "hadaq/TrbProcessor.h"

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbIterator.h"
#include "hadaq/TdcProcessor.h"

hadaq::TrbProcessor::TrbProcessor(unsigned brdid) :
   base::StreamProc("TRB", brdid),
   fBrdId(brdid),
   fMap()
{
   mgr()->RegisterProc(this, base::proc_TRBEvent, brdid);

   fEvSize = MakeH1("EvSize", "Event size", 500, 0, 5000, "bytes");
   fSubevSize = MakeH1("SubevSize", "Subevent size", 500, 0, 5000, "bytes");

   fLostRate = MakeH1("LostRate", "Relative number of lost packets", 1000, 0, 1., "data lost");

   fLastTriggerId = 0;
   fLostTriggerCnt = 0;
   fTakenTriggerCnt = 0;
}

hadaq::TrbProcessor::~TrbProcessor()
{
}

void hadaq::TrbProcessor::AddSub(TdcProcessor* tdc, unsigned id)
{
   fMap[id] = tdc;
}


void hadaq::TrbProcessor::SetTriggerWindow(double left, double right)
{
   base::StreamProc::SetTriggerWindow(left, right);

   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetTriggerWindow(left, right);
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

//   buf().format;
//   buf().boardid;

//   buf().buf

//   buf().datalen


//   printf("*********** Start TRB processing len %u ************ \n", buf().datalen);

   hadaq::TrbIterator iter(buf().buf, buf().datalen);


   hadaq::RawEvent* ev = 0;

   while ((ev = iter.nextEvent()) != 0) {

//      printf("************ Find event len %u *********** \n", ev->GetSize());

      FillH1(fEvSize, ev->GetSize());

      hadaq::RawSubevent* sub = 0;

      while ((sub = iter.nextSubevent()) != 0) {

//         printf("************ Find subevent len %u ********** \n", sub->GetSize());

         FillH1(fSubevSize, sub->GetSize());

         // use only 16-bit in trigger number while CTS make a lot of errors in higher 8 bits
         AccountTriggerId((sub->GetTrigNr() >> 8) & 0xffff);

         ProcessTDCV3(sub);

      }

//      printf("Finish event processing\n");

   }

   return true;
}

void hadaq::TrbProcessor::ProcessTDCV3(hadaq::RawSubevent* sub)
{
   // this is first scan of subevent from TRB3 data
   // our task is statistic over all messages we will found
   // also for trigger-type 1 we should add SYNC message to each processor


   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->BeforeFirstScan();

   unsigned ix = 0;           // cursor

   unsigned trbSubEvSize = sub->GetSize() / 4 - 4;

   unsigned syncNumber(0);
   bool findSync(false);

   while (ix < trbSubEvSize) {
      //! Extract data portion from the whole packet (in a loop)
      uint32_t data = sub->Data(ix++);

      unsigned datalen = (data >> 16) & 0xFFFF;

      // ===========  this is header for TDC, build inside the TRB3 =================
      if ((data & 0xFF00) == 0xB000) {
         // do that ever you want
         // unsigned brdid = data & 0xFF;

         // EPRINT ("***  --- tdc header: 0x%08x, TRB3-buildin TDC-FPGA brd=%d, size=%d  IGNORED\n", data, curBrd, tdcLen);

         ix+=datalen;
         continue;
      }

      //! ================= RICH-FEE TDC header ========================
      if ((data & 0xFF00) == 0xC000) {

         unsigned tdcid = data & 0xFF;

         TdcProcessor* subproc = fMap[tdcid];

         if (subproc)
            subproc->FirstSubScan(sub, ix, datalen);
         else
            printf("Did not find processor for TDC %u - skip data %u\n", tdcid, datalen);

         ix+=datalen;

         continue; // go to next block
      }  // end of if TDC header


      //! ==================== CTS header and inside ================
      if ((data & 0xFFFF) == 0x8000) {
         //EPRINT ("***  --- CTS header: 0x%x, size=%d\n", data, centHubLen);
         //hTrbTriggerCount->Fill(5);          //! TRB - CTS
         //hTrbTriggerCount->Fill(0);          //! TRB TOTAL


         data = sub->Data(ix++); datalen--;
         unsigned trigtype = (data & 0xFFFF);
         unsigned trignum = (data >> 16) & 0xF;
         // printf("***  --- CTS trigtype: 0x%x %x, trignum=0x%x\n", trigtype, trigtype & 0x3, trignum);
         //fCurrentTriggerType = trigtype;

         // printf("trigtype = %x trignum\n", trigtype & 0x3);

         while (datalen-- > 0) {
            //! Look only for the CTS data
            data = sub->Data(ix++);
            // printf("***  --- CTS word: 0x%x, skipping\n", data);
            //hTrbTriggerCount->Fill(5);       //! TRB - CTS
            //hTrbTriggerCount->Fill(0);       //! TRB TOTAL
         }

         //! Last CTS word - SYNC number
         syncNumber = (data & 0xFFFFFF);

         if ((trigtype & 0x3) == 1) {
            findSync = true;

            // printf("Find SYNC %u\n", syncNum);
         }

         // printf("***  --- CTS word - SYNC number: 0x%x, num=%d\n", data, syncNum);
         //hTrbTriggerCount->Fill(5);       //! TRB - CTS
         //hTrbTriggerCount->Fill(0);       //! TRB TOTAL
         continue;
      }

      //! ==================  Dummy header and inside ==========================
      if ((data & 0xFFFF) == 0x5555) {
         //EPRINT ("***  --- dummy header: 0x%x, size=%d\n", data, centHubLen);
         //hTrbTriggerCount->Fill(4);          //! TRB - DUMMY
         //hTrbTriggerCount->Fill(0);          //! TRB TOTAL
         while (datalen-- > 0) {
            //! In theory here must be only one word - termination package with the status
            data = sub->Data(ix++);
            //EPRINT ("***  --- status word: 0x%x\n", data);
            //hTrbTriggerCount->Fill(4);       //! TRB - DUMMY
            //hTrbTriggerCount->Fill(0);       //! TRB TOTAL
            uint32_t fSubeventStatus = data;
            if (fSubeventStatus != 0x00000001) {
               // hTrbTriggerCount->Fill(1);    //! TRB - INVALID
            }
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
//      printf("Unknown header %x length %u in TDC subevent\n", data & 0xFFFF, datalen);
//      ix+=datalen;
   }

   if (findSync)
      for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {

         // for each TDC own SYNC marker will be created to
         // synchronize local time with external markers
         double localtm = iter->second->AddSyncIfFound(syncNumber);

         // use time of first TDC in list for synchronization
         // it is required to find regions where event selection should be done
         if ((iter == fMap.begin()) && (localtm!=0)) {
            base::SyncMarker marker;
            marker.uniqueid = syncNumber;
            marker.localid = 0;
            marker.local_stamp = localtm;
            marker.localtm = localtm;
            AddSyncMarker(marker);
         }
      }
}


bool hadaq::TrbProcessor::SecondBufferScan(const base::Buffer& buf)
{
   return true;
}
