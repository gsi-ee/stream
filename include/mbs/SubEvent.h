#ifndef MBS_SUBEVENT_H
#define MBS_SUBEVENT_H

#include "base/SubEvent.h"

namespace mbs {

   /** subevent with MBS data */
   class SubEvent : public base::SubEvent {

      public:
         /** constructor */
         SubEvent() : base::SubEvent() {}

         /** destructor */
         virtual ~SubEvent() {}

         /** clear */
         virtual void Clear() {}

         /** sort */
         virtual void Sort() {}

         /** Method returns event multiplicity - that ever it means */
         virtual unsigned Multiplicity() const { return 1; }
   };
}



#endif
