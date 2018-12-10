#include "hadaq/SpillProcessor.h"

#include "hadaq/definess.h"
#include "hadaq/TrbIterator.h"
#include "hadaq/TdcIterator.h"


const unsigned NUMHISTBINS = 0x1000;
const double EPOCHLEN = 5e-9*0x800;
const unsigned FASTEPOCHS = 2;
const double BINWIDTHFAST = EPOCHLEN*FASTEPOCHS;
const unsigned SLOWEPOCHS = 0x1000;
const double BINWIDTHSLOW = EPOCHLEN*SLOWEPOCHS;
const unsigned NUMSTAT = 100; // use 100 bins for stat calculations
const unsigned NUMSPILLBINS = 1000; // approx 40s for spill recording

//const unsigned ChannelsLookup[33] = {0,
//             0,   0,   1,   1,   2,   2,   3,   3,   4,   4,   5,  5,   6,   6,  7,  7,
//             8,   8,   9,   9,  10,  10,  11,  11,  12,  12,  13, 13,  14,  14, 15, 15 };

hadaq::SpillProcessor::SpillProcessor() :
   base::StreamProc("HLD", 0, false)
{
   mgr()->RegisterProc(this, base::proc_TRBEvent, 0);

   fEvType = MakeH1("EvType", "Event type", 16, 0, 16, "id");
   fEvSize = MakeH1("EvSize", "Event size", 500, 0, 5000, "bytes");
   fSubevSize = MakeH1("SubevSize", "Subevent size", 500, 0, 5000, "bytes");

   char title[200];
   snprintf(title, sizeof(title), "Current spill Quality factor, %5.2f ms bins", BINWIDTHSLOW*1e3);
   fSpill = MakeH1("Spill_Q_factor", "Current spill Quality factor", NUMSPILLBINS, 0., NUMSPILLBINS*BINWIDTHSLOW, "sec");
   snprintf(title, sizeof(title), "Last spill Quality factor, %5.2f ms bins", BINWIDTHSLOW*1e3);
   fLastSpill = MakeH1("LastSpill_Q_factor", "Last spill Quality factor", NUMSPILLBINS, 0., NUMSPILLBINS*BINWIDTHSLOW, "sec");

   snprintf(title, sizeof(title), "Fast hits distribution, %5.2f us bins", BINWIDTHFAST*1e6);
   fHitsFast = MakeH1("HitsFast", title, NUMHISTBINS, 0., BINWIDTHFAST*NUMHISTBINS*1e3, "Time[ms];Ncounts_in_20.48us_bin");
   snprintf(title, sizeof(title), "Slow hits distribution, %5.2f ms bins", BINWIDTHSLOW*1e3);
   fHitsSlow = MakeH1("HitsSlow", title, NUMHISTBINS, 0., BINWIDTHSLOW*NUMHISTBINS, "sec;Ncounts_in_41.94ms_bin");
   snprintf(title, sizeof(title), "Quality factor, %5.2f ms bins", BINWIDTHSLOW*1e3);
   fQualitySlow = MakeH1("QSlow", title, NUMHISTBINS, 0., BINWIDTHSLOW*NUMHISTBINS, "hmin:0;hmax:20;sec;Q_Factor_Slow");
   snprintf(title, sizeof(title), "Slow BEAM POSITION_X, %5.2f ms bins", BINWIDTHSLOW*1e3);
   fTrendXSlow = MakeH1("XSlow", title, NUMHISTBINS, 0., BINWIDTHSLOW*NUMHISTBINS, "hmin:0;hmax:20;Time [sec];X_Beam_Pos[strip]");
   snprintf(title, sizeof(title), "Slow BEAM POSITION_Y, %5.2f ms bins", BINWIDTHSLOW*1e3);
   fTrendYSlow = MakeH1("YSlow", title, NUMHISTBINS, 0., BINWIDTHSLOW*NUMHISTBINS, "hmin:0;hmax:20;Time [sec];Y_Beam_Pos[strip]");

   snprintf(title, sizeof(title), "Slow HALO POSITION_X, %5.2f ms bins", BINWIDTHSLOW*1e3);
   fTrendXHaloSlow = MakeH1("XHALOSlow", title, NUMHISTBINS, 0., BINWIDTHSLOW*NUMHISTBINS, "hmin:0;hmax:4;Time [sec];Y_HALO_Pos[strip]");
   snprintf(title, sizeof(title), "Slow HALO POSITION_Y, %5.2f ms bins", BINWIDTHSLOW*1e3);
   fTrendYHaloSlow = MakeH1("YHALOSlow", title, NUMHISTBINS, 0., BINWIDTHSLOW*NUMHISTBINS, "hmin:0;hmax:4;Time [sec];Y_HALO_Pos[strip]");

   snprintf(title, sizeof(title), "Fast trigger distribution, %5.2f us bins", BINWIDTHFAST*1e6);
   fTriggerFast = MakeH1("TriggerFast", title, NUMHISTBINS, 0., BINWIDTHFAST*NUMHISTBINS*1e3, "Time[ms];Ncounts_in_20.48us_bin");
   snprintf(title, sizeof(title), "Slow trigger distribution, %5.2f ms bins", BINWIDTHSLOW*1e3);
   fTriggerSlow = MakeH1("TriggerSlow", title, NUMHISTBINS, 0., BINWIDTHSLOW*NUMHISTBINS, "sec;Ncounts_in_41.94ms_bin");

   fBeamX = MakeH1("BeamX", "BEAM_X (ONE SPILL)", 20, 0, 20, "STRIP_X");
   fBeamY = MakeH1("BeamY", "BEAM_Y (ONE SPILL)", 20, 0, 20, "STRIP_Y");

   fTrendX = MakeH1("TrendX", "BEAM_X POSITION", NUMSPILLBINS, 0., NUMSPILLBINS*BINWIDTHSLOW, "hmin:0;hmax:20;Time [sec];Strip_X");
   fTrendY = MakeH1("TrendY", "BEAM_Y POSITION", NUMSPILLBINS, 0., NUMSPILLBINS*BINWIDTHSLOW, "hmin:0;hmax:20;Time [sec];Strip_Y");

   fHaloPattern = MakeH2("HALO_Patt", "HALO_PATTERN", 4,0,4,4,0,4,"opt:colz,text;X_dir;Y_dir");
   fVetoPattern = MakeH2("VETO_Patt", "VETO_PATTERN", 4,0,4,4,0,4,"opt:colz,text;X_dir;Y_dir");

   fLastBinFast = 0;
   fLastBinSlow = 0;
   fLastEpoch = 0;
   fFirstEpoch = true;

   fTdcMin = 0xc000;
   fTdcMax = 0xc010;

   fSpillOnLevel = 500;
   fSpillOffLevel = 10;
   fSpillMinCnt = 5;
   fSpillStartEpoch = 0;
   fAutoSpillDetect = true;

   fLastSpillEpoch = 0;
   fLastSpillBin = 0;


   fMaxSpillLength = 10.;
   fMinSpillLength = 0.1;
   fLastQSlowValue = 0;

   for (unsigned n=0;n<33;++n) {
      fChannelsLookup1[n] = 0;
      fChannelsLookup2[n] = 0;
      fChannelsLookup3[n] = 0;
   }

   fSumX = fCntX = fSumY = fCntY = 0;
   fSumHaloX = fCntHaloX = fSumHaloY = fCntHaloY = 0;
   fLastX = fLastY = fLastHaloX = fLastHaloY = 0.;
}

hadaq::SpillProcessor::~SpillProcessor()
{
}

/** returns -1 when leftbin<rightbin, taking into account overflow around 0x1000)
 *          +1 when leftbin>rightbin
 *          0  when leftbin==rightbin */
int hadaq::SpillProcessor::CompareHistBins(unsigned leftbin, unsigned rightbin)
{
   if (leftbin == rightbin) return 0;

   if (leftbin < rightbin)
      return (rightbin - leftbin) < NUMHISTBINS/2 ? -1 : 1;

   return (leftbin - rightbin) < NUMHISTBINS/2 ? 1 : -1;
}

/** Return time difference between epochs in seconds */
double hadaq::SpillProcessor::EpochTmDiff(unsigned ep1, unsigned ep2)
{
   return EpochDiff(ep1, ep2) * EPOCHLEN;
}

void hadaq::SpillProcessor::StartSpill(unsigned epoch)
{
   fSpillStartEpoch = epoch;
   fSpillEndEpoch = 0;
   fLastSpillEpoch = epoch & ~(FASTEPOCHS-1); // mask not used bins
   fLastSpillBin = 0;
   ClearH1(fSpill);
   ClearH1(fBeamX);
   ClearH1(fBeamY);
   ClearH1(fTrendX);
   ClearH1(fTrendY);
   ClearH2(fHaloPattern);
   ClearH2(fVetoPattern);
   // fSumX = fCntX = fSumY = fCntY = 0;
   // fLastX = fLastY = 0.;
   fCurrXYBin = 0;
   printf("SPILL ON  0x%08x tm  %6.2f s\n", epoch, EpochTmDiff(0, epoch));
}

void hadaq::SpillProcessor::StopSpill(unsigned epoch)
{
   printf("SPILL OFF 0x%08x len %6.2f s\n", epoch, EpochTmDiff(fSpillStartEpoch, epoch));

   fSpillStartEpoch = 0;
   fSpillEndEpoch = epoch;
   fLastSpillEpoch = 0;
   fLastSpillBin = 0;
   CopyH1(fLastSpill, fSpill);
}

double hadaq::SpillProcessor::CalcQuality(unsigned fastbin, unsigned len)
{
   double sum = 0., max = 0.;

   for (unsigned n=0;n<len;++n) {
      double cont = GetH1Content(fHitsFast, fastbin++);
      sum += cont;
      if (cont>max) max = cont;
      if (fastbin >= NUMHISTBINS) fastbin = 0;
   }

   sum = sum/len;

   return sum>0 ? max/sum : 0.;
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
            unsigned epochch0 = 0;
            bool firtst_epoch = true, use_hits = false;

            if (istdc) {

               TdcIterator iter;
               iter.assign(sub, ix, datalen);

               hadaq::TdcMessage &msg = iter.msg();

               //unsigned *lookup_table = (dataid==fTdcMin) ? fChannelsLookup1 : fChannelsLookup2;
               unsigned *lookup_table = fChannelsLookup1;
               if (dataid==fTdcMin)   lookup_table = fChannelsLookup1;
               if (dataid==fTdcMin+1) lookup_table = fChannelsLookup2;
               if (dataid==fTdcMin+2) lookup_table = fChannelsLookup3;

               while (iter.next()) {
                  if (msg.isHitMsg() && use_hits) {
                     unsigned chid = msg.getHitChannel();
                     //unsigned fine = msg.getHitTmFine();
                     //unsigned coarse = msg.getHitTmCoarse();
                     //bool isrising = msg.isHitRisingEdge();

                     numhits++;

                     if (chid>0) {
                        // fill these histograms only for StartX and StartY // Sergey
                        //FastFillH1(fHitsFast, fastbin);
                        //FastFillH1(fHitsSlow, slowbin);

                        if (chid < 34)  {

                           unsigned lookup = lookup_table[chid], pos = lookup % 100;

                           //unsigned diff = EpochDiff(fSpillStartEpoch, fLastEpoch);
                           //unsigned bin = diff / (FASTEPOCHS*NUMSTAT);

                           if (lookup && pos) {
                              if (lookup < 100) {
                                 // fill these histograms only for StartX and StartY
                                 FastFillH1(fHitsFast, fastbin);
                                 FastFillH1(fHitsSlow, slowbin);
                                 FastFillH1(fBeamX, pos);
                                 fSumX += pos;
                                 fCntX++;
                              } else if (lookup > 100 && lookup < 200) {
                                 // fill these histograms only for StartX and StartY
                                 FastFillH1(fHitsFast, fastbin);
                                 FastFillH1(fHitsSlow, slowbin);
                                 FastFillH1(fBeamY, pos);
                                 fSumY += pos;
                                 fCntY++;
                              } else if (lookup > 200 && lookup < 300) { // Halo channels, Up, Down, L, R
                                 switch (pos) {
                                 // fill Veto pattern
                                    case 1: DefFastFillH2(fVetoPattern, 1, 2); break;
                                    case 2: DefFastFillH2(fVetoPattern, 2, 1); break;
                                    case 3: DefFastFillH2(fVetoPattern, 1, 0); break;
                                    case 4: DefFastFillH2(fVetoPattern, 0, 1); break;
                                    case 5: DefFastFillH2(fVetoPattern, 0, 2); break;
                                    case 6: DefFastFillH2(fVetoPattern, 2, 2); break;
                                    case 7: DefFastFillH2(fVetoPattern, 2, 0); break;
                                    case 8: DefFastFillH2(fVetoPattern, 0, 0); break;
                                    case 9:  // Halo Up
                                       DefFastFillH2(fHaloPattern, 2, 3);
                                       fSumHaloY += 3;
                                       fCntHaloY++;
                                       break;
                                    case 10: // Halo Down
                                       DefFastFillH2(fHaloPattern, 2, 1);
                                       fSumHaloY += 1;
                                       fCntHaloY++;
                                       break;
                                    case 11: // Halo Left
                                       DefFastFillH2(fHaloPattern, 1, 2);
                                       fSumHaloX += 1;
                                       fCntHaloX++;
                                       break;
                                    case 12: // Halo Right
                                       DefFastFillH2(fHaloPattern, 3, 2);
                                       fSumHaloX += 3;
                                       fCntHaloX++;
                                       break;
                                  }
                             } else if (lookup == 401) { // spill detection
                                if (!fAutoSpillDetect) {
                                   bool new_spill = true;

                                   // stop previous spill
                                   if (fSpillStartEpoch) {
                                      if (EpochTmDiff(fSpillStartEpoch, fLastEpoch) > fMinSpillLength)
                                         StopSpill(fLastEpoch);
                                      else
                                         new_spill = false;
                                   }

                                   // start new spill
                                   if (new_spill) StartSpill(fLastEpoch);
                                }
                             } else if (lookup == 402) { // trigger counts
                                FastFillH1(fTriggerFast, fastbin);
                                FastFillH1(fTriggerSlow, slowbin);
                             }
                          }
                       }
                     }

                  } else if (msg.isEpochMsg()) {
                     if (firtst_epoch) {
                        epochch0 = iter.getCurEpoch();
                        firtst_epoch = false;
                        continue;
                     }

                     unsigned val = iter.getCurEpoch();
                     double diff = EpochTmDiff(val, epochch0);

                     if ((diff < -0.001) || (diff > 0.001)) {
                        use_hits = false;
                        continue;
                     }

                     fLastEpoch = val;
                     use_hits = true;

                     fastbin = (fLastEpoch >> 1) % NUMHISTBINS; // use lower bits from epoch
                     slowbin = (fLastEpoch >> 12) % NUMHISTBINS; // use only 12 bits, skipping lower 12 bits

                     if (fFirstEpoch) {
                        fFirstEpoch = false;
                        fLastBinFast = fastbin;
                        fLastBinSlow = slowbin;
                     } else {
                        // clear all previous bins in-between
                        while (CompareHistBins(fLastBinFast, fastbin) < 0) {
                           fLastBinFast = (fLastBinFast+1) % NUMHISTBINS;
                           SetH1Content(fHitsFast, fLastBinFast, 0.);
                           SetH1Content(fTriggerFast, fLastBinFast, 0.);
                        }

                        int diff = 0;

                        while ((diff = CompareHistBins(fLastBinSlow, slowbin)) < 0) {
                           if (diff == -1) {
                              fLastQSlowValue = CalcQuality((fLastBinSlow % 2 == 0) ? 0 : NUMHISTBINS / 2, NUMHISTBINS / 2);
                              fLastX = (fCntX > 0) ? 1.*fSumX/fCntX : 0.;
                              fLastY = (fCntY > 0) ? 1.*fSumY/fCntY : 0;
                              fSumX = fCntX = fSumY = fCntY = 0;

                              fLastHaloX = (fCntHaloX > 0) ? 1.*fSumHaloX/fCntHaloX : 0.;
                              fLastHaloY = (fCntHaloY > 0) ? 1.*fSumHaloY/fCntHaloY : 0;
                              fSumHaloX = fCntHaloX = fSumHaloY = fCntHaloY = 0;
                           }

                           SetH1Content(fQualitySlow, fLastBinSlow, fLastQSlowValue);
                           if (fSpillStartEpoch) {
                              // bin on slow histogram where spill started
                              unsigned startbin = (fSpillStartEpoch >> 12) % NUMHISTBINS;
                              unsigned histbin = (fLastBinSlow + NUMHISTBINS - startbin) % NUMHISTBINS;
                              if (histbin < NUMSPILLBINS) {
                                 SetH1Content(fSpill, histbin, fLastQSlowValue);
                                 SetH1Content(fTrendX, histbin, fLastX);
                                 SetH1Content(fTrendY, histbin, fLastY);
                              } else {
                                 StopSpill(fLastEpoch);
                              }
                           }

                           SetH1Content(fTrendXSlow, fLastBinSlow, fLastX);
                           SetH1Content(fTrendYSlow, fLastBinSlow, fLastY);
                           //halo trends
                           SetH1Content(fTrendXHaloSlow, fLastBinSlow, fLastHaloX);
                           SetH1Content(fTrendYHaloSlow, fLastBinSlow, fLastHaloY);

                           fLastBinSlow = (fLastBinSlow+1) % NUMHISTBINS;
                           SetH1Content(fHitsSlow, fLastBinSlow, 0.);
                           SetH1Content(fQualitySlow, fLastBinSlow, 0.);
                           SetH1Content(fTriggerSlow, fLastBinSlow, 0.);
                        }
                     }
                  }
               }
            }

            ix+=datalen;
         } // while (ix < trbSubEvSize)

      } // subevents

      // try to detect start or stop of the spill
      if (fAutoSpillDetect) {
         if (!fSpillStartEpoch && (fLastBinSlow >= fSpillMinCnt)) {
            // detecting spill ON
            bool all_over = true;

            // keep 1 sec for spill OFF signal
            if (fSpillEndEpoch && EpochTmDiff(fSpillEndEpoch, fLastEpoch)<1.) all_over = false;

            for (unsigned bin=(fLastBinSlow-fSpillMinCnt); (bin<fLastBinSlow) && all_over; ++bin)
               if (GetH1Content(fHitsSlow, bin) < fSpillOnLevel) all_over = false;

            if (all_over) StartSpill(fLastEpoch);

         } else if (fSpillStartEpoch && (fLastBinSlow>=fSpillMinCnt)) {
            // detecting spill OFF
            bool all_below = true;

            for (unsigned bin=(fLastBinSlow-fSpillMinCnt); (bin<fLastBinSlow) && all_below; ++bin)
               if (GetH1Content(fHitsSlow, bin) > fSpillOffLevel) all_below = false;

            if (all_below) StopSpill(fLastEpoch);
         }
      }

      // workaround - always set to zero first bin to preserve Y scaling
      SetH1Content(fHitsSlow, 0, 0.);
      SetH1Content(fTriggerSlow, 0, 0.);

      // check length of the current spill
      if (fSpillStartEpoch && (EpochTmDiff(fSpillStartEpoch, fLastEpoch) > fMaxSpillLength))
         StopSpill(fLastEpoch);

      // old code to fill spill structure with different binning

/*
      if (fLastSpillEpoch) {
         // check when last bin in spill statistic was calculated
         unsigned diff = EpochDiff(fLastSpillEpoch, fLastEpoch);
         if (diff > 0.8*FASTEPOCHS*NUMHISTBINS) {
            // too large different jump in epoch value - skip most of them

            unsigned jump = diff / (FASTEPOCHS*NUMSTAT);
            if (jump>1) jump--;

            fLastSpillBin += jump;
            fLastSpillEpoch += jump*FASTEPOCHS*NUMSTAT;

            diff = EpochDiff(fLastSpillEpoch, fLastEpoch);
         }


         // calculate statistic for all following bins
         while ((diff > 1.5*FASTEPOCHS*NUMSTAT) && (fLastSpillBin < NUMSPILLBINS)) {

            // first bin for statistic
            SetH1Content(fSpill, fLastSpillBin, CalcQuality((fLastEpoch >> 1) % NUMHISTBINS, NUMSTAT));
            SetH1Content(fTrendX, fLastSpillBin, fLastX);
            SetH1Content(fTrendY, fLastSpillBin, fLastY);

            fLastSpillBin++;
            fLastSpillEpoch += FASTEPOCHS*NUMSTAT;

            diff = EpochDiff(fLastSpillEpoch, fLastEpoch);
         }


         if (fLastSpillBin >= NUMSPILLBINS) StopSpill(fLastEpoch);
      }
*/

   } // events
   return true;

}
