#include "hadaq/TdcProcessor.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbProcessor.h"
#include "hadaq/TdcSubEvent.h"
#include <iostream>

#define RAWPRINT( args ...) if(IsPrintRawData()) printf( args )


#define ADDERROR(code, args ...) if(((1 << code) & gErrorMask) || mgr()->DoLog()) AddError( code, args )


unsigned hadaq::TdcProcessor::gNumFineBins = FineCounterBins;
unsigned hadaq::TdcProcessor::gTotRange = 100;
unsigned hadaq::TdcProcessor::gErrorMask = 0xffffffffU;
bool hadaq::TdcProcessor::gAllHistos = false;
int hadaq::TdcProcessor::gBubbleMode = 0;
int hadaq::TdcProcessor::gBubbleMask = 0;
int hadaq::TdcProcessor::gBubbleShift = 0;
bool hadaq::TdcProcessor::gDRICHReapir = false;
double hadaq::TdcProcessor::gTrigDWindowLow = 0;
double hadaq::TdcProcessor::gTrigDWindowHigh = 0;
bool hadaq::TdcProcessor::gUseDTrigForRef = false;

unsigned BUBBLE_SIZE = 19;


void hadaq::TdcProcessor::SetDefaults(unsigned numfinebins, unsigned totrange)
{
   gNumFineBins = numfinebins;
   gTotRange = totrange;
}

void hadaq::TdcProcessor::SetErrorMask(unsigned mask)
{
   gErrorMask = mask;
}


void hadaq::TdcProcessor::SetAllHistos(bool on)
{
   gAllHistos = on;
}

void hadaq::TdcProcessor::SetBubbleMode(int on, unsigned sz, int maskid, int shift)
{
   gBubbleMode = on;
   BUBBLE_SIZE = sz;
   gBubbleMask = maskid;
   gBubbleShift = shift;
}

void hadaq::TdcProcessor::SetDRICHReapir(bool on)
{
   gDRICHReapir = on;
}

bool hadaq::TdcProcessor::IsDRICHReapir()
{
   return gDRICHReapir;
}

void hadaq::TdcProcessor::SetTriggerDWindow(double low, double high)
{
   gTrigDWindowLow = low;
   gTrigDWindowHigh = high;
}

void hadaq::TdcProcessor::SetUseDTrigForRef(bool on)
{
   gUseDTrigForRef = on;
}


hadaq::TdcProcessor::TdcProcessor(TrbProcessor* trb, unsigned tdcid, unsigned numchannels, unsigned edge_mask) :
   SubProcessor(trb, "TDC_%04X", tdcid),
   fIter1(),
   fIter2(),
   fNumChannels(numchannels),
   fNumFineBins(gNumFineBins),
   fCh(),
   fCalibrTemp(30.),
   fCalibrTempCoef(0.004432),
   fCalibrUseTemp(false),
   fCalibrTriggerMask(0xFFFF),
   fCalibrProgress(0),
   fCalibrStatus("NoCalibr"),
   fCalibrQuality(0.),
   fTempCorrection(0.),
   fCurrentTemp(-1.),
   fDesignId(0),
   fCalibrTempSum0(0),
   fCalibrTempSum1(0),
   fCalibrTempSum2(0),
   fDummyVect(),
   pStoreVect(0),
   fStoreFloat(),
   pStoreFloat(0),
   fStoreDouble(),
   pStoreDouble(0),
   fEdgeMask(edge_mask),
   fCalibrCounts(0),
   fAutoCalibr(false),
   fAutoCalibrOnce(false),
   fAllCalibrMode(-1),
   fAllTotMode(-1),
   fWriteCalibr(),
   fWriteEveryTime(false),
   fUseLinear(false),
   fEveryEpoch(false),
   fUseLastHit(false),
   fUseNativeTrigger(false),
   fCompensateEpochReset(false),
   fCompensateEpochCounter(0),
   fCh0Enabled(false),
   fLastTdcHeader(),
   fLastTdcTrailer()
{
   fIsTDC = true;

   if (fNumFineBins==0) fNumFineBins = FineCounterBins;

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
   fTempDistr = 0;
   fhRaisingFineCalibr = 0;
   fhTotVsChannel = 0;
   fhTotMoreCounter = 0;
   fhTotMinusCounter = 0;
   fHitsRate = 0;
   fRateCnt = 0;
   fLastRateTm = -1;
   fBubbleErrDistr = 0;

   fHldId = 0;                   //! sequence number of processor in HLD
   fHitsPerHld = nullptr;
   fErrPerHld = nullptr;
   fChHitsPerHld = nullptr;
   fChErrPerHld = nullptr; //! errors per TDC channel - from HLD

   fToTvalue = ToTvalue;
   fToThmin = ToThmin;
   fToThmax = ToThmax;

   if (HistFillLevel() > 1) {
      fChannels = MakeH1("Channels", "Messages per TDC channels", numchannels, 0, numchannels, "ch");
      if (DoFallingEdge() && DoRisingEdge())
         fHits = MakeH1("Edges", "Edges counts TDC channels (rising/falling)", numchannels*2, 0, numchannels, "ch");
      fErrors = MakeH1("Errors", "Errors in TDC channels", numchannels, 0, numchannels, "ch");
      fUndHits = MakeH1("UndetectedHits", "Undetected hits in TDC channels", numchannels, 0, numchannels, "ch");

      fMsgsKind = MakeH1("MsgKind", "kind of messages", 8, 0, 8, "xbin:Trailer,Header,Debug,Epoch,Hit,-,-,Calibr;kind");

      fAllFine = MakeH2("FineTm", "fine counter value", numchannels, 0, numchannels, (fNumFineBins==1000 ? 100 : fNumFineBins), 0, fNumFineBins, "ch;fine");
      fhRaisingFineCalibr = MakeH2("RaisingFineTmCalibr", "raising calibrated fine counter value", numchannels, 0, numchannels, (fNumFineBins==1000 ? 100 : fNumFineBins), 0, fNumFineBins, "ch;calibrated fine");
      fAllCoarse = MakeH2("CoarseTm", "coarse counter value", numchannels, 0, numchannels, 2048, 0, 2048, "ch;coarse");

      fhTotVsChannel = MakeH2("TotVsChannel", "ToT", numchannels, 0, numchannels, 100, 0., 50., "ch;ToT [ns]");

      fhTotMoreCounter = MakeH1("TotMoreCounter", "ToT > 20 ns counter in TDC channels", numchannels, 0, numchannels, "ch");
      fhTotMinusCounter = MakeH1("TotMinusCounter", "ToT < 0 ns counter in TDC channels", numchannels, 0, numchannels, "ch");

      if (DoRisingEdge())
         fRisingCalibr  = MakeH2("RisingCalibr",  "rising edge calibration", numchannels, 0, numchannels, fNumFineBins, 0, fNumFineBins, "ch;fine");

      if (DoFallingEdge()) {
         fFallingCalibr = MakeH2("FallingCalibr", "falling edge calibration", numchannels, 0, numchannels, fNumFineBins, 0, fNumFineBins, "ch;fine");
         fTotShifts = MakeH1("TotShifts", "Calibrated time shift for falling edge", numchannels, 0, numchannels, "kind:F;ch;ns");
      }

      if (gBubbleMode > 1)
         fBubbleErrDistr = MakeH1("AllBubbleErrors", "All bubble errors from all channels", BUBBLE_SIZE*16, 0, BUBBLE_SIZE*16, "bubble");
   }

   for (unsigned ch=0;ch<numchannels;ch++)
      fCh.push_back(ChannelRec());

   for (unsigned ch=0;ch<numchannels;ch++) {
      fCh[ch].CreateCalibr(fNumFineBins);
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
   for (unsigned ch=0;ch<NumChannels();ch++)
      fCh[ch].ReleaseCalibr();
}

bool hadaq::TdcProcessor::CheckPrintError()
{
   return fTrb ? fTrb->CheckPrintError() : true;
}


void hadaq::TdcProcessor::AddError(unsigned code, const char *fmt, ...)
{
   va_list args;
   int length(256);
   char *buffer(0);

   std::string sbuf(GetName());
   while (true) {
      if (buffer) delete [] buffer;
      buffer = new char [length];
      va_start(args, fmt);
      auto result = vsnprintf(buffer, length, fmt, args);
      va_end(args);

      if (result < 0) length *= 2; else                 // this is error handling in glibc 2.0
      if (result >= length) length = result + 1; else   // this is error handling in glibc 2.1
      break;
   }
   sbuf.append(" ");
   sbuf.append(buffer);
   delete [] buffer;

   if (CheckPrintError()) {
      printf(sbuf.c_str());
      printf("\n");
   }

   if (fTrb) fTrb->EventError(sbuf.c_str());
}


bool hadaq::TdcProcessor::CreateChannelHistograms(unsigned ch)
{
   if ((HistFillLevel() < 3) || (ch>=NumChannels())) return false;

   SetSubPrefix("Ch", ch);

   if ((gBubbleMode>1) && (fCh[ch].fBubbleRising == 0)) {
      fCh[ch].fBubbleRising = MakeH1("BRising", "Bubble rising edge", BUBBLE_SIZE*16, 0, BUBBLE_SIZE*16, "bubble");
      fCh[ch].fBubbleFalling = MakeH1("BFalling", "Bubble falling edge", BUBBLE_SIZE*16, 0, BUBBLE_SIZE*16, "bubble");
      fCh[ch].fBubbleRisingErr = MakeH1("BErrRising", "Bubble rising edge simple error", BUBBLE_SIZE*16, 0, BUBBLE_SIZE*16, "bubble");
      fCh[ch].fBubbleFallingErr = MakeH1("BErrFalling", "Bubble falling edge simple error", BUBBLE_SIZE*16, 0, BUBBLE_SIZE*16, "bubble");
      fCh[ch].fBubbleRisingAll = MakeH1("BRestRising", "All other hits with rising edge", BUBBLE_SIZE*16, 0, BUBBLE_SIZE*16, "bubble");
      fCh[ch].fBubbleFallingAll = MakeH1("BRestFalling", "All other hits with falling edge", BUBBLE_SIZE*16, 0, BUBBLE_SIZE*16, "bubble");
   }

   if ((fCh[ch].fBubbleRising == 0) && (gBubbleMode==1)) {
      fCh[ch].fBubbleRising = MakeH1("BRising", "Bubble rising edge", BUBBLE_SIZE*16, 0, BUBBLE_SIZE*16, "bubble");
      fCh[ch].fBubbleFalling = MakeH1("BFalling", "Bubble falling edge", BUBBLE_SIZE*16, 0, BUBBLE_SIZE*16, "bubble");
   }

   if (DoRisingEdge() && (fCh[ch].fRisingFine==0)) {
      fCh[ch].fRisingFine = MakeH1("RisingFine", "Rising fine counter", fNumFineBins, 0, fNumFineBins, "fine");
      if (gBubbleMode < 2)
         fCh[ch].fRisingMult = MakeH1("RisingMult", "Rising event multiplicity", 128, 0, 128, "nhits");
      fCh[ch].fRisingCalibr = MakeH1("RisingCalibr", "Rising calibration function", fNumFineBins, 0, fNumFineBins, "fine;kind:F");
      // copy calibration only when histogram created
      CopyCalibration(fCh[ch].rising_calibr, fCh[ch].fRisingCalibr, ch, fRisingCalibr);
   }

   if (DoFallingEdge() && (fCh[ch].fFallingFine==0)) {
      fCh[ch].fFallingFine = MakeH1("FallingFine", "Falling fine counter", fNumFineBins, 0, fNumFineBins, "fine");
      fCh[ch].fFallingCalibr = MakeH1("FallingCalibr", "Falling calibration function", fNumFineBins, 0, fNumFineBins, "fine;kind:F");
      if (gBubbleMode < 2) {
         fCh[ch].fFallingMult = MakeH1("FallingMult", "Falling event multiplicity", 128, 0, 128, "nhits");
         fCh[ch].fTot = MakeH1("Tot", "Time over threshold", gTotRange*100, 0, gTotRange, "ns");
         fCh[ch].fTot0D = MakeH1("Tot0D", "Time over threshold with 0xD trigger", TotBins, fToThmin, fToThmax, "ns");
      }
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

void hadaq::TdcProcessor::SetToTRange(double tot, double hmin, double hmax)
{
   // set real ToT value for 0xD trigger and min/max for histogram accumulation
   // default is 30ns, and 50ns - 80ns range

   fToTvalue = tot;
   fToThmin = hmin;
   fToThmax = hmax;
}

void hadaq::TdcProcessor::UserPostLoop()
{
//   printf("************************* hadaq::TdcProcessor postloop *******************\n");

   if (!fWriteCalibr.empty() && fAutoCalibr) {
      if (fCalibrCounts == 0) ProduceCalibration(true, fUseLinear);
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

   if (((reftdc == 0xffff) || (reftdc>0xfffff)) && ((reftdc & 0xf0000) != 0x70000)) reftdc = GetID();

   // ignore invalid settings
   if ((ch==refch) && ((reftdc & 0xffff) == GetID())) return;

   if ((refch==0) && (ch!=0) && ((reftdc & 0xffff) != GetID())) {
      printf("%s cannot set reference to zero channel from other TDC\n", GetName());
      return;
   }

   fCh[ch].refch = refch;
   fCh[ch].reftdc = reftdc & 0xffff;
   fCh[ch].refabs = ((reftdc & 0xf0000) == 0x70000);

   // if other TDC configured as ref channel, enable cross processing
   if (fTrb) fTrb->SetCrossProcess(true);

   CreateChannelHistograms(ch);
   if (fCh[ch].reftdc == GetID()) CreateChannelHistograms(refch);

   char sbuf[1024], saxis[1024], refname[512];
   if (fCh[ch].reftdc == GetID()) {
      snprintf(refname, sizeof(refname), "Ch%u", fCh[ch].refch);
   } else {
      snprintf(refname, sizeof(refname), "TDC 0x%04x Ch%u", fCh[ch].reftdc, fCh[ch].refch);
   }

   if ((left < right) && (npoints>1)) {
      SetSubPrefix("Ch", ch);
      if (DoRisingEdge()) {

         if (fCh[ch].fRisingRef == 0) {
            snprintf(sbuf, sizeof(sbuf), "difference to %s", refname);
            snprintf(saxis, sizeof(saxis), "Ch%u - %s, ns", ch, refname);
            fCh[ch].fRisingRef = MakeH1("RisingRef", sbuf, npoints, left, right, saxis);
         }

         if (twodim && (fCh[ch].fRisingRef2D==0)) {
            snprintf(sbuf, sizeof(sbuf), "corr diff %s and fine counter", refname);
            snprintf(saxis, sizeof(saxis), "Ch%u - %s, ns;fine counter", ch, refname);
            fCh[ch].fRisingRef2D = MakeH2("RisingRef2D", sbuf, 500, left, right, 100, 0, 500, saxis);
         }
      }

      SetSubPrefix();
   }
}

bool hadaq::TdcProcessor::SetDoubleRefChannel(unsigned ch1, unsigned ch2,
                                              int npx, double xmin, double xmax,
                                              int npy, double ymin, double ymax)
{
   if (HistFillLevel()<4) return false;

   if ((ch1>=NumChannels()) || (fCh[ch1].refch>=NumChannels()))  return false;

   unsigned reftdc = 0xffff;
   if (ch2 > 0xffff) { reftdc = ch2 >> 16; ch2 = ch2 & 0xFFFF; }
   if (reftdc >= 0xffff) reftdc = GetID();

   unsigned ch = ch1, refch = ch2;

   if (reftdc == GetID()) {
      if ((ch2>=NumChannels()) || (fCh[ch2].refch>=NumChannels()))  return false;
      if (ch1<ch2) { ch = ch2; refch = ch1; }
   } else {
      //if (reftdc > GetID())
      //   printf("WARNING - double ref channel from TDC with higher ID %u > %u\n", reftdc, GetID());
      if (fTrb) fTrb->SetCrossProcess(true);
   }

   fCh[ch].doublerefch = refch;
   fCh[ch].doublereftdc = reftdc;

   char sbuf[1024];
   char saxis[1024];

   if (DoRisingEdge()) {

      if ((fCh[ch].fRisingRefRef == 0) && (npy == 0)) {
         if (reftdc == GetID()) {
            sprintf(sbuf, "double reference with Ch%u", refch);
            sprintf(saxis, "(ch%u-ch%u) - (refch%u) ns", ch, fCh[ch].refch, refch);
         } else {
            sprintf(sbuf, "double reference with TDC 0x%04x Ch%u", reftdc, refch);
            sprintf(saxis, "(ch%u-ch%u)  - (tdc 0x%04x refch%u) ns", ch, fCh[ch].refch, reftdc, refch);
         }

         SetSubPrefix("Ch", ch);
         fCh[ch].fRisingRefRef = MakeH1("RisingRefRef", sbuf, npx, xmin, xmax, saxis);
         SetSubPrefix();
      }


      if ((fCh[ch].fRisingDoubleRef == 0) && (npy>0)) {
         if (reftdc == GetID()) {
            sprintf(sbuf, "double correlation to Ch%u", refch);
            sprintf(saxis, "ch%u-ch%u ns;ch%u-ch%u ns", ch, fCh[ch].refch, refch, fCh[refch].refch);
         } else {
            sprintf(sbuf, "double correlation to TDC 0x%04x Ch%u", reftdc, refch);
            sprintf(saxis, "ch%u-ch%u ns;tdc 0x%04x refch%u ns", ch, fCh[ch].refch, reftdc, refch);
         }

         SetSubPrefix("Ch", ch);
         fCh[ch].fRisingDoubleRef = MakeH2("RisingDoubleRef", sbuf, npx, xmin, xmax, npy, ymin, ymax, saxis);
         SetSubPrefix();
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

   SetSubPrefix();

   return true;
}


void hadaq::TdcProcessor::BeforeFill()
{
   for (unsigned ch=0;ch<NumChannels();ch++) {
      fCh[ch].rising_hit_tm = 0;
      fCh[ch].rising_last_tm = 0;
      fCh[ch].rising_ref_tm = 0.;
      fCh[ch].rising_new_value = false;
   }
}

void hadaq::TdcProcessor::AfterFill(SubProcMap* subprocmap)
{
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

      // printf("TDC %s %d %d same %d %p %p  %f %f \n", GetName(), ch, ref, (this==refproc), this, refproc, fCh[ch].rising_hit_tm, refproc->fCh[ref].rising_hit_tm);

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
            if ((ref!=0) || (refproc != this)) DefFillH1(fCh[ch].fRisingRef, diff, 1.);

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
      if ((fCh[ch].doublerefch < NumChannels()) && ((fCh[ch].fRisingDoubleRef!=0) || (fCh[ch].fRisingRefRef!=0))) {

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
            if ((fCh[ch].rising_ref_tm != 0) && (refproc->fCh[ref].rising_ref_tm != 0)) {
               DefFillH1(fCh[ch].fRisingRefRef, (fCh[ch].rising_ref_tm - refproc->fCh[ref].rising_ref_tm)*1e9, 1.);
               DefFillH2(fCh[ch].fRisingDoubleRef, fCh[ch].rising_ref_tm*1e9, refproc->fCh[ref].rising_ref_tm*1e9, 1.);
            }
         }
      }
   }

   fCalibrProgress = TestCanCalibrate();
   if ((fCalibrProgress>=1.) && fAutoCalibr) PerformAutoCalibrate();
}

long hadaq::TdcProcessor::CheckChannelStat(unsigned ch)
{
   ChannelRec &rec = fCh[ch];
   if (!rec.docalibr) return 0;

   if (fEdgeMask == edge_CommonStatistic)
      return rec.all_rising_stat + rec.all_falling_stat;

   long stat = 0;

   if (DoRisingEdge() && (rec.all_rising_stat>0)) stat = rec.all_rising_stat;

   if (DoFallingEdge() && (rec.all_falling_stat>0) && (fEdgeMask == edge_BothIndepend))
      if ((stat == 0) || (rec.all_falling_stat < stat)) stat = rec.all_falling_stat;

   return stat;
}

double hadaq::TdcProcessor::TestCanCalibrate()
{
   if (fCalibrCounts<=0) return 0.;

   bool isany = false;
   long min = 1000000000L;

   for (unsigned ch=0;ch<NumChannels();ch++) {
      long stat = CheckChannelStat(ch);
      if (stat>0) {
         isany = true;
         if (stat<min) min = stat;
      }
   }

   return isany ? 1.*min/fCalibrCounts : 0.;
}

bool hadaq::TdcProcessor::PerformAutoCalibrate()
{
   ProduceCalibration(true, fUseLinear || ((fCalibrCounts>0) && (fCalibrCounts % 10000 == 77)));
   if (!fWriteCalibr.empty() && fWriteEveryTime)
      StoreCalibration(fWriteCalibr);
   if (fAutoCalibrOnce && (fCalibrCounts>0)) {
      fAutoCalibrOnce = false;
      fAutoCalibr = false;
   }

   return true;
}

void hadaq::TdcProcessor::BeginCalibration(long cnt)
{
   fCalibrCounts = cnt;
   fAutoCalibrOnce = false;
   fAutoCalibr = false;
   fAllCalibrMode = 1;
   fAllTotMode = -1; // first detect 10 0xD triggers
   fAllDTrigCnt = 0;

   fCalibrStatus = "Accumulating";
   fCalibrQuality = 0.7;
   fCalibrProgress = 0.;

   // ensure that calibration statistic is empty
   for (unsigned ch=0;ch<NumChannels();ch++)
      ClearChannelStat(ch);
}


void hadaq::TdcProcessor::CompleteCalibration(bool dummy, const std::string &filename, const std::string &subname)
{
   if (fAllCalibrMode<=0) return;
   fAllCalibrMode = 0;
   fCalibrCounts = 0;
   fAllTotMode = -1;
   fAllDTrigCnt = 0;

   ProduceCalibration(true, fUseLinear, dummy);

   if (!filename.empty()) StoreCalibration(filename);
   if (!subname.empty()) StoreCalibration(subname);
   // if (!fWriteCalibr.empty()) StoreCalibration(fWriteCalibr);
}

float hadaq::TdcProcessor::ExtractCalibr(float* func, unsigned bin)
{
   if (!fCalibrUseTemp || (fCalibrTemp <= 0) || (fCurrentTemp <= 0) || (fCalibrTempCoef<=0)) return func[bin];

   float temp = fCurrentTemp + fTempCorrection;

   float val = func[bin] * (1+fCalibrTempCoef*(temp-fCalibrTemp));

   if ((temp < fCalibrTemp) && (func[bin] >= hadaq::TdcMessage::CoarseUnit()*0.9999)) {
      // special case - lower temperature and bin which was not observed during calibration
      // just take linear extrapolation, using points bin-30 and bin-80

      float val0 = func[bin-30] + (func[bin-30] - func[bin-80]) / 50 * 30;

      val = val0 * (1+fCalibrTempCoef*(temp-fCalibrTemp));
   }

   return val < hadaq::TdcMessage::CoarseUnit() ? val : hadaq::TdcMessage::CoarseUnit();
}

unsigned hadaq::TdcProcessor::TransformTdcData(hadaqs::RawSubevent* sub, uint32_t *rawdata, unsigned indx, unsigned datalen, hadaqs::RawSubevent* tgt, unsigned tgtindx)
{
   // do nothing in case of empty TDC sub-sub-event
   if (datalen == 0) return 0;

   hadaq::TdcMessage msg, calibr;

   unsigned tgtindx0(tgtindx), cnt(0),
         hitcnt(0), epochcnt(0), errcnt(0), calibr_indx(0), calibr_num(0),
         nrising(0), nfalling(0);

   bool use_in_calibr = ((1 << sub->GetTrigTypeTrb3()) & fCalibrTriggerMask) != 0;
   bool is_0d_trig = (sub->GetTrigTypeTrb3() == 0xD);

   if (fAllCalibrMode == 0) {
      use_in_calibr = false;
   } else if (fAllCalibrMode > 0) {
      use_in_calibr = true;
   }

   bool check_calibr_progress = false;

   if (use_in_calibr) {
      fCalibrAmount += datalen;
      if (fCalibrAmount > 200000) {
         fCalibrAmount = 0;
         check_calibr_progress = true;
      }
   }

   uint32_t epoch(0), chid, fine, kind, coarse(0), new_fine,
            idata, *tgtraw = tgt ? (uint32_t *) tgt->RawData() : 0;
   bool isrising, hard_failure, fast_loop = HistFillLevel() < 2;
   double corr;

   // if (fAllTotMode==1) printf("%s dtrig %d do_tot %d dofalling %d\n", GetName(), is_0d_trig, do_tot, DoFallingEdge());

   while (datalen-- > 0) {

      idata = rawdata[indx++];
      msg.assign(HADAQ_SWAP4(idata));

      cnt++;

      // HERE ONLY FOR DEBUG PURPOSES - check performance
      //if (tgt) {
      //   tgt->SetData(tgtindx++, msg.getData());
      //   continue;
      //}

      kind = msg.getKind();

      // do not fill for every hit, try to use counters
      // DefFastFillH1(fMsgsKind, kind >> 29, 1);

      if (kind != hadaq::tdckind_Hit) {
         if (kind == hadaq::tdckind_Epoch) {
            epoch = msg.getEpochValue();
            epochcnt++;
         } else if (kind == hadaq::tdckind_Calibr) {
            DefFastFillH1(fMsgsKind, kind >> 29, 1);
            calibr.assign(msg.getData()); // copy message into
            calibr_indx = tgtindx;
            calibr_num = 0;
         } else {
            DefFastFillH1(fMsgsKind, kind >> 29, 1);
         }
         if (tgtraw) tgtraw[tgtindx++] = idata; // tgt->SetData(tgtindx++, msg.getData());
         continue;
      }

      hitcnt++;

      chid = msg.getHitChannel();
      fine = msg.getHitTmFine();
      isrising = msg.isHitRisingEdge();

      // if (fAllTotMode==1) printf("%s ch %u rising %d fine %x\n", GetName(), chid, isrising, fine);

      hard_failure = false;
      if (chid >= NumChannels()) {
         hard_failure = true;
         chid = 0; // use dummy channel
         errcnt++;
      }

      ChannelRec &rec = fCh[chid];

      if (fine >= fNumFineBins) {
         hard_failure = true;
         errcnt++;
      }

      // double corr = hard_failure ? 0. : ExtractCalibr(isrising ? rec.rising_calibr : rec.falling_calibr, fine);
      corr = hard_failure ? 0. : (isrising ? rec.rising_calibr[fine] : rec.falling_calibr[fine]);

      if (!tgt) {
         coarse = msg.getHitTmCoarse();
         if (isrising) {
            // simple approach for rising edge - replace data in the source buffer
            // value from 0 to 1000 is 5 ps unit, should be SUB from coarse time value
            uint32_t new_fine = (uint32_t) (corr/5e-12);
            if (new_fine>=1000) new_fine = 1000;
            if (hard_failure) new_fine = 0x3ff;
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

            if (hard_failure) new_fine = 0x3ff;

            msg.setAsHit1(new_fine);
            if (corr_coarse > 0)
               msg.setHitTmCoarse(coarse - corr_coarse);
         }
         sub->SetData(indx-1, msg.getData());
      } else {
         // copy data to the target, introduce extra messages with calibrated

         if (isrising) {
            // rising edge correction used for 5 ns range, approx 0.305ps binning
            new_fine = (uint32_t) (corr/5e-9*0x3ffe);
         } else {
            // account TOT shift
            corr += rec.tot_shift*1e-9;
            // falling edge correction used for 50 ns, approx 3.05ps binning
            new_fine = (uint32_t) (corr/5e-8*0x3ffe);
         }
         if ((new_fine>0x3ffe) || hard_failure) new_fine = 0x3ffe;

         if (calibr_indx == 0) {
            calibr_indx = tgtindx++;
            calibr_num = 0;
            calibr.assign(tdckind_Calibr);
         }

         // set data of calibr message, do not use fast access
         calibr.setCalibrFine(calibr_num++, new_fine);

         // tgt->SetData(calibr_indx, calibr.getData());
         if (calibr_num == 2) {
            tgtraw[calibr_indx] = HADAQ_SWAP4(calibr.getData());
            calibr_indx = 0;
            calibr_num = 0;
         }

         // copy original hit message
         // tgt->SetData(tgtindx++, msg.getData());
         tgtraw[tgtindx++] = idata;
      }

      if (hard_failure) continue;

      if (use_in_calibr) {
         coarse = msg.getHitTmCoarse();
         if (isrising) {
            if (chid>0) nrising++;
            rec.rising_stat[fine]++;
            rec.all_rising_stat++;
            rec.rising_last_tm = ((epoch << 11) | coarse) * 5e-9 - corr;
            rec.rising_new_value = true;
         } else {
            nfalling++;
            rec.falling_stat[fine]++;
            rec.all_falling_stat++;
            if (rec.rising_new_value) {
               double tot = ((((epoch << 11) | coarse) * 5e-9 - corr) - rec.rising_last_tm)*1e9;

               // DefFillH1(rec.fTot, tot, 1.);
               // add shift again to be on safe side when calculate new shift at the end
               rec.last_tot = tot + rec.tot_shift;
               rec.rising_new_value = false;
            }
         }

         // trigger check of calibration only when enough statistic in that channel
         // done only once for specified channel
         if (!check_calibr_progress && rec.docalibr && !rec.check_calibr && (fCalibrCounts > 0)) {
            long stat = CheckChannelStat(chid);
            // if ToT mode enabled, make first check at half of the statistic to make preliminary calibrations
            if (stat >= fCalibrCounts * ((fAllTotMode==0) ? 0.5 : 1.)) {
               rec.check_calibr = true;
               check_calibr_progress = true;
            }
         }
      }

      if (fast_loop) continue;

      FastFillH1(fChannels, chid);
      FastFillH1(fHits, (chid*2 + (isrising ? 0 : 1)));
      DefFastFillH2(fAllFine, chid, fine);
      DefFastFillH2(fAllCoarse, chid, coarse);
      if ((HistFillLevel()>2) && ((rec.fRisingFine==0) || (rec.fFallingFine==0)))
         CreateChannelHistograms(chid);

      if (isrising) {
         FastFillH1(rec.fRisingFine, fine);
      } else {
         FastFillH1(rec.fFallingFine, fine);
      }
   }

   // if last calibration message not yet copied into output
   if ((calibr_num == 1) && tgtraw && calibr_indx) {
      tgtraw[calibr_indx] = HADAQ_SWAP4(calibr.getData());
      calibr_indx = 0;
   }

   if (hitcnt) DefFastFillH1(fMsgsKind, hadaq::tdckind_Hit >> 29, hitcnt);
   if (epochcnt) DefFastFillH1(fMsgsKind, hadaq::tdckind_Epoch >> 29, epochcnt);

   if (cnt && fMsgPerBrd) FastFillH1(*fMsgPerBrd, fSeqeunceId, cnt);
   // fill number of "good" hits
   if (hitcnt && fHitsPerBrd) FastFillH1(*fHitsPerBrd, fSeqeunceId, hitcnt);
   if (errcnt && fErrPerBrd) FastFillH1(*fErrPerBrd, fSeqeunceId, errcnt);

   if ((hitcnt>0) && use_in_calibr && (fCurrentTemp>0)) {
      fCalibrTempSum0 += 1.;
      fCalibrTempSum1 += fCurrentTemp;
      fCalibrTempSum2 += fCurrentTemp*fCurrentTemp;
   }

   if (fAllCalibrMode > 0) {
      // detect 0xD trigger ourself
      if (!is_0d_trig && (nrising == nfalling) && (nrising+1 == NumChannels())) is_0d_trig = true;

      if (is_0d_trig) fAllDTrigCnt++;
      if ((fAllDTrigCnt>10) && (fAllTotMode<0)) fAllTotMode = 0;
   }

   // do ToT calculations only when preliminary calibration was produced
   bool do_tot = use_in_calibr && is_0d_trig && DoFallingEdge() && (fAllTotMode==1);

   if (do_tot)
      for (unsigned ch=1;ch<NumChannels();ch++) {
         ChannelRec& rec = fCh[ch];

         if (rec.hascalibr && (rec.last_tot >= fToThmin) && (rec.last_tot < fToThmax)) {
            int bin = (int) ((rec.last_tot - fToThmin) / (fToThmax - fToThmin) * TotBins);
            rec.tot0d_hist[bin]++;
            rec.tot0d_cnt++;
         }

         rec.last_tot = 0.;
         rec.rising_new_value = false;
      }

   if (check_calibr_progress) {
      fCalibrProgress = TestCanCalibrate();
      fCalibrQuality = (fCalibrProgress > 2) ? 0.9 : 0.7 + fCalibrProgress*0.1;

      if ((fAllTotMode == 0) && (fCalibrProgress>=0.5)) {
         ProduceCalibration(false, fUseLinear, false, true);
         fAllTotMode = 1; // now can start accumulate ToT values
      }

      if ((fCalibrProgress>=1.) && fAutoCalibr) PerformAutoCalibrate();
   }

   return tgt ? (tgtindx - tgtindx0) : cnt;
}

void hadaq::TdcProcessor::EmulateTransform(int dummycnt)
{
   if (fAllCalibrMode>0) {
      fCalibrProgress = dummycnt*1e-3;
      fCalibrQuality = (fCalibrProgress > 2) ? 0.9 : 0.7 + fCalibrProgress*0.1;
   }
}


void PrintBubble(unsigned* bubble) {
   // print in original order
   // for (unsigned d=BUBBLE_SIZE;d>0;d--) printf("%04x",bubble[d-1]);

   // print in reverse order
   for (unsigned d=0;d<BUBBLE_SIZE;d++) {
      unsigned origin = bubble[d], swap = 0;
      for (unsigned dd = 0;dd<16;++dd) {
         swap = (swap << 1) | (origin & 1);
         origin = origin >> 1;
      }
      printf("%04x",swap);
   }
}

void PrintBubbleBinary(unsigned* bubble, int p1 = -1, int p2 = -1) {
   if (p1<0) p1 = 0;
   if (p2<=p1) p2 = BUBBLE_SIZE*16;

   int pos = 0;
   char sbuf[1000];
   char* ptr  = sbuf;

   for (unsigned d=0;d<BUBBLE_SIZE;d++) {
      unsigned origin = bubble[d];
      for (unsigned dd = 0;dd<16;++dd) {
         if ((pos>=p1) && (pos<=p2))
            *ptr++ = (origin & 0x1) ? '1' : '0';
         origin = origin >> 1;
         pos++;
      }
   }

   *ptr++ = 0;
   printf(sbuf);
}

unsigned BubbleCheck(unsigned* bubble, int &p1, int &p2) {
   p1 = 0; p2 = 0;

   unsigned pos = 0, last = 1, nflip = 0;

   int b1 = 0, b2 = 0;

   unsigned fliparr[BUBBLE_SIZE*16];

   for (unsigned n=0;n<BUBBLE_SIZE; n++) {
      unsigned data = bubble[n] & 0xFFFF;
      if (n < BUBBLE_SIZE-1) data = data | ((bubble[n+1] & 0xFFFF) << 16); // use word to recognize bubble

      // this is error - first bit always 1
      if ((n==0) && ((data & 1) == 0)) { return -1; }

      for (unsigned b=0;b<16;b++) {
         if ((data & 1) != last) {
            if (last==1) {
               if (p1==0) p1 = pos; // take first change from 1 to 0
            } else {
               p2 = pos; // use last change from 0 to 1
            }
            nflip++;
         }

         fliparr[pos] = nflip; // remember flip counts to analyze them later

         // check for simple bubble at the beginning 1101000 or 0x0B in swapped order
         // set position on last 1 ? Expecting following sequence
         //  1110000 - here pos=4
         //     ^
         //  1110100 - here pos=5
         //      ^
         //  1111100 - here pos=6
         //       ^
         if ((data & 0xFF) == 0x0B) b1 = pos+3;

         // check for simple bubble at the end 00001011 or 0xD0 in swapped order
         // set position of 0 in bubble, expecting such sequence
         //  0001111 - here pos=4
         //     ^
         //  0001011 - here pos=5
         //      ^
         //  0000011 - here pos=6
         //       ^
         if ((data & 0xFF) == 0xD0) b2 = pos+5;

         // simple bubble at very end 00000101 or 0xA0 in swapped order
         // here not enough space for two bits
         if (((pos == BUBBLE_SIZE*16 - 8)) && (b2 == 0) && ((data & 0xFF) == 0xA0))
            b2 = pos + 6;

         last = (data & 1);
         data = data >> 1;
         pos++;
      }
   }

   if (nflip == 2) return 0; // both are ok

   if ((nflip == 4) && (b1>0) && (b2==0)) { p1 = b1; return 0x10; } // bubble in the begin

   if ((nflip == 4) && (b1==0) && (b2>0)) { p2 = b2; return 0x01; } // bubble at the end

   if ((nflip == 6) && (b1>0) && (b2>0)) { p1 = b1; p2 = b2; return 0x11; } // bubble on both side

   // up to here was simple errors, now we should do more complex analysis

   if (p1 < p2 - 8) {
      // take flip count at the middle and check how many transitions was in between
      int mid = (p2+p1)/2;
      // hard error in the beginning
      if (fliparr[mid] + 1 == fliparr[p2]) return 0x20;
      // hard error in begin, bubble at the end
      if ((fliparr[mid] + 3 == fliparr[p2]) && (b2>0)) { p2 = b2; return 0x21; }

      // hard error at the end
      if (fliparr[p1] == fliparr[mid]) return 0x02;
      // hard error at the end, bubble at the begin
      if ((fliparr[p1] + 2 == fliparr[mid]) && (b1>0)) { p1 = b1; return 0x12; }
   }

   return 0x22; // mark both as errors, should analyze better
}


unsigned* rom_encoder_rising = 0;
unsigned rom_encoder_rising_cnt = 0;
unsigned* rom_encoder_falling = 0;
unsigned rom_encoder_falling_cnt = 0;

// this is how it used in the FPGA, table provided by Cahit

unsigned BubbleCheckCahit(unsigned* bubble, int &p1, int &p2, bool debug = false, int maskid = 0, int shift = 0, bool = false) {

   if (rom_encoder_rising == 0) {

      rom_encoder_rising = new unsigned[400];
      rom_encoder_falling = new unsigned[400];

      const char* fname = "rom_encoder_bin.mem";

      FILE* f = fopen(fname, "r");
      if (!f) return -1;

      char sbuf[1000], ddd[100], *res;
      unsigned  *tgt = 0, *tgtcnt = 0;

      unsigned mask, code;
      int num, dcnt = 1;

      while ((res = fgets(sbuf, sizeof(sbuf)-1, f)) != 0) {
         if (strlen(res) < 5) continue;
         if (strstr(res,"Rising")) { tgt = rom_encoder_rising; tgtcnt = &rom_encoder_rising_cnt; continue; }
         if (strstr(res,"Falling")) { tgt = rom_encoder_falling; tgtcnt = &rom_encoder_falling_cnt; continue; }
         const char* comment = strstr(res,"#");
         if (comment && ((comment-res)<5)) continue;

         num = sscanf(res,"%x %s : %u", &mask, ddd, &code);

         if ((num!=3) || (tgt==0)) continue;

         //unsigned swap = 0;
         //for (unsigned k=0;k<9;++k)
         //   if (mask & (1<<k)) swap = swap | (1<<(8-k));
         // printf("read: 0x%03x 0x%03x %u   %s  ", mask, swap, code, res);


         if (maskid == dcnt) {
            printf("SHIFT MASK %03x : %u + %d\n", mask, code, shift);
            if ((int)code + shift >= 0) code += shift;
         }

         tgt[(*tgtcnt)++] = mask;
         tgt[(*tgtcnt)++] = code;
         dcnt++;

         if (*tgtcnt >= 400) { printf("TOO MANY ENTRIES!!!\n"); exit(43); }
      }

      printf("READ %u rising %u falling %d dcnt entries from %s\n", rom_encoder_rising_cnt, rom_encoder_falling_cnt, dcnt, fname);

      fclose(f);
   }

   if ((rom_encoder_rising_cnt == 0) || (rom_encoder_falling_cnt == 0)) return 0x22;

   int pos = 0;
   unsigned data = 0xFFFF0000;
   //unsigned selmask = 0;
   //bool seen_selected = false;

   //static int mcnt = 0;

   /*
   if (maskid>0) {
      if (maskid <= (int) rom_encoder_rising_cnt/2) selmask = rom_encoder_rising[(maskid-1)*2]; else
      if (maskid <= (int) (rom_encoder_rising_cnt + rom_encoder_falling_cnt)/2) selmask = rom_encoder_falling[(maskid-1)*2 - rom_encoder_rising_cnt];
      //if (mcnt++ < 5) printf("SELMASK 0x%x\n", selmask);
   }
   */

   p1 = 0; p2 = 0;

   for (unsigned n=0;n<BUBBLE_SIZE+2; n++) {

      if (n<BUBBLE_SIZE) {
         unsigned origin = bubble[n], swap = 0;
         for (unsigned dd = 0;dd<16;++dd) {
            swap = (swap << 1) | (origin & 1);
            origin = origin >> 1;
         }
         data = data | swap;
      } else {
         data = data | 0xFFFF;
      }

      for (int half=0;half<2;half++) {

         unsigned search = (data & 0x3FF0000) >> 16;

         if ((search != 0) && (search != 0x3FF)) {

//            if (search == selmask) seen_selected = true;

            if (debug) printf("search %04x pos %03d\n", search, pos);

            for (unsigned k=0;k<rom_encoder_rising_cnt;k+=2)
               if (rom_encoder_rising[k] == search) {
                  if (p2==0) p2 = pos - rom_encoder_rising[k+1] - 1;
                  // printf("rising  pos:%3u data %08x found 0x%03x shift: %u p2: %d\n", pos, data, search, rom_encoder_rising[k+1], p2);
               }

            for (unsigned k=0;k<rom_encoder_falling_cnt;k+=2)
               if (rom_encoder_falling[k] == search) {
                  if (p1==0) p1 = pos - rom_encoder_falling[k+1] - 1;
                  // printf("falling pos:%3u data %08x found 0x%03x shift: %u p1: %d\n", pos, data, search, rom_encoder_falling[k+1], p1);
               }
         }

         pos += 8;
         data = data << 8;
      }
   }

   // if (selmask && !seen_selected && useselmask) return 0x22; // ignore all messages if we do not see selected mask

   return ((p1>0) ? 0x00 : 0x20) | ((p2>0) ? 0x00 : 0x02);
}


bool hadaq::TdcProcessor::DoBubbleScan(const base::Buffer& buf, bool first_scan)
{

   TdcIterator& iter = first_scan ? fIter1 : fIter2;

   if (buf.datalen() < 4) { printf("ZERO\n"); exit(4); }

   if (buf().format==0)
      iter.assign((uint32_t*) buf.ptr(4), buf.datalen()/4-1, false);
   else
      iter.assign((uint32_t*) buf.ptr(0), buf.datalen()/4, buf().format==2);

   hadaq::TdcMessage& msg = iter.msg();

   unsigned lastch = 0xFFFF;
   unsigned bubble[(BUBBLE_SIZE*5)]; // use more memory for safety
   unsigned bcnt = 0, chid = 0;
   double localtm = 0;

   while (iter.next()) {

      if (first_scan) {
         if (!fMsgsKind || (msg.getKind() >> 29) >= 8) { printf("ERRRRRRRR\n"); exit(4); }

         FastFillH1(fMsgsKind, (msg.getKind() >> 29));
      }

      chid = 0xFFFF;

      switch (gBubbleMode) {
         case 2:
            if (!msg.isHitMsg()) continue;
            chid = (msg.getData() >> 22) & 0x7F;
            break;
         case 3: {
            if (msg.isHeaderMsg() && (msg.getHeaderFmt() != 0xF) && CheckPrintError())
               printf("Bubble 3 should be marked with 0xF, but seen 0x%x\n", msg.getHeaderFmt());

            if (msg.isHeaderMsg()) { fLastTdcHeader = msg; continue; }

            if (msg.getKind() == 0xe0000000) {
               chid = (msg.getData() >> 22) & 0x7F;
            } else
            if (msg.isTrailerMsg()) {
               fLastTdcTrailer = msg;
               chid = 0xFFFF; // artificially complete analysis of previous chain
            } else {
               if (CheckPrintError())
                  printf("Bubble 3 should have 0xe0000000 message but seen 0x%x\n", msg.getKind());
               continue;
            }

            break;
         }
      }

      if (chid != lastch) {
         if ((lastch != 0xFFFF) && (lastch < NumChannels()) && (bcnt==BUBBLE_SIZE)) {

            DefFillH1(fChannels, lastch, BUBBLE_SIZE);

            ChannelRec& rec = fCh[lastch];

            int p1, p2, pp1, pp2;

            // only channel 2 signals selects special fine-counter values, all other works as before

            unsigned res = BubbleCheckCahit(bubble, p1, p2, false, gBubbleMask, gBubbleShift, (lastch==2));

//            if (false)
//            if ((res == 0x22) || (((res & 0x0F) == 0x02) && ((p2<195) || (p2>225))) || (((res & 0xF0) == 0x20) && ((p1<195) || (p1>220)))) {

            //if (res != 0)  {
            if (res && ((gBubbleMask==0) || (lastch!=2)) && CheckPrintError() ) {

               //int pp1, pp2;
               //BubbleCheck(bubble, pp1, pp2);

               // only to detect limits for printout
               BubbleCheck(bubble, pp1, pp2);

               printf("%s ch:%2u ", GetName(), lastch);
               PrintBubble(bubble);
               printf(" p1:%3d %2d  p2:%3d %2d  ", pp1,  (p1-pp1), pp2, (p2-pp2));
               PrintBubbleBinary(bubble, pp1-3, pp2+3);
               printf("\n");

               // BubbleCheckCahit(bubble, p1, p2, true);
            }

            // if ((res & 0xF0) == 0x00) p1 -= 1;

            switch (res & 0xF0) {
               case 0x00: FastFillH1(rec.fBubbleFalling, p1); break;
               case 0x10: FastFillH1(rec.fBubbleFallingErr, p1); break;
               default: FastFillH1(rec.fBubbleFallingAll, p1); break;
            }

            switch (res & 0x0F) {
               case 0x00: FastFillH1(rec.fBubbleRising, p2); break;
               case 0x01: FastFillH1(rec.fBubbleRisingErr, p2); break;
               default: FastFillH1(rec.fBubbleRisingAll, p2); break;
            }

            rec.rising_hit_tm = 0;

            unsigned fine = p1 + p2;

            // artificial compensation for hits around errors

            if (res == 0x22) {
               fine = 0;
            } else
            if ((res & 0x0F) == 0x02) { p2 = round(rec.bubble_a + p1*rec.bubble_b); fine = p1 + p2; } else
            if ((res & 0xF0) == 0x20) { p1 = round((p2 - rec.bubble_a) / rec.bubble_b); fine = p1 + p2; }

            if (res != 0x22) {
               if ((res & 0x0F) == 0x02) FastFillH1(fBubbleErrDistr, p2);
               if ((res & 0xF0) == 0x20) FastFillH1(fBubbleErrDistr, p1);
            }

            if ((p2 < 16) || (p1 < 4)) fine = 0;

            // measure linear coefficients between leading and trailing edge
            if ((p2>15) && (p1 > 3) && ((res & 0x0F) < 0x02) && ((res & 0xF0) < 0x20)) {
               rec.sum0 += 1;
               rec.sumx1 += p1;
               rec.sumx2 += p1*p1;
               rec.sumy1 += p2;
               rec.sumxy += p1*p2;
            }

            // take only simple data
            if (fine > 0) {
               rec.rising_stat[fine]++;
               rec.all_rising_stat++;
               rec.rising_cnt++;
               FastFillH1(rec.fRisingFine, fine);
               localtm = -rec.rising_calibr[fine];

//               if ((p2>18) && (p2<204) && (p1>10) && (p1<200)) {
               rec.rising_last_tm = localtm;
               rec.rising_new_value = true;
               rec.rising_hit_tm = 1e-8 + localtm; // artificially shift all values on 10 ns to allow histogram filling
               rec.rising_coarse = 0;
               rec.rising_fine = fine;
//               }
            }
         }

         lastch = chid; bcnt = 0;
      }

      if (bcnt >= BUBBLE_SIZE*5) {
         printf("Too long BUBBLE %u\n", bcnt);
         return true;
      }
      bubble[bcnt++] = msg.getData() & 0xFFFF;
      lastch = chid;
   }

   if (first_scan) AfterFill();

   return true;
}



bool hadaq::TdcProcessor::DoBufferScan(const base::Buffer& buf, bool first_scan)
{
   if (buf.null()) {
      if (first_scan) printf("Something wrong - empty buffer should not appear in the first scan/n");
      return false;
   }

   if (gBubbleMode > 1)
      return DoBubbleScan(buf, first_scan);

   uint32_t syncid(0xffffffff);
   // copy first 4 bytes - it is syncid
   if (buf().format==0)
      memcpy(&syncid, buf.ptr(), 4);

   // if data could be used for calibration
   int use_for_calibr = first_scan && (((1 << buf().kind) & fCalibrTriggerMask) != 0) ? 1 : 0;

   // use in ref calculations only physical trigger, exclude 0xD or 0xE
   bool use_for_ref = buf().kind < (gUseDTrigForRef ? 0xE : 0xD);

   // disable taking last hit for trigger DD
   if ((use_for_calibr > 0) && (buf().kind == 0xD)) {
      if (IsTriggeredAnalysis() &&  (gTrigDWindowLow < gTrigDWindowHigh)) use_for_calibr = 3; // accept time stamps only for inside window
      // use_for_calibr = 2; // always use only last hit
   }


   // if data could be used for TOT calibration
   bool do_tot = (use_for_calibr>0) && (buf().kind == 0xD) && DoFallingEdge();

   // use temperature compensation only when temperature available
   bool do_temp_comp = fCalibrUseTemp && (fCurrentTemp > 0) && (fCalibrTemp > 0) && (fabs(fCurrentTemp - fCalibrTemp) < 30.);

   unsigned cnt(0), hitcnt(0);

   bool iserr(false), isfirstepoch(false), rawprint(false), missinghit(false), dostore(false);

   if (first_scan && IsTriggeredAnalysis() && IsStoreEnabled() && mgr()->HasTrigEvent()) {
      dostore = true;
      switch (GetStoreKind()) {
         case 1: {
            hadaq::TdcSubEvent* subevnt = new hadaq::TdcSubEvent(buf.datalen()/6);
            mgr()->AddToTrigEvent(GetName(), subevnt);
            pStoreVect = subevnt->vect_ptr();
            break;
         }
         case 2: {
            hadaq::TdcSubEventFloat* subevnt = new hadaq::TdcSubEventFloat(buf.datalen()/6);
            mgr()->AddToTrigEvent(GetName(), subevnt);
            pStoreFloat = subevnt->vect_ptr();
            break;
         }
         case 3: {
            hadaq::TdcSubEventDouble* subevnt = new hadaq::TdcSubEventDouble(buf.datalen()/6);
            mgr()->AddToTrigEvent(GetName(), subevnt);
            pStoreDouble = subevnt->vect_ptr();
            break;
         }

         default: break; // not supported
      }
   }

   //static int ddd = 0;
   //if (ddd++ % 10000 == 0) printf("%s dostore %d istriggered %d hasevt %d kind %d\n", GetName(), dostore, IsTriggeredAnalysis(), mgr()->HasTrigEvent(), GetStoreKind());

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
   hadaq::TdcMessage calibr, first_double_msg(0);
   unsigned ncalibr(20), temp(0), lowid(0), highid(0), fine1(0), fine0(0); // clear indicate that no calibration data present
   bool double_edges(false), accept_next_falling(true);

   while (iter.next()) {

      // if (!first_scan) msg.print();

      cnt++;

      if (first_scan)
         FastFillH1(fMsgsKind, msg.getKind() >> 29);

      if (cnt==1) {
         if (!msg.isHeaderMsg()) {
            if (!gDRICHReapir) {
               iserr = true;
               ADDERROR(errNoHeader, "Missing header message");
            }
         } else {

            if (first_scan) fLastTdcHeader = msg;
            //unsigned errbits = msg.getHeaderErr();
            //if (errbits && first_scan)
            //   if (CheckPrintError()) printf("%5s found error bits: 0x%x\n", GetName(), errbits);

            // printf("%s format %x\n", GetName(), msg.getHeaderFmt());
            // double_edges = (msg.getHeaderFmt() == 1);
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

         if (gDRICHReapir && (chid!=0)) {
            // try to filter out rising/falling hits in interval [25-35] ns
            if (!isrising) {
               if (!accept_next_falling) continue;
               accept_next_falling = false; // only single falling is accepted
            } else {
               accept_next_falling = false;
               hadaq::TdcMessage msg2;
               if (!iter.lookForwardMsg(msg2)) continue;
               if (!msg2.isHitMsg() || msg2.isHitRisingEdge() || (chid!=msg2.getHitChannel())) continue;
               if ((fine == 0x3ff) || (msg2.getHitTmFine() == 0x3fff)) continue;

               double tm1 = coarse * hadaq::TdcMessage::CoarseUnit() + hadaq::TdcMessage::SimpleFineCalibr(fine);
               double tm2 = msg2.getHitTmCoarse() * hadaq::TdcMessage::CoarseUnit() + hadaq::TdcMessage::SimpleFineCalibr(msg2.getHitTmFine());

               // printf("Ch: %u tm1 %g tm2 %g diff %5.2f\n", chid, tm1, tm2, (tm2-tm1)*1e9);

               if (((tm2-tm1)<25e-9) || ((tm2-tm1)>40e-9)) {
                  ADDERROR(errMisc, "Reject Ch: %u tm1 %g tm2 %g diff %5.2f", chid, tm1, tm2, (tm2-tm1)*1e9);
                  continue;
               }
               accept_next_falling = true;
            }
         }

         localtm = iter.getMsgTimeCoarse();

         if (chid >= NumChannels()) {
            ADDERROR(errChId, "Channel number %u bigger than configured %u", chid, NumChannels());
            iserr = true;
            continue;
         }

         if (double_edges) {
            if (first_double_msg.getData() == 0) {
               // do not analyze, just go further
               first_double_msg.assign(msg.getData());
               fine0 = 0; fine1 = 0;
               continue;
            }

            fine0 = first_double_msg.getHitTmFine();
            fine1 = fine;

            if ((chid != first_double_msg.getHitChannel()) ||
                (coarse != first_double_msg.getHitTmCoarse()) ||
                (isrising != first_double_msg.isHitRisingEdge())) {
               ADDERROR(errMismatchDouble, "Mismatch in double edges readout from channel %u", chid);
               first_double_msg.assign(0);
               continue;
            }

            first_double_msg.assign(0);

            // we just want to went through several checks
            if ((fine0==0x3FF) && (fine1==0x3FF)) fine = 0x3FF; else fine = 0;
         }

         if (!iter.isCurEpoch()) {
            // one expects epoch before each hit message, if not data are corrupted and we can ignore it
            ADDERROR(errEpoch, "Missing epoch for hit from channel %u", chid);
            iserr = true;
            if (fChErrPerHld) DefFillH2(*fChErrPerHld, fHldId, chid, 1);
            continue;
         }

         if (IsEveryEpoch())
            iter.clearCurEpoch();

         if (fine == 0x3FF) {
            if (first_scan) {
               // special case - missing hit, just ignore such value
               ADDERROR(err3ff, "Missing hit in channel %u fine counter is %x", chid, fine);

               missinghit = true;
               FastFillH1(fChannels, chid);
               FastFillH1(fUndHits, chid);

               if (fChErrPerHld) DefFillH2(*fChErrPerHld, fHldId, chid, 1);
            }
            continue;
         }

         ChannelRec& rec = fCh[chid];

         if (double_edges) {
            // try to reconstruct edges if some missing

            fine = fine0 + fine1;

            if (fine0 == 0x3FF) {
               fine = fine1 + round(fine1 * rec.bubble_b + rec.bubble_a);
               //printf("Got0 fine0:%03x fine1:%03x fine:%03x a:%6.4f b:%6.4f \n", fine0, fine1, fine,rec.bubble_a,rec.bubble_b);
            } else
            if (fine1 == 0x3FF) {
               fine = fine0 + round((fine0 - rec.bubble_a) / rec.bubble_b );
               //printf("Got1 fine0:%03x fine1:%03x fine:%03x a:%6.4f b:%6.4f \n", fine0, fine1, fine,rec.bubble_a,rec.bubble_b);
            } else {
               rec.sum0 += 1;
               rec.sumx1 += fine1;
               rec.sumx2 += fine1*fine1;
               rec.sumy1 += fine0;
               rec.sumxy += fine1*fine0;

               if (rec.sum0 > 1000) {
                  double meanx = rec.sumx1/rec.sum0;
                  double meany = rec.sumy1/rec.sum0;
                  double b = (rec.sumxy/rec.sum0 - meanx*meany)/ (rec.sumx2/rec.sum0 - meanx * meanx);
                  double a = meany - b* meanx;
                  // printf("Ch:%2d B:%6.4f A:%4.2f\n", chid, b, a);
                  rec.bubble_a = a;
                  rec.bubble_b = b;
                  rec.sum0 = rec.sumx1 = rec.sumx2 = rec.sumy1 = rec.sumxy = 0;
               }
            }
         }


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
         } else {
            if (fine >= fNumFineBins) {
               FastFillH1(fErrors, chid);
               if (fChErrPerHld) DefFillH2(*fChErrPerHld, fHldId, chid, 1);
               ADDERROR(errFine, "Fine counter %u out of allowed range 0..%u in channel %u", fine, fNumFineBins, chid);
               iserr = true;
               continue;
            }

            if (ncalibr < 2) {
               // use correction from special message
               corr = calibr.getCalibrFine(ncalibr++)*5e-9/0x3ffe;
               if (isrising) DefFillH2(fhRaisingFineCalibr, chid, calibr.getCalibrFine(ncalibr++), 1.);
               if (!isrising) corr *= 10.; // range for falling edge is 50 ns.
            } else {

               // main calibration for fine counter
               corr = ExtractCalibr(isrising ? rec.rising_calibr : rec.falling_calibr, fine);

               // apply TOT shift for falling edge (should it be also temp dependent)?
               if (!isrising) corr += rec.tot_shift*1e-9;

               // negative while value should be add to the stamp
               if (do_temp_comp) corr -= (fCurrentTemp + fTempCorrection - fCalibrTemp) * rec.time_shift_per_grad * 1e-9;
            }
         }

         // apply correction
         localtm -= corr;

         if ((chid==0) && (ch0time==0)) ch0time = localtm;

         if (IsTriggeredAnalysis()) {
            if (ch0time==0)
               ADDERROR(errCh0, "channel 0 time not found when first HIT in channel %u appears", chid);

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

            bool use_fine_for_stat = true;

            if (use_for_calibr == 3)
               use_fine_for_stat = (gTrigDWindowLow <= localtm*1e9) && (localtm*1e9 <= gTrigDWindowHigh);

            FastFillH1(fChannels, chid);
            DefFillH1(fHits, (chid + (isrising ? 0.25 : 0.75)), 1.);
            if (raw_hit && use_fine_for_stat) DefFillH2(fAllFine, chid, fine, 1.);
            DefFillH2(fAllCoarse, chid, coarse, 1.);

            if (fChHitsPerHld) DefFillH2(*fChHitsPerHld, fHldId, chid, 1);

            if (isrising) {
               // processing of rising edge
               if (raw_hit) {
                  switch (use_for_calibr) {
                     case 1:
                     case 3:
                        if (use_fine_for_stat) {
                           rec.rising_stat[fine]++;
                           rec.all_rising_stat++;
                        }
                        break;
                     case 2:
                        rec.last_rising_fine = fine;
                        break;
                  }
               }

               rec.rising_cnt++;
               if (raw_hit) {
                  if (use_fine_for_stat) FastFillH1(rec.fRisingFine, fine);
                  if (double_edges) {
                     DefFillH1(rec.fBubbleRising, fine0!=0x3ff ? fine0 : BUBBLE_SIZE*16 - 1 , 1.);
                     DefFillH1(rec.fBubbleFalling, fine1!=0x3ff ? fine1 : BUBBLE_SIZE*16 - 1, 1.);
                  }
               }

               bool print_cond = false;

               rec.rising_last_tm = localtm;
               rec.rising_new_value = true;

               if (use_for_ref && ((rec.rising_hit_tm == 0.) || fUseLastHit)) {
                  rec.rising_hit_tm = (chid > 0) ? localtm : ch0time;
                  rec.rising_coarse = coarse;
                  rec.rising_fine = fine;

                  if ((rec.rising_cond_prnt > 0) && (rec.reftdc == GetID()) &&
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
               if ((chid!=0) && (rec.refch==0) && (rec.reftdc == GetID()) && use_for_ref) {
                  rec.rising_ref_tm = localtm;

                  DefFillH1(rec.fRisingRef, (localtm*1e9), 1.);

                  if (IsPrintRawData() || print_cond)
                  printf("Difference rising %04x:%02u\t %04x:%02u\t %12.3f\t %12.3f\t %7.3f  coarse %03x - %03x = %4d  fine %03x %03x \n",
                          GetID(), chid, rec.reftdc, rec.refch,
                          localtm*1e9,  fCh[0].rising_hit_tm*1e9, localtm*1e9,
                          coarse, fCh[0].rising_coarse, (int) (coarse - fCh[0].rising_coarse),
                          fine, fCh[0].rising_fine);
               }

            } else {
               // processing of falling edge
               if (raw_hit) {
                  switch (use_for_calibr) {
                     case 1:
                        if (use_fine_for_stat) {
                           rec.falling_stat[fine]++;
                           rec.all_falling_stat++;
                        }
                        break;
                     case 2:
                        rec.last_falling_fine = fine;
                        break;
                  }
               }
               rec.falling_cnt++;

               if (raw_hit && use_fine_for_stat) FastFillH1(rec.fFallingFine, fine);

               if (rec.rising_new_value && (rec.rising_last_tm!=0)) {
                  double tot = (localtm - rec.rising_last_tm)*1e9;
                  // TODO chid
                  DefFillH2(fhTotVsChannel, chid, tot, 1.);
                  if (tot < 0. ) {
                      DefFillH1(fhTotMinusCounter, chid, 1.);
                  }
                  if (tot > 20.) {
                      DefFillH1(fhTotMoreCounter, chid, 1.);
                  }
                  DefFillH1(rec.fTot, tot, 1.);
                  rec.rising_new_value = false;

                  // use only raw hit
                  if (raw_hit && do_tot) rec.last_tot = tot + rec.tot_shift;
               }
            }

            if (!iserr) {
               hitcnt++;
               if ((minimtm==0) || (localtm < minimtm)) minimtm = localtm;

               if (dostore)
                  switch(GetStoreKind()) {
                     case 1:
                        pStoreVect->push_back(hadaq::TdcMessageExt(msg, (chid>0) ? localtm : ch0time));
                        break;
                     case 2:
                        if (chid>0)
                           pStoreFloat->push_back(hadaq::MessageFloat(chid, isrising, localtm*1e9));
                        break;
                     case 3:
                        pStoreDouble->push_back(hadaq::MessageDouble(chid, isrising, ch0time + localtm));
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
               switch(GetStoreKind()) {
                  case 1:
                     AddMessage(indx, (hadaq::TdcSubEvent*) fGlobalMarks.item(indx).subev, hadaq::TdcMessageExt(msg, chid>0 ? globaltm : ch0time));
                     break;
                  case 2:
                     if (chid>0)
                        AddMessage(indx, (hadaq::TdcSubEventFloat*) fGlobalMarks.item(indx).subev, hadaq::MessageFloat(chid, isrising, (globaltm - ch0time)*1e9));
                     break;
                  case 3:
                     AddMessage(indx, (hadaq::TdcSubEventDouble*) fGlobalMarks.item(indx).subev, hadaq::MessageDouble(chid, isrising, globaltm));
                     break;
               }
            }
         }

         continue;
      }

      // process other messages kinds

      switch (msg.getKind()) {
        case tdckind_Trailer:
           if (first_scan) fLastTdcTrailer = msg;
           break;
        case tdckind_Header: {
           // never come here - handle in very begin
           if (first_scan) fLastTdcHeader = msg;
           /*unsigned errbits = msg.getHeaderErr();
           if (errbits && first_scan) {
              if (CheckPrintError())
                 printf("%5s found error bits: 0x%x\n", GetName(), errbits);
           }
           */

           break;
        }
        case tdckind_Debug: {
           switch (msg.getDebugKind()) {
              case 0x0E: temp = msg.getDebugValue();  break;
              case 0x10: lowid = msg.getDebugValue(); break;
              case 0x11: highid = msg.getDebugValue(); break;
           }

           break;
        }
        case tdckind_Epoch:
           break;
        default:
           ADDERROR(errUncknHdr, "Unknown message header 0x%x", msg.getKind());
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

      // special case for 0xD trigger - use only last hit messages for accumulating statistic
      if (use_for_calibr == 2)
         for (unsigned ch=0;ch<NumChannels();ch++) {
            ChannelRec& rec = fCh[ch];
            if (rec.last_rising_fine > 0) {
               rec.rising_stat[rec.last_rising_fine]++;
               rec.all_rising_stat++;
               rec.last_rising_fine = 0;
            }
            if (rec.last_falling_fine > 0) {
               rec.falling_stat[rec.last_falling_fine]++;
               rec.all_falling_stat++;
               rec.last_falling_fine = 0;
            }
         }

      // when doing TOT calibration, use only last TOT value - before one could find other signals
      if (do_tot)
         for (unsigned ch=1;ch<NumChannels();ch++) {
            if (fCh[ch].hascalibr && (fCh[ch].last_tot >= fToThmin) && (fCh[ch].last_tot < fToThmax)) {
               int bin = (int) ((fCh[ch].last_tot - fToThmin) / (fToThmax - fToThmin) * TotBins);
               fCh[ch].tot0d_hist[bin]++;
               fCh[ch].tot0d_cnt++;
            }
            fCh[ch].last_tot = 0.;
         }

      if (lowid || highid) {
         unsigned id = (highid << 16) | lowid;

         if ((fDesignId != 0) && (fDesignId != id))
            ADDERROR(errDesignId, "mismatch in design id before:0x%x now:0x%x", fDesignId, id);

         fDesignId = id;
      }

      if ((temp!=0) && (temp < 2400)) {
         fCurrentTemp = temp/16.;

         if ((HistFillLevel() > 1) && (fTempDistr == 0))  {
            printf("%s FirstTemp:%5.2f CalibrTemp:%5.2f UseTemp:%d\n", GetName(), fCurrentTemp, fCalibrTemp, fCalibrUseTemp);

            int mid = round(fCurrentTemp);
            SetSubPrefix();
            fTempDistr = MakeH1("Temperature", "Temperature distribution", 600, mid-30, mid+30, "C");
         }
         FillH1(fTempDistr, fCurrentTemp);
      }

      if ((hitcnt>0) && (use_for_calibr>0) && (fCurrentTemp>0)) {
         fCalibrTempSum0 += 1.;
         fCalibrTempSum1 += fCurrentTemp;
         fCalibrTempSum2 += fCurrentTemp*fCurrentTemp;
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

      // number of hits per TDC in HLD
      if (fHitsPerHld) DefFillH1(*fHitsPerHld, fHldId, hitcnt);

      if (iserr || missinghit) {
         if (fErrPerBrd) DefFillH1(*fErrPerBrd, fSeqeunceId, 1.);
         if (fErrPerHld) DefFillH1(*fErrPerHld, fHldId, 1.);
      }
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

   //DoHistAnalysis();

   return !iserr;
}

void hadaq::TdcProcessor::DoHistAnalysis()
{
    int nofChannels = NumChannels();
    for (int iCh = 0; iCh < nofChannels; iCh++) {
        double testFine = DoTestFineTimeH2(iCh, fAllFine);
        double testTot = DoTestToT(iCh);
        double testEdges = DoTestEdges(iCh);
        double testErrors = DoTestErrors(iCh);
        int hValue = testFine;
        if (fQaFinePerHld) SetH2Content(*fQaFinePerHld, fHldId, iCh, testFine);
        if (fQaToTPerHld) SetH2Content(*fQaToTPerHld, fHldId, iCh, testTot);
        if (fQaEdgesPerHld) SetH2Content(*fQaEdgesPerHld, fHldId, iCh, testEdges);
        if (fQaErrorsPerHld) SetH2Content(*fQaErrorsPerHld, fHldId, iCh, testErrors);
    }
}

double hadaq::TdcProcessor::DoTestToT(int iCh)
{
    int nBins1, nBins2;
    GetH2NBins(fhTotVsChannel, nBins1, nBins2);

    int nBins = GetH1NBins(fhTotMoreCounter);
    if (iCh < 0 || iCh >= nBins) return 0.;
    double moreContent = GetH1Content(fhTotMoreCounter, iCh);
    double minusContent = GetH1Content(fhTotMinusCounter, iCh);

    double nEntries = 0.;
    for (int i = 0; i < nBins2; i++){
        nEntries += GetH2Content(fhTotVsChannel, iCh, i);
    }

    if (nEntries == 0) return 0.;
    double ratio = (moreContent + minusContent) / nEntries;
    double k = 1.;
    double result = 1. - k * ratio;
    if (result < 0.) return 0.;
    return (result > 1.)?100.:100 * result;
}

double hadaq::TdcProcessor::DoTestErrors(int iCh)
{
    int nBins = GetH1NBins(fErrors);
    if (iCh < 0 || iCh >= nBins) return 0.;
    double nEntries = 0.;
    for (int i = 0; i < nBins; i++){
        nEntries += GetH1Content(fErrors, i);
    }
    if (nEntries == 0) return 0.;
    double nofErrors = GetH1Content(fErrors, iCh);

    double ratio = nofErrors / nEntries;
    double k = 1.;
    double result = 1. - k * ratio;
    if (result < 0.) return 0.;
    return (result > 1.)?100.:100 * result;
}

double hadaq::TdcProcessor::DoTestEdges(int iCh)
{
    int nBins = GetH1NBins(fHits);
    if (iCh < 0 || iCh * 2  + 1 >= nBins) return false;
    double raising = GetH1Content(fHits, 2 * iCh);
    double falling = GetH1Content(fHits, 2 * iCh + 1);
    double ratio = (falling != 0)?raising / falling:0.;
    ratio = (ratio > 1.)?ratio - 1.:1. - ratio;
    double k = 1.;
    double result = 1. - k * ratio;
    if (result < 0.) return 0.;
    return (result > 1.)?100.:100 * result;
}

double hadaq::TdcProcessor::DoTestFineTimeH2(int iCh, base::H2handle h)
{
    int nBins1, nBins2;
    GetH2NBins(h, nBins1, nBins2);
    const int binSizeRebin = 32;
    const int nBinsRebin = nBins2 / binSizeRebin + 1;

    double hRebin[nBinsRebin];
    int nEntries = 0;
    for (int i = 0; i < nBinsRebin; i++){
        hRebin[i] = 0;
        for (int j = i * binSizeRebin; j < (i + 1) * binSizeRebin; j++){
            if (j >= nBins2) break;
            hRebin[i] += GetH2Content(h, iCh, j);
        }
        nEntries += hRebin[i];
    }
    return DoTestFineTime(hRebin, nBinsRebin, nEntries);
}

double hadaq::TdcProcessor::DoTestFineTime(double hRebin[], int nBinsRebin, int nEntries)
{
    if (nEntries < 100) return 0.;

    // shrink zero Edges
    int indMinRebin = 0;
    int indMaxRebin = 0;
    for (int i = 0; i < nBinsRebin; i++){
        if (hRebin[i] != 0){
            indMinRebin = i;
            break;
        }
    }
    for (int i = nBinsRebin - 1; i >= 0; i--){
        if (hRebin[i] != 0){
            indMaxRebin = i;
            break;
        }
    }
    int nBinsRebinShrinked = indMaxRebin - indMinRebin + 1;

    double mean = 0.;
    for (int i = indMinRebin; i <= indMaxRebin; i++) {
        mean += hRebin[i];
    }
    mean = mean / (double)nBinsRebinShrinked;
    if (mean == 0) return 0.;

    double min = 1.e10;
    double max = 0.;
    double mad = 0.;
    double nBinsMeanMore = 0;
    for (int i = indMinRebin; i <= indMaxRebin; i++) {
        if (hRebin[i] < min) min = hRebin[i];
        if (hRebin[i] > max) max = hRebin[i];
        if (hRebin[i] > mean) nBinsMeanMore++;
        mad += std::abs(hRebin[i] - mean);
    }
    mad /= nBinsRebinShrinked;
    nBinsMeanMore /= nBinsRebinShrinked;

    double madMean = (mad/mean > 1.)?1.:mad/mean;
    double k = 1.;
    double result = 1. - k * madMean;
    if (result < 0.) return 0.;
    return (result > 1.)?100.:100 * result;
}

void hadaq::TdcProcessor::AppendTrbSync(uint32_t syncid)
{
   if (fQueue.size() == 0) {
      printf("something went wrong!!!\n");
      exit(765);
   }

   memcpy(fQueue.back().ptr(), &syncid, 4);
}

void hadaq::TdcProcessor::SetLinearCalibration(unsigned nch, unsigned finemin, unsigned finemax)
{
   if (nch>=NumChannels()) return;

   for (unsigned fine=0;fine<fNumFineBins;++fine) {
      float value = 0;
      if (fine<=finemin) value = 0; else
      if (fine>=finemax) value = hadaq::TdcMessage::CoarseUnit(); else
      value = hadaq::TdcMessage::CoarseUnit() * (fine - finemin) / (0. + finemax - finemin);
      fCh[nch].rising_calibr[fine] = value;
      fCh[nch].falling_calibr[fine] = value;
   }
}


bool hadaq::TdcProcessor::CalibrateChannel(unsigned nch, long* statistic, float* calibr, bool use_linear, bool preliminary)
{
   double sum(0.), limits(use_linear ? 100 : 1000);
   unsigned finemin(0), finemax(0);
   for (unsigned n=0;n<fNumFineBins;n++) {
      if (statistic[n]) {
         if (sum == 0.) finemin = n; else finemax = n;
      }

      sum += statistic[n];
      calibr[n] = sum;
   }

/*   if (sum <= limits) {
      if (sum>0)
         printf("%s Ch:%u Too few counts %5.0f for calibration of fine counter, use predefined\n", GetName(), nch, sum);
   } else {
      printf("%s Ch:%u Cnts: %7.0f produce %s calibration min:%u max:%u\n", GetName(), nch, sum, (use_linear ? "linear" : "normal"),  finemin, finemax);
   }
*/

   if (!preliminary && (finemin>50) && (fCalibrQuality>0.4)) {
      fCalibrStatus = "BadFineMin"; fCalibrStatus+=finemin;
      fCalibrQuality = 0.4;
   }

   if (!preliminary && (finemax<400) && (fCalibrQuality>0.4)) {
      fCalibrStatus = "BadFineMax"; fCalibrStatus+=finemax;
      fCalibrQuality = 0.4;
   }

   if (sum <= limits) {

      if ((fCalibrQuality>0.15) && !preliminary)  {
         fCalibrStatus = "LowStat";
         fCalibrQuality = 0.15;
      }

      for (unsigned n=0;n<fNumFineBins;n++)
         calibr[n] = hadaq::TdcMessage::SimpleFineCalibr(n);

      return false;
   }

   double sum1 = 0., sum2 = 0., linear = 0, exact = 0.;

   for (unsigned n=0;n<fNumFineBins;n++) {
      if (n<=finemin) linear = 0.; else
         if (n>=finemax) linear = 1.; else linear = (n - finemin) / (0. + finemax - finemin);

      sum1 += 1.;

      if (use_linear) {
         exact = linear;
      } else {
         exact = (calibr[n] - statistic[n]/2) / sum;
         sum2 += (exact - linear) * (exact - linear);
      }

      calibr[n] = exact * hadaq::TdcMessage::CoarseUnit();
   }


   if (!preliminary && (fCalibrQuality>0.6) && !use_linear && (sum1>100)) {
      double dev = sqrt(sum2/sum1); // average deviation

      printf("%s ch %u deviation %5.4f\n", GetName(), nch, dev);
      if (dev > 0.05) {
         fCalibrStatus = "NonLinear";
         fCalibrQuality = 0.6;
      }
   }

   return true;
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
      double x = fToThmin + (n + 0.5) / TotBins * (fToThmax-fToThmin); // x coordinate of center bin

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

   if (rms > 0.15) {
      printf("%s Ch:%u TOT failed - RMS %5.3f too high\n", GetName(), nch, rms);
      return false;
   }

   tot_shift = mean - fToTvalue;

   printf("%s Ch:%u TOT: %6.3f rms: %5.3f offset %6.3f\n", GetName(), nch, mean, rms, tot_shift);

   return true;
}


void hadaq::TdcProcessor::CopyCalibration(float* calibr, base::H1handle hcalibr, unsigned ch, base::H2handle h2calibr)
{
   ClearH1(hcalibr);

   for (unsigned n=0;n<fNumFineBins;n++) {
      DefFillH1(hcalibr, n, calibr[n]*1e12);
      DefFillH2(h2calibr, ch, n, calibr[n]*1e12);
   }
}

void hadaq::TdcProcessor::ProduceCalibration(bool clear_stat, bool use_linear, bool dummy, bool preliminary)
{
   if (!preliminary) {
      if (fCalibrProgress >= 1) {
         fCalibrStatus = "Ready";
         fCalibrQuality = 1.;
      } else if (fCalibrProgress >= 0.3) {
         fCalibrStatus = "LowStat";
         fCalibrQuality = 0.5;
      } else {
         fCalibrStatus = "BadStat";
         fCalibrQuality = 0.2;
      }
   }

   if (dummy) return;

   if (!preliminary)
      printf("%s produce %s calibrations \n", GetName(), (use_linear ? "linear" : "normal"));

   if (fCalibrTempSum0 > 5) {
      double mean = fCalibrTempSum1/fCalibrTempSum0;
      double rms = fCalibrTempSum2 / fCalibrTempSum0 - mean*mean;
      if (rms>0) rms = sqrt(rms); else rms = (rms >=-1e-8) ? 0 : -1;
      printf("   temp %5.2f +- %3.2f during calibration\n", mean, rms);
      if ((rms>0) && (rms<3)) fCalibrTemp = mean;
   }

   fCalibrTempSum0 = 0;
   fCalibrTempSum1 = 0;
   fCalibrTempSum2 = 0;

   if (!preliminary) {
      ClearH2(fRisingCalibr);
      ClearH2(fFallingCalibr);
      ClearH1(fTotShifts);
   }

   for (unsigned ch=0;ch<NumChannels();ch++) {

      if ((gBubbleMode>1) && (fCh[ch].sum0 > 100)) {
         ChannelRec& rec = fCh[ch];

         double meanx = rec.sumx1/rec.sum0;
         double meany = rec.sumy1/rec.sum0;

         double b = (rec.sumxy/rec.sum0 - meanx*meany)/ (rec.sumx2/rec.sum0 - meanx * meanx);
         double a = meany - b* meanx;

         printf("Ch:%2d B:%6.4f A:%4.2f\n", ch, b, a);

         rec.bubble_a = a;
         rec.bubble_b = b;
      }

      if (fCh[ch].docalibr) {

         fCh[ch].check_calibr = false; // reset flag, used in auto calibration

         // special case - use common statistic
         if (fEdgeMask == edge_CommonStatistic) {
            fCh[ch].all_rising_stat += fCh[ch].all_falling_stat;
            fCh[ch].all_falling_stat = 0;
            for (unsigned n=0;n<fNumFineBins;n++) {
               fCh[ch].rising_stat[n] += fCh[ch].falling_stat[n];
               fCh[ch].falling_stat[n] = 0;
            }
         }

         bool res = false;

         if (DoRisingEdge() && (fCh[ch].all_rising_stat>0))
            res = CalibrateChannel(ch, fCh[ch].rising_stat, fCh[ch].rising_calibr, use_linear, preliminary);

         if (DoFallingEdge() && (fCh[ch].all_falling_stat>0) && (fEdgeMask == edge_BothIndepend))
            if (!CalibrateChannel(ch, fCh[ch].falling_stat, fCh[ch].falling_calibr, use_linear, preliminary)) res = false;

         if ((ch>0) && DoFallingEdge() && (fCh[ch].tot0d_cnt > 100) && !preliminary) {
            CalibrateTot(ch, fCh[ch].tot0d_hist, fCh[ch].tot_shift, 0.05);
            for (unsigned n=0;n<TotBins;n++) {
               double x = fToThmin + (n + 0.1) / TotBins * (fToThmax-fToThmin);
               DefFillH1(fCh[ch].fTot0D, x, fCh[ch].tot0d_hist[n]);
            }
         }

         fCh[ch].hascalibr = res;

         if ((fEdgeMask == edge_CommonStatistic) || (fEdgeMask == edge_ForceRising))
            for (unsigned n=0;n<fNumFineBins;n++)
               fCh[ch].falling_calibr[n] = fCh[ch].rising_calibr[n];

         if (clear_stat && !preliminary)
            ClearChannelStat(ch);
      }

      if (!preliminary) {
         if (DoRisingEdge())
            CopyCalibration(fCh[ch].rising_calibr, fCh[ch].fRisingCalibr, ch, fRisingCalibr);
         if (DoFallingEdge())
            CopyCalibration(fCh[ch].falling_calibr, fCh[ch].fFallingCalibr, ch, fFallingCalibr);
         DefFillH1(fTotShifts, ch, fCh[ch].tot_shift);
      }
   }
}

void hadaq::TdcProcessor::ClearChannelStat(unsigned ch)
{
   for (unsigned n=0;n<fNumFineBins;n++) {
      fCh[ch].falling_stat[n] = 0;
      fCh[ch].rising_stat[n] = 0;
   }
   fCh[ch].all_falling_stat = 0;
   fCh[ch].all_rising_stat = 0;
   fCh[ch].tot0d_cnt = 0;
   for (unsigned n=0;n<TotBins;n++)
      fCh[ch].tot0d_hist[n] = 0;

}

void hadaq::TdcProcessor::StoreCalibration(const std::string& fprefix, unsigned fileid)
{
   if (fprefix.empty()) return;

   if (fileid == 0) fileid = GetID();
   char fname[1024];
   snprintf(fname, sizeof(fname), "%s%04x.cal", fprefix.c_str(), fileid);

   FILE* f = fopen(fname,"w");
   if (f==0) {
      printf("%s Cannot open file %s for writing calibration\n", GetName(), fname);
      return;
   }

   uint64_t num = NumChannels();

   fwrite(&num, sizeof(num), 1, f);

   // calibration curves
   for (unsigned ch=0;ch<NumChannels();ch++) {
      fwrite(fCh[ch].rising_calibr, sizeof(float)*fNumFineBins, 1, f);
      fwrite(fCh[ch].falling_calibr, sizeof(float)*fNumFineBins, 1, f);
   }

   // tot shifts
   for (unsigned ch=0;ch<NumChannels();ch++) {
      fwrite(&(fCh[ch].tot_shift), sizeof(fCh[ch].tot_shift), 1, f);
   }

   // temperature
   fwrite(&fCalibrTemp, sizeof(fCalibrTemp), 1, f);
   fwrite(&fCalibrTempCoef, sizeof(fCalibrTempCoef), 1, f);

   for (unsigned ch=0;ch<NumChannels();ch++) {
      fwrite(&(fCh[ch].time_shift_per_grad), sizeof(fCh[ch].time_shift_per_grad), 1, f);
      fwrite(&(fCh[ch].trig0d_coef), sizeof(fCh[ch].trig0d_coef), 1, f);
      fwrite(&(fCh[ch].bubble_a), sizeof(fCh[ch].bubble_a), 1, f);
      fwrite(&(fCh[ch].bubble_b), sizeof(fCh[ch].bubble_b), 1, f);
   }

   fclose(f);

   printf("%s storing calibration in %s\n", GetName(), fname);
}

bool hadaq::TdcProcessor::LoadCalibration(const std::string& fprefix)
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
        fseek(f, 2*sizeof(float)*fNumFineBins, SEEK_CUR);
        continue;
     }

     fCh[ch].hascalibr =
        (fread(fCh[ch].rising_calibr, sizeof(float)*fNumFineBins, 1, f) == 1) &&
        (fread(fCh[ch].falling_calibr, sizeof(float)*fNumFineBins, 1, f) == 1);

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

      if (!feof(f)) {
         fread(&fCalibrTemp, sizeof(fCalibrTemp), 1, f);
         fread(&fCalibrTempCoef, sizeof(fCalibrTempCoef), 1, f);

         for (unsigned ch=0;ch<NumChannels();ch++)
            if (!feof(f)) {
               fread(&(fCh[ch].time_shift_per_grad), sizeof(fCh[ch].time_shift_per_grad), 1, f);
               fread(&(fCh[ch].trig0d_coef), sizeof(fCh[ch].trig0d_coef), 1, f);
               fread(&(fCh[ch].bubble_a), sizeof(fCh[ch].bubble_a), 1, f);
               fread(&(fCh[ch].bubble_b), sizeof(fCh[ch].bubble_b), 1, f);
            }
      }
   }

   fclose(f);

   // workaround for testing, remove later !!!
   if (strstr(fname, "cal/global_")!=0)
      switch (GetID()) {
         case 0x941: fTempCorrection = -1.58; break;
         case 0x942: fTempCorrection = -1.68; break;
         case 0x943: fTempCorrection = -1.32; break;
         default: fTempCorrection = 0; break;
      }

   printf("%s reading calibration from %s, tcorr:%5.1f uset:%d done\n", GetName(), fname, fTempCorrection, fCalibrUseTemp);

   fCalibrStatus = "CalibrFile";

   fCalibrQuality = 0.99;

   return true;
}

void hadaq::TdcProcessor::IncCalibration(unsigned ch, bool rising, unsigned fine, unsigned value)
{
   if ((ch>=NumChannels()) || (fine>=fNumFineBins)) return;

   if (rising && DoRisingEdge()) {
      fCh[ch].rising_stat[fine] += value;
      fCh[ch].all_rising_stat += value;
   }

   if (!rising && DoFallingEdge()) {
      fCh[ch].falling_stat[fine] += value;
      fCh[ch].all_falling_stat += value;
   }
}

void hadaq::TdcProcessor::CreateBranch(TTree*)
{
   switch(GetStoreKind()) {
      case 1:
         pStoreVect = &fDummyVect;
         mgr()->CreateBranch(GetName(), "std::vector<hadaq::TdcMessageExt>", (void**) &pStoreVect);
         break;
      case 2:
         pStoreFloat = &fStoreFloat;
         mgr()->CreateBranch(GetName(), "std::vector<hadaq::MessageFloat>", (void**) &pStoreFloat);
         break;
      case 3:
         pStoreDouble = &fStoreDouble;
         mgr()->CreateBranch(GetName(), "std::vector<hadaq::MessageDouble>", (void**) &pStoreDouble);
         break;
      default:
         break;
   }
}

void hadaq::TdcProcessor::Store(base::Event* ev)
{
   // in case of triggered analysis all pointers already set
   if (!ev || IsTriggeredAnalysis()) return;

   base::SubEvent* sub0 = ev->GetSubEvent(GetName());
   if (!sub0) return;

   switch (GetStoreKind()) {
      case 1: {
         hadaq::TdcSubEvent* sub = dynamic_cast<hadaq::TdcSubEvent*> (sub0);
         // when subevent exists, use directly pointer on messages vector
         pStoreVect = sub ? sub->vect_ptr() : &fDummyVect;
         break;
      }
      case 2: {
         hadaq::TdcSubEventFloat* sub = dynamic_cast<hadaq::TdcSubEventFloat*> (sub0);
         // when subevent exists, use directly pointer on messages vector
         pStoreFloat = sub ? sub->vect_ptr() : &fStoreFloat;
         break;
      }
      case 3: {
         hadaq::TdcSubEventDouble* sub = dynamic_cast<hadaq::TdcSubEventDouble*> (sub0);
         // when subevent exists, use directly pointer on messages vector
         pStoreDouble = sub ? sub->vect_ptr() : &fStoreDouble;
         break;
      }
   }
}
