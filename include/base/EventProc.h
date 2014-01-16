#ifndef BASE_EVENTPROC_H
#define BASE_EVENTPROC_H

#include "base/Processor.h"

namespace base {

   /** Class base::EventProc is abstract processor of build events */

   class Event;

   class EventProc : public Processor {
      friend class ProcMgr;

      protected:

         /** Make constructor protected - no way to create base class instance */
         EventProc(const char* name = "", unsigned brdid = DummyBrdId);

      public:

         virtual ~EventProc() {}

         /** Generic event processor
          * If returns false, processing will be aborted and event will not be stored */

         virtual bool Process(Event*) { return true; }

   };

}

#endif
