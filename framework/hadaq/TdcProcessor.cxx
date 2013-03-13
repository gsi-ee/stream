#include "hadaq/TdcProcessor.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbProcessor.h"
#include "hadaq/TdcSubEvent.h"


unsigned hadaq::TdcProcessor::fMaxBrdId = 8;


hadaq::TdcProcessor::TdcProcessor(TrbProcessor* trb, unsigned tdcid, unsigned numchannels, bool all_histos) :
   base::StreamProc("TDC", tdcid),
   fIter1(),
   fIter2(),
   fAllHistos(all_histos)
{
   fMsgPerBrd = mgr()->MakeH1("MsgPerTDC", "Number of messages per TDC", fMaxBrdId, 0, fMaxBrdId, "tdc");
   fErrPerBrd = mgr()->MakeH1("ErrPerTDC", "Number of errors per TDC", fMaxBrdId, 0, fMaxBrdId, "tdc");
   fHitsPerBrd = mgr()->MakeH1("HitsPerTDC", "Number of data hits per TDC", fMaxBrdId, 0, fMaxBrdId, "tdc");

   fChannels = MakeH1("Channels", "TDC channels", NumTdcChannels, 0, NumTdcChannels, "ch");
   fMsgsKind = MakeH1("MsgKind", "kind of messages", 8, 0, 8, "xbin:Reserved,Header,Debug,Epoch,Hit,-,-,-;kind");

   fAllFine = MakeH1("FineTm", "fine counter value", 1024, 0, 1024, "fine");
   fAllCoarse = MakeH1("CoarseTm", "coarse counter value", 2048, 0, 2048, "coarse");

   if (all_histos)
      for (unsigned ch=0;ch<NumTdcChannels;ch++) {
         fCh[ch].disabled = (ch>=numchannels);
         if (fCh[ch].disabled) continue;

         SetSubPrefix("Ch", ch);

         fCh[ch].fRisingFine = MakeH1("RisingFine", "Rising fine counter", FineCounterBins, 0, FineCounterBins, "fine");
         fCh[ch].fRisingCoarse = MakeH1("RisingCoarse", "Rising coarse counter", 2048, 0, 2048, "coarse");
         fCh[ch].fRisingRef = MakeH1("RisingRef", "Difference to reference channel", 4000, -1000., +1000., "ns");
         fCh[ch].fRisingCalibr = MakeH1("RisingCalibr", "Rising calibration function", FineCounterBins, 0, FineCounterBins, "fine");

         fCh[ch].fFallingFine = MakeH1("FallingFine", "Falling fine counter", FineCounterBins, 0, FineCounterBins, "fine");
         fCh[ch].fFallingCoarse = MakeH1("FallingCoarse", "Falling coarse counter", 2048, 0, 2048, "coarse");
         fCh[ch].fFallingRef = MakeH1("FallingRef", "Difference to reference channel", 4000, -1000., +1000., "ns");
         fCh[ch].fFallingCalibr = MakeH1("FallingCalibr", "Falling calibration function", FineCounterBins, 0, FineCounterBins, "fine");
      }



   if (trb) trb->AddSub(this, tdcid);

   fNewDataFlag = false;

   fWriteCalibr.clear();
}

hadaq::TdcProcessor::~TdcProcessor()
{
}

void hadaq::TdcProcessor::DisableCalibrationFor(int firstch, int lastch)
{
   if (lastch<0) lastch = firstch;
   if (lastch>=NumTdcChannels-1) lastch = NumTdcChannels-1;
   for (int n=firstch;n<=lastch;n++)
      fCh[n].docalibr = false;
}

void hadaq::TdcProcessor::DisableChannels(int firstch, int lastch)
{
   if (lastch<0) lastch = firstch;
   if (lastch>=NumTdcChannels-1) lastch = NumTdcChannels-1;
   for (int n=firstch;n<=lastch;n++)
      fCh[n].disabled = true;

}



void hadaq::TdcProcessor::UserPreLoop()
{
//   printf("************************* hadaq::TdcProcessor preloop *******************\n");
   for (int ch=0;ch<NumTdcChannels;ch++) {
      if (fCh[ch].disabled) continue;

      CopyCalibration(fCh[ch].rising_calibr, fCh[ch].fRisingCalibr);

      CopyCalibration(fCh[ch].falling_calibr, fCh[ch].fFallingCalibr);
   }
}

void hadaq::TdcProcessor::UserPostLoop()
{
//   printf("************************* hadaq::TdcProcessor postloop *******************\n");

   if (!fWriteCalibr.empty()) {
      ProduceCalibration();
      StoreCalibration(fWriteCalibr);
   }
}


void hadaq::TdcProcessor::SetRefChannel(unsigned ch, unsigned refch)
{
   if ((ch<NumTdcChannels) && (refch<NumTdcChannels))
      fCh[ch].refch = refch;
}


void hadaq::TdcProcessor::BeforeFill()
{
   for (unsigned ch=0;ch<NumTdcChannels;ch++) {
      fCh[ch].last_rising_tm = 0;
      fCh[ch].last_falling_tm = 0;
   }
}

void hadaq::TdcProcessor::FillHistograms()
{
   hadaq::TdcMessage& msg = fIter1.msg();

   unsigned chid = msg.getHitChannel();
   ChannelRec& rec = fCh[chid];

   double tm = fIter1.getMsgTimeCoarse();

   if (msg.isHitRisingEdge()) {
      tm += rec.rising_calibr[msg.getHitTmFine()];

      FillH1(rec.fRisingFine, msg.getHitTmFine());
      FillH1(rec.fRisingCoarse, msg.getHitTmCoarse());
      if (rec.last_rising_tm == 0.) rec.last_rising_tm = tm;
   } else {
      tm += rec.falling_calibr[msg.getHitTmFine()];

      FillH1(rec.fFallingFine, msg.getHitTmFine());
      FillH1(rec.fFallingCoarse, msg.getHitTmCoarse());
      if (rec.last_falling_tm == 0.) rec.last_falling_tm = tm;
   }

}

void hadaq::TdcProcessor::AfterFill()
{
   for (unsigned ch=0;ch<NumTdcChannels;ch++) {
      unsigned ref = fCh[ch].refch;
      if ((ref>=NumTdcChannels) || (ref==ch)) continue;

      if ((fCh[ch].last_rising_tm !=0 ) && (fCh[ref].last_rising_tm !=0))
         FillH1(fCh[ch].fRisingRef, (fCh[ch].last_rising_tm - fCh[ref].last_rising_tm)*1e9);

      if ((fCh[ch].last_falling_tm !=0 ) && (fCh[ref].last_falling_tm !=0))
         FillH1(fCh[ch].fFallingRef, (fCh[ch].last_falling_tm - fCh[ref].last_falling_tm)*1e9);
   }
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

   if (fAllHistos && first_scan) BeforeFill();

   hadaq::TdcMessage& msg = iter.msg();

   while (iter.next()) {

//      msg.print();

      if ((cnt==0) && !msg.isHeaderMsg()) iserr = true;

      cnt++;

      if (first_scan)
         FillH1(fMsgsKind, msg.getKind() >> 29);

      if ((cnt==2) && msg.isEpochMsg()) {
         isfirstepoch = true;
         first_epoch = msg.getEpochValue();
      }

      if (msg.isHitMsg()) {

         unsigned chid = msg.getHitChannel();

         if (!iter.isCurEpoch()) {
            // printf("*** LOST EPOCH - ignore hit data ***\n");
            iserr = true;
         } else
         if (chid >= NumTdcChannels) {
            printf("TDC Channel number problem %u\n", chid);
            iserr = true;
         } else {

            // fill histograms only for normal channels
            if (first_scan) {

               FillH1(fChannels, chid);

               if (chid>0) {
                  FillH1(fAllFine, msg.getHitTmFine());
                  FillH1(fAllCoarse, msg.getHitTmCoarse());
               }

               if (msg.isHitRisingEdge()) {
                  fCh[chid].rising_stat[msg.getHitTmFine()]++;
               } else {
                  fCh[chid].falling_stat[msg.getHitTmFine()]++;
               }

               if (fAllHistos) FillHistograms();

               if (!iserr) hitcnt++;
            }

            if (!iserr) {
               localtm = iter.getMsgTimeCoarse();

               if (msg.isHitRisingEdge())
                  localtm += fCh[chid].rising_calibr[msg.getHitTmFine()];
               else
                  localtm += fCh[chid].falling_calibr[msg.getHitTmFine()];

               if ((minimtm<1.) || (localtm < minimtm))
                  minimtm = localtm;

               // remember position of channel 0 - it could be used for SYNC settings
               if ((chid==0) && msg.isHitRisingEdge()) {
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
                  AddMessage(indx, (hadaq::TdcSubEvent*) fGlobalMarks.item(indx).subev, hadaq::TdcMessageExt(msg, globaltm));
            }

         }

         iter.clearCurEpoch();

         continue;
      }

      switch (msg.getKind()) {
        case tdckind_Reserved:
           break;
        case tdckind_Header: {
           unsigned errbits = msg.getHeaderErr();
           if (errbits && first_scan)
              printf("!!! found error bits: 0x%x at tdc 0x%x\n", errbits, GetBoardId());

           break;
        }
        case tdckind_Debug:
           break;
        case tdckind_Epoch:
           break;
        default:
           printf("Unknown bits 0x%x in header\n", msg.getKind());
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

   if (fAllHistos && first_scan) AfterFill();

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

void hadaq::TdcProcessor::CalibrateChannel(long* statistic, float* calibr)
{
   double sum(0.);
   for (int n=0;n<FineCounterBins;n++) {
      calibr[n] = sum;
      sum+=statistic[n];
   }

   if (sum<1000) {
      printf("Too few counts %5.0f for calibration of fine counter\n", sum);
   } else {
//      printf("There are %5.0f counts \n", sum);
   }


   for (unsigned n=0;n<FineCounterBins;n++) {
      if (sum<=0)
         calibr[n] = hadaq::TdcMessage::SimpleFineCalibr(n);
      else
         calibr[n] = calibr[n] / sum * hadaq::TdcMessage::CoarseUnit();

//      printf("   Ch[%3u] = %5.3f\n", n, calibr[n]*1e12);
   }
}

void hadaq::TdcProcessor::CopyCalibration(float* calibr, base::H1handle hcalibr)
{
   ClearH1(hcalibr);

   for (unsigned n=0;n<FineCounterBins;n++)
      FillH1(hcalibr, n, calibr[n]*1e12);

}

void hadaq::TdcProcessor::ProduceCalibration()
{
   for (int ch=0;ch<NumTdcChannels;ch++) {
      if (fCh[ch].disabled) continue;
      if (fCh[ch].docalibr) {
         CalibrateChannel(fCh[ch].rising_stat, fCh[ch].rising_calibr);
         CalibrateChannel(fCh[ch].falling_stat, fCh[ch].falling_calibr);
      }
      CopyCalibration(fCh[ch].rising_calibr, fCh[ch].fRisingCalibr);
      CopyCalibration(fCh[ch].falling_calibr, fCh[ch].fFallingCalibr);
   }
}

void hadaq::TdcProcessor::StoreCalibration(const std::string& fname)
{
   if (fname.empty()) return;

   FILE* f = fopen(fname.c_str(),"w");
   if (f==0) {
      printf("Cannot open file %s for writing calibration\n", fname.c_str());
      return;
   }

   for (int ch=0;ch<NumTdcChannels;ch++) {
      fwrite(fCh[ch].rising_calibr, sizeof(fCh[ch].rising_calibr), 1, f);
      fwrite(fCh[ch].falling_calibr, sizeof(fCh[ch].falling_calibr), 1, f);
   }

   fclose(f);

   printf("Storing calibration in %s\n", fname.c_str());
}

bool hadaq::TdcProcessor::LoadCalibration(const std::string& fname)
{
   if (fname.empty()) return false;

   FILE* f = fopen(fname.c_str(),"r");
   if (f==0) {
      printf("Cannot open file %s for reading calibration\n", fname.c_str());
      return false;
   }

   for (int ch=0;ch<NumTdcChannels;ch++) {
      fread(fCh[ch].rising_calibr, sizeof(fCh[ch].rising_calibr), 1, f);
      fread(fCh[ch].falling_calibr, sizeof(fCh[ch].falling_calibr), 1, f);
   }

   fclose(f);

   printf("Reading calibration from %s done\n", fname.c_str());
   return true;
}

