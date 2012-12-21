#ifndef BASE_TIMESTAMP_H
#define BASE_TIMESTAMP_H


#include <stdint.h>


namespace base {

   /** Current time model
    *  There are three different time representations:
    *    local stamp - 64-bit unsigned integer (LocalStamp_t), can be analyzed only locally
    *    local time  - double in seconds (GlobalTime_t), adjust all local differences, should be without wrap
    *    global time - double in seconds (GlobalTime_t), universal time used for global actions like RoI declaration
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


   /** LocalStampConverter class should perform
    *  conversion of time stamps to time in seconds.
    *  Main problem to solve - handle correctly time stamp wraps.
    */

   class LocalStampConverter {
      protected:
         int64_t  fT0;          //! time stamp, used as t0 for time production
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

         void SetT0(int64_t t0) { fT0 = t0; }

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
         int64_t distance(LocalStamp_t t1, LocalStamp_t t2) const
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
         double ToSeconds(LocalStamp_t stamp) const
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
            if (newref < fRef) {
               int64_t shift = distance(fRef, newref);
               if ((shift>0) && (shift < (int64_t) fWrapSize/2))
                  fCurrentWrap+=fWrapSize;
            }

            fRef = newref;

            // precalculate value, which will be used for conversion
            fConvRef = int64_t(fCurrentWrap + fRef) - fT0;
         }
   };

}

#endif
