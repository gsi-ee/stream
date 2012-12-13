#ifndef GET4_ITERATOR_H
#define GET4_ITERATOR_H

#include "base/Iterator.h"

#include "get4/Message.h"

namespace get4 {

   class Iterator : public base::Iterator {
      protected:
         enum { MaxGet4=16 };

         get4::Message    fMsg;              // current read message
         uint32_t         fEpoch2[MaxGet4];  // current epoch2 for each Get4

      public:
         Iterator(int fmt = base::formatNormal);

         Iterator(const Iterator& src);

         virtual ~Iterator();

         void setRocNumber(uint16_t rocnum = 0);

         void resetEpochs();

         inline bool next()
         {
            if ((fBuffer==0) || (fBufferPos>=fBufferLen)) return false;

            if (fMsg.assign((uint8_t*) fBuffer + fBufferPos, fFormat)) {
               fBufferPos += fMsgSize;
               switch (fMsg.getMessageType()) {
                  case base::MSG_EPOCH:
                     fEpoch = fMsg.getEpochNumber();
                     break;
                  case base::MSG_EPOCH2:
                     if (fMsg.getEpoch2ChipNumber() < MaxGet4)
                        fEpoch2[fMsg.getEpoch2ChipNumber()] = fMsg.getEpoch2Number();
                     break;
               }
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

         uint32_t getLastEpoch2(unsigned n=0) const { return n<MaxGet4 ? fEpoch2[n] : 0; }

         inline Message& msg() { return fMsg; }

         inline uint32_t getMsgEpoch() const
         {
            switch(fMsg.getMessageType()) {
               case base::MSG_EPOCH2:
//     return (fMsg.getEpoch2ChipNumber() < MaxGet4) ? fEpoch2[fMsg.getEpoch2ChipNumber()] : 0;
                  return fMsg.getEpoch2Number();
               case base::MSG_GET4:
                  return (fMsg.getGet4Number() < MaxGet4) ? fEpoch2[fMsg.getGet4Number()] : 0;
            }
            return fEpoch;
         }

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
