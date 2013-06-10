#ifndef GET4_MESSAGE_H
#define GET4_MESSAGE_H

#include "base/Message.h"

#include <string.h>

namespace get4 {

   enum SysMessageTypesGet4 {
         SYSMSG_GET4V1_32BIT_0  = 240,     // Get4 V1.0, 32bit mode, Raw messages chip 0 link 1
         SYSMSG_GET4V1_32BIT_1  = 241,     // Get4 V1.0, 32bit mode, Raw messages chip 1 link 1
         SYSMSG_GET4V1_32BIT_2  = 242,     // Get4 V1.0, 32bit mode, Raw messages chip 2 link 1
         SYSMSG_GET4V1_32BIT_3  = 243,     // Get4 V1.0, 32bit mode, Raw messages chip 3 link 1
         SYSMSG_GET4V1_32BIT_4  = 244,     // Get4 V1.0, 32bit mode, Raw messages chip 4 link 1
         SYSMSG_GET4V1_32BIT_5  = 245,     // Get4 V1.0, 32bit mode, Raw messages chip 5 link 1
         SYSMSG_GET4V1_32BIT_6  = 246,     // Get4 V1.0, 32bit mode, Raw messages chip 6 link 1
         SYSMSG_GET4V1_32BIT_7  = 247,     // Get4 V1.0, 32bit mode, Raw messages chip 7 link 1
         SYSMSG_GET4V1_32BIT_8  = 248,     // Get4 V1.0, 32bit mode, Raw messages chip 0 link 2
         SYSMSG_GET4V1_32BIT_9  = 249,     // Get4 V1.0, 32bit mode, Raw messages chip 1 link 2
         SYSMSG_GET4V1_32BIT_10 = 250,     // Get4 V1.0, 32bit mode, Raw messages chip 2 link 2
         SYSMSG_GET4V1_32BIT_11 = 251,     // Get4 V1.0, 32bit mode, Raw messages chip 3 link 2
         SYSMSG_GET4V1_32BIT_12 = 252,     // Get4 V1.0, 32bit mode, Raw messages chip 4 link 2
         SYSMSG_GET4V1_32BIT_13 = 253,     // Get4 V1.0, 32bit mode, Raw messages chip 5 link 2
         SYSMSG_GET4V1_32BIT_14 = 254,     // Get4 V1.0, 32bit mode, Raw messages chip 6 link 2
         SYSMSG_GET4V1_32BIT_15 = 255      // Get4 V1.0, 32bit mode, Raw messages chip 7 link 2
   };

   enum MessageTypes32 {
      MSG_GET4_EPOCH    = 0,
      MSG_GET4_SLOWCTRL = 1,
      MSG_GET4_ERROR    = 2,
      MSG_GET4_HIT      = 3
   };

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


         // ---------- 24bit Epoch2 marker access methods ------------

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

         // ---------- 32 bit: Get4 Hit data access methods ----------------

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

         // ==========================================================================

         // Get4 v1,0 32bits message function (later  general?)
         inline unsigned getFieldBE(unsigned shift, unsigned len) const
            { return (dataBE() >> shift) & ((1 << len) - 1); }
         inline uint8_t getBitBE(unsigned shift) const
            { return (dataBE() >> shift) & 1; }
         inline uint64_t dataBE() const
            { return ( ((uint64_t) getField( 0,  8) & 0x00000000000000FFLU)<<56)+
                     ( ((uint64_t) getField( 0, 16) & 0x000000000000FF00LU)<<40)+
                     ( ((uint64_t) getField( 0, 24) & 0x0000000000FF0000LU)<<24)+
                     ( (((uint64_t)getField( 8, 24) <<8) & 0x00000000FF000000LU)<< 8)+
                     ( (((uint64_t)getField(16, 24))<<8) & 0x00000000FF000000LU)+
                     ( ((uint64_t) getField(24, 24) )& 0x0000000000FF0000LU)+
                     ( ((uint64_t) getField(40, 16) )& 0x000000000000FF00LU)+
                     ( ((uint64_t) getField(56,  8) )& 0x00000000000000FFLU);
              }


         /** \brief Returns true, when 32-bit read message is detected */
         inline bool   isGet4V10R32() const
         { return (getMessageType() == base::MSG_SYS) && (getSysMesType() >= SYSMSG_GET4V1_32BIT_0); }

         inline bool     is32bitEpoch2() const { return isGet4V10R32() && (getGet4V10R32MessageType() == MSG_GET4_EPOCH); }
         inline bool     is32bitSlow() const { return isGet4V10R32() && (getGet4V10R32MessageType() == MSG_GET4_SLOWCTRL); }
         inline bool     is32bitError() const { return isGet4V10R32() && (getGet4V10R32MessageType() == MSG_GET4_ERROR); }
         inline bool     is32bitHit() const { return isGet4V10R32() && (getGet4V10R32MessageType() == MSG_GET4_HIT); }

         inline uint8_t  getGet4V10R32ChipId()      const { return getSysMesType() & 0xF; }
         inline uint8_t  getGet4V10R32MessageType() const { return getFieldBE(46, 2); }
            // type 0 => Epoch message
         inline uint32_t getGet4V10R32EpochNumber() const { return getFieldBE(17,24); }
         inline uint8_t  getGet4V10R32SyncFlag()    const { return getBitBE(16); }
            // type 1 => Slow control
         inline uint32_t getGet4V10R32SlData()     const { return getFieldBE(16, 24); }
         inline uint8_t  getGet4V10R32SlType()     const { return getFieldBE(40, 2); }
         inline uint8_t  getGet4V10R32SlEdge()     const { return getBitBE(42); }
         inline uint8_t  getGet4V10R32SlChan()     const { return getFieldBE(43, 2); }
            // type 2 => Error message
         inline uint8_t  getGet4V10R32ErrorData()  const { return getFieldBE(16, 7); }
         inline uint8_t  getGet4V10R32ErrorChan()  const { return getFieldBE(42, 2); }
         inline uint8_t  getGet4V10R32ErrorEdge()  const { return getBitBE(44); }
            // type 3 => Hit Data
         inline uint8_t  getGet4V10R32HitTot()     const { return getFieldBE(16, 8); }
         inline uint8_t  getGet4V10R32HitFt()      const { return getFieldBE(24, 7); }
         inline uint32_t getGet4V10R32HitTs()      const { return getFieldBE(31,12); }
         inline uint32_t getGet4V10R32HitTimeBin() const { return getFieldBE(24,19); }
         inline uint8_t  getGet4V10R32HitChan()    const { return getFieldBE(43, 2); }
         inline uint8_t  getGet4V10R32HitDllFlag() const { return getBitBE(45); }


         void printData(unsigned kind = base::msg_print_Prefix | base::msg_print_Data, uint32_t epoch = 0, double localtm = 0) const;

         void printData(std::ostream& os, unsigned kind = base::msg_print_Human,  uint32_t epoch = 0, double localtm = 0) const;

   };


}


#endif
