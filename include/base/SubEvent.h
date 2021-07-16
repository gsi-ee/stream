#ifndef BASE_SUBEVENT_H
#define BASE_SUBEVENT_H

#include <algorithm>
#include <vector>

namespace base {


   /** SubEvent - base class for all event structures
    * Need for: virtual destructor - to be able delete any instance
    *   Reset - to be able reset (clean) all collections */

   class SubEvent {
      public:
         /** default constructor */
         SubEvent() {}

         /** destructor */
         virtual ~SubEvent() {}

         /** clear sub event */
         virtual void Clear() {}

         /** sort data in sub event */
         virtual void Sort() {}

         /** Method returns event multiplicity - that ever it means */
         virtual unsigned Multiplicity() const { return 0; }

   };

   /** Extended message - any message plus global time stamp */

   template<class MsgClass>
   class MessageExt {
      protected:
         MsgClass fMessage; ///< original roc message

         double fGlobalTime;  ///< full time stamp without correction

      public:

         /** default constructor */
         MessageExt() :
            fMessage(),
            fGlobalTime(0.)
         {
         }

         /** constructor */
         MessageExt(const MsgClass& _msg, double globaltm) :
            fMessage(_msg),
            fGlobalTime(globaltm)
         {
         }

         /** copy constructor */
         MessageExt(const MessageExt& src) :
            fMessage(src.fMessage),
            fGlobalTime(src.fGlobalTime)
         {
         }

         /** assign operator */
         MessageExt& operator=(const MessageExt& src)
         {
            fMessage = src.fMessage;
            fGlobalTime = src.fGlobalTime;
            return *this;
         }

         /** destructor */
         ~MessageExt() {}

         /** this is used for timesorting the messages in the filled vectors */
         bool operator<(const MessageExt &rhs) const
            { return (fGlobalTime < rhs.fGlobalTime); }

         /** message */
         const MsgClass& msg() const { return fMessage; }

         /** global time stamp */
         double GetGlobalTime() const { return fGlobalTime; }
   };


   /** Subevent with vector of extended messages*/

   template<class MsgClass>
   class SubEventEx : public base::SubEvent {
      protected:
         std::vector<MsgClass> fExtMessages;   ///< vector of extended messages

      public:

         /** constructor */
         SubEventEx(unsigned capacity = 0) : base::SubEvent(), fExtMessages()  { fExtMessages.reserve(capacity); }

         /** destructor */
         ~SubEventEx() {}

         /** Add new message to sub-event */
         void AddMsg(const MsgClass &_msg) { fExtMessages.emplace_back(_msg); }

         /** Returns number of messages */
         unsigned Size() const { return fExtMessages.size(); }

         /** Returns capacity of the message container */
         unsigned Capacity() const { return fExtMessages.capacity(); }

         /** Change capacity of the container */
         void SetCapacity(unsigned sz) { fExtMessages.reserve(sz); }

         /** Returns message with specified index */
         MsgClass &msg(unsigned indx) { return fExtMessages[indx]; }

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
