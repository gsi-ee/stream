#ifndef BASE_MARKERS_H
#define BASE_MARKERS_H

#include <vector>

#include "base/TimeStamp.h"

namespace base {

   class SubEvent;

   struct SyncMarker {
      unsigned      uniqueid;    //!< unique identifier for marker like 24-bit number of SYNC message
      unsigned      localid;     //!< some local id, custom number for each subsystem
      LocalStamp_t  local_stamp; //!< 64-bit integer for local stamp representation
      GlobalTime_t  localtm;     //!< local time (ns),
      GlobalTime_t  globaltm;    //!< global time (ns), used for all selections
      unsigned      bufid;       //!< use it for keep reference from which buffer it is

      SyncMarker() : uniqueid(0), localid(0), local_stamp(0), localtm(0.), globaltm(0.), bufid(0) {}
   };


   struct LocalTriggerMarker {
      GlobalTime_t  localtm;     //!< local time in ns,
      unsigned      localid;     //!< arbitrary id, like aux number or sync number

      LocalTriggerMarker() : localtm(0.), localid(0)  {}
   };


   struct GlobalTriggerMarker {
      GlobalTime_t globaltm;     //!< global time - reference time of marker
      GlobalTime_t lefttm;       //!< left range for hit selection
      GlobalTime_t righttm;      //!< right range for hit selection

      SubEvent*     subev;        //!< structure with data, selected for the trigger, ownership
      bool          isflush;      //!< indicate that trigger is just for flushing, no real data is important

      GlobalTriggerMarker(GlobalTime_t tm = 0.) :
         globaltm(tm), lefttm(0.), righttm(0.), subev(0), isflush(false) {}

      GlobalTriggerMarker(const GlobalTriggerMarker& src) :
         globaltm(src.globaltm), lefttm(src.lefttm), righttm(src.righttm), subev(src.subev), isflush(src.isflush) {}

      ~GlobalTriggerMarker() { /** should we here destroy subevent??? */ }

      bool null() const { return globaltm<=0.; }
      void reset() { globaltm = 0.; isflush = false; }

      /** return true when interval defines normal event */
      bool normal() const { return !isflush; }

      void SetInterval(double left, double right);

      int TestHitTime(const GlobalTime_t& hittime, double* dist = 0);
   };


   typedef std::vector<base::GlobalTriggerMarker> GlobalTriggerMarksQueue;


}


#endif
