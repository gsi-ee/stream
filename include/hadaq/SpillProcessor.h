#ifndef HADAQ_SPILLPROCESSOR_H
#define HADAQ_SPILLPROCESSOR_H

#include "base/StreamProc.h"

#include "base/EventProc.h"


namespace hadaq {

   /** This is specialized process for spill structures  */

   class SpillProcessor : public base::StreamProc {

      protected:

         base::H1handle fEvType;     ///< HADAQ event type
         base::H1handle fEvSize;     ///< HADAQ event size
         base::H1handle fSubevSize;  ///< HADAQ sub-event size

         base::H1handle fSpill;      ///< Current SPILL histogram
         base::H1handle fLastSpill;  ///< Last SPILL histogram
         base::H1handle fHitsData;       ///< Current SPILL histogram
         int fSpillCnt;
         int fSpillSize;
         unsigned fTotalCnt;
         unsigned fLastEpBin;
         bool fFirstEpoch;

         unsigned fTdcMin;   // minimal TDC id
         unsigned fTdcMax;   // maximal TDC id

         /** returns -1 when leftbin<rightbin, taking into account overflow around 0x1000)
          *          +1 when leftbin>rightbin
          *          0  when leftbin==rightbin */
         int CompareEpochBins(unsigned leftbin, unsigned rightbin);

      public:

         SpillProcessor();
         virtual ~SpillProcessor();

         /** Scan all messages, find reference signals */
         virtual bool FirstBufferScan(const base::Buffer& buf);

         void SetTdcRange(unsigned min, unsigned max)
         {
            fTdcMin = min;
            fTdcMax = max;
         }
   };

}

#endif
