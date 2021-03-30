#ifndef HADAQ_MONITORSUBEVENT_H
#define HADAQ_MONITORSUBEVENT_H

#include "base/SubEvent.h"

#include <vector>

namespace hadaq {

   class MonitorSubEvent : public base::SubEvent {
   public:
      std::vector<uint32_t> addr0;
      std::vector<uint32_t> addr;
      std::vector<uint32_t> values;

      MonitorSubEvent() {}
      virtual ~MonitorSubEvent() {}

      virtual unsigned Multiplicity() const { return values.size(); }
   };
}

#endif
