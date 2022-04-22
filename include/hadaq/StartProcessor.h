#ifndef HADAQ_STARTPROCESSOR_H
#define HADAQ_STARTPROCESSOR_H

#include "base/StreamProc.h"

namespace hadaq {

/** This is specialized process for spill structures  */

class StartProcessor : public base::StreamProc {

protected:
   base::H1handle fEvType{nullptr};    ///< HADAQ event type
   base::H1handle fEvSize{nullptr};    ///< HADAQ event size
   base::H1handle fSubevSize{nullptr}; ///< HADAQ sub-event size

   base::H1handle fEpochDiff{nullptr};  ///< epoch differences

   unsigned fStartId{0}; ///< HUB with start detector

   unsigned fTdcMin{0}; ///< minimal TDC id
   unsigned fTdcMax{0}; ///< maximal TDC id

   /** Calculates most realistic difference between epochs */
   inline int EpochDiff(unsigned ep1, unsigned ep2)
   {
      unsigned res = ep1 <= ep2 ? ep2 - ep1 : ep2 + 0x10000000 - ep1;

      return res < 0x8000000 ? res : (int) res - 0x10000000;
   }

   /** Return time difference between epochs in seconds */
   double EpochTmDiff(unsigned ep1, unsigned ep2);

public:
   StartProcessor();
   virtual ~StartProcessor();

   bool FirstBufferScan(const base::Buffer &buf) override;

   /** set start id */
   void SetStartId(unsigned id)
   {
      fStartId = id;
   }

   /** set tdc range */
   void SetTdcRange(unsigned min, unsigned max)
   {
      fTdcMin = min;
      fTdcMax = max;
   }
};
}

#endif
