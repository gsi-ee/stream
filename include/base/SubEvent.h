#ifndef BASE_SUBEVENT_H
#define BASE_SUBEVENT_H

#include <stdint.h>


namespace base {


   /** Current time model
    *  There are three different time representations:
    *    local stamp - 64-bit unsigned integer (LocalStamp_t), can be analyzed only locally
    *    local time  - double in ns units (GlobalTime_t), used to adjust all local differences to common basis
    *    global time - double in ns units (GlobalTime_t), universal time used for global actions like ROI declaration
    *  In case when stream does not required time synchronization local time automatically used as global
    */

   /** type for generic representation of local stamp
     * if necessary, can be made more complex,
     * for a moment arbitrary 64-bit value */
   typedef uint64_t LocalStamp_t;

   /** type for global time stamp, valid for all subsystems
     * should be reasonable values in nanoseconds
     * for a moment double precision should be enough */
   typedef double GlobalTime_t;


   /** SubEvent - base class for all event structures
    * Need for: virtual destructor - to be able delete any instance
    *                   Reset - to be able reset (clean) all collections
    * */

   class SubEvent {
      public:
         SubEvent() {}

         virtual ~SubEvent() {}

         virtual void Reset() {}

   };

}



#endif
