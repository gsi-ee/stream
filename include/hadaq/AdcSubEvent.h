#ifndef HADAQ_ADCSUBEVENT_H
#define HADAQ_ADCSUBEVENT_H

#include "base/SubEvent.h"

#include "hadaq/AdcMessage.h"

namespace hadaq {

   // SL: extended message not defined while time is not used
   // typedef base::MessageExt<hadaq::TdcMessage> TdcMessageExt;

   typedef base::SubEventEx<hadaq::AdcMessage> AdcSubEvent;
}

#endif
