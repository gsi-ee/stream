#ifndef NX_SUBEVENT_H
#define NX_SUBEVENT_H

#include "base/SubEvent.h"

#include "nx/Message.h"

namespace nx {

   /**  Extended message container.
    * Keeps original ROC message, but adds full timestamp and
    * optionally corrected adc valules. */

   class MessageExt : public base::MessageExt<nx::Message> {
      protected:
         /* corrected adc value*/
         float fCorrectedADC;

      public:

         MessageExt() : base::MessageExt<nx::Message>(), fCorrectedADC(0) {}

         MessageExt(const nx::Message& msg, double globaltm, float adc = 0.) :
            base::MessageExt<nx::Message>(msg, globaltm),
            fCorrectedADC(adc)
         {
         }

         MessageExt(const MessageExt& src) :
            base::MessageExt<nx::Message>(src),
            fCorrectedADC(src.fCorrectedADC)
         {
         }

         ~MessageExt() {}

         void SetCorrectedADC(float val) { fCorrectedADC = val; }
         float GetCorrectedNxADC() const { return fCorrectedADC; }
         bool isCorrectedADC() const { return fCorrectedADC!=0.; }
   };


   typedef base::SubEventEx<nx::MessageExt> SubEvent;
}



#endif
