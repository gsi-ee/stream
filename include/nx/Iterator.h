#ifndef NX_ITERATOR_H
#define NX_ITERATOR_H

#include "base/Iterator.h"

#include <vector>

#include "base/TimeStamp.h"

#include "nx/Message.h"

namespace nx {

   class Iterator : public base::Iterator {
      protected:

         uint32_t                   fEpoch;     //! current epoch

         nx::Message                fMsg;       //! current read message

         base::LocalStampConverter  fConv;      //! use to covert time stamps in seconds

         bool                       fCorrecion; //! is message correction is enabled

         std::vector<uint64_t>      fLastNxHit; //! last stamp of nx hit

         unsigned                   fNxTmDistance; //! allowed negative distance between consequent hits - to recognize problems

         int                        fVerifyRes;

         void VerifyMessage();

      public:
         Iterator(int fmt = base::formatNormal);

         ~Iterator();

         void setRocNumber(uint16_t rocnum = 0);

         void SetCorrection(bool enabled, unsigned nxmask = 0xf, unsigned nx_distance = 800);

         bool IsCorrection() const { return fCorrecion; }
         int LastCorrectionRes() const { return fVerifyRes; }

         inline bool next()
         {
            if ((fBuffer==0) || (fBufferPos>=fBufferLen)) return false;

            if (fMsg.assign((uint8_t*) fBuffer + fBufferPos, fFormat)) {
               fBufferPos += fMsgSize;
               if (fMsg.getMessageType() == base::MSG_EPOCH) {
                  fEpoch = fMsg.getEpochNumber();
                  // set new point for the time reference
                  fConv.MoveRef(((uint64_t) fEpoch) << 14);
               }
               if (fCorrecion && fMsg.isHitMsg())
                  VerifyMessage();
               return true;
            }
            fBufferPos = fBufferLen;
            return false;
         }

         int GetVerifyResult() const { return fVerifyRes; }

         // can be used only inside buffer, not with board source
         inline bool last()
         {
            if ((fBuffer==0) || (fBufferLen<fMsgSize)) return false;

            fBufferPos = (fBufferLen - fMsgSize) / fMsgSize * fMsgSize;

            return next();
         }

         // can be used only inside buffer, not with board source
         inline bool prev()
         {
            if ((fBuffer==0) || (fBufferPos<fMsgSize*2)) return false;

            fBufferPos -= fMsgSize*2;

            return next();
         }

         inline Message& msg() { return fMsg; }


         /** Return message stamp in ns units, 46 = 14+32 bits wide */
         inline uint64_t getMsgStamp() const
         {
            switch (fMsg.getMessageType()) {
               case base::MSG_HIT:
                  return FullTimeStamp(fMsg.getNxLastEpoch() ? fEpoch - 1 : fEpoch, fMsg.getNxTs());
               case base::MSG_EPOCH:
                  return FullTimeStamp(fMsg.getEpochNumber(), 0);
               case base::MSG_SYNC:
                  return FullTimeStamp((fMsg.getSyncEpochLSB() == (fEpoch & 0x1)) ? fEpoch : fEpoch - 1, fMsg.getSyncTs());
               case base::MSG_AUX:
                  return FullTimeStamp((fMsg.getAuxEpochLSB() == (fEpoch & 0x1)) ? fEpoch : fEpoch - 1, fMsg.getAuxTs());
               case base::MSG_SYS:
                  return FullTimeStamp(fEpoch, 0);
            }
            // this is not important, but just return meaningful value not far away from last
            return FullTimeStamp(fEpoch, 0);
         }

         /** Return message time in seconds */
         inline double getMsgTime() const
         {
            return fConv.ToSeconds(getMsgStamp());
         }

         /** Method converts stamp value to double */
         inline double StampToTime(uint64_t stamp) const
         {
            return fConv.ToSeconds(stamp);
         }

         void printMessage(unsigned kind = base::msg_print_Prefix | base::msg_print_Data);

         void printMessages(unsigned cnt = 100, unsigned kind = base::msg_print_Prefix | base::msg_print_Data);
   };
}


#endif
