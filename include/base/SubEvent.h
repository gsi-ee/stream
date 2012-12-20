#ifndef BASE_SUBEVENT_H
#define BASE_SUBEVENT_H

#include <stdint.h>

#include <algorithm>
#include <vector>
#include <exception>


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


   /** class TimeEngine should handle stamps.
    *
    */

   class LocalStampConverter {
      protected:
         uint64_t fT0;          //! time stamp, used as t0 for time production
         uint64_t fWrapSize;    //! value of time stamp which wraps - MUST be power of 2
         uint64_t fHalfWrapSize; //! fWrapSize/2 - used very often in calculations
         uint64_t fValueMask;   //! mask to extract bits related to stamp

         uint64_t fCurrentWrap; //! summed wraps since begin
         LocalStamp_t fRef;     //! reference time, used to detect wraps of timestamp
         int64_t  fConvRef;     //! value used for time conversion

         double fCoef;  // time coefficient to convert to seconds

      public:

         LocalStampConverter() :
            fT0(0),
            fWrapSize(2),
            fHalfWrapSize(1),
            fValueMask(1),
            fCurrentWrap(0),
            fRef(0),
            fConvRef(0),
            fCoef(1.)
         {
         }

         ~LocalStampConverter() {}

         void SetT0(uint64_t t0) { fT0 = t0; }

         /** Set major timing parameters - wrap value and coefficient */
         void SetTimeSystem(unsigned wrapbits, double coef)
         {
            fWrapSize = ((uint64_t) 1) << wrapbits;
            fHalfWrapSize = fWrapSize/2;
            fValueMask = fWrapSize - 1;
            fCoef = coef;

            // TODO: should it be done here???
            MoveRef(0);
         }

         /** Method calculates distance between two stamps
          * In simplest case just should return t2-t1 */
         int64_t distance(LocalStamp_t t1, LocalStamp_t t2)
         {
            uint64_t diff = (t2 - t1) & fValueMask;
            return diff < fWrapSize/2 ? (int64_t) diff : (int64_t)diff - fWrapSize;
         }

         /** Method returns abs(t1-t2) */
         uint64_t abs_distance(LocalStamp_t t1, LocalStamp_t t2)
         {
            uint64_t diff = (t2 - t1) & fValueMask;
            return diff < fWrapSize/2 ? diff : fWrapSize - diff;
         }

         /** Method convert time stamp to seconds,
          * taking into account probable wrap relative to fRef value */
         double ToSeconds(LocalStamp_t stamp)
         {
            // simplest way just return stamp*fCoef,
            // but one should account all possible wraps

            // first estimate distance to the reference
            // stamp could be left or right
            int64_t dist = distance(fRef, stamp);

            // one we found distance, we could calculate full stamp as
            // return (fCurrentWrap + fRef + dist - fT0) * fCoef;
            return (fConvRef + dist) * fCoef;
         }

         /** Move reference to the new position */
         void MoveRef(LocalStamp_t newref)
         {
            // when new reference smaller than previous
            // one should check if distance small that we could believe it is normal
            if (newref < fRef)
               if (abs_distance(fRef, newref) < fWrapSize/2)
                  fCurrentWrap+=fWrapSize;

            fRef = newref;

            // precalculate value, which will be used for conversion
            fConvRef = int64_t(fCurrentWrap + fRef) - fT0;
         }
   };


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
