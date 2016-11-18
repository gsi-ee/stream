#ifndef GET4_ITERATOR_H
#define GET4_ITERATOR_H

#include "base/Iterator.h"

#include "get4/Message.h"

#include "base/TimeStamp.h"


namespace get4 {

   class Iterator : public base::Iterator {
      protected:
         enum { MaxGet4=16 };

         uint32_t         fEpoch;            //! current epoch

         get4::Message    fMsg;              //! current read message
         uint32_t         fEpoch2[MaxGet4];  //! current epoch2 for each Get4

         base::LocalStampConverter fConvRoc;   //! use to covert time stamps from ROC messages
         base::LocalStampConverter fConvGet4;  //! use to covert time stamps from Get4 messages

      public:
         Iterator(int fmt = base::formatNormal);

         ~Iterator();

         void setRocNumber(uint16_t rocnum = 0);

         inline bool next()
         {
            if ((fBuffer==0) || (fBufferPos>=fBufferLen)) return false;

            if (fMsg.assign((uint8_t*) fBuffer + fBufferPos, fFormat)) {
               fBufferPos += fMsgSize;
               switch (fMsg.getMessageType()) {
                  case base::MSG_EPOCH:
                     fEpoch = fMsg.getEpochNumber();
                     fConvRoc.MoveRef(((uint64_t) fEpoch) << 14);
                     break;
                  case base::MSG_EPOCH2:
                     fEpoch2[fMsg.getEpoch2ChipNumber() & 0xf] = fMsg.getEpoch2Number();
                     fConvGet4.MoveRef(((uint64_t) fMsg.getEpoch2Number()) << 19);
                     break;
                  case base::MSG_SYS:
                     if (fMsg.is32bitEpoch2()) {
                        fEpoch2[fMsg.getGet4V10R32ChipId() & 0xf] = fMsg.getGet4V10R32EpochNumber();
                        fConvGet4.MoveRef(((uint64_t) fMsg.getGet4V10R32EpochNumber()) << 19);
                     }
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

         inline Message& msg() { return fMsg; }

         /** Return message stamp in ns units, 46 = 14+32 bits wide */
         inline uint64_t getMsgStamp() const
         {
            switch (fMsg.getMessageType()) {
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

         /** Return message stamp in 50 ps units, 47 = 19+28 bits wide */
         inline uint64_t getMsgStamp2() const
         {
            switch (fMsg.getMessageType()) {
               case base::MSG_EPOCH2:
                  return FullTimeStamp2(fMsg.getEpoch2Number(), 0);
               case base::MSG_GET4:
                  return FullTimeStamp2(fEpoch2[fMsg.getGet4Number() & 0xf], fMsg.getGet4Ts());
               case base::MSG_SYS:
                  if (fMsg.isGet4V10R32()) {
                     switch (fMsg.getGet4V10R32MessageType()) {
                        case MSG_GET4_EPOCH: return FullTimeStamp2(fMsg.getGet4V10R32EpochNumber(), 0);
                        case MSG_GET4_SLOWCTRL: return FullTimeStamp2(fEpoch2[fMsg.getGet4V10R32ChipId() & 0xf], 0);
                        case MSG_GET4_ERROR: return FullTimeStamp2(fEpoch2[fMsg.getGet4V10R32ChipId() & 0xf], 0);
                        case MSG_GET4_HIT: return FullTimeStamp2(fEpoch2[fMsg.getGet4V10R32ChipId() & 0xf], fMsg.getGet4V10R32HitTimeBin());
                     }
                  }
            }
            return 0;
         }

         /** Method return consistent time in seconds */
         double getMsgTime() const
         {
            switch (fMsg.getMessageType()) {
               case base::MSG_EPOCH:
                  return fConvRoc.ToSeconds(FullTimeStamp(fMsg.getEpochNumber(), 0));
               case base::MSG_SYNC:
                  return fConvRoc.ToSeconds(FullTimeStamp((fMsg.getSyncEpochLSB() == (fEpoch & 0x1)) ? fEpoch : fEpoch - 1, fMsg.getSyncTs()));
               case base::MSG_AUX:
                  return fConvRoc.ToSeconds(FullTimeStamp((fMsg.getAuxEpochLSB() == (fEpoch & 0x1)) ? fEpoch : fEpoch - 1, fMsg.getAuxTs()));
               case base::MSG_EPOCH2:
                  return fConvGet4.ToSeconds(FullTimeStamp2(fMsg.getEpoch2Number(), 0));
               case base::MSG_GET4:
                  return fConvGet4.ToSeconds(FullTimeStamp2(fEpoch2[fMsg.getGet4Number() & 0xf], fMsg.getGet4Ts()));
               case base::MSG_SYS:
                  if (fMsg.isGet4V10R32())
                     return fConvGet4.ToSeconds(getMsgStamp2());
                  else
                     return fConvRoc.ToSeconds(FullTimeStamp(fEpoch, 0));
            }
            return 0;
         }

         void printMessage(unsigned kind = base::msg_print_Prefix | base::msg_print_Data);

         void printMessages(unsigned cnt = 100, unsigned kind = base::msg_print_Prefix | base::msg_print_Data);

         //! Expanded timestamp for 250/8*5 MHz * 19 bit epochs
         inline static uint64_t FullTimeStamp2(uint32_t epoch, uint32_t stamp)
            { return ((uint64_t) epoch << 19) | (stamp & 0x7ffff); }

   };
}


#endif
