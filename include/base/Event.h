#ifndef BASE_EVENT_H
#define BASE_EVENT_H


#include "base/SubEvent.h"

#include <map>
#include <string>

namespace base {

   /** Event - collection of several subevents
    *  Implementation is still open, one could use any external classes at this place */


   typedef std::map<std::string, base::SubEvent*> EventsMap;

   class Event {
      protected:
         EventsMap  fMap;

         GlobalTime_t  fTriggerTm;

      public:
         Event() : fMap(), fTriggerTm(0.) {}

         virtual ~Event()
         {
            DestroyEvents();
         }

         void SetTriggerTime(GlobalTime_t tm) { fTriggerTm = tm; }

         GlobalTime_t GetTriggerTime() const { return fTriggerTm; }

         void DestroyEvents()
         {
            for (EventsMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
               delete iter->second;
            fMap.clear();
         }

         void ResetEvents()
         {
            for (EventsMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
               if (iter->second)
                  iter->second->Reset();

            fTriggerTm = 0;
         }

         void AddSubEvent(const std::string& name, base::SubEvent* ev)
         {
            EventsMap::iterator iter = fMap.find(name);
            if (iter != fMap.end()) delete iter->second;
            fMap[name] = ev;
         }

         base::SubEvent* GetSubEvent(const std::string& name) const
         {
            EventsMap::const_iterator iter = fMap.find(name);
            return (iter != fMap.end()) ? iter->second : 0;
         }

   };

}



#endif
