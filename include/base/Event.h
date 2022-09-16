#ifndef BASE_EVENT_H
#define BASE_EVENT_H

#include "base/SubEvent.h"

#include "base/TimeStamp.h"

#include <map>
#include <string>

namespace base {

   typedef std::map<std::string, base::SubEvent*> EventsMap;

   /** Event - collection of several subevents */

   class Event {
      protected:
         EventsMap  fMap;   ///< subevents map

         GlobalTime_t  fTriggerTm;  ///< trigger time

      public:
         /** constructor */
         Event() : fMap(), fTriggerTm(0.) {}

         /** destructor */
         virtual ~Event()
         {
            DestroyEvents();
         }

         /** set trigger time */
         void SetTriggerTime(GlobalTime_t tm) { fTriggerTm = tm; }

         /** get trigger time */
         GlobalTime_t GetTriggerTime() const { return fTriggerTm; }

         /** destroy all events */
         void DestroyEvents()
         {
            for (auto &elem : fMap)
               delete elem.second;
            fMap.clear();
         }

         /** reset events */
         void ResetEvents()
         {
            for (auto &elem : fMap)
               if (elem.second)
                  elem.second->Clear();

            fTriggerTm = 0;
         }

         /** add subevent */
         void AddSubEvent(const std::string& name, base::SubEvent* ev)
         {
            auto iter = fMap.find(name);
            if (iter != fMap.end()) {
               delete iter->second;
               iter->second = nullptr;
            }
            fMap[name] = ev;
         }

         /** Return number of subevents */
         unsigned NumSubEvents() const { return fMap.size(); }

         /** Return subevent by name */
         base::SubEvent* GetSubEvent(const std::string& name) const
         {
            EventsMap::const_iterator iter = fMap.find(name);
            return (iter != fMap.end()) ? iter->second : nullptr;
         }

         /** Return subevent by name with index
          * GetSubEvent("ROC",2) is same as GetSubEvent("ROC2") */
         base::SubEvent* GetSubEvent(const std::string& name, unsigned subindx) const;

         /** Return events map */
         EventsMap &GetEventsMap() { return fMap; }
   };

}

#endif
