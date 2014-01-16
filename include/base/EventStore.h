#ifndef BASE_EVENTSTORE_H
#define BASE_EVENTSTORE_H

#include "base/Processor.h"

namespace base {

   /** Class base::EventProc is abstract processor of build events */

   class EventStore  {

      protected:

         /** Make constructor protected - no way to create base class instance */
         EventStore() {}

      public:

         virtual ~EventStore() {}

         virtual bool DataBranch(const char* name, void* ptr, const char* options) = 0;

         virtual bool EventBranch(const char* name, const char* classname, void* ptr) = 0;

         virtual void Fill() = 0;

         // returns handle on the store object, in most cases it is TTree
         virtual void* GetHandle() { return 0; }
   };

}

#endif
