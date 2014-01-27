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
   base::StreamProc("TDC_%04x", tdcid, false),
   fTrb(trb),
   fIter1(),
   fIter2(),
   fEdgeMask(edge_mask),
   fAutoCalibration(0),
   fPrintRawData(false),
   fEveryEpoch(false),
   fCrossProcess(false),
   fUseLastHit(false),
   fUseNativeTrigger(false),
   fCompensateEpochReset(false),
   fCompensateEpochCounter(0)
{

   if (trb) {
      trb->AddSub(this, tdcid);
      // when normal trigger is used as sync points, than use trigger time on right side to calculate global time
      if (trb->IsUseTriggerAsSync()) SetSynchronisationKind(sync_Right);
      if ((trb->NumSubProc()==1) && trb->IsUseTriggerAsSync()) fUseNativeTrigger = true;
      fPrintRawData = trb->IsPrintRawData();
      fCrossProcess = trb->IsCrossProcess();
      fCompensateEpochReset = trb->fCompensateEpochReset;

      SetHistFilling(trb->HistFillLevel());
   }

   fMsgPerBrd = trb ? trb->fMsgPerBrd : 0;
   fErrPerBrd = trb ? trb->fErrPerBrd : 0;
   fHitsPerBrd = trb ? trb->fHitsPerBrd : 0;

   fChannels = 0;
   fErrors = 0;
   fUndHits = 0;
   fMsgsKind = 0;
   fAllFine = 0;
   fAllCoarse = 0;

   if (HistFillLevel() > 1) {
      fChannels = MakeH1("Channels", "TDC channels", numchannels, 0, numchannels, "ch");
      fErrors = MakeH1("Errors", "Errors in TDC channels", numchannels, 0, numchannels, "ch");
      fUndHits = MakeH1("UndetectedHits", "Undetected hits in TDC channels", numchannels, 0, numchannels, "ch");

      fMsgsKind = MakeH1("MsgKind", "kind of messages", 8, 0, 8, "xbin:Reserved,Header,Debug,Epoch,Hit,-,-,-;kind");

      fAllFine = MakeH1("FineTm", "fine counter value", 1024, 0, 1024, "fine");
      fAllCoarse = MakeH1("CoarseTm", "coarse counter value", 2048, 0, 2048, "coarse");
   }

   for (unsigned ch=0;ch<numchannels;ch++) {
      fCh.push_back(ChannelRec());
   }

   // always create histograms for channel 0
   CreateChannelHistograms(0);

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
   if ((HistFillLevel() < 3) || (ch>=NumChannels())) return false;

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
   if ((ch>=NumChannels()) || (HistFillLevel()<4)) return;
   fCh[ch].refch = refch;
   fCh[ch].reftdc = (reftdc >= 0xffff) ? GetID() : reftdc;

   CreateChannelHistograms(ch);
   if (fCh[ch].reftdc == GetID()) CreateChannelHistograms(refch);

   char sbuf[1024], saxis[1024], refname[1024];
   if (fCh[ch].reftdc == GetID()) {
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
            sprintf(sbuf, "double correlation to TDC%u Ch%u", reftdc, refch);
            sprintf(saxis, "ch%u-ch%u ns;tdc%u refch%u ns", ch, fCh[ch].refch, reftdc, refch);
         }

         fCh[ch].fRisingDoubleRef = MakeH2("RisingDoubleRef", sbuf, npx, xmin, xmax, npy, ymin, ymax, saxis);
      }
   }

   return true;
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

   if (DoRisingEdge()) {
      fCh[ch].fRisingRefCond = MakeC1("RisingRefPrint", left, right, fCh[ch].fRisingRef);
      fCh[ch].rising_cond_prnt = numprint > 0 ? numprint : 100000000;
   }

   if (DoFallingEdge()) {
      fCh[ch].fFallingRefCond = MakeC1("FallingRefPrint", left, right, fCh[ch].fFallingRef);
      fCh[ch].falling_cond_prnt = numprint > 0 ? numprint : 100000000;
   }

   return true;
}


void hadaq::TdcProcessor::BeforeFill()
{
   for (unsigned ch=0;ch<NumChannels();ch++) {
      fCh[ch].rising_hit_tm = 0;
      fCh[ch].rising_ref_tm = 0.;
      fCh[ch].falling_hit_tm = 0;
   }
}

void hadaq::TdcProcessor::AfterFill(SubProcMap* subprocmap)
{
   // complete logic only when hist level is specified
   if (HistFillLevel()>=4)
   for (unsigned ch=0;ch<NumChannels();ch++) {

      unsigned ref = fCh[ch].refch;
      unsigned reftdc = fCh[ch].reftdc;
      if (reftdc>=0xffff) reftdc = GetID();

      TdcProcessor* refproc = 0;
      if (reftdc == GetID()) refproc = this; else
      if (subprocmap!=0) {
         SubProcMap::iterator iter = subprocmap->find(reftdc);
         if (iter != subprocmap->end()) refproc = iter->second;
      }

      FillH1(fCh[ch].fRisingMult, fCh[ch].rising_cnt); fCh[ch].rising_cnt = 0;
      FillH1(fCh[ch].fFallingMult, fCh[ch].falling_cnt); fCh[ch].falling_cnt = 0;

      // RAWPRINT("TDC%u Ch:%u Try to use as ref TDC%u %u proc:%p\n", GetID(), ch, reftdc, ref, refproc);

      if ((refproc!=0) && (ref>0) && (ref<refproc->NumChannels()) && ((ref!=ch) || (refproc!=this))) {
         if (DoRisingEdge() && (fCh[ch].rising_hit_tm != 0) && (refproc->fCh[ref].rising_hit_tm != 0)) {

            double tm = fCh[ch].rising_hit_tm;
            double tm_ref = refproc->fCh[ref].rising_hit_tm;
            if ((refproc!=this) && (ch>0) && (ref>0)) {
               tm -= fCh[0].rising_hit_tm;
               tm_ref -= refproc->fCh[0].rising_hit_tm;
            }

            fCh[ch].rising_ref_tm = tm - tm_ref;

            double diff = fCh[ch].rising_ref_tm*1e9;

            // when refch is 0 on same board, histogram already filled
            if ((ref!=0) || (refproc!=this)) FillH1(fCh[ch].fRisingRef, diff);

            FillH1(fCh[ch].fRisingCoarseRef, 0. + fCh[ch].rising_coarse - refproc->fCh[ref].rising_coarse);
            FillH2(fCh[ch].fRisingRef2D, diff, fCh[ch].rising_fine);
            FillH2(fCh[ch].fRisingRef2D, diff-1., refproc->fCh[ref].rising_fine);
            FillH2(fCh[ch].fRisingRef2D, diff-2., fCh[ch].rising_coarse/4);
            RAWPRINT("Difference rising %u:%u\t %u:%u\t %12.3f\t %12.3f\t %7.3f  coarse %03x - %03x = %4d  fine %03x %03x \n",
                  GetID(), ch, reftdc, ref,
                  tm*1e9,  tm_ref*1e9, diff,
                  fCh[ch].rising_coarse, refproc->fCh[ref].rising_coarse, (int) (fCh[ch].rising_coarse - refproc->fCh[ref].rising_coarse),
                  fCh[ch].rising_fine, refproc->fCh[ref].rising_fine);

            // make double reference only for local channels
            // if ((fCh[ch].doublerefch < NumChannels()) &&
            //    (fCh[ch].fRisingDoubleRef != 0) &&
            //    (fCh[fCh[ch].doublerefch].rising_ref_tm != 0)) {
            //   FillH1(fCh[ch].fRisingDoubleRef, diff, fCh[fCh[ch].doublerefch].rising_ref_tm*1e9);
            // }
         }

         if (DoFallingEdge() && (fCh[ch].falling_hit_tm != 0) && (refproc->fCh[ref].falling_hit_tm != 0)) {

            double tm = fCh[ch].falling_hit_tm;
            double tm_ref = refproc->fCh[ref].falling_hit_tm;

            if ((refproc!=this) && (ch>0) && (ref>0)) {
               tm -= fCh[0].falling_hit_tm;
               tm_ref -= refproc->fCh[0].falling_hit_tm;
            }

            double diff = (tm - tm_ref)*1e9;

            // when refch is 0 on same board, histogram already filled
            if ((ref!=0) || (refproc!=this)) FillH1(fCh[ch].fFallingRef, diff);

            FillH1(fCh[ch].fFallingCoarseRef, 0. + fCh[ch].falling_coarse - refproc->fCh[ref].falling_coarse);
            RAWPRINT("Difference falling %u:%u %u:%u  %12.3f  %12.3f  %7.3f  coarse %03x - %03x = %4d  fine %03x %03x \n",
                     GetID(), ch, reftdc, ref,
                     tm*1e9, tm_ref*1e9, diff,
                     fCh[ch].falling_coarse, fCh[ref].falling_coarse, (int) (fCh[ch].falling_coarse - refproc->fCh[ref].falling_coarse),
                     fCh[ch].falling_fine, refproc->fCh[ref].falling_fine);
         }
      }

      // fill double-reference histogram, using data from any reference TDC
      if ((fCh[ch].doublerefch < NumChannels()) && (fCh[ch].fRisingDoubleRef!=0)) {
         ref = fCh[ch].doublerefch;
         reftdc = fCh[ch].doublereftdc;
         if (reftdc>=0xffff) reftdc = GetID();
         refproc = 0;
         if (reftdc == GetID()) refproc = this; else
         if (subprocmap!=0) {
             SubProcMap::iterator iter = subprocmap->find(reftdc);
             if (iter != subprocmap->end()) refproc = iter->second;
         }

         if ((refproc!=0) && (ref<refproc->NumChannels()) && ((ref!=ch) || (refproc!=this))) {
            if ((fCh[ch].rising_ref_tm != 0) && (refproc->fCh[ref].rising_ref_tm != 0))
               FillH1(fCh[ch].fRisingDoubleRef, fCh[ch].rising_ref_tm*1e9, refproc->fCh[ref].rising_ref_tm*1e9);
         }
      }
   }

   if (fAutoCalibration>0) {
      bool docalibration(true), isany(false);
      for (unsigned ch=0;ch<NumChannels();ch++) {
         if (fCh[ch].docalibr) {
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

   unsigned cnt(0), hitcnt(0);

   bool iserr(false), isfirstepoch(false), rawprint(false);

   uint32_t first_epoch(0);

   unsigned epoch_shift = 0;
   if (fCompensateEpochReset) {
      if (first_scan) buf().user_tag = fCompensateEpochCounter;
      epoch_shift = buf().user_tag;
   }

   TdcIterator& iter = first_scan ? fIter1 : fIter2;

   iter.assign((uint32_t*) buf.ptr(4), buf.datalen()/4 -1, false);

   unsigned help_index(0);

   double localtm(0.), minimtm(0), ch0time(0);

   if (first_scan) BeforeFill();

   hadaq::TdcMessage& msg = iter.msg();

   while (iter.next()) {

      // if (!first_scan) msg.print();

      cnt++;

      if (first_scan)
         FillH1(fMsgsKind, msg.getKind() >> 29);

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

         if ((chid==0) && (ch0time==0)) ch0time = localtm;

         if (IsTriggeredAnalysis()) {
            if ((ch0time==0) && CheckPrintError())
               printf("%5s channel 0 time not found when first HIT in channel %u appears\n", GetName(), chid);

            localtm -= ch0time;
         }

         // printf("%s first %d ch %3u tm %12.9f\n", GetName(), first_scan, chid, localtm);

         // fill histograms only for normal channels
         if (first_scan) {

            // if (chid>0) printf("%s HIT ch %u tm %12.9f diff %12.9f\n", GetName(), chid, localtm, localtm - ch0time);

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

               bool print_cond = false;

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
               if ((chid!=0) && (rec.refch==0) && (rec.reftdc == GetID()) && (fCh[0].rising_hit_tm!=0)) {
                  FillH1(rec.fRisingRef, (localtm - fCh[0].rising_hit_tm)*1e9);

                  if (IsPrintRawData() || print_cond)
                  printf("Difference rising %u:%u\t %u:%u\t %12.3f\t %12.3f\t %7.3f  coarse %03x - %03x = %4d  fine %03x %03x \n",
                          GetID(), chid, rec.reftdc, rec.refch,
                          localtm*1e9,  fCh[0].rising_hit_tm*1e9, (localtm - fCh[0].rising_hit_tm)*1e9,
                          coarse, fCh[0].rising_coarse, (int) (coarse - fCh[0].rising_coarse),
                          fine, fCh[0].rising_fine);
               }

            } else {
               rec.falling_stat[fine]++;
               rec.all_falling_stat++;
               rec.falling_cnt++;

               FillH1(rec.fFallingFine, fine);
               FillH1(rec.fFallingCoarse, coarse);
               if ((rec.falling_hit_tm == 0.) || fUseLastHit) {
                  rec.falling_hit_tm = localtm;
                  rec.falling_coarse = coarse;
                  rec.falling_fine = fine;

                  if ((rec.falling_cond_prnt>0) && (rec.reftdc == GetID()) &&
                      (rec.refch < NumChannels()) && (fCh[rec.refch].falling_hit_tm!=0)) {
                     double diff = (localtm - fCh[rec.refch].falling_hit_tm) * 1e9;
                     if (TestC1(rec.fFallingRefCond, diff) == 0) {
                        rec.falling_cond_prnt--;
                        rawprint = true;
                        // printf ("TDC%u ch%u detect falling diff %8.3f ns\n", GetID(), chid, diff);
                     }
                  }

               }

               if ((chid!=0) && (rec.refch==0) && (rec.reftdc == GetID()) && (fCh[0].falling_hit_tm!=0)) {
                  FillH1(rec.fFallingRef, (localtm - fCh[0].falling_hit_tm)*1e9);
               }
            }

            if (!iserr) {
               hitcnt++;
               if ((minimtm==0) || (localtm < minimtm)) minimtm = localtm;
            }
         } else

         // for second scan we check if hit can be assigned to the events
         if ((chid>0) && !iserr) {

            base::GlobalTime_t globaltm = LocalToGlobalTime(localtm, &help_index);

            // printf("TDC%u Test TDC message local:%11.9f global:%11.9f\n", GetID(), localtm, globaltm);

            // we test hist, but do not allow to close events
            unsigned indx = TestHitTime(globaltm, true, false);

            if (indx < fGlobalMarks.size()) {
               // printf("!!!!!!!!!!!!!!!!!!!!!! ADD %s message ch %u diff %12.9f !!!!!!!!!!!!!!!!\n", GetName(), chid, globaltm - fGlobalMarks.item(indx).globaltm);
               AddMessage(indx, (hadaq::TdcSubEvent*) fGlobalMarks.item(indx).subev, hadaq::TdcMessageExt(msg, globaltm));
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

   if (isfirstepoch && !iserr) {
      // if we want to compensate epoch reset, use epoch of trigger channel+1
      // +1 to exclude small probability of hits after trigger with epoch+1 value
      if (fCompensateEpochReset && first_scan)
         fCompensateEpochCounter = first_epoch+1;

      iter.setRefEpoch(first_epoch);
   }

   if (first_scan) {

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

      FillH1(fMsgPerBrd, GetID(), cnt);

      // fill number of "good" hits
      FillH1(fHitsPerBrd, GetID(), hitcnt);

      if (iserr)
         FillH1(fErrPerBrd, GetID());
   } else {

      // use first channel only for flushing
      if (ch0time!=0)
         TestHitTime(LocalToGlobalTime(ch0time, &help_index), false, true);
   }

   if (first_scan && !fCrossProcess) AfterFill();

   if (rawprint && first_scan) {
      printf("RAW data of %s\n", GetName());
      TdcIterator iter;
      iter.assign((uint32_t*) buf.ptr(4), buf.datalen()/4 -1, false);
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

void hadaq::TdcProcessor::CalibrateChannel(unsigned nch, long* statistic, float* calibr)
{
   double sum(0.);
     for (int n=0;n<FineCounterBins;n++) {
        sum+=statistic[n];
        calibr[n] = sum;
     }

   if (sum<1000) {
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
}

void hadaq::TdcProcessor::CopyCalibration(float* calibr, base::H1handle hcalibr)
{
   if (hcalibr==0) return;

   ClearH1(hcalibr);

   for (unsigned n=0;n<FineCounterBins;n++)
      FillH1(hcalibr, n, calibr[n]*1e12);

}

void hadaq::TdcProcessor::ProduceCalibration(bool clear_stat)
{
   printf("%s produce channels calibrations\n", GetName());

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
      printf("%s Cannot open file %s for writing calibration\n", GetName(), fname.c_str());
      return;
   }

   uint64_t num = NumChannels();

   fwrite(&num, sizeof(num), 1, f);

   for (unsigned ch=0;ch<NumChannels();ch++) {
      fwrite(fCh[ch].rising_calibr, sizeof(fCh[ch].rising_calibr), 1, f);
      fwrite(fCh[ch].falling_calibr, sizeof(fCh[ch].falling_calibr), 1, f);
   }

   fclose(f);

   printf("%s storing calibration in %s\n", GetName(), fname.c_str());
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
      printf("%s mismatch of channels number in calibration file %u and in processor %u\n", GetName(), (unsigned) num, NumChannels());
   } else {
      for (unsigned ch=0;ch<NumChannels();ch++) {
         fread(fCh[ch].rising_calibr, sizeof(fCh[ch].rising_calibr), 1, f);
         fread(fCh[ch].falling_calibr, sizeof(fCh[ch].falling_calibr), 1, f);

         CreateChannelHistograms(ch);

         CopyCalibration(fCh[ch].rising_calibr, fCh[ch].fRisingCalibr);

         CopyCalibration(fCh[ch].falling_calibr, fCh[ch].fFallingCalibr);
      }
   }

   fclose(f);

   printf("%s reading calibration from %s done\n", GetName(), fname.c_str());
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

