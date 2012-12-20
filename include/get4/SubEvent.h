#ifndef GET4_SUBEVENT_H
#define GET4_SUBEVENT_H

#include "base/SubEvent.h"

#include "get4/Message.h"

#include <vector>
#include <algorithm>

namespace get4 {


   /*
    * Extended message container. Keeps original ROC message, but adds full timestamp and
    * optionally corrected adc valules.
    * Note that extended messages inside the vector will be sorted after full timestamp
    * by the TRocProc::FinalizeEvent
    *
    */

   class MessageExtended {
      protected:
         /* original get4 message*/
         get4::Message fMessage;

         /* full time stamp without correction*/
         double fGlobalTime;

      public:

         MessageExtended() :
            fMessage(),
            fGlobalTime(0.)
         {
         }

         MessageExtended(const get4::Message& msg, double globaltm) :
            fMessage(msg),
            fGlobalTime(globaltm)
         {
         }

         MessageExtended(const MessageExtended& src) :
            fMessage(src.fMessage),
            fGlobalTime(src.fGlobalTime)
         {
         }

         MessageExtended& operator=(const MessageExtended& src)
         {
            fMessage = src.fMessage;
            fGlobalTime = src.fGlobalTime;
            return *this;
         }

         ~MessageExtended() {}

         /* this is used for timesorting the messages in the filled vectors */
         bool operator<(const MessageExtended &rhs) const
            { return (fGlobalTime < rhs.fGlobalTime); }

         const get4::Message& msg() const { return fMessage; }

         double GetGlobalTime() const { return fGlobalTime; }
   };


   class SubEvent : public base::SubEvent {
      public:
         std::vector<get4::MessageExtended> fExtMessages;   //!< vector of extended messages

         SubEvent() : base::SubEvent(), fExtMessages()  {}
         virtual ~SubEvent() {}

         virtual void Reset()
         {
            fExtMessages.clear();
         }

         virtual void Sort()
         {
            std::sort(fExtMessages.begin(), fExtMessages.end());
         }
   };

}


#endif
