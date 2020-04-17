#ifndef HADAQ_SPILLPROCESSOR_H
#define HADAQ_SPILLPROCESSOR_H

#include "base/StreamProc.h"

namespace hadaq {

/** This is specialized process for spill structures  */

class SpillProcessor : public base::StreamProc {

protected:
   base::H1handle fEvType;    ///< HADAQ event type
   base::H1handle fEvSize;    ///< HADAQ event size
   base::H1handle fSubevSize; ///< HADAQ sub-event size

   base::H1handle fHitsFast; ///< Hits fast (with 20 us binning)
   base::H1handle fHitsSlow; ///< Hits slow (with 40 ms  binning)
   base::H1handle fQualitySlow; ///< Quality histogram (with 40 ms  binning)
   base::H1handle fTrendXSlow; ///< Beam X with Quality histogram (with 40 ms  binning)
   base::H1handle fTrendYSlow; ///< Quality histogram (with 40 ms  binning)

   base::H1handle fHitsSpill; ///< Hits in current spill (with 1 ms binning)
   base::H1handle fHitsLastSpill; ///< Hits in last spill (with 1 ms binning)

   base::H1handle fTrendXHaloSlow; ///< Quality histogram (with 40 ms  binning)
   base::H1handle fTrendYHaloSlow; ///< Quality histogram (with 40 ms  binning)

   base::H1handle fTriggerFast; ///< Quality histogram (with 20 us  binning)
   base::H1handle fTriggerSlow; ///< Quality histogram (with 40 ms  binning)

   base::H1handle fSpill;     ///< Current SPILL histogram
   base::H1handle fLastSpill; ///< Last SPILL histogram

   base::H1handle fBeamX;     ///< Accumulated X position
   base::H1handle fBeamY;     ///< Accumulated Y position

   base::H1handle fTrendX;    ///< Beam X trending
   base::H1handle fTrendY;    ///< Beam Y trending

   base::H2handle fHaloPattern; ///<2dim Halo detector Pattern
   base::H2handle fVetoPattern; ///<2dim Veto detector Pattern

   long fSumX, fCntX, fSumY, fCntY;
   long fSumHaloX, fCntHaloX, fSumHaloY, fCntHaloY;
   unsigned fCurrXYBin;       ///< bin where current XY is calculated
   double fLastX, fLastY, fLastHaloX, fLastHaloY;

   unsigned fLastBinFast;
   unsigned fLastBinSlow;
   unsigned fLastEpoch;
   unsigned fLastSpillEpoch; ///< last epoch used to calculate spill quality
   unsigned fLastSpillBin;   ///< last bin number filled in the histogram
   bool fAutoSpillDetect;    ///< true when spill will be detected automatically

   bool fFirstEpoch;

   double fSpillOnLevel; ///< number of hits in 40ms bin to detect spill on (at least N bins after each other over limit)
   double fSpillOffLevel; ///< number of hits in 40ms bin to detect spill off (at least N bins below limit)
   unsigned fSpillMinCnt; ///< minimal number of counts to detect spill

   unsigned fSpillStartEpoch; ///< epoch value which assumed to be spill start, 0 - off
   unsigned fSpillEndEpoch;   ///< epoch value when switch off spill
   unsigned fSpillStartCoarse;

   double fMinSpillLength;  ///< minimal spill time in seconds
   double fMaxSpillLength; ///< maximal spill time in seconds

   unsigned fTdcMin; // minimal TDC id
   unsigned fTdcMax; // maximal TDC id

   unsigned fChannelsLookup1[33];   // first tdc
   unsigned fChannelsLookup2[33];   // second tdc
   unsigned fChannelsLookup3[33];   // third  tdc

   double fLastQSlowValue; // last value of Q factor for slow histogram

   /** returns -1 when leftbin<rightbin, taking into account overflow around 0x1000)
    *          +1 when leftbin>rightbin
    *          0  when leftbin==rightbin */
   int CompareHistBins(unsigned leftbin, unsigned rightbin);

   /** Hard difference between epochs */
   inline unsigned EpochDiff(unsigned ep1, unsigned ep2) { return ep1 <= ep2 ? ep2 - ep1 : ep2 + 0x10000000 - ep1; }

   inline unsigned Get1msBin(unsigned ep1, unsigned c1, unsigned ep2, unsigned c2)
   {
      unsigned ediff = EpochDiff(ep1,ep2) * 2048 + c2 - c1; // 5ns
      return ediff / 200000;
   }

   /** Return time difference between epochs in seconds */
   double EpochTmDiff(unsigned ep1, unsigned ep2);

   void StartSpill(unsigned epoch, unsigned coarse = 0);
   void StopSpill(unsigned epoch);

   double CalcQuality(unsigned firstbin, unsigned len);

public:
   SpillProcessor();
   virtual ~SpillProcessor();

   /** Scan all messages, find reference signals */
   virtual bool FirstBufferScan(const base::Buffer &buf);

   void SetTdcRange(unsigned min, unsigned max)
   {
      fTdcMin = min;
      fTdcMax = max;
   }

   void SetSpillDetect(double lvl_on, double lvl_off, unsigned cnt = 3)
   {
      fSpillOnLevel = lvl_on;
      fSpillOffLevel = lvl_off;
      fSpillMinCnt = cnt;
   }

   void SetMinSpillLength(double tm = 0.1) { fMinSpillLength = tm; }
   void SetMaxSpillLength(double tm = 10) { fMaxSpillLength = tm; }

   void SetChannelsLookup1(unsigned ch, unsigned lookup) { fChannelsLookup1[ch] = lookup; }
   void SetChannelsLookup2(unsigned ch, unsigned lookup) { fChannelsLookup2[ch] = lookup; }
   void SetChannelsLookup3(unsigned ch, unsigned lookup) { fChannelsLookup3[ch] = lookup; }

   /** Set channel id on third TDC, which should detect spill-start signal */
   void SetSpillChannel(unsigned ch)
   {
      SetChannelsLookup3(ch, 401);
      fAutoSpillDetect = false;
   }

   /** Set special channel with trigger info */
   void SetTriggerChannel(unsigned ch)
   {
      SetChannelsLookup3(ch, 402);
   }

};
}

#endif
