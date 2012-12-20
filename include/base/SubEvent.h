#ifndef BASE_SUBEVENT_H
#define BASE_SUBEVENT_H

#include <stdint.h>

#include <algorithm>
#include <vector>


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

         MessageExt(const MsgClass& msg, double globaltm) :
            fMessage(msg),
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
         void AddMsg(const MsgClass& msg) { fExtMessages.push_back(msg); }

         /** Returns number of messages */
         unsigned Size() const { return fExtMessages.size(); }

         /** Returns message with specified index */
         MsgClass& msg(unsigned indx) { return fExtMessages[indx]; }

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
