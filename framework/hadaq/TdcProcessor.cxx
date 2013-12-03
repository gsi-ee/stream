#include "hadaq/TdcProcessor.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbProcessor.h"
#include "hadaq/TdcSubEvent.h"


unsigned hadaq::TdcProcessor::fMaxBrdId = 8;

#define RAWPRINT( args ...) if(IsPrintRawData()) printf( args )

hadaq::TdcProcessor::TdcProcessor(TrbProcessor* trb, unsigned tdcid, unsigned numchannels, unsigned edge_mask) :
   base::StreamProc("TDC", tdcid),
   fTrb(trb),
   fIter1(),
   fIter2(),
   fEdgeMask(edge_mask),
   fAutoCalibration(0),
   fPrintRawData(false),
   fEveryEpoch(false),
   fCrossProcess(false)
{
   fMsgPerBrd = trb ? trb->fMsgPerBrd : 0;
   fErrPerBrd = trb ? trb->fErrPerBrd : 0;
   fHitsPerBrd = trb ? trb->fHitsPerBrd : 0;

   fChannels = MakeH1("Channels", "TDC channels", numchannels, 0, numchannels, "ch");
   fErrors = MakeH1("Errors", "Errors in TDC channels", numchannels, 0, numchannels, "ch");
   fUndHits = MakeH1("UndetectedHits", "Undetected hits in TDC channels", numchannels, 0, numchannels, "ch");

   fMsgsKind = MakeH1("MsgKind", "kind of messages", 8, 0, 8, "xbin:Reserved,Header,Debug,Epoch,Hit,-,-,-;kind");

   fAllFine = MakeH1("FineTm", "fine counter value", 1024, 0, 1024, "fine");
   fAllCoarse = MakeH1("CoarseTm", "coarse counter value", 2048, 0, 2048, "coarse");

   for (unsigned ch=0;ch<numchannels;ch++) {
      fCh.push_back(ChannelRec());
   }

   // always create histograms for channel 0
   CreateChannelHistograms(0);

   if (trb) {
      trb->AddSub(this, tdcid);
      fPrintRawData = trb->IsPrintRawData();
      fCrossProcess = trb->IsCrossProcess();
   }

   fNewDataFlag = false;

   fWriteCalibr.clear();
}


hadaq::TdcProcessor::~TdcProcessor()
{
}

bool hadaq::TdcProcessor::CheckPrintError()
{
   return fTrb ? fTrb->CheckPrintError() : true;
}

bool hadaq::TdcProcessor::CreateChannelHistograms(unsigned ch)
{
   if (ch>=NumChannels()) return false;

   if (fCh[ch].fRisingFine || fCh[ch].fFallingFine) return true;

   SetSubPrefix("Ch", ch);

   if (DoRisingEdge()) {
      fCh[ch].fRisingFine = MakeH1("RisingFine", "Rising fine counter", FineCounterBins, 0, FineCounterBins, "fine");
      fCh[ch].fRisingCoarse = MakeH1("RisingCoarse", "Rising coarse counter", 2048, 0, 2048, "coarse");
      fCh[ch].fRisingMult = MakeH1("RisingMult", "Rising event multiplicity", 128, 0, 128, "nhits");
      fCh[ch].fRisingCalibr = MakeH1("RisingCalibr", "Rising calibration function", FineCounterBins, 0, FineCounterBins, "fine");
      // copy calibration only when histogram created
      CopyCalibration(fCh[ch].rising_calibr, fCh[ch].fRisingCalibr);
   }

   if (DoFallingEdge()) {
      fCh[ch].fFallingFine = MakeH1("FallingFine", "Falling fine counter", FineCounterBins, 0, FineCounterBins, "fine");
      fCh[ch].fFallingCoarse = MakeH1("FallingCoarse", "Falling coarse counter", 2048, 0, 2048, "coarse");
      fCh[ch].fFallingMult = MakeH1("FallingMult", "Falling event multiplicity", 128, 0, 128, "nhits");
      fCh[ch].fFallingCalibr = MakeH1("FallingCalibr", "Falling calibration function", FineCounterBins, 0, FineCounterBins, "fine");
      // copy calibration only when histogram created
      CopyCalibration(fCh[ch].falling_calibr, fCh[ch].fFallingCalibr);
   }

   return true;
}


void hadaq::TdcProcessor::DisableCalibrationFor(unsigned firstch, unsigned lastch)
{
   if (lastch<=firstch) lastch = firstch+1;
   if (lastch>=NumChannels()) lastch = NumChannels();
   for (unsigned n=firstch;n<lastch;n++)
      fCh[n].docalibr = false;
}


void hadaq::TdcProcessor::UserPreLoop()
{
//   printf("************************* hadaq::TdcProcessor preloop *******************\n");
//   for (unsigned ch=0;ch<NumChannels();ch++) {

//      CopyCalibration(fCh[ch].rising_calibr, fCh[ch].fRisingCalibr);

//      CopyCalibration(fCh[ch].falling_calibr, fCh[ch].fFallingCalibr);
//   }
}

void hadaq::TdcProcessor::UserPostLoop()
{
//   printf("************************* hadaq::TdcProcessor postloop *******************\n");

   if (!fWriteCalibr.empty()) {
      if (fAutoCalibration == 0) ProduceCalibration(true);
      StoreCalibration(fWriteCalibr);
   }
}

void hadaq::TdcProcessor::CreateHistograms(int *arr)
{
   if (arr==0) {
      for (unsigned ch=0;ch<NumChannels();ch++)
         CreateChannelHistograms(ch);
   } else
   while ((*arr>0) && (*arr < (int) NumChannels())) {
      CreateChannelHistograms(*arr++);
   }
}



void hadaq::TdcProcessor::SetRefChannel(unsigned ch, unsigned refch, unsigned reftdc,
                                        int npoints, double left, double right, bool twodim)
{
   if (ch>=NumChannels()) return;
   fCh[ch].refch = refch;
   if (refch<NumChannels()) {
      fCh[ch].reftdc = (reftdc >= 0xffff) ? GetBoardId() : reftdc;
   } else {
      fCh[ch].reftdc = GetBoardId();
   }
   CreateChannelHistograms(ch);
   if (fCh[ch].reftdc == GetBoardId()) CreateChannelHistograms(refch);

   char sbuf[1024], saxis[1024], refname[1024];
   if (fCh[ch].reftdc == GetBoardId()) {
      sprintf(refname, "Ch%u", fCh[ch].refch);
   } else {
      sprintf(refname, "Ch%u TDC%u", fCh[ch].refch, fCh[ch].reftdc);
   }

   if ((left < right) && (npoints>1)) {
      SetSubPrefix("Ch", ch);
      if (DoRisingEdge()) {

         if (fCh[ch].fRisingRef == 0) {
            sprintf(sbuf, "difference to %s", refname);
            sprintf(saxis, "Ch%u - %s, ns", ch, refname);
            fCh[ch].fRisingRef = MakeH1("RisingRef", sbuf, npoints, left, right, saxis);
         }
         if (fCh[ch].fRisingCoarseRef == 0)
            fCh[ch].fRisingCoarseRef = MakeH1("RisingCoarseRef", "Difference to rising coarse counter in ref channel", 4096, -2048, 2048, "coarse");
         if (twodim && (fCh[ch].fRisingRef2D==0)) {
            sprintf(sbuf, "corr diff %s and fine counter", refname);
            sprintf(saxis, "Ch%u - %s, ns;fine counter", ch, refname);
            fCh[ch].fRisingRef2D = MakeH2("RisingRef2D", sbuf, 500, left, right, 100, 0, 500, saxis);
         }
      }

      if (DoFallingEdge()) {
         if (fCh[ch].fFallingRef == 0) {
            sprintf(sbuf, "difference to %s", refname);
            fCh[ch].fFallingRef = MakeH1("FallingRef", sbuf, npoints, left, right, "ns");
         }
         if (fCh[ch].fFallingCoarseRef == 0)
            fCh[ch].fFallingCoarseRef = MakeH1("FallingCoarseRef", "Difference to falling coarse counter in ref channel", 4096, -2048, 2048, "coarse");
      }
   }
}

bool hadaq::TdcProcessor::SetDoubleRefChannel(unsigned ch1, unsigned ch2,
                                              int npx, double xmin, double xmax,
                                              int npy, double ymin, double ymax)
{
   if ((ch1>=NumChannels()) || (ch2>=NumChannels())) return false;

   if ((fCh[ch1].refch>=NumChannels()) || (fCh[ch2].refch>=NumChannels())) return false;

   unsigned ch = ch1, refch = ch2;
   if (ch1<ch2) { ch = ch2; refch = ch1; }

   fCh[ch].doublerefch = refch;

   SetSubPrefix("Ch", ch);

   char sbuf[1024];
   char saxis[1024];

   if (DoRisingEdge()) {
      if (fCh[ch].fRisingDoubleRef == 0) {
         sprintf(sbuf, "double correlation to Ch%u", fCh[ch].doublerefch);
         sprintf(saxis, "ch%u-ch%u ns;ch%u-ch%u ns", ch, fCh[ch].refch, refch, fCh[refch].refch);

         fCh[ch].fRisingDoubleRef = MakeH2("RisingDoubleRef", sbuf, npx, xmin, xmax, npy, ymin, ymax, saxis);
      }
   }

   return true;
}


void hadaq::TdcProcessor::BeforeFill()
{
   for (unsigned ch=0;ch<NumChannels();ch++) {
      fCh[ch].first_rising_tm = 0;
      fCh[ch].rising_ref_tm = 0.;
      fCh[ch].first_falling_tm = 0;
   }
}

void hadaq::TdcProcessor::AfterFill(SubProcMap* subprocmap)
{
   bool docalibration(true), isany(false);

   for (unsigned ch=0;ch<NumChannels();ch++) {

      unsigned ref = fCh[ch].refch;
      unsigned reftdc = fCh[ch].reftdc;
      if (reftdc==0xffffffff) reftdc = GetBoardId();

      TdcProcessor* refproc = 0;
      if (reftdc == GetBoardId()) refproc = this; else
      if (subprocmap!=0) {
         SubProcMap::iterator iter = subprocmap->find(reftdc);
         if (iter != subprocmap->end()) refproc = iter->second;
      }

      FillH1(fCh[ch].fRisingMult, fCh[ch].rising_cnt); fCh[ch].rising_cnt = 0;
      FillH1(fCh[ch].fFallingMult, fCh[ch].falling_cnt); fCh[ch].falling_cnt = 0;

      // RAWPRINT("TDC%u Ch:%u Try to use as ref TDC%u %u proc:%p\n", GetBoardId(), ch, reftdc, ref, refproc);

      if ((refproc!=0) && (ref<refproc->NumChannels()) && ((ref!=ch) || (refproc!=this))) {
         if (DoRisingEdge() && (fCh[ch].first_rising_tm !=0) && (refproc->fCh[ref].first_rising_tm !=0)) {

            double tm = fCh[ch].first_rising_tm;
            double tm_ref = refproc->fCh[ref].first_rising_tm;
            if ((refproc!=this) && (ch>0) && (ref>0)) {
               tm -= fCh[0].first_rising_tm;
               tm_ref -= refproc->fCh[0].first_rising_tm;
            }

            fCh[ch].rising_ref_tm = tm - tm_ref;

            double diff = fCh[ch].rising_ref_tm*1e9;

            FillH1(fCh[ch].fRisingRef, diff);
            FillH1(fCh[ch].fRisingCoarseRef, 0. + fCh[ch].first_rising_coarse - refproc->fCh[ref].first_rising_coarse);
            FillH2(fCh[ch].fRisingRef2D, diff, fCh[ch].first_rising_fine);
            FillH2(fCh[ch].fRisingRef2D, diff-1., refproc->fCh[ref].first_rising_fine);
            FillH2(fCh[ch].fRisingRef2D, diff-2., fCh[ch].first_rising_coarse/4);
            RAWPRINT("Difference rising %u:%u\t %u:%u\t %12.3f\t %12.3f\t %7.3f  coarse %03x - %03x = %4d  fine %03x %03x \n",
                  GetBoardId(), ch, reftdc, ref,
                  tm*1e9,  tm_ref*1e9, diff,
                  fCh[ch].first_rising_coarse, refproc->fCh[ref].first_rising_coarse, (int) (fCh[ch].first_rising_coarse - refproc->fCh[ref].first_rising_coarse),
                  fCh[ch].first_rising_fine, refproc->fCh[ref].first_rising_fine);

            // make double reference only for local channels
            if ((fCh[ch].doublerefch < NumChannels()) &&
                (fCh[ch].fRisingDoubleRef != 0) &&
                (fCh[fCh[ch].doublerefch].rising_ref_tm != 0)) {
               FillH1(fCh[ch].fRisingDoubleRef, diff, fCh[fCh[ch].doublerefch].rising_ref_tm*1e9);
            }
         }

         if (DoFallingEdge() && (fCh[ch].first_falling_tm !=0 ) && (refproc->fCh[ref].first_falling_tm !=0)) {

            double tm = fCh[ch].first_falling_tm;
            double tm_ref = refproc->fCh[ref].first_falling_tm;

            if ((refproc!=this) && (ch>0) && (ref>0)) {
               tm -= fCh[0].first_falling_tm;
               tm_ref -= refproc->fCh[0].first_falling_tm;
            }

            double diff = (tm - tm_ref)*1e9;

            FillH1(fCh[ch].fFallingRef, diff);
            FillH1(fCh[ch].fFallingCoarseRef, 0. + fCh[ch].first_falling_coarse - refproc->fCh[ref].first_falling_coarse);
            RAWPRINT("Difference falling %u:%u %u:%u  %12.3f  %12.3f  %7.3f  coarse %03x - %03x = %4d  fine %03x %03x \n",
                     GetBoardId(), ch, reftdc, ref,
                     tm*1e9, tm_ref*1e9, diff,
                     fCh[ch].first_falling_coarse, fCh[ref].first_falling_coarse, (int) (fCh[ch].first_falling_coarse - refproc->fCh[ref].first_falling_coarse),
                     fCh[ch].first_falling_fine, refproc->fCh[ref].first_falling_fine);
         }
      }

      if ((fAutoCalibration>0) && fCh[ch].docalibr) {
         if (DoRisingEdge() && (fCh[ch].all_rising_stat > 0)) {
            isany = true;
            if (fCh[ch].all_rising_stat<fAutoCalibration) docalibration = false;
         }
         if (DoFallingEdge() && (fCh[ch].all_falling_stat > 0)) {
            isany = true;
            if (fCh[ch].all_falling_stat<fAutoCalibration) docalibration = false;
         }
      }
   }

   if (docalibration && isany)
      ProduceCalibration(true);
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

   if (first_scan) BeforeFill();

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
         unsigned fine = msg.getHitTmFine();
         unsigned coarse = msg.getHitTmCoarse();
         bool isrising = msg.isHitRisingEdge();
         localtm = iter.getMsgTimeCoarse();

         if (!iter.isCurEpoch()) {
            // one expects epoch before each hit message, if not data are corrupted and we can ignore it
            if (CheckPrintError())
               printf("%5s Missing epoch for hit from channel %u\n", GetName(), chid);
            iserr = true;
            continue;
         }

         if (IsEveryEpoch())
            iter.clearCurEpoch();

         if (chid >= NumChannels()) {
            if (CheckPrintError())
               printf("%5s Channel number problem %u\n", GetName(), chid);
            iserr = true;
            continue;
         }

         if (fine == 0x3FF) {
            // special case - missing hit, just ignore such value
            if (CheckPrintError())
               printf("%5s Missing hit in channel %u fine counter is %x\n", GetName(), chid, fine);
            FillH1(fUndHits, chid);
            continue;
         }

         if (fine >= FineCounterBins) {
            FillH1(fErrors, chid);
            if (CheckPrintError())
               printf("%5s Fine counter %u out of allowed range 0..%u in channel %u\n", GetName(), fine, FineCounterBins, chid);
            iserr = true;
            continue;
         }

         ChannelRec& rec = fCh[chid];

         if (isrising)
            localtm -= rec.rising_calibr[fine];
         else
            localtm -= rec.falling_calibr[fine];

         // fill histograms only for normal channels
         if (first_scan) {

            // ensure that histograms are created
            CreateChannelHistograms(chid);

            FillH1(fChannels, chid);
            FillH1(fAllFine, fine);
            FillH1(fAllCoarse, coarse);

            if (isrising) {
               rec.rising_stat[fine]++;
               rec.all_rising_stat++;
               rec.rising_cnt++;

               FillH1(rec.fRisingFine, fine);
               FillH1(rec.fRisingCoarse, coarse);
               if (rec.first_rising_tm == 0.) {
                  rec.first_rising_tm = localtm;
                  rec.first_rising_coarse = coarse;
                  rec.first_rising_fine = fine;
               }

            } else {
               rec.falling_stat[fine]++;
               rec.all_falling_stat++;
               rec.falling_cnt++;

               FillH1(rec.fFallingFine, fine);
               FillH1(rec.fFallingCoarse, coarse);
               if (rec.first_falling_tm == 0.) {
                  rec.first_falling_tm = localtm;
                  rec.first_falling_coarse = coarse;
                  rec.first_falling_fine = fine;
               }
            }

            if (!iserr) hitcnt++;
         }

         if (!iserr) {
            if ((minimtm<1.) || (localtm < minimtm))
               minimtm = localtm;

            // remember position of channel 0 - it could be used for SYNC settings
            if ((chid==0) && isrising) {
               ch0time = localtm;
            }
         }

         // here we check if hit can be assigned to the events
         if (!first_scan && !iserr && (chid>0)) {
            base::GlobalTime_t globaltm = LocalToGlobalTime(localtm, &help_index);

            // use first channel only for flushing
            unsigned indx = TestHitTime(globaltm, true, false);

            if (indx < fGlobalMarks.size())
               AddMessage(indx, (hadaq::TdcSubEvent*) fGlobalMarks.item(indx).subev, hadaq::TdcMessageExt(msg, globaltm));
         }

         continue;
      }

      // process other messages kinds

      switch (msg.getKind()) {
        case tdckind_Reserved:
           break;
        case tdckind_Header: {
           unsigned errbits = msg.getHeaderErr();
           if (errbits && first_scan)
              if (CheckPrintError())
                 printf("%5s found error bits: 0x%x\n", GetName(), errbits);

           break;
        }
        case tdckind_Debug:
           break;
        case tdckind_Epoch:
           break;
        default:
           if (CheckPrintError())
              printf("%5s Unknown bits 0x%x in header\n", GetName(), msg.getKind());
           break;
      }
   }

   if (isfirstepoch && !iserr)
      iter.setRefEpoch(first_epoch);

   if (first_scan) {

      if ((syncid != 0xffffffff) && (ch0time>=0)) {

//         printf("SYNC %u localtm %12.9f\n", syncid, ch0time);
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

   if (first_scan && !fCrossProcess) AfterFill();

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

void hadaq::TdcProcessor::CalibrateChannel(unsigned nch, long* statistic, float* calibr)
{
   double sum(0.);
     for (int n=0;n<FineCounterBins;n++) {
        sum+=statistic[n];
        calibr[n] = sum;
     }

   if (sum<1000) {
      if (sum>0)
         printf("TDC:%u Ch:%u Too few counts %5.0f for calibration of fine counter, use linear\n", GetBoardId(), nch, sum);
   } else {
      printf("TDC:%u Ch:%u Cnts: %7.0f produce calibration\n", GetBoardId(), nch, sum);
   }

   for (unsigned n=0;n<FineCounterBins;n++) {
      if (sum<=1000)
         calibr[n] = hadaq::TdcMessage::SimpleFineCalibr(n);
      else
         calibr[n] = (calibr[n]-statistic[n]/2) / sum * hadaq::TdcMessage::CoarseUnit();
   }
}

void hadaq::TdcProcessor::CopyCalibration(float* calibr, base::H1handle hcalibr)
{
   ClearH1(hcalibr);

   for (unsigned n=0;n<FineCounterBins;n++)
      FillH1(hcalibr, n, calibr[n]*1e12);

}

void hadaq::TdcProcessor::ProduceCalibration(bool clear_stat)
{
   printf("TDC%d Produce channels calibrations\n", GetBoardId());

   for (unsigned ch=0;ch<NumChannels();ch++) {
      if (fCh[ch].docalibr) {
         if (DoRisingEdge() && (fCh[ch].rising_stat>0)) {
            CalibrateChannel(ch, fCh[ch].rising_stat, fCh[ch].rising_calibr);
            if (clear_stat) {
               for (unsigned n=0;n<FineCounterBins;n++) fCh[ch].rising_stat[n] = 0;
               fCh[ch].all_rising_stat = 0;
            }
         }
         if (DoFallingEdge() && (fCh[ch].falling_stat>0)) {
            CalibrateChannel(ch, fCh[ch].falling_stat, fCh[ch].falling_calibr);
            if (clear_stat) {
               for (unsigned n=0;n<FineCounterBins;n++) fCh[ch].falling_stat[n] = 0;
               fCh[ch].all_falling_stat = 0;
            }

         }
      }
      if (DoRisingEdge())
         CopyCalibration(fCh[ch].rising_calibr, fCh[ch].fRisingCalibr);

      if (DoFallingEdge())
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

   uint64_t num = NumChannels();

   fwrite(&num, sizeof(num), 1, f);

   for (unsigned ch=0;ch<NumChannels();ch++) {
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

   uint64_t num(0);

   fread(&num, sizeof(num), 1, f);

   if (num!=NumChannels()) {
      printf("Mismatch of channels number in calibration file %u and in processor %u\n", (unsigned) num, NumChannels());
   } else {
      for (unsigned ch=0;ch<NumChannels();ch++) {
         fread(fCh[ch].rising_calibr, sizeof(fCh[ch].rising_calibr), 1, f);
         fread(fCh[ch].falling_calibr, sizeof(fCh[ch].falling_calibr), 1, f);
      }
   }

   fclose(f);

   printf("Reading calibration from %s done\n", fname.c_str());
   return true;
}


