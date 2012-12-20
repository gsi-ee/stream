#ifndef HADAQ_TDCSUBEVENT_H
#define HADAQ_TDCSUBEVENT_H

#include "base/SubEvent.h"

#include "hadaq/TdcMessage.h"

namespace hadaq {

   typedef base::MessageExt<hadaq::TdcMessage> TdcMessageExt;

   typedef base::SubEventEx<hadaq::TdcMessageExt> TdcSubEvent;
}

#endif
