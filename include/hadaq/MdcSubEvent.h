#ifndef HADAQ_MDCSUBEVENT_H
#define HADAQ_MDCSUBEVENT_H

#include "base/SubEvent.h"

namespace hadaq {

   /** \brief Output float message for TDC
     *
     * \ingroup stream_hadaq_classes
     *
     * Stores channel, stamp relative to channel 0 and tot as float in ns
     * Configured when calling base::ProcMgr::instance()->SetStoreKind(2); */

   struct MdcMessage {
      uint8_t ch{0};    ///< channel
      float   stamp{0.}; ///< time stamp minus channel0 time, ns
      float   tot{0.}; ///< time stamp minus channel0 time, ns

      /**  channel */
      uint8_t getCh() const { return ch; }
      /**  stamp */
      float getStamp() const { return stamp; }
      /**  stamp */
      float getToT() const { return tot; }


      /** constructor */
      MdcMessage() : ch(0), stamp(0.), tot(0.) {}
      /** constructor */
      MdcMessage(const MdcMessage& src) : ch(src.ch), stamp(src.stamp), tot(src.tot) {}
      /** constructor */
      MdcMessage(unsigned _ch, float _stamp, float _tot) :
         ch(_ch), stamp(_stamp), tot(_tot)
      {
      }

      /** compare operator - used for time sorting */
      bool operator<(const MdcMessage &rhs) const
         { return stamp < rhs.stamp; }

   };

   /** subevent with \ref hadaq::MdcMessage */
   typedef base::SubEventEx<hadaq::MdcMessage> MdcSubEvent;
}

#endif
