#ifndef NX_ITERATOR_H
#define NX_ITERATOR_H

#include "base/Iterator.h"

#include "nx/Message.h"

namespace nx {

   class Iterator : public base::Iterator {
      protected:

         nx::Message    fMsg;                // current read message

      public:
         Iterator(int fmt = base::formatNormal);

         Iterator(const Iterator& src);

         virtual ~Iterator();

         void setRocNumber(uint16_t rocnum = 0);

         inline bool next()
         {
            if ((fBuffer==0) || (fBufferPos>=fBufferLen)) return false;

            if (fMsg.assign((uint8_t*) fBuffer + fBufferPos, fFormat)) {
               fBufferPos += fMsgSize;
               if (fMsg.getMessageType() == base::MSG_EPOCH)
                  fEpoch = fMsg.getEpochNumber();
               return true;
            }
            fBufferPos = fBufferLen;
            return false;
         }

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

         inline uint64_t getMsgFullTime() const
         {
            return fMsg.getMsgFullTime(getMsgEpoch());
         }

         inline double getMsgFullTimeD() const
         {
            return fMsg.getMsgFullTimeD(getMsgEpoch());
         }

         void printMessage(unsigned kind = base::msg_print_Prefix | base::msg_print_Data);

         void printMessages(unsigned cnt = 100, unsigned kind = base::msg_print_Prefix | base::msg_print_Data);
   };
}


#endif
