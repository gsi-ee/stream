#ifndef HADAQ_TDCSUBEVENT_H
#define HADAQ_TDCSUBEVENT_H

#include "base/SubEvent.h"

#include "hadaq/TdcMessage.h"

#include <vector>

#include <algorithm>

namespace hadaq {


   /** Extended message container.
    * Keeps original TDC message and adds full timestamp
    * in global form. */

   class TdcMessageExt {
      protected:
         /* original roc message*/
         hadaq::TdcMessage fMessage;

         /* full time stamp without correction*/
         double fGlobalTime;

      public:

         TdcMessageExt() :
            fMessage(),
            fGlobalTime(0.)
         {
         }

         TdcMessageExt(const hadaq::TdcMessage& msg, double globaltm) :
            fMessage(msg),
            fGlobalTime(globaltm)
         {
         }

         TdcMessageExt(const TdcMessageExt& src) :
            fMessage(src.fMessage),
            fGlobalTime(src.fGlobalTime)
         {
         }

         TdcMessageExt& operator=(const TdcMessageExt& src)
         {
            fMessage = src.fMessage;
            fGlobalTime = src.fGlobalTime;
            return *this;
         }

         ~TdcMessageExt() {}

         /* this is used for timesorting the messages in the filled vectors */
         bool operator<(const TdcMessageExt &rhs) const
            { return (fGlobalTime < rhs.fGlobalTime); }

         const hadaq::TdcMessage& msg() const { return fMessage; }

         double GetGlobalTime() const { return fGlobalTime; }
   };


   class TdcSubEvent : public base::SubEvent {
      public:
         std::vector<hadaq::TdcMessageExt> fExtMessages;   //!< vector of extended messages

         TdcSubEvent() : base::SubEvent(), fExtMessages()  {}

         ~TdcSubEvent() {}

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
