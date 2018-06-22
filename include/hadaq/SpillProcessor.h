#ifndef HADAQ_SPILLPROCESSOR_H
#define HADAQ_SPILLPROCESSOR_H

#include "base/StreamProc.h"

#include "base/EventProc.h"

namespace hadaq {

/** This is specialized process for spill structures  */

class SpillProcessor : public base::StreamProc {

protected:
   base::H1handle fEvType;    ///< HADAQ event type
   base::H1handle fEvSize;    ///< HADAQ event size
   base::H1handle fSubevSize; ///< HADAQ sub-event size

   base::H1handle fHitsFast; ///< Hits fast (with 20 us binning)
   base::H1handle fHitsSlow; ///< Hits slow (with 40 ms  binning)

   base::H1handle fSpill;     ///< Current SPILL histogram
   base::H1handle fLastSpill; ///< Last SPILL histogram

   unsigned fLastBinFast;
   unsigned fLastBinSlow;
   unsigned fLastEpoch;
   unsigned fLastSpillEpoch; ///< last epoch used to calculate spill quality
   unsigned fLastSpillBin;   ///< last bin number filled in the histogram

   bool fFirstEpoch;

   double fSpillOnLevel; ///< number of hits in 40ms bin to detect spill on (at least N bins after each other over limit)
   double fSpillOffLevel; ///< number of hits in 40ms bin to detect spill off (at least N bins below limit)
   unsigned fSpillMinCnt; ///< minimal number of counts to detect spill

   unsigned fSpillStartEpoch; ///< epoch value which assumed to be spill start, 0 - off
   unsigned fSpillEndEpoch;   ///< epoch value when switch off spill

   double fMaxSpillLength; ///< maximal spill time in seconds

   unsigned fTdcMin; // minimal TDC id
   unsigned fTdcMax; // maximal TDC id

   /** returns -1 when leftbin<rightbin, taking into account overflow around 0x1000)
    *          +1 when leftbin>rightbin
    *          0  when leftbin==rightbin */
   int CompareEpochBins(unsigned leftbin, unsigned rightbin);

   /** Hard difference between epochs */
   inline unsigned EpochDiff(unsigned ep1, unsigned ep2) { return ep1 <= ep2 ? ep2 - ep1 : ep2 + 0x10000000 - ep1; }

   /** Return time difference between epochs in seconds */
   double EpochTmDiff(unsigned ep1, unsigned ep2);

   void StartSpill(unsigned epoch);
   void StopSpill(unsigned epoch);

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

   void SetMaxSpillLength(double tm = 10) { fMaxSpillLength = tm; }
};
}

#endif
