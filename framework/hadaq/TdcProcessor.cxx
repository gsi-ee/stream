#include "hadaq/TdcProcessor.h"

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbProcessor.h"

#include "hadaq/TdcIterator.h"

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
}

hadaq::TdcProcessor::~TdcProcessor()
{
}

void hadaq::TdcProcessor::FirstSubScan(hadaq::RawSubevent* sub, unsigned indx, unsigned datalen)
{
   hadaq:TdcIterator iter;
   iter.assign(sub, indx, datalen);

   // printf("========= Data for TDC %u ====== \n", GetBoardId());

   unsigned cnt(0), hitcnt(0);

   bool iserr = false;

   while (iter.next()) {

      // iter.msg().print();

      if ((cnt==0) && !iter.msg().isHeaderMsg()) iserr = true;

      cnt++;

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
            if (chid>0) {
               FillH1(fChannels, chid);

               FillH1(fAllFine, iter.msg().getHitTmFine());
               FillH1(fAllCoarse, iter.msg().getHitTmCoarse());

               if (!iserr) hitcnt++;
            }

            // remember position of channel 0 - it could be used for SYNC settings
            if ((chid==0) && iter.msg().isHitRisingEdge())
               fCh0Time = iter.getMsgStampCoarse() * CoarseUnit() + SimpleFineCalibr(iter.msg().getHitTmFine());
         }

         iter.clearCurEpoch();

         continue;
      }

      switch (iter.msg().getKind()) {
        case tdckind_Reserved:
           break;
        case tdckind_Header: {
           unsigned errbits = iter.msg().getHeaderErr();
           if (errbits)
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

   FillH1(fMsgPerBrd, GetBoardId(), cnt);

   // fill number of "good" hits
   FillH1(fHitsPerBrd, GetBoardId(), hitcnt);

   if (iserr)
      FillH1(fErrPerBrd, GetBoardId());
}

double hadaq::TdcProcessor::AddSyncIfFound(unsigned syncid)
{
   if (fCh0Time==0.) return 0.;

   base::SyncMarker marker;
   marker.uniqueid = syncid;
   marker.localid = 0;
   marker.local_stamp = fCh0Time;
   marker.localtm = fCh0Time;

   AddSyncMarker(marker);
   return fCh0Time;

}
