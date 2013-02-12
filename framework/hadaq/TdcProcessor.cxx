#include "hadaq/TdcProcessor.h"

#include <stdlib.h>
#include <string.h>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbProcessor.h"
#include "hadaq/TdcSubEvent.h"


unsigned hadaq::TdcProcessor::fMaxBrdId = 8;

unsigned hadaq::TdcProcessor::fFineMinValue = 31;
unsigned hadaq::TdcProcessor::fFineMaxValue = 480;


hadaq::TdcProcessor::TdcProcessor(TrbProcessor* trb, unsigned tdcid) :
   base::StreamProc("TDC", tdcid),
   fIter1(),
   fIter2()
{
   fMsgPerBrd = mgr()->MakeH1("MsgPerTDC", "Number of messages per TDC", fMaxBrdId, 0, fMaxBrdId, "tdc");
   fErrPerBrd = mgr()->MakeH1("ErrPerTDC", "Number of errors per TDC", fMaxBrdId, 0, fMaxBrdId, "tdc");
   fHitsPerBrd = mgr()->MakeH1("HitsPerTDC", "Number of data hits per TDC", fMaxBrdId, 0, fMaxBrdId, "tdc");

   fChannels = MakeH1("Channels", "TDC channels", NumTdcChannels, 0, NumTdcChannels, "ch");
   fMsgsKind = MakeH1("MsgKind", "kind of messages", 8, 0, 8, "xbin:Reserved,Header,Debug,Epoch,Hit,-,-,-;kind");

   fAllFine = MakeH1("FineTm", "fine counter value", 1024, 0, 1024, "fine");
   fAllCoarse = MakeH1("CoarseTm", "coarse counter value", 2048, 0, 2048, "coarse");

   if (trb) trb->AddSub(this, tdcid);

   fNewDataFlag = false;
}

hadaq::TdcProcessor::~TdcProcessor()
{
}




bool hadaq::TdcProcessor::DoBufferScan(const base::Buffer& buf, bool first_scan)
{
   // in the first scan we

   if (buf.null()) {
      if (first_scan) printf("Something wrong - empty buffer should not appear in the first scan/n");
      return false;
   }

   uint32_t syncid(0xffffffff);
   // copy first 4 bytes - it is syncid
   memcpy(&syncid, buf.ptr(), 4);

   // printf("TDC board %u SYNC %x\n", GetBoardId(), (unsigned) syncid);

   unsigned cnt(0), hitcnt(0);

   bool iserr(false), isfirstepoch(false);

   uint32_t first_epoch(0);

   TdcIterator& iter = first_scan ? fIter1 : fIter2;

   iter.assign((uint32_t*) buf.ptr(4), buf.datalen()/4 -1, false);

   unsigned help_index(0);

   double localtm(0.), minimtm(-1.), ch0time(-1);

//   printf("TDC%u scan\n", GetBoardId());

   while (iter.next()) {

  //    iter.msg().print();

      if ((cnt==0) && !iter.msg().isHeaderMsg()) iserr = true;

      cnt++;

      if (first_scan)
         FillH1(fMsgsKind, iter.msg().getKind() >> 29);

      if ((cnt==2) && iter.msg().isEpochMsg()) {
         isfirstepoch = true;
         first_epoch = iter.msg().getEpochValue();
      }

      if (iter.msg().isHitMsg()) {

         unsigned chid = iter.msg().getHitChannel();

         if (!iter.isCurEpoch()) {
            // printf("*** LOST EPOCH - ignore hit data ***\n");
            iserr = true;
         } else
         if (chid >= NumTdcChannels) {
            printf("TDC Channel number problem %u\n", chid);
            iserr = true;
         } else {

            // fill histograms only for normal channels
            if (first_scan && (chid>0)) {
               FillH1(fChannels, chid);

               FillH1(fAllFine, iter.msg().getHitTmFine());
               FillH1(fAllCoarse, iter.msg().getHitTmCoarse());

               if (!iserr) hitcnt++;
            }

            if (!iserr) {
               localtm = iter.getMsgTime() + SimpleFineCalibr(iter.msg().getHitTmFine());

               //printf("   msg time %12.9f   stamp %12.9f\n", localtm, iter.getMsgStamp()*5e-9);

               if ((minimtm<1.) || (localtm < minimtm))
                  minimtm = localtm;

               // remember position of channel 0 - it could be used for SYNC settings
               if ((chid==0) && iter.msg().isHitRisingEdge()) {
                  ch0time = localtm;
               }
            }


            // here we check if hit can be assigned to the events
            if (!first_scan && !iserr && (chid>0)) {
               base::GlobalTime_t globaltm = LocalToGlobalTime(localtm, &help_index);

               // use first channel only for flushing
               unsigned indx = TestHitTime(globaltm, true, false);

//               printf("Test hit time %12.9f trigger %u\n", globaltm*1e-9, trig_indx);

               if (indx < fGlobalMarks.size())
                  AddMessage(indx, (hadaq::TdcSubEvent*) fGlobalMarks.item(indx).subev, hadaq::TdcMessageExt(iter.msg(), globaltm));
            }

         }

         iter.clearCurEpoch();

         continue;
      }

      switch (iter.msg().getKind()) {
        case tdckind_Reserved:
           break;
        case tdckind_Header: {
           unsigned errbits = iter.msg().getHeaderErr();
           if (errbits && first_scan)
              printf("!!! found error bits: 0x%x at tdc 0x%x\n", errbits, GetBoardId());

           break;
        }
        case tdckind_Debug:
           break;
        case tdckind_Epoch:
           break;
        default:
           printf("Unknown bits 0x%x in header\n", iter.msg().getKind());
           break;
      }
   }

   if (isfirstepoch && !iserr)
      iter.setRefEpoch(first_epoch);

   if (first_scan) {

      if ((syncid != 0xffffffff) && (ch0time>=0)) {
         base::SyncMarker marker;
         marker.uniqueid = syncid;
         marker.localid = 0;
         marker.local_stamp = ch0time;
         marker.localtm = ch0time;
         AddSyncMarker(marker);
      }

      if (!iserr && (minimtm>=0.))
         buf().local_tm = minimtm;
      else
         iserr = true;

//      printf("Proc:%p first scan iserr:%d  minm: %12.9f\n", this, iserr, minimtm*1e-9);

      FillH1(fMsgPerBrd, GetBoardId(), cnt);

      // fill number of "good" hits
      FillH1(fHitsPerBrd, GetBoardId(), hitcnt);

      if (iserr)
         FillH1(fErrPerBrd, GetBoardId());
   } else {

      // use first channel only for flushing
      if (ch0time>=0)
         TestHitTime(LocalToGlobalTime(ch0time, &help_index), false, true);
   }

   return !iserr;
}

void hadaq::TdcProcessor::AppendTrbSync(uint32_t syncid)
{
   if (fQueue.size() == 0) {
      printf("something went wrong!!!\n");
      exit(765);
   }

   memcpy(fQueue.back().ptr(), &syncid, 4);
}
