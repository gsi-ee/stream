#ifndef BASE_SUBEVENT_H
#define BASE_SUBEVENT_H

#include <algorithm>
#include <vector>
#include <exception>

namespace base {


   /** SubEvent - base class for all event structures
    * Need for: virtual destructor - to be able delete any instance
    *                   Reset - to be able reset (clean) all collections
    * */

   class SubEvent {
      public:
         SubEvent() {}

         virtual ~SubEvent() {}

         virtual void Clear() {}

         virtual void Sort() {}

         /** Method returns event multiplicity - that ever it means */
         virtual unsigned Multiplicity() const { return 0; }

   };

   template<class MsgClass>
   class MessageExt {
      protected:
         /* original roc message*/
         MsgClass fMessage;

         /* full time stamp without correction*/
         double fGlobalTime;

      public:

         MessageExt() :
            fMessage(),
            fGlobalTime(0.)
         {
         }

         MessageExt(const MsgClass& _msg, double globaltm) :
            fMessage(_msg),
            fGlobalTime(globaltm)
         {
         }

         MessageExt(const MessageExt& src) :
            fMessage(src.fMessage),
            fGlobalTime(src.fGlobalTime)
         {
         }

         MessageExt& operator=(const MessageExt& src)
         {
            fMessage = src.fMessage;
            fGlobalTime = src.fGlobalTime;
            return *this;
         }

         ~MessageExt() {}

         /* this is used for timesorting the messages in the filled vectors */
         bool operator<(const MessageExt &rhs) const
            { return (fGlobalTime < rhs.fGlobalTime); }

         const MsgClass& msg() const { return fMessage; }

         double GetGlobalTime() const { return fGlobalTime; }
   };


   template<class MsgClass>
   class SubEventEx : public base::SubEvent {
      protected:
         std::vector<MsgClass> fExtMessages;   //!< vector of extended messages

      public:

         SubEventEx() : base::SubEvent(), fExtMessages()  {}

         ~SubEventEx() {}

         /** Add new message to sub-event */
         void AddMsg(const MsgClass& _msg) { fExtMessages.push_back(_msg); }

         /** Returns number of messages */
         unsigned Size() const { return fExtMessages.size(); }

         /** Returns capacity of the message container */
         unsigned Capacity() const { return fExtMessages.capacity(); }

         /** Change capacity of the container */
         void SetCapacity(unsigned sz) { fExtMessages.reserve(sz); }

         /** Returns message with specified index */
         MsgClass& msg(unsigned indx) { return fExtMessages[indx]; }

         /** Returns pointer on vector with messages, used in the store */
         std::vector<MsgClass>* vect_ptr() { return &fExtMessages; }

         /** Returns subevent multiplicity  */
         virtual unsigned Multiplicity() const { return Size(); }

         /** Clear subevent - remove all messages */
         virtual void Clear() { fExtMessages.clear(); }

         /** Do time sorting of messages */
         virtual void Sort()
         {
            std::sort(fExtMessages.begin(), fExtMessages.end());
         }

   };

}

#endif
