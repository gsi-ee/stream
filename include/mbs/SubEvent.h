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
         void Clear() override {}

         /** sort */
         void Sort() override {}

         /** Method returns event multiplicity - that ever it means */
         unsigned Multiplicity() const override { return 1; }
   };
}



#endif
