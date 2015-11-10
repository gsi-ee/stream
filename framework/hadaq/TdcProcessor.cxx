#include "hadaq/TdcProcessor.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbProcessor.h"
#include "hadaq/TdcSubEvent.h"


#define RAWPRINT( args ...) if(IsPrintRawData()) printf( args )

unsigned hadaq::TdcProcessor::gNumFineBins = FineCounterBins;
unsigned hadaq::TdcProcessor::gTotRange = 100;
bool hadaq::TdcProcessor::gAllHistos = false;

void hadaq::TdcProcessor::SetDefaults(unsigned numfinebins, unsigned totrange)
{
   gNumFineBins = numfinebins;
   gTotRange = totrange;
}

void hadaq::TdcProcessor::SetAllHistos(bool on)
{
   gAllHistos = on;
}

hadaq::TdcProcessor::TdcProcessor(TrbProcessor* trb, unsigned tdcid, unsigned numchannels, unsigned edge_mask) :
   SubProcessor(trb, "TDC_%04X", tdcid),
   fIter1(),
   fIter2(),
   fNumChannels(numchannels),
   fCh(),
   fCalibrTemp(30.),
   fCalibrTrigger(0xFFFF),
   fCalibrProgress(0.),
   fCalibrStatus("Init"),
   fDummyVect(),
   pStoreVect(0),
   fStoreFloat(),
   pStoreFloat(0),
   fStoreDouble(),
   pStoreDouble(0),
   fEdgeMask(edge_mask),
   fAutoCalibration(0),
   fWriteCalibr(),
   fWriteEveryTime(false),
   fEveryEpoch(false),
   fUseLastHit(false),
   fUseNativeTrigger(false),
   fCompensateEpochReset(false),
   fCompensateEpochCounter(0),
   fCh0Enabled(false),
   fLastTdcHeader()
{
   fIsTDC = true;

   if (trb) {
      // when normal trigger is used as sync points, than use trigger time on right side to calculate global time
      if (trb->IsUseTriggerAsSync()) SetSynchronisationKind(sync_Right);
      if ((trb->NumSubProc()==1) && trb->IsUseTriggerAsSync()) fUseNativeTrigger = true;
      fCompensateEpochReset = trb->fCompensateEpochReset;
   }

   fChannels = 0;
   fHits = 0;
   fErrors = 0;
   fUndHits = 0;
   fMsgsKind = 0;
   fAllFine = 0;
   fAllCoarse = 0;
   fRisingCalibr = 0;
   fFallingCalibr = 0;
   fTotShifts = 0;
   fHitsRate = 0;
   fRateCnt = 0;
   fLastRateTm = -1;

   if (HistFillLevel() > 1) {
      fChannels = MakeH1("Channels", "Messages per TDC channels", numchannels, 0, numchannels, "ch");
      if (DoFallingEdge() && DoRisingEdge())
         fHits = MakeH1("Edges", "Edges counts TDC channels (rising/falling)", numchannels*2, 0, numchannels, "ch");
      fErrors = MakeH1("Errors", "Errors in TDC channels", numchannels, 0, numchannels, "ch");
      fUndHits = MakeH1("UndetectedHits", "Undetected hits in TDC channels", numchannels, 0, numchannels, "ch");

      fMsgsKind = MakeH1("MsgKind", "kind of messages", 8, 0, 8, "xbin:Reserved,Header,Debug,Epoch,Hit,-,-,Calibr;kind");

      fAllFine = MakeH2("FineTm", "fine counter value", numchannels, 0, numchannels, (gNumFineBins==1000 ? 100 : gNumFineBins), 0, gNumFineBins, "ch;fine");
      fAllCoarse = MakeH2("CoarseTm", "coarse counter value", numchannels, 0, numchannels, 2048, 0, 2048, "ch;coarse");

      if (DoRisingEdge())
         fRisingCalibr  = MakeH2("RisingCalibr",  "rising edge calibration", numchannels, 0, numchannels, FineCounterBins, 0, FineCounterBins, "ch;fine");

      if (DoFallingEdge()) {
         fFallingCalibr = MakeH2("FallingCalibr", "falling edge calibration", numchannels, 0, numchannels, FineCounterBins, 0, FineCounterBins, "ch;fine");
         fTotShifts = MakeH1("TotShifts", "Calibrated time shift for falling edge", numchannels, 0, numchannels, "kind:F;ch;ns");
      }
   }

   for (unsigned ch=0;ch<numchannels;ch++) {
      fCh.push_back(ChannelRec());
   }

   // always create histograms for channel 0
   CreateChannelHistograms(0);
   if (gAllHistos)
      for (unsigned ch=1;ch<numchannels;ch++)
         CreateChannelHistograms(ch);

   fWriteCalibr.clear();
   fWriteEveryTime = false;
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
   if ((HistFillLevel() < 3) || (ch>=NumChannels())) return false;

   if (fCh[ch].fRisingFine || fCh[ch].fFallingFine) return true;

   SetSubPrefix("Ch", ch);

   if (DoRisingEdge()) {
      fCh[ch].fRisingFine = MakeH1("RisingFine", "Rising fine counter", gNumFineBins, 0, gNumFineBins, "fine");
      fCh[ch].fRisingCoarse = MakeH1("RisingCoarse", "Rising coarse counter", 2048, 0, 2048, "coarse");
      fCh[ch].fRisingMult = MakeH1("RisingMult", "Rising event multiplicity", 128, 0, 128, "nhits");
      fCh[ch].fRisingCalibr = MakeH1("RisingCalibr", "Rising calibration function", FineCounterBins, 0, FineCounterBins, "fine");
      // copy calibration only when histogram created
      CopyCalibration(fCh[ch].rising_calibr, fCh[ch].fRisingCalibr, ch, fRisingCalibr);
   }

   if (DoFallingEdge()) {
      fCh[ch].fFallingFine = MakeH1("FallingFine", "Falling fine counter", gNumFineBins, 0, gNumFineBins, "fine");
      fCh[ch].fFallingCoarse = MakeH1("FallingCoarse", "Falling coarse counter", 2048, 0, 2048, "coarse");
      fCh[ch].fFallingMult = MakeH1("FallingMult", "Falling event multiplicity", 128, 0, 128, "nhits");
      fCh[ch].fFallingCalibr = MakeH1("FallingCalibr", "Falling calibration function", FineCounterBins, 0, FineCounterBins, "fine");
      fCh[ch].fTot = MakeH1("Tot", "Time over threshold", gTotRange*100, 0, gTotRange, "ns");
      fCh[ch].fTot0D = MakeH1("Tot0D", "Time over threshold with 0xD trigger", TotBins, TotLeft, TotRight, "ns");
      // copy calibration only when histogram created
      CopyCalibration(fCh[ch].falling_calibr, fCh[ch].fFallingCalibr, ch, fFallingCalibr);
   }

   SetSubPrefix();

   return true;
}


void hadaq::TdcProcessor::DisableCalibrationFor(unsigned firstch, unsigned lastch)
{
   if (lastch<=firstch) lastch = firstch+1;
   if (lastch>=NumChannels()) lastch = NumChannels();
   for (unsigned n=firstch;n<lastch;n++)
      fCh[n].docalibr = false;
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
   if ((ch>=NumChannels()) || (HistFillLevel()<4)) return;
   fCh[ch].refch = refch;

   if (((reftdc == 0xffff) || (reftdc>0xfffff)) && ((reftdc & 0xf0000) != 0x70000)) {
      fCh[ch].reftdc = GetID();
   } else {
      fCh[ch].reftdc = reftdc & 0xffff;
      fCh[ch].refabs = ((reftdc & 0xf0000) == 0x70000);

      // if other TDC configured as ref channel, enable cross processing
      if (fTrb) fTrb->SetCrossProcess(true);

   }

   CreateChannelHistograms(ch);
   if (fCh[ch].reftdc == GetID()) CreateChannelHistograms(refch);

   char sbuf[1024], saxis[1024], refname[1024];
   if (fCh[ch].reftdc == GetID()) {
      sprintf(refname, "Ch%u", fCh[ch].refch);
   } else {
      sprintf(refname, "TDC 0x%04x Ch%u", fCh[ch].reftdc, fCh[ch].refch);
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
         if (fCh[ch].fFallingCoarseRef == 0)
            fCh[ch].fFallingCoarseRef = MakeH1("FallingCoarseRef", "Difference to falling coarse counter in ref channel", 4096, -2048, 2048, "coarse");
      }
   }
}

bool hadaq::TdcProcessor::SetDoubleRefChannel(unsigned ch1, unsigned ch2,
                                              int npx, double xmin, double xmax,
                                              int npy, double ymin, double ymax)
{
   if (HistFillLevel()<4) return false;

   if ((ch1>=NumChannels()) || (fCh[ch1].refch>=NumChannels()))  return false;

   unsigned reftdc = 0xffff;
   if (ch2 > 1000) { reftdc = (ch2-1000) / 1000; ch2 = ch2 % 1000; }
   if (reftdc >= 0xffff) reftdc = GetID();

   unsigned ch = ch1, refch = ch2;

   if (reftdc == GetID()) {
      if ((ch2>=NumChannels()) || (fCh[ch2].refch>=NumChannels()))  return false;
      if (ch1<ch2) { ch = ch2; refch = ch1; }
   } else {
      if (reftdc > GetID())
         printf("WARNING - double ref channel from TDC with higher ID %u > %u\n", reftdc, GetID());
   }

   fCh[ch].doublerefch = refch;
   fCh[ch].doublereftdc = reftdc;

   SetSubPrefix("Ch", ch);

   char sbuf[1024];
   char saxis[1024];

   if (DoRisingEdge()) {
      if (fCh[ch].fRisingDoubleRef == 0) {
         if (reftdc == GetID()) {
            sprintf(sbuf, "double correlation to Ch%u", refch);
            sprintf(saxis, "ch%u-ch%u ns;ch%u-ch%u ns", ch, fCh[ch].refch, refch, fCh[refch].refch);
         } else {
            sprintf(sbuf, "double correlation to TDC 0x%04x Ch%u", reftdc, refch);
            sprintf(saxis, "ch%u-ch%u ns;tdc 0x%04x refch%u ns", ch, fCh[ch].refch, reftdc, refch);
         }

         fCh[ch].fRisingDoubleRef = MakeH2("RisingDoubleRef", sbuf, npx, xmin, xmax, npy, ymin, ymax, saxis);
      }
   }

   return true;
}

void hadaq::TdcProcessor::CreateRateHisto(int np, double xmin, double xmax)
{
   SetSubPrefix();
   fHitsRate = MakeH1("HitsRate", "Hits rate", np, xmin, xmax, "hits/sec");
}



bool hadaq::TdcProcessor::EnableRefCondPrint(unsigned ch, double left, double right, int numprint)
{
   if (ch>=NumChannels()) return false;
   if (fCh[ch].refch >= NumChannels()) {
      fprintf(stderr,"Reference channel not specified, conditional print cannot work\n");
      return false;
   }
   if (fCh[ch].reftdc != GetID()) {
      fprintf(stderr,"Only when reference channel on same TDC specified, conditional print can be enabled\n");
      return false;
   }
   if (fCh[ch].refch > ch) {
      fprintf(stderr,"Reference channel %u bigger than channel id %u, conditional print may not work\n", fCh[ch].refch, ch);
   }

   SetSubPrefix("Ch", ch);

   fCh[ch].fRisingRefCond = MakeC1("RisingRefPrint", left, right, fCh[ch].fRisingRef);
   fCh[ch].rising_cond_prnt = numprint > 0 ? numprint : 100000000;

   return true;
}


void hadaq::TdcProcessor::BeforeFill()
{
   for (unsigned ch=0;ch<NumChannels();ch++) {
      fCh[ch].rising_hit_tm = 0;
      fCh[ch].rising_last_tm = 0;
      fCh[ch].rising_ref_tm = 0.;
      fCh[ch].rising_new_value = false;
      fCh[ch].last_tot = 0.;
   }
}

void hadaq::TdcProcessor::AfterFill(SubProcMap* subprocmap)
{
   // when doing TOT calibration, use only last TOT value - before one could find other signals
   if ((fCalibrTrigger == 0xD) && DoFallingEdge())
      for (unsigned ch=1;ch<NumChannels();ch++) {
         if (fCh[ch].hascalibr && (fCh[ch].last_tot >= TotLeft) && (fCh[ch].last_tot < TotRight)) {
            int bin = (int) ((fCh[ch].last_tot - TotLeft) / (TotRight - TotLeft) * TotBins);
            fCh[ch].tot0d_hist[bin]++;
            fCh[ch].tot0d_cnt++;
         }
      }

   // complete logic only when hist level is specified
   if (HistFillLevel()>=4)
   for (unsigned ch=0;ch<NumChannels();ch++) {

      DefFillH1(fCh[ch].fRisingMult, fCh[ch].rising_cnt, 1.); fCh[ch].rising_cnt = 0;
      DefFillH1(fCh[ch].fFallingMult, fCh[ch].falling_cnt, 1.); fCh[ch].falling_cnt = 0;

      unsigned ref = fCh[ch].refch;
      if (ref > 0xffff) continue; // no any settings for ref channel, can ignore

      unsigned reftdc = fCh[ch].reftdc;
      if (reftdc>=0xffff) reftdc = GetID();

      TdcProcessor* refproc = 0;
      if (reftdc == GetID()) refproc = this; else
      if (fTrb!=0) refproc = fTrb->FindTDC(reftdc);

      if ((refproc==0) && (subprocmap!=0)) {
         SubProcMap::iterator iter = subprocmap->find(reftdc);
         if ((iter != subprocmap->end()) && iter->second->IsTDC())
            refproc = (TdcProcessor*) iter->second;
      }

      // RAWPRINT("TDC%u Ch:%u Try to use as ref TDC%u %u proc:%p\n", GetID(), ch, reftdc, ref, refproc);

      if ((refproc!=0) && ((ref>0) || (refproc!=this)) && (ref<refproc->NumChannels()) && ((ref!=ch) || (refproc!=this))) {
         if (DoRisingEdge() && (fCh[ch].rising_hit_tm != 0) && (refproc->fCh[ref].rising_hit_tm != 0)) {

            double tm = fCh[ch].rising_hit_tm;
            double tm_ref = refproc->fCh[ref].rising_hit_tm;
            if ((refproc!=this) && (ch>0) && (ref>0) && !fCh[ch].refabs) {
               tm -= fCh[0].rising_hit_tm;
               tm_ref -= refproc->fCh[0].rising_hit_tm;
            }

            fCh[ch].rising_ref_tm = tm - tm_ref;

            double diff = fCh[ch].rising_ref_tm*1e9;

            // when refch is 0 on same board, histogram already filled
            if ((ref!=0) || (refproc!=this)) DefFillH1(fCh[ch].fRisingRef, diff, 1.);

            double coarse_diff = 0. + fCh[ch].rising_coarse - refproc->fCh[ref].rising_coarse;
            DefFillH1(fCh[ch].fRisingCoarseRef, coarse_diff, 1.);
            DefFillH2(fCh[ch].fRisingRef2D, diff, fCh[ch].rising_fine, 1.);
            DefFillH2(fCh[ch].fRisingRef2D, (diff-1.), refproc->fCh[ref].rising_fine, 1.);
            DefFillH2(fCh[ch].fRisingRef2D, (diff-2.), fCh[ch].rising_coarse/4, 1.);
            RAWPRINT("Difference rising %04x:%02u\t %04x:%02u\t %12.3f\t %12.3f\t %7.3f  coarse %03x - %03x = %4d  fine %03x %03x \n",
                  GetID(), ch, reftdc, ref,
                  tm*1e9,  tm_ref*1e9, diff,
                  fCh[ch].rising_coarse, refproc->fCh[ref].rising_coarse, (int) (fCh[ch].rising_coarse - refproc->fCh[ref].rising_coarse),
                  fCh[ch].rising_fine, refproc->fCh[ref].rising_fine);

            // make double reference only for local channels
            // if ((fCh[ch].doublerefch < NumChannels()) &&
            //    (fCh[ch].fRisingDoubleRef != 0) &&
            //    (fCh[fCh[ch].doublerefch].rising_ref_tm != 0)) {
            //   DefFillH1(fCh[ch].fRisingDoubleRef, diff, fCh[fCh[ch].doublerefch].rising_ref_tm*1e9);
            // }
         }
      }

      // fill double-reference histogram, using data from any reference TDC
      if ((fCh[ch].doublerefch < NumChannels()) && (fCh[ch].fRisingDoubleRef!=0)) {
         ref = fCh[ch].doublerefch;
         reftdc = fCh[ch].doublereftdc;
         if (reftdc>=0xffff) reftdc = GetID();
         refproc = 0;
         if (reftdc == GetID()) refproc = this; else
         if (fTrb!=0) refproc = fTrb->FindTDC(reftdc);

         if ((refproc==0) && (subprocmap!=0)) {
            SubProcMap::iterator iter = subprocmap->find(reftdc);
            if ((iter != subprocmap->end()) && iter->second->IsTDC())
               refproc = (TdcProcessor*) iter->second;
         }

         if ((refproc!=0) && (ref<refproc->NumChannels()) && ((ref!=ch) || (refproc!=this))) {
            if ((fCh[ch].rising_ref_tm != 0) && (refproc->fCh[ref].rising_ref_tm != 0))
               DefFillH2(fCh[ch].fRisingDoubleRef, fCh[ch].rising_ref_tm*1e9, refproc->fCh[ref].rising_ref_tm*1e9, 1.);
         }
      }
   }

   if (TestCanCalibrate()>=1.)
      PerformAutoCalibrate();
}

double hadaq::TdcProcessor::TestCanCalibrate()
{
   if (fAutoCalibration<=0) return 0.;

   bool isany = false;
   double min = 1000.;

   for (unsigned ch=0;ch<NumChannels();ch++) {
      if (fCh[ch].docalibr) {
         long stat1(fCh[ch].all_rising_stat), stat2(0);

         switch (fEdgeMask) {
            case edge_BothIndepend: stat2 = fCh[ch].all_falling_stat; break;
            case edge_ForceRising: break;
            case edge_CommonStatistic: stat1 += fCh[ch].all_falling_stat; break;
            default: break;
         }
         if (stat1>0) {
            isany = true;
            double val = 1.*stat1/fAutoCalibration;
            if (val<min) min = val;
         }
         if (stat2>0) {
            isany = true;
            double val = 1.*stat2/fAutoCalibration;
            if (val<min) min = val;
         }
      }
   }

   return isany ? min : 0.;
}

bool hadaq::TdcProcessor::PerformAutoCalibrate()
{
   ProduceCalibration(true);
   if (!fWriteCalibr.empty() && fWriteEveryTime)
      StoreCalibration(fWriteCalibr);
   return true;
}


unsigned hadaq::TdcProcessor::TransformTdcData(hadaqs::RawSubevent* sub, unsigned indx, unsigned datalen, hadaqs::RawSubevent* tgt, unsigned tgtindx)
{
   hadaq::TdcMessage msg, calibr;

   int cnt(0), hitcnt(0), errcnt(0);
   unsigned tgtindx0(tgtindx), calibr_indx(0), calibr_num(0), epoch(0);

   bool use_in_calibr = (fCalibrTrigger > 0xFF) || (fCalibrTrigger == sub->GetTrigTypeTrb3());
   bool is_0d_trig = sub->GetTrigTypeTrb3() == 0xD;
   bool do_tot = is_0d_trig && DoFallingEdge();

   while (datalen-- > 0) {
      msg.assign(sub->Data(indx++));

      cnt++;
      if (fMsgsKind) DefFastFillH1(fMsgsKind, (msg.getKind() >> 29));
      if (!msg.isHit0Msg()) {
         if (msg.isEpochMsg()) { epoch = msg.getEpochValue(); } else
         if (msg.isCalibrMsg()) { calibr_indx = tgtindx; calibr_num = 0; }
         if (tgt) tgt->SetData(tgtindx++, msg.getData());
         continue;
      }
      hitcnt++;

      unsigned chid = msg.getHitChannel();
      unsigned fine = msg.getHitTmFine();
      unsigned coarse = msg.getHitTmCoarse();
      bool isrising = msg.isHitRisingEdge();

      if ((chid >= NumChannels()) || (fine >= FineCounterBins)) {
         msg.setAsHit1(0x3ff);
         sub->SetData(indx-1, msg.getData());
         errcnt++;
         continue;
      }

      ChannelRec& rec = fCh[chid];

      double corr = isrising ? rec.rising_calibr[fine] : rec.falling_calibr[fine];

      if (tgt==0) {
         if (isrising) {
            // simple approach for rising edge - replace data in the source buffer
            // value from 0 to 1000 is 5 ps unit, should be SUB from coarse time value
            uint32_t new_fine = (uint32_t) (corr/5e-12);
            if (new_fine>=1000) new_fine = 1000;
            msg.setAsHit1(new_fine);
         } else {
            // simple approach for falling edge - replace data in the source buffer
            // value from 0 to 500 is 10 ps unit, should be SUB from coarse time value

            unsigned corr_coarse = 0;
            if (rec.tot_shift > 0) {
               // if tot_shift calibrated (in ns), included it into correction
               // in such case which should add correction into coarse counter
               corr += rec.tot_shift*1e-9;
               corr_coarse = (unsigned) (corr/5e-9);
               corr -= corr_coarse*5e-9;
            }

            uint32_t new_fine = (uint32_t) (corr/10e-12);
            if (new_fine>=500) new_fine = 500;

            if (corr_coarse > coarse)
               new_fine |= 0x200; // indicate that corrected time belongs to the previous epoch

            msg.setAsHit1(new_fine);
            if (corr_coarse > 0)
               msg.setHitTmCoarse(coarse - corr_coarse);
         }
         sub->SetData(indx-1, msg.getData());
      } else {
         // copy data to the target, introduce extra messages with calibrated

         uint32_t new_fine = 0;
         if (isrising) {
            // rising edge correction used for 5 ns range, approx 0.305ps binning
            new_fine = (uint32_t) (corr/5e-9*0x3ffe);
         } else {
            // account TOT shift
            corr += rec.tot_shift*1e-9;
            // falling edge correction used for 50 ns, approx 3.05ps binning
            new_fine = (uint32_t) (corr/5e-8*0x3ffe);
         }
         if (new_fine>0x3ffe) new_fine = 0x3ffe;

         if (calibr_indx==0) {
            calibr_indx = tgtindx++;
            calibr_num = 0;
            calibr.assign(tdckind_Calibr);
         } else {
            calibr.assign(tgt->Data(calibr_indx));
         }

         // set data of calibr message
         calibr.setCalibrFine(calibr_num++, new_fine);
         tgt->SetData(calibr_indx, calibr.getData());
         if (calibr_num>1) calibr_indx = 0;

         // copy original hit message
         tgt->SetData(tgtindx++, msg.getData());
      }

      if (isrising) {
         rec.rising_cnt++;
         if (use_in_calibr) { rec.rising_stat[fine]++; rec.all_rising_stat++; }
         if (do_tot) {
            rec.rising_last_tm = ((epoch << 11) | coarse) * 5e-9 - corr;
            rec.rising_new_value = true;
         }
      } else {
         rec.falling_cnt++;
         if (use_in_calibr) { rec.rising_stat[fine]++; rec.all_rising_stat++; }
         if (do_tot && rec.rising_new_value) {
            rec.last_tot = ((epoch << 11) | coarse) * 5e-9 - corr - rec.rising_last_tm;
            rec.rising_new_value = false;
         }
      }

      if (HistFillLevel()<1) continue;

      DefFastFillH1(fChannels, chid);
      DefFastFillH1(fHits, (chid*2 + (isrising ? 0 : 1)));
      DefFastFillH2(fAllFine, chid, fine);
      DefFastFillH2(fAllCoarse, chid, coarse);
      if ((HistFillLevel()>2) && ((rec.fRisingFine==0) || (rec.fFallingFine==0)))
         CreateChannelHistograms(chid);

      if (isrising) {
         DefFastFillH1(rec.fRisingFine, fine);
         DefFastFillH1(rec.fRisingCoarse, coarse);
      } else {
         DefFastFillH1(rec.fFallingFine, fine);
         DefFastFillH1(rec.fFallingCoarse, coarse);
      }
   }

   if (fMsgPerBrd) DefFillH1(*fMsgPerBrd, fSeqeunceId, cnt);
   // fill number of "good" hits
   if (fHitsPerBrd) DefFillH1(*fHitsPerBrd, fSeqeunceId, hitcnt);
   if (fErrPerBrd && (errcnt>0)) DefFillH1(*fErrPerBrd, fSeqeunceId, errcnt);

   if (do_tot)
      for (unsigned ch=1;ch<NumChannels();ch++) {
         ChannelRec& rec = fCh[ch];

         if (rec.hascalibr && (rec.last_tot >= TotLeft) && (rec.last_tot < TotRight)) {
            int bin = (int) ((rec.last_tot - TotLeft) / (TotRight - TotLeft) * TotBins);
            rec.tot0d_hist[bin]++;
            rec.tot0d_cnt++;
         }

         rec.last_tot = 0.;
         rec.rising_new_value = false;
      }

   return tgt ? (tgtindx - tgtindx0) : cnt;
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
   if (buf().format==0)
      memcpy(&syncid, buf.ptr(), 4);

   bool use_for_calibr = (fCalibrTrigger > 0xFF) || (buf().kind == fCalibrTrigger);

   unsigned cnt(0), hitcnt(0);

   bool iserr(false), isfirstepoch(false), rawprint(false), missinghit(false), dostore(false);

   hadaq::TdcSubEvent* trig_subevnt = 0;
   if (first_scan && IsTriggeredAnalysis() && IsStoreEnabled()) {
      dostore = true;
      switch (GetStoreKind()) {
         case 1: trig_subevnt = new hadaq::TdcSubEvent; break;
         case 2: fStoreFloat.clear(); break;
         case 3: fStoreDouble.clear(); break;
         default: break; // not supported
      }
   }

   uint32_t first_epoch(0);

   unsigned epoch_shift = 0;
   if (fCompensateEpochReset) {
      if (first_scan) buf().user_tag = fCompensateEpochCounter;
      epoch_shift = buf().user_tag;
   }

   TdcIterator& iter = first_scan ? fIter1 : fIter2;

   if (buf().format==0)
      iter.assign((uint32_t*) buf.ptr(4), buf.datalen()/4-1, false);
   else
      iter.assign((uint32_t*) buf.ptr(0), buf.datalen()/4, buf().format==2);

   unsigned help_index(0);

   double localtm(0.), minimtm(0), ch0time(0);

   hadaq::TdcMessage& msg = iter.msg();
   hadaq::TdcMessage calibr;
   unsigned ncalibr = 20; // clear indicate that no calibration data present

   while (iter.next()) {

      // if (!first_scan) msg.print();

      cnt++;

      if (first_scan)
         DefFastFillH1(fMsgsKind, (msg.getKind() >> 29));

      if (cnt==1) {
         if (!msg.isHeaderMsg()) {
            iserr = true;
            if (CheckPrintError())
               printf("%5s Missing header message\n", GetName());
         }
         continue;
      }

      if (msg.isEpochMsg()) {

         uint32_t ep = iter.getCurEpoch();

         if (fCompensateEpochReset) {
            ep += epoch_shift;
            iter.setCurEpoch(ep);
         }

         // second message always should be epoch of channel 0
         if (cnt==2) {
            isfirstepoch = true;
            first_epoch = ep;
         }
         continue;
      }

      if (msg.isCalibrMsg()) {
         ncalibr = 0;
         calibr = msg;
         continue;
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
               printf("%5s Channel number %u bigger than configured %u\n", GetName(), chid, NumChannels());
            iserr = true;
            continue;
         }

         if (fine == 0x3FF) {
            if (first_scan) {
               // special case - missing hit, just ignore such value
               if (CheckPrintError())
                  printf("%5s Missing hit in channel %u fine counter is %x\n", GetName(), chid, fine);

               missinghit = true;
               DefFastFillH1(fChannels, chid);
               DefFastFillH1(fUndHits, chid);
            }
            continue;
         }

         ChannelRec& rec = fCh[chid];

         double corr(0.);
         bool raw_hit(true);

         if (msg.getKind() == tdckind_Hit1) {
            if (isrising) {
               corr = fine * 5e-12;
            } else {
               corr = (fine & 0x1FF) * 10e-12;
               if (fine & 0x200) corr += 0x800 * 5e-9; // complete epoch should be subtracted
            }
            raw_hit = false;
         } else
         if (ncalibr<2) {
            // use correction from special message
            corr = calibr.getCalibrFine(ncalibr++)*5e-9/0x3ffe;
            if (!isrising) corr *= 10.; // range for falling edge is 50 ns.
         } else {
            if (fine >= FineCounterBins) {
               DefFastFillH1(fErrors, chid);
               if (CheckPrintError())
                  printf("%5s Fine counter %u out of allowed range 0..%u in channel %u\n", GetName(), fine, FineCounterBins, chid);
               iserr = true;
               continue;
            }

            corr = isrising ? rec.rising_calibr[fine] : rec.falling_calibr[fine] + rec.tot_shift*1e-9;
         }

         // apply correction
         localtm -= corr;

         if ((chid==0) && (ch0time==0)) ch0time = localtm;

         if (IsTriggeredAnalysis()) {
            if (ch0time==0) {
               if (CheckPrintError())
                  printf("%5s channel 0 time not found when first HIT in channel %u appears\n", GetName(), chid);
            }

            localtm -= ch0time;
         }

         // printf("%s first %d ch %3u tm %12.9f\n", GetName(), first_scan, chid, localtm);

         // fill histograms only for normal channels
         if (first_scan) {

            // if (chid>0) printf("%s HIT ch %u tm %12.9f diff %12.9f\n", GetName(), chid, localtm, localtm - ch0time);

            if (chid==0) {
               if (fLastRateTm < 0) fLastRateTm = ch0time;
               if (ch0time - fLastRateTm > 1) {
                  FillH1(fHitsRate, fRateCnt / (ch0time - fLastRateTm));
                  fLastRateTm = ch0time;
                  fRateCnt = 0;
               }
            } else {
               fRateCnt++;
            }

            // ensure that histograms are created
            if ((HistFillLevel()>2) && (rec.fRisingFine == 0))
               CreateChannelHistograms(chid);

            DefFastFillH1(fChannels, chid);
            DefFillH1(fHits, (chid + (isrising ? 0.25 : 0.75)), 1.);
            DefFillH2(fAllFine, chid, fine, 1.);
            DefFillH2(fAllCoarse, chid, coarse, 1.);

            if (isrising) {
               if (raw_hit && use_for_calibr) {
                  rec.rising_stat[fine]++;
                  rec.all_rising_stat++;
               }

               rec.rising_cnt++;
               DefFastFillH1(rec.fRisingFine, fine);
               DefFastFillH1(rec.fRisingCoarse, coarse);

               bool print_cond = false;

               rec.rising_last_tm = localtm;
               rec.rising_new_value = true;

               if ((rec.rising_hit_tm == 0.) || fUseLastHit) {
                  rec.rising_hit_tm = localtm;
                  rec.rising_coarse = coarse;
                  rec.rising_fine = fine;

                  if ((rec.rising_cond_prnt>0) && (rec.reftdc == GetID()) &&
                      (rec.refch < NumChannels()) && (fCh[rec.refch].rising_hit_tm!=0)) {
                     double diff = (localtm - fCh[rec.refch].rising_hit_tm) * 1e9;
                     if (TestC1(rec.fRisingRefCond, diff) == 0) {
                        rec.rising_cond_prnt--;
                        print_cond = true;
                     }
                  }
               }

               if (print_cond) rawprint = true;

               // special case - when ref channel defined as 0, fill all hits
               if ((chid!=0) && (rec.refch==0) && (rec.reftdc == GetID())) {

                  DefFillH1(rec.fRisingRef, ((localtm - fCh[0].rising_hit_tm)*1e9), 1.);

                  if (IsPrintRawData() || print_cond)
                  printf("Difference rising %04x:%02u\t %04x:%02u\t %12.3f\t %12.3f\t %7.3f  coarse %03x - %03x = %4d  fine %03x %03x \n",
                          GetID(), chid, rec.reftdc, rec.refch,
                          localtm*1e9,  fCh[0].rising_hit_tm*1e9, (localtm - fCh[0].rising_hit_tm)*1e9,
                          coarse, fCh[0].rising_coarse, (int) (coarse - fCh[0].rising_coarse),
                          fine, fCh[0].rising_fine);
               }

            } else {
               if (raw_hit && use_for_calibr) {
                  rec.falling_stat[fine]++;
                  rec.all_falling_stat++;
               }
               rec.falling_cnt++;

               if (raw_hit) DefFastFillH1(rec.fFallingFine, fine);
               DefFastFillH1(rec.fFallingCoarse, coarse);

               if (rec.rising_new_value && (rec.rising_last_tm!=0)) {
                  double tot = (localtm - rec.rising_last_tm)*1e9;

                  DefFillH1(rec.fTot, tot, 1.);
                  rec.rising_new_value = false;

                  // use only raw hit
                  if (raw_hit && (ncalibr<3) && (buf().kind == 0xD)) rec.last_tot = tot + rec.tot_shift;
               }
            }

            if (!iserr) {
               hitcnt++;
               if ((minimtm==0) || (localtm < minimtm)) minimtm = localtm;

               if (dostore)
                  switch(GetStoreKind()) {
                     case 1:
                        if (trig_subevnt)
                           trig_subevnt->AddMsg(hadaq::TdcMessageExt(msg, (chid>0) ? localtm : ch0time));
                        break;
                     case 2:
                        if (chid>0)
                           fStoreFloat.push_back(hadaq::MessageFloat(chid, isrising, localtm*1e9));
                        break;
                     case 3:
                        fStoreDouble.push_back(hadaq::MessageDouble(chid, isrising, ch0time + localtm));
                        break;
                     default: break;
                  }
            }
         } else

         // for second scan we check if hit can be assigned to the events
         if (((chid>0) || fCh0Enabled) && !iserr) {

            base::GlobalTime_t globaltm = LocalToGlobalTime(localtm, &help_index);

            // printf("TDC%u Test TDC message local:%11.9f global:%11.9f\n", GetID(), localtm, globaltm);

            // we test hits, but do not allow to close events
            unsigned indx = TestHitTime(globaltm, true, false);

            if (indx < fGlobalMarks.size()) {
               // printf("!!!!!!!!!!!!!!!!!!!!!! ADD %s message ch %u diff %12.9f !!!!!!!!!!!!!!!!\n", GetName(), chid, globaltm - fGlobalMarks.item(indx).globaltm);
               AddMessage(indx, (hadaq::TdcSubEvent*) fGlobalMarks.item(indx).subev, hadaq::TdcMessageExt(msg, chid>0 ? globaltm : ch0time));
            }
         }

         continue;
      }

      // process other messages kinds

      switch (msg.getKind()) {
        case tdckind_Reserved:
           break;
        case tdckind_Header: {
           unsigned errbits = msg.getHeaderErr();
           if (first_scan) fLastTdcHeader = msg;
           if (errbits && first_scan) {
              if (CheckPrintError())
                 printf("%5s found error bits: 0x%x\n", GetName(), errbits);
           }

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

   if (isfirstepoch && !iserr) {
      // if we want to compensate epoch reset, use epoch of trigger channel+1
      // +1 to exclude small probability of hits after trigger with epoch+1 value
      if (fCompensateEpochReset && first_scan)
         fCompensateEpochCounter = first_epoch+1;

      iter.setRefEpoch(first_epoch);
   }

   if (first_scan) {

      if (trig_subevnt!=0) {
         // put event back to the trigger event
         mgr()->AddToTrigEvent(GetName(), trig_subevnt);
      }

      // if we use trigger as time marker
      if (fUseNativeTrigger && (ch0time!=0)) {
         base::LocalTimeMarker marker;
         marker.localid = 1;
         marker.localtm = ch0time;

         // printf("%s Create TRIGGER %11.9f\n", GetName(), ch0time);

         AddTriggerMarker(marker);
      }

      if ((syncid != 0xffffffff) && (ch0time!=0)) {

         // printf("%s Create SYNC %u tm %12.9f\n", GetName(), syncid, ch0time);
         base::SyncMarker marker;
         marker.uniqueid = syncid;
         marker.localid = 0;
         marker.local_stamp = ch0time;
         marker.localtm = ch0time;
         AddSyncMarker(marker);
      }

      buf().local_tm = minimtm;

//      printf("Proc:%p first scan iserr:%d  minm: %12.9f\n", this, iserr, minimtm*1e-9);

      if (fMsgPerBrd) DefFillH1(*fMsgPerBrd, fSeqeunceId, cnt);

      // fill number of "good" hits
      if (fHitsPerBrd) DefFillH1(*fHitsPerBrd, fSeqeunceId, hitcnt);

      if (iserr || missinghit)
         if (fErrPerBrd) DefFillH1(*fErrPerBrd, fSeqeunceId, 1.);
   } else {

      // use first channel only for flushing
      if (ch0time!=0)
         TestHitTime(LocalToGlobalTime(ch0time, &help_index), false, true);
   }

   if (first_scan && !fCrossProcess) AfterFill();

   if (rawprint && first_scan) {
      printf("RAW data of %s\n", GetName());
      TdcIterator iter;
      if (buf().format==0)
         iter.assign((uint32_t*) buf.ptr(4), buf.datalen()/4-1, false);
      else
         iter.assign((uint32_t*) buf.ptr(0), buf.datalen()/4, buf().format==2);
      while (iter.next()) iter.printmsg();
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

bool hadaq::TdcProcessor::CalibrateChannel(unsigned nch, long* statistic, float* calibr)
{
   double sum(0.);
   for (int n=0;n<FineCounterBins;n++) {
      sum+=statistic[n];
      calibr[n] = sum;
   }

   if (sum<=1000) {
      if (sum>0)
         printf("%s Ch:%u Too few counts %5.0f for calibration of fine counter, use linear\n", GetName(), nch, sum);
   } else {
      printf("%s Ch:%u Cnts: %7.0f produce calibration\n", GetName(), nch, sum);
   }

   for (unsigned n=0;n<FineCounterBins;n++) {
      if (sum<=1000)
         calibr[n] = hadaq::TdcMessage::SimpleFineCalibr(n);
      else
         calibr[n] = (calibr[n]-statistic[n]/2) / sum * hadaq::TdcMessage::CoarseUnit();
   }

   return sum > 1000;
}

bool hadaq::TdcProcessor::CalibrateTot(unsigned nch, long* hist, float& tot_shift, float cut)
{
   int left(0), right(TotBins);
   double sum0(0), sum1(0), sum2(0);

   for (int n=left;n<right;n++) sum0 += hist[n];
   if (sum0 < 10) {
      printf("%s Ch:%u TOT failed - not enough statistic\n", GetName(), nch);
      return false; // no statistic for small number of counts
   }

   if ((cut>0) && (cut<0.3)) {
      double suml(0.), sumr(0.);

      while ((left<right-1) && (suml < sum0*cut))
         suml+=hist[left++];

      while ((right > left+1) && (sumr < sum0*cut))
         sumr+=hist[--right];
   }

   sum0 = 0;

   for (int n=left;n<right;n++) {
      double x = TotLeft + (n + 0.5) / TotBins * (TotRight-TotLeft); // x coordinate of center bin

      sum0 += hist[n];
      sum1 += hist[n]*x;
      sum2 += hist[n]*x*x;
   }

   double mean = sum1/sum0;
   double rms = sum2/sum0 - mean*mean;
   if (rms<0) {
      printf("%s Ch:%u TOT failed - error in RMS calculation\n", GetName(), nch);
      return false;
   }
   rms = sqrt(rms);

   if (rms > 0.1) {
      printf("%s Ch:%u TOT failed - RMS %5.3f too high\n", GetName(), nch, rms);
      return false;
   }

   tot_shift = mean - 30.;

   printf("%s Ch:%u TOT: %6.3f rms: %5.3f\n", GetName(), nch, mean, rms);

   return true;
}


void hadaq::TdcProcessor::CopyCalibration(float* calibr, base::H1handle hcalibr, unsigned ch, base::H2handle h2calibr)
{
   ClearH1(hcalibr);

   for (unsigned n=0;n<FineCounterBins;n++) {
      DefFillH1(hcalibr, n, calibr[n]*1e12);
      DefFillH2(h2calibr, ch, n, calibr[n]*1e12);
   }
}

void hadaq::TdcProcessor::ProduceCalibration(bool clear_stat)
{
   printf("%s produce channels calibrations\n", GetName());

   ClearH2(fRisingCalibr);
   ClearH2(fFallingCalibr);
   ClearH1(fTotShifts);

   for (unsigned ch=0;ch<NumChannels();ch++) {
      if (fCh[ch].docalibr) {

         // special case - use common statistic
         if (fEdgeMask == edge_CommonStatistic) {
            fCh[ch].all_rising_stat += fCh[ch].all_falling_stat;
            fCh[ch].all_falling_stat = 0;
            for (unsigned n=0;n<FineCounterBins;n++) {
               fCh[ch].rising_stat[n] += fCh[ch].falling_stat[n];
               fCh[ch].falling_stat[n] = 0;
            }
         }

         bool res = false;

         if (DoRisingEdge() && (fCh[ch].all_rising_stat>0))
            res = CalibrateChannel(ch, fCh[ch].rising_stat, fCh[ch].rising_calibr);

         if (DoFallingEdge() && (fCh[ch].all_falling_stat>0) && (fEdgeMask == edge_BothIndepend))
            if (!CalibrateChannel(ch, fCh[ch].falling_stat, fCh[ch].falling_calibr)) res = false;

         if (DoFallingEdge() && (fCh[ch].tot0d_cnt > 100)) {
            CalibrateTot(ch, fCh[ch].tot0d_hist, fCh[ch].tot_shift, 0.05);
            for (unsigned n=0;n<TotBins;n++) {
               double x = TotLeft + (n + 0.1) / TotBins * (TotRight-TotLeft);
               DefFillH1(fCh[ch].fTot0D, x, fCh[ch].tot0d_hist[n]);
            }
         }

         fCh[ch].hascalibr = res;

         if ((fEdgeMask == edge_CommonStatistic) || (fEdgeMask == edge_ForceRising))
            for (unsigned n=0;n<FineCounterBins;n++)
               fCh[ch].falling_calibr[n] = fCh[ch].rising_calibr[n];

         if (clear_stat) {
            for (unsigned n=0;n<FineCounterBins;n++) {
               fCh[ch].falling_stat[n] = 0;
               fCh[ch].rising_stat[n] = 0;
            }
            fCh[ch].all_falling_stat = 0;
            fCh[ch].all_rising_stat = 0;
            fCh[ch].tot0d_cnt = 0;
            for (unsigned n=0;n<TotBins;n++)
               fCh[ch].tot0d_hist[n] = 0;
         }
      }
      if (DoRisingEdge())
         CopyCalibration(fCh[ch].rising_calibr, fCh[ch].fRisingCalibr, ch, fRisingCalibr);

      if (DoFallingEdge())
         CopyCalibration(fCh[ch].falling_calibr, fCh[ch].fFallingCalibr, ch, fFallingCalibr);

      DefFillH1(fTotShifts, ch, fCh[ch].tot_shift);
   }

   fCalibrStatus = "Ready";
}

void hadaq::TdcProcessor::StoreCalibration(const std::string& fprefix)
{
   if (fprefix.empty()) return;

   char fname[1024];
   snprintf(fname, sizeof(fname), "%s%04x.cal", fprefix.c_str(), GetID());

   FILE* f = fopen(fname,"w");
   if (f==0) {
      printf("%s Cannot open file %s for writing calibration\n", GetName(), fname);
      return;
   }

   uint64_t num = NumChannels();

   fwrite(&num, sizeof(num), 1, f);

   // calibration curves
   for (unsigned ch=0;ch<NumChannels();ch++) {
      fwrite(fCh[ch].rising_calibr, sizeof(fCh[ch].rising_calibr), 1, f);
      fwrite(fCh[ch].falling_calibr, sizeof(fCh[ch].falling_calibr), 1, f);
   }

   // tot shifts
   for (unsigned ch=0;ch<NumChannels();ch++) {
      fwrite(&(fCh[ch].tot_shift), sizeof(fCh[ch].tot_shift), 1, f);
   }

   // temperature
   // fwrite(&fCalibrTemp, sizeof(fCalibrTemp), 1, f);

   fclose(f);

   printf("%s storing calibration in %s\n", GetName(), fname);
}

bool hadaq::TdcProcessor::LoadCalibration(const std::string& fprefix, double koef)
{
   if (fprefix.empty()) return false;

   char fname[1024];

   snprintf(fname, sizeof(fname), "%s%04x.cal", fprefix.c_str(), GetID());

   FILE* f = fopen(fname,"r");
   if (f==0) {
      printf("Cannot open file %s for reading calibration\n", fname);
      return false;
   }

   uint64_t num(0);

   fread(&num, sizeof(num), 1, f);

   ClearH2(fRisingCalibr);
   ClearH2(fFallingCalibr);
   ClearH1(fTotShifts);

   if (num!=NumChannels()) {
      printf("%s in file %s mismatch of channels number in calibrations  %u and in processor %u\n", GetName(), fname, (unsigned) num, NumChannels());
   }

   for (unsigned ch=0;ch<num;ch++) {
     fCh[ch].hascalibr = false;

     if (ch>=NumChannels()) {
        fseek(f, sizeof(fCh[0].rising_calibr) + sizeof(fCh[0].rising_calibr), SEEK_CUR);
        continue;
     }

     fCh[ch].hascalibr =
        (fread(fCh[ch].rising_calibr, sizeof(fCh[ch].rising_calibr), 1, f) ==1) &&
        (fread(fCh[ch].falling_calibr, sizeof(fCh[ch].falling_calibr), 1, f) == 1);

     if ((koef!=1.) && fCh[ch].hascalibr)
        for (unsigned n=0;n<FineCounterBins;n++) {
           fCh[ch].rising_calibr[n] *= koef;
           fCh[ch].falling_calibr[n] *= koef;
           if (fCh[ch].rising_calibr[n] > hadaq::TdcMessage::CoarseUnit())
              fCh[ch].rising_calibr[n] = hadaq::TdcMessage::CoarseUnit();

           if (fCh[ch].falling_calibr[n] > hadaq::TdcMessage::CoarseUnit())
              fCh[ch].falling_calibr[n] = hadaq::TdcMessage::CoarseUnit();
        }

     CopyCalibration(fCh[ch].rising_calibr, fCh[ch].fRisingCalibr, ch, fRisingCalibr);

     CopyCalibration(fCh[ch].falling_calibr, fCh[ch].fFallingCalibr, ch, fFallingCalibr);
   }

   if (!feof(f)) {
      for (unsigned ch=0;ch<num;ch++) {
         if (ch>=NumChannels())
            fseek(f, sizeof(fCh[0].tot_shift), SEEK_CUR);
         else
            fread(&(fCh[ch].tot_shift), sizeof(fCh[ch].tot_shift), 1, f);

         DefFillH1(fTotShifts, ch, fCh[ch].tot_shift);
      }

      // fread(&fCalibrTemp, sizeof(fCalibrTemp), 1, f);
   }

   fclose(f);

   printf("%s reading calibration from %s done\n", GetName(), fname);

   fCalibrStatus = std::string("File ") + fname;

   return true;
}

void hadaq::TdcProcessor::IncCalibration(unsigned ch, bool rising, unsigned fine, unsigned value)
{
   if ((ch>=NumChannels()) || (fine>=FineCounterBins)) return;

   if (rising && DoRisingEdge()) {
      fCh[ch].rising_stat[fine] += value;
      fCh[ch].all_rising_stat += value;
   }

   if (!rising && DoFallingEdge()) {
      fCh[ch].falling_stat[fine] += value;
      fCh[ch].all_falling_stat += value;
   }
}

void hadaq::TdcProcessor::CreateBranch(TTree* t)
{
   switch(GetStoreKind()) {
      case 1:
         pStoreVect = &fDummyVect;
         mgr()->CreateBranch(t, GetName(), "std::vector<hadaq::TdcMessageExt>", (void**) &pStoreVect);
         break;
      case 2:
         pStoreFloat = &fStoreFloat;
         mgr()->CreateBranch(t, GetName(), "std::vector<hadaq::MessageFloat>", (void**) &pStoreFloat);
         break;
      case 3:
         pStoreDouble = &fStoreDouble;
         mgr()->CreateBranch(t, GetName(), "std::vector<hadaq::MessageDouble>", (void**) &pStoreDouble);
         break;
      default:
         break;
   }
}

void hadaq::TdcProcessor::Store(base::Event* ev)
{
   if ((GetStoreKind() == 1) && (ev!=0)) {
      // only for stream analysis use special handling when many events could be produced at once

      fDummyVect.clear();

      hadaq::TdcSubEvent* sub =
            dynamic_cast<hadaq::TdcSubEvent*> (ev->GetSubEvent(GetName()));

      // when subevent exists, use directly pointer on messages vector
      if (sub!=0)
         pStoreVect = sub->vect_ptr();
      else
         pStoreVect = &fDummyVect;
   }
}
