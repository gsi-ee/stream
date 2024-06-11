#ifndef HADAQ_SCALERSUBEVENT_H
#define HADAQ_SCALERSUBEVENT_H

#include "base/SubEvent.h"

namespace hadaq {

   /** \brief Output float message for Scaler
     *
     * \ingroup stream_hadaq_classes */

   struct ScalerSubEvent : public base::SubEvent {
      uint64_t scaler1 = 0;    ///< scaler 1
      uint64_t scaler2 = 0;    ///< scaler 1

      ScalerSubEvent() = default;

      ScalerSubEvent(uint64_t s1, uint64_t s2)
      {
         scaler1 = s1;
         scaler2 = s2;
      }

      /** clear sub event */
      void Clear() override
      {
         scaler1 = 0;
         scaler2 = 0;
      }
   };
}

#endif
