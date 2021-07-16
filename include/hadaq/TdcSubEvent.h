#ifndef HADAQ_TDCSUBEVENT_H
#define HADAQ_TDCSUBEVENT_H

#include "base/SubEvent.h"

#include "hadaq/TdcMessage.h"

namespace hadaq {

   /** Extended message for \ref hadaq::TdcMessage */
   typedef base::MessageExt<hadaq::TdcMessage> TdcMessageExt;

   /** subevent with \ref hadaq::TdcMessage */
   typedef base::SubEventEx<hadaq::TdcMessageExt> TdcSubEvent;

   /** \brief Output float message
     *
     * \ingroup stream_hadaq_classes
     *
     * Stores channel, edge and relative to channel 0 time stamp as float
     * Configured when calling base::ProcMgr::instance()->SetStoreKind(2); */

   struct MessageFloat {
      uint8_t ch;    ///< channel and edge
      float   stamp; ///< time stamp minus channel0 time, ns

      /**  stamp */
      float getStamp() const { return stamp; }
      /**  channel */
      uint8_t getCh() const { return ch & 0x7F; }
      /**  edge 0 - rising, 1 - falling */
      uint8_t getEdge() const { return ch >> 7; } // 0 - rising, 1 - falling
      /**  is rising */
      bool isRising() const { return getEdge() == 0; }
      /**  is falling */
      bool isFalling() const { return getEdge() == 1; }

      /** constructor */
      MessageFloat() : ch(0), stamp(0.) {}
      /** constructor */
      MessageFloat(const MessageFloat& src) : ch(src.ch), stamp(src.stamp) {}
      /** constructor */
      MessageFloat(unsigned _ch, bool _rising, float _stamp) :
         ch(_ch | (_rising ? 0x00 : 0x80)),
         stamp(_stamp)
      {
      }

      /** compare operator - used for time sorting */
      bool operator<(const MessageFloat &rhs) const
         { return (stamp < rhs.stamp); }

   };

   /** subevent with \ref hadaq::MessageFloat */
   typedef base::SubEventEx<hadaq::MessageFloat> TdcSubEventFloat;

   /** \brief Output double message
     *
     * \ingroup stream_hadaq_classes
     *
     * Stores channel, edge and full time stamp as double
     * Configured when calling base::ProcMgr::instance()->SetStoreKind(3); */

   struct MessageDouble {
      uint8_t ch;    ///< channel and edge
      double  stamp; ///< full time stamp, s

      /**  stamp */
      double getStamp() const { return stamp; }
      /**  channel */
      uint8_t getCh() const { return ch & 0x7F; }
      /**  edge 0 - rising, 1 - falling */
      uint8_t getEdge() const { return ch >> 7; }
      /**  is rising */
      bool isRising() const { return getEdge() == 0; }
      /**  is falling */
      bool isFalling() const { return getEdge() == 1; }

      /** constructor */
      MessageDouble() : ch(0), stamp(0.) {}
      /** constructor */
      MessageDouble(const MessageDouble& src) : ch(src.ch), stamp(src.stamp) {}
      /** constructor */
      MessageDouble(unsigned _ch, bool _rising, double _stamp) :
         ch(_ch | (_rising ? 0x00 : 0x80)),
         stamp(_stamp)
      {
      }
      /** compare operator - used for time sorting */
      bool operator<(const MessageDouble &rhs) const
         { return (stamp < rhs.stamp); }
   };

   /** subevent with \ref hadaq::MessageDouble */
   typedef base::SubEventEx<hadaq::MessageDouble> TdcSubEventDouble;

}

#endif
