#ifndef HADAQ_MONITORSUBEVENT_H
#define HADAQ_MONITORSUBEVENT_H

#include "base/SubEvent.h"

#include <vector>

namespace hadaq {

   /** \brief Monitor message
     *
     * \ingroup stream_hadaq_classes
     *
     * Record addr0, addr and value readout by MonitorModule */

   struct MessageMonitor {
      uint32_t addr0;   // first addr
      uint32_t addr;    // second addr
      uint32_t value;   // value

      MessageMonitor(uint32_t _addr0 = 0, uint32_t _addr = 0, uint32_t _value = 0)
      {
         addr0 = _addr0;
         addr = _addr;
         value = _value;
      }

      bool operator<(const MessageMonitor &rhs) const
         { return (addr0 < rhs.addr0); }

   };

   typedef base::SubEventEx<hadaq::MessageMonitor> MonitorSubEvent;
}

#endif
