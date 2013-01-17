#ifndef GET4_MESSAGE_H
#define GET4_MESSAGE_H

#include "base/Message.h"

#include <string.h>

namespace get4 {

   class Message : public base::Message {
      public:
         Message() : base::Message() {}

         inline bool assign(void* src, int fmt = base::formatNormal)
         {
            switch (fmt) {
               case base::formatEth2:
                  memcpy(&data, src, 6);
                  return true;
               case base::formatOptic2:
                  data = (*((uint64_t*) src) >> 16) | (*((uint64_t*) src) << 48);
                  // memcpy(&data, (uint8_t*) src + 2, 6);
                  // setRocNumber(*((uint16_t*) src));
                  return true;
               case base::formatNormal:
                  memcpy(&data, src, 8);
                  return true;
            }

            return false;
         }


         // ---------- Epoch2 marker access methods ------------

         //! For Epoch2 data: Returns epoch missmatch flag (set in ROC when
         //! ROC timestamp and timestamp send by GET4 did not match) (1 bit field)
         inline uint32_t getEpoch2EpochMissmatch() const { return getBit(4); }

         //! For Epoch2 data: Returns epoch-lost flag (1 bit field)
         inline uint32_t getEpoch2EpochLost() const { return getBit(5); }

         //! For Epoch2 data: Returns data-lost flag (1 bit field)
         inline uint32_t getEpoch2DataLost() const { return getBit(6); }

         //! For Epoch2 data: Returns sync flag (1 bit field)
         inline uint32_t getEpoch2Sync() const { return getBit(7); }

         //! For Epoch2 data: Returns the LTS156 bits 11 to 8. This
         //! gives information at what time in the Epoche the epoche number
         //! was set (4 bit field)
         inline uint32_t getEpoch2StampTime() const { return getField(8, 4); }

         //! For Epoch2 data: Returns the epoch number (28 bit field)
         // inline uint32_t getEpoch2Number() const { return getField(12, 20); }
         // on some machines 32-bit field is not working
         // inline uint32_t getEpoch2Number() const { return getField(12, 32); }
         inline uint32_t getEpoch2Number() const { return data >> 12; }

         //! For Epoch2 data: Returns the number of the GET4 chip that send
         //! the epoche message (8 bit field)
         inline uint32_t getEpoch2ChipNumber() const { return getField(44, 4); }

         //! For Epoch2 data: Returns the CRC-8 of the rest of the message
         //! (not yet implemented in HW) (8 bit field)
         inline uint32_t getEpoch2CRC() const { return getField(40, 8); }

         //! For Epoch2 data: Set epoch missmatch flag (1 bit field)
         inline void setEpoch2EpochMissmatch(uint32_t v) { setBit(4, v); }

         //! For Epoch2 data: Set epoch-lost flag (1 bit field)
         inline void setEpoch2EpochLost(uint32_t v) { setBit(5, v); }

         //! For Epoch2 data: Set data-lost flag (1 bit field)
         inline void setEpoch2DataLost(uint32_t v) { setBit(6, v); }

         //! For Epoch2 data: Set sync flag (1 bit field)
         inline void setEpoch2Sync(uint32_t v) { setBit(7, v); }

         //! For Epoch2 data: Set the LTS156 bits 11 to 8. This
         //! gives information at what time in the Epoche the epoche number
         //! was set (4 bit field)
         inline void setEpoch2StampTime(uint32_t v) { setField(8, 4, v); }

         //! For Epoch2 data: Set the epoch number (20 bit field)
         inline void setEpoch2Number(uint32_t v) { setField(12, 32, v); }

         //! For Epoch2 data: Set the number of the GET4 chip that send
         //! the epoche message (8 bit field)
         inline void setEpoch2ChipNumber(uint32_t v) { setField(44, 4, v); }

         //! For Epoch2 data: Set the CRC-8 of the rest of the message
         //! (not yet implemented in HW) (8 bit field)
         inline void setEpoch2CRC(uint32_t v) { setField(40, 8, v); }

         // ---------- Get4 Hit data access methods ----------------

         //! For Get4 data: Returns Get4 chip number (6 bit field)
         inline uint8_t getGet4Number() const { return getField(6, 6); }

         //! For Get4 data: Returns Get4 channel number (2 bit field)
         inline uint8_t getGet4ChNum() const { return getField(12, 2); }

         //! For Get4 data: Returns Get4 time stamp, 50 ps binning (19 bit field)
         inline uint32_t getGet4Ts() const { return getField(14, 19); }

         //! For Get4 data: Returns Get4 rising (=1) or falling (=0) edge (1 bit field)
         inline uint32_t getGet4Edge() const { return getBit(33); }

         //! For Get4 data: Returns the CRC-8 of the rest of the message.
         //! For details check the ROC documentation. (8 bit field)
         inline uint32_t getGet4CRC() const { return getField(40, 8); }


         //! For Get4 data: Sets Get4 chip number (6 bit field)
         inline void setGet4Number(uint8_t v) { setField(6, 6, v); }

         //! For Get4 data: Sets Get4 channel number (2 bit field)
         inline void setGet4ChNum(uint8_t v) { setField(12, 2, v); }

         //! For Get4 data: Sets Get4 time stamp, 50 ps binning (19 bit field)
         inline void setGet4Ts(uint32_t v) { setField(14, 19, v); }

         //! For Get4 data: Sets Get4 rising or falling edge (1 bit field)
         inline void setGet4Edge(uint32_t v) { setBit(33, v); }

         //! For Get4 data: Set the CRC-8 of the rest of the message
         //! For details check the ROC documentation. (8 bit field)
         inline void setGet4CRC(uint32_t v) { setField(40, 8, v); }


         //! Returns \a true is message type is #MSG_EPOCH2 (epoch2 marker)
         inline bool isEpoch2Msg() const { return getMessageType() == base::MSG_EPOCH2;}
         //! Returns \a true is message type is #MSG_GET4 (Get4 hit data)
         inline bool isGet4Msg() const { return getMessageType() == base::MSG_GET4; }

         void printData(unsigned kind = base::msg_print_Prefix | base::msg_print_Data, uint32_t epoch = 0, double localtm = 0) const;

         void printData(std::ostream& os, unsigned kind = base::msg_print_Human,  uint32_t epoch = 0, double localtm = 0) const;

      protected:
   };


}


#endif
