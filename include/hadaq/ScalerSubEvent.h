#ifndef HADAQ_SCALERSUBEVENT_H
#define HADAQ_SCALERSUBEVENT_H

#include "base/SubEvent.h"

namespace hadaq {

   /** \brief Output float message for Scaler
     *
     * \ingroup stream_hadaq_classes */

   struct ScalerSubEvent : public base::SubEvent {
      bool valid = true;
      uint64_t scaler1 = 0;    ///< scaler 1
      uint64_t scaler2 = 0;    ///< scaler 2

      ScalerSubEvent() = default;

      ScalerSubEvent(uint64_t s1, uint64_t s2, bool v = true)
      {
         valid = v;
         scaler1 = s1;
         scaler2 = s2;
      }

      /** clear sub event */
      void Clear() override
      {
         valid = true;
         scaler1 = 0;
         scaler2 = 0;
      }
   };
}

#endif
