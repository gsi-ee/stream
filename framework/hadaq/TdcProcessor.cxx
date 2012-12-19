#include "hadaq/TdcProcessor.h"

#include <stdlib.h>
#include <string.h>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbProcessor.h"

unsigned hadaq::TdcProcessor::fMaxBrdId = 8;

unsigned hadaq::TdcProcessor::fFineMinValue = 31;
unsigned hadaq::TdcProcessor::fFineMaxValue = 480;


hadaq::TdcProcessor::TdcProcessor(TrbProcessor* trb, unsigned tdcid) :
   base::StreamProc("TDC", tdcid),
   fTdcId(tdcid)
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

   uint32_t syncid(0xffffffff);
   // copy first 4 bytes - it is syncid
   memcpy(&syncid, buf.ptr(), 4);

   // printf("TDC board %u SYNC %x\n", GetBoardId(), (unsigned) syncid);

   unsigned cnt(0), hitcnt(0);

   bool iserr = false;

   TdcIterator iter((uint32_t*) buf.ptr(4), buf.datalen()/4 -1, false);

   unsigned help_index(0);

   while (iter.next()) {

      // iter.msg().print();

      if ((cnt==0) && !iter.msg().isHeaderMsg()) iserr = true;

      cnt++;

      if (first_scan)
         FillH1(fMsgsKind, iter.msg().getKind() >> 29);

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

            // remember position of channel 0 - it could be used for SYNC settings
            if ((chid==0) && iter.msg().isHitRisingEdge() && first_scan && (syncid != 0xffffffff) && !iserr) {

               double synctm = iter.getMsgStampCoarse() * CoarseUnit() + SimpleFineCalibr(iter.msg().getHitTmFine());

               base::SyncMarker marker;
               marker.uniqueid = syncid;
               marker.localid = 0;
               marker.local_stamp = synctm;
               marker.localtm = synctm;

               AddSyncMarker(marker);
            }

            if (!first_scan && !iserr) {
               double stamp = iter.getMsgStampCoarse() * CoarseUnit() + SimpleFineCalibr(iter.msg().getHitTmFine());

               base::GlobalTime_t globaltm = LocalToGlobalTime(stamp, &help_index);

               // use first channel only for flushing
               unsigned trig_indx = TestHitTime(globaltm, (chid>0));

/*               if (trig_indx < fGlobalTrig.size()) {
                  hadaq::TdcSubEvent* ev = (hadaq::TdcSubEvent*) fGlobalTrig[trig_indx].subev;

                  if (ev==0) {
                     ev = new nx::SubEvent;
                     fGlobalTrig[trig_indx].subev = ev;
                  }

                  ev->fExtMessages.push_back(hadaq::TdcMessageExt(iter.msg(), globaltm));
               }
*/


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

   if (first_scan) {

      FillH1(fMsgPerBrd, GetBoardId(), cnt);

      // fill number of "good" hits
      FillH1(fHitsPerBrd, GetBoardId(), hitcnt);

      if (iserr)
         FillH1(fErrPerBrd, GetBoardId());
   }
}

void hadaq::TdcProcessor::AppendTrbSync(uint32_t syncid)
{
   if (fQueue.size() == 0) {
      printf("something went wrong!!!\n");
      exit(765);
   }

   memcpy(fQueue.back().ptr(), &syncid, 4);
}
