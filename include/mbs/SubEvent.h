#ifndef MBS_SUBEVENT_H
#define MBS_SUBEVENT_H

#include "base/SubEvent.h"

namespace mbs {

   class SubEvent : public base::SubEvent {
      protected:

      public:
         SubEvent() : base::SubEvent() {}

         virtual ~SubEvent() {}

         virtual void Clear() {}

         virtual void Sort() {}

         /** Method returns event multiplicity - that ever it means */
         virtual unsigned Multiplicity() const { return 0; }
   };
}



#endif
