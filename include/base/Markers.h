#ifndef BASE_MARKERS_H
#define BASE_MARKERS_H

#include "base/TimeStamp.h"

#include "base/Queue.h"

namespace base {

   class SubEvent;

   /** sync marker */

   struct SyncMarker {
      unsigned      uniqueid;    ///< unique identifier for marker like 24-bit number of SYNC message
      unsigned      localid;     ///< some local id, custom number for each subsystem
      LocalStamp_t  local_stamp; ///< 64-bit integer for local stamp representation
      GlobalTime_t  localtm;     ///< local time (ns),
      GlobalTime_t  globaltm;    ///< global time (ns), used for all selections
      unsigned      bufid;       ///< use it for keep reference from which buffer it is

      /** constructor */
      SyncMarker() : uniqueid(0), localid(0), local_stamp(0), localtm(0.), globaltm(0.), bufid(0) {}

      /** constructor */
      SyncMarker(const SyncMarker& src) : uniqueid(src.uniqueid), localid(src.localid), local_stamp(src.local_stamp), localtm(src.localtm), globaltm(src.globaltm), bufid(src.bufid) {}

      /** reset marker */
      void reset() { uniqueid=0; localid=0; local_stamp=0; localtm=0; globaltm=0; bufid=0; }
   };

   // =========================================================================

   /** local time marker */

   struct LocalTimeMarker {
      GlobalTime_t  localtm;     ///< local time in ns,
      unsigned      localid;     ///< arbitrary id, like aux number or sync number

      /** constructor */
      LocalTimeMarker() : localtm(0.), localid(0)  {}

      /** constructor */
      LocalTimeMarker(const LocalTimeMarker& src) : localtm(src.localtm), localid(src.localid)  {}

      // ~LocalTimeMarker() {}

      /** reset marker
       * we need function, but it is not really used while no memory allocation */
      void reset() {}
   };

   typedef RecordsQueue<LocalTimeMarker, false> LocalMarkersQueue;

   // =========================================================================

   /** global time marker */

   struct GlobalMarker {
      GlobalTime_t globaltm;     ///< global time - reference time of marker
      GlobalTime_t lefttm;       ///< left range for hit selection
      GlobalTime_t righttm;      ///< right range for hit selection

      SubEvent*     subev;        ///< structure with data, selected for the trigger, ownership
      bool          isflush;      ///< indicate that trigger is just for flushing, no real data is important

      /** constructor */
      GlobalMarker(GlobalTime_t tm = 0.) :
         globaltm(tm), lefttm(0.), righttm(0.), subev(0), isflush(false) {}

      /** constructor */
      GlobalMarker(const GlobalMarker& src) :
         globaltm(src.globaltm), lefttm(src.lefttm), righttm(src.righttm), subev(src.subev), isflush(src.isflush) {}

      /** destructor */
      ~GlobalMarker() { /** should we here destroy subevent??? */ }

      /** is null */
      bool null() const { return globaltm<=0.; }
      /** reset */
      void reset() { globaltm = 0.; isflush = false; subev = 0; }

      /** return true when interval defines normal event */
      bool normal() const { return !isflush; }

      void SetInterval(double left, double right);

      int TestHitTime(const GlobalTime_t& hittime, double* dist = 0);
   };

   typedef RecordsQueue<GlobalMarker, false> GlobalMarksQueue;

}

#endif
