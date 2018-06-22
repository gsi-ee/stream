#include "hadaq/SpillProcessor.h"

#include "hadaq/definess.h"
#include "hadaq/TrbIterator.h"
#include "hadaq/TdcIterator.h"


const unsigned NUMEPOCHBINS = 0x1000;
const double EPOCHLEN = 5e-9*0x800;
const double BINWIDTHFAST = EPOCHLEN*2;
const double BINWIDTHSLOW = EPOCHLEN*0x1000;
const unsigned NUMSTAT = 100; // use 100 bins for stat calculations
const double BINWIDTHSTAT = BINWIDTHFAST * NUMSTAT;
const unsigned NUMSTATBINS = 1000; // use 100 bins for stat calculations

hadaq::SpillProcessor::SpillProcessor() :
   base::StreamProc("HLD", 0, false)
{
   mgr()->RegisterProc(this, base::proc_TRBEvent, 0);

   fEvType = MakeH1("EvType", "Event type", 16, 0, 16, "id");
   fEvSize = MakeH1("EvSize", "Event size", 500, 0, 50000, "bytes");
   fSubevSize = MakeH1("SubevSize", "Subevent size", 500, 0, 5000, "bytes");

   fSpill = MakeH1("Spill", "Spill structure", NUMSTATBINS, 0., NUMSTATBINS*BINWIDTHSTAT, "sec");
   fLastSpill = MakeH1("Last", "Last spill structure", NUMSTATBINS, 0., NUMSTATBINS*BINWIDTHSTAT, "sec");

   char title[200];
   snprintf(title, sizeof(title), "Fast hits distribution, %5.2f us bins", BINWIDTHFAST*1e6);
   fHitsFast = MakeH1("HitsFast", title, NUMEPOCHBINS, 0., BINWIDTHFAST*NUMEPOCHBINS*1e3, "ms");
   snprintf(title, sizeof(title), "Slow hits distribution, %5.2f ms bins", BINWIDTHSLOW*1e3);
   fHitsSlow = MakeH1("HitsSlow", title, NUMEPOCHBINS, 0., BINWIDTHSLOW*NUMEPOCHBINS, "sec");

   fLastBinFast = 0;
   fLastBinSlow = 0;
   fLastEpoch = 0;
   fFirstEpoch = true;

   fTdcMin = 0xc000;
   fTdcMax = 0xc010;

   fSpillOnLevel = 5000;
   fSpillOffLevel = 500;
   fSpillMinCnt = 3;
   fSpillStartEpoch = 0;

   fLastSpillEpoch = 0;
   fLastSpillBin = 0;

   fMaxSpillLength = 10.;
}

hadaq::SpillProcessor::~SpillProcessor()
{
}

/** returns -1 when leftbin<rightbin, taking into account overflow around 0x1000)
 *          +1 when leftbin>rightbin
 *          0  when leftbin==rightbin */
int hadaq::SpillProcessor::CompareEpochBins(unsigned leftbin, unsigned rightbin)
{
   if (leftbin == rightbin) return 0;

   if (leftbin < rightbin)
      return (rightbin - leftbin) < NUMEPOCHBINS/2 ? -1 : 1;

   return (leftbin - rightbin) < NUMEPOCHBINS/2 ? 1 : -1;
}

/** Return time difference between epochs in seconds */
double hadaq::SpillProcessor::EpochTmDiff(unsigned ep1, unsigned ep2)
{
   unsigned diff = ep2-ep1;
   if (diff > 0xF0000000) diff += 0x10000000;
   return diff * EPOCHLEN;
}


bool hadaq::SpillProcessor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

   hadaq::TrbIterator iter(buf().buf, buf().datalen);

   hadaqs::RawEvent* ev = 0;

   unsigned evcnt = 0;

   while ((ev = iter.nextEvent()) != 0) {

      evcnt++;

      DefFillH1(fEvType, (ev->GetId() & 0xf), 1.);
      DefFillH1(fEvSize, ev->GetPaddedSize(), 1.);

      // fMsg.trig_type = ev->GetId() & 0xf;
      // fMsg.seq_nr = ev->GetSeqNr();
      // fMsg.run_nr = ev->GetRunNr();

      // if ((fEventTypeSelect <= 0xf) && ((ev->GetId() & 0xf) != fEventTypeSelect)) continue;

      // if (IsPrintRawData()) ev->Dump();

      unsigned hubid = 0x7654;

      int numhits = 0;

      hadaqs::RawSubevent* sub = 0;

      while ((sub = iter.nextSubevent()) != 0) {
         DefFillH1(fSubevSize, sub->GetSize(), 1.);

         unsigned ix = 0;           // cursor
         unsigned trbSubEvSize = sub->GetSize() / 4 - 4;

         while (ix < trbSubEvSize) {
            //! Extract data portion from the whole packet (in a loop)
            uint32_t data = sub->Data(ix++);

            unsigned datalen = (data >> 16) & 0xFFFF;
            unsigned dataid = data & 0xFFFF;

            // ignore HUB HEADER
            if (dataid == hubid) continue;

            bool istdc = (dataid>=fTdcMin) && (dataid<fTdcMax);

            unsigned slowbin = 0, fastbin = 0; // current bin number in histogram

            if (istdc) {

               TdcIterator iter;
               iter.assign(sub, ix, datalen);

               hadaq::TdcMessage &msg = iter.msg();

               while (iter.next()) {
                  if (msg.isHitMsg()) {
                     unsigned chid = msg.getHitChannel();
                     //unsigned fine = msg.getHitTmFine();
                     //unsigned coarse = msg.getHitTmCoarse();
                     //bool isrising = msg.isHitRisingEdge();

                     numhits++;

                     if (chid!=0) {
                        FastFillH1(fHitsFast, fastbin);
                        FastFillH1(fHitsSlow, slowbin);
                     }

                  } else if (msg.isEpochMsg()) {
                     fLastEpoch = iter.getCurEpoch();

                     fastbin = (fLastEpoch >> 1) % NUMEPOCHBINS; // use lower bits from epoch
                     slowbin = (fLastEpoch >> 12) % NUMEPOCHBINS; // use only 12 bits, skipping lower 12 bits

                     if (fFirstEpoch) {
                        fFirstEpoch = false;
                        fLastBinFast = fastbin;
                        fLastBinSlow = slowbin;
                     } else {
                        // clear all previous bins in-between
                        while (CompareEpochBins(fLastBinSlow, slowbin) < 0) {
                           fLastBinSlow = (fLastBinSlow+1) % NUMEPOCHBINS;
                           SetH1Content(fHitsSlow, fLastBinSlow, 0.);
                        }
                        while (CompareEpochBins(fLastBinFast, fastbin) < 0) {
                           fLastBinFast = (fLastBinFast+1) % NUMEPOCHBINS;
                           SetH1Content(fHitsFast, fLastBinFast, 0.);
                        }
                     }
                  }
               }
            }

            ix+=datalen;
         } // while (ix < trbSubEvSize)

      } // subevents


      // try to detect start or stop of the spill

      if (!fSpillStartEpoch && (fLastBinSlow>=fSpillMinCnt)) {
         // detecting spill ON
         bool all_over = true;

         // keep 1 sec for spill OFF signal
         if (fSpillEndEpoch && EpochTmDiff(fSpillEndEpoch, fLastEpoch)<1.) all_over = false;

         for (unsigned bin=(fLastBinSlow-fSpillMinCnt); (bin<fLastBinSlow) && all_over; ++bin)
            if (GetH1Content(fHitsSlow, bin) < fSpillOnLevel) all_over = false;
         if (all_over) {
            fSpillStartEpoch = fLastEpoch;
            fSpillEndEpoch = 0;
            fLastSpillEpoch = fSpillStartEpoch & 0xFFFFFFFE; // mask last bin
            fLastSpillBin = 0;
            ClearH1(fSpill);
         }
      } else if (!fSpillStartEpoch && (fLastBinSlow>=fSpillMinCnt)) {
         // detecting spill OFF
         bool all_below = true;
         for (unsigned bin=(fLastBinSlow-fSpillMinCnt); (bin<fLastBinSlow) && all_below; ++bin)
            if (GetH1Content(fHitsSlow, bin) > fSpillOffLevel) all_below = false;
         if (all_below) {
            fSpillEndEpoch = fLastEpoch;
            fSpillStartEpoch = 0;
            fLastSpillEpoch = 0;
            fLastSpillBin = 0;
            CopyH1(fLastSpill, fSpill);
         }
      }

      // check length of the current spill
      if (fSpillStartEpoch && (EpochTmDiff(fSpillStartEpoch, fLastEpoch) > fMaxSpillLength)) {
         fSpillStartEpoch = 0;
         fSpillEndEpoch = fLastEpoch;
         fLastSpillEpoch = 0; // mask last bin
         fLastSpillBin = 0;
         CopyH1(fLastSpill, fSpill);
      }



   } // events
   return true;

}
