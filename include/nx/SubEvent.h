#ifndef NX_SUBEVENT_H
#define NX_SUBEVENT_H

#include "base/SubEvent.h"

#include "nx/Message.h"

#include <vector>

namespace nx {


   /*
    * Extended message container. Keeps original ROC message, but adds full timestamp and
    * optionally corrected adc valules.
    * Note that extended messages inside the vector will be sorted after full timestamp
    * by the TRocProc::FinalizeEvent
    *
    */

   class MessageExtended {
      protected:
         /* original roc message*/
         nx::Message fMessage;

         /* full time stamp without correction*/
         double fGlobalTime;

         /* corrected adc value*/
         float fCorrectedADC;

      public:

         MessageExtended() :
            fMessage(),
            fGlobalTime(0.),
            fCorrectedADC(0)
         {
         }

         MessageExtended(const nx::Message& msg, double globaltm) :
            fMessage(msg),
            fGlobalTime(globaltm),
            fCorrectedADC(0)
         {
         }

         MessageExtended(const MessageExtended& src) :
            fMessage(src.fMessage),
            fGlobalTime(src.fGlobalTime),
            fCorrectedADC(src.fCorrectedADC)
         {
         }

         MessageExtended& operator=(const MessageExtended& src)
         {
            fMessage = src.fMessage;
            fGlobalTime = src.fGlobalTime;
            fCorrectedADC = src.fCorrectedADC;
            return *this;
         }

         ~MessageExtended() {}

         /* this is used for timesorting the messages in the filled vectors */
         bool operator<(const MessageExtended &rhs) const
            { return (fGlobalTime < rhs.fGlobalTime); }

         const nx::Message& msg() const { return fMessage; }

         void SetCorrectedADC(float val) { fCorrectedADC = val; }
         float GetCorrectedNxADC() const { return fCorrectedADC; }

         double GetGlobalTime() const { return fGlobalTime; }
   };


   class SubEvent : public base::SubEvent {
      public:
         std::vector<nx::MessageExtended> fExtMessages;   //!< vector of extended messages

         SubEvent() : base::SubEvent(), fExtMessages()  {}
         ~SubEvent() {}

         virtual void Reset()
         {
            fExtMessages.clear();
         }
   };

}



#endif
