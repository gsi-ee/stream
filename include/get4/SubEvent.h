#ifndef GET4_SUBEVENT_H
#define GET4_SUBEVENT_H

#include "base/SubEvent.h"

#include "get4/Message.h"

namespace get4 {

   typedef base::MessageExt<get4::Message> MessageExt;

   typedef base::SubEventEx<get4::MessageExt> SubEvent;

}

#endif
