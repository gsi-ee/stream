#ifndef NX_MESSAGE_H
#define NX_MESSAGE_H

#include "base/Message.h"

#include <cstring>

namespace nx {

   class Message : public base::Message {
      protected:

         /** Keep for historical reasons - no any use for current software */
         bool convertFromOld(void* src);
         bool convertToOld(void* tgt);

      public:
         Message() : base::Message() {}

         inline bool assign(void* src, int fmt = base::formatNormal)
         {
            switch (fmt) {
               case base::formatEth1:
                  convertFromOld(src);
                  return true;
               case base::formatOptic1:
                  convertFromOld((uint8_t*) src + 2);
                  setRocNumber((*((uint8_t*) src) << 8) | *((uint8_t*) src + 1) );
                  return true;
               case base::formatEth2:
                  memcpy(&data, src, 6);
                  return true;
               case base::formatOptic2:
                  data = (*((uint64_t*) src) >> 16) | (*((uint64_t*) src) << 48);
                  //memcpy(&data, (uint8_t*) src + 2, 6);
                  //setRocNumber(*((uint16_t*) src));
                  return true;
               case base::formatNormal:
                  memcpy(&data, src, 8);
                  return true;
            }

            return false;
         }


         // ---------- nXYTER Hit data access methods ----------------

         //! For Hit data: Returns nXYTER number (2 bit field)
         /*!
          * A ROC can support up to 2 FEBs with a total of 4 nXYTER chips.
          * This field identifies FEB as well as chip. Each supported
          * configuration (either 2 x FEB1nx/FEB2nx or a single FEB4nx)
          * as a unique nXYTER chip numbering.
          */
         inline uint8_t getNxNumber() const { return getField(6, 2); }

         //! For Hit data: Returns 3 most significant bits of ROC LTS (3 bit field)
         /*!
          * The 3 MSBs of the ROC local time stamp counter at the time of data
          * capture are stored in this field. A comparison of this number,
          * which reflects the time of readout, and the 3 MSBs of the timestamp
          * (returned by getNxLtsMsb() const), which reflects the time of the
          * hit, allows to estimate the time the hit stayed in the nXYTER internal
          * FIFOs. This in turn gives an indication of the data load in the
          * nXYTER.
          */
         inline uint8_t getNxLtsMsb() const { return getField(8, 3); }

         //! For Hit data: Returns nXYTER time stamp (14 bit field)
         /*!
          * Raw timestamp as delivered by the nXYTER. The last-epoch flag
          * (returned by getNxLastEpoch() const) determines whether the
          * hit belongs to the current or the last epoch. Use the
          * getMsgStamp(uint32_t) const method to determine the expanded timestamp.
          */
         inline uint16_t getNxTs() const { return getField(11, 14); }

         //! For Hit data: Returns nXYTER channel number (7 bit field)
         inline uint8_t getNxChNum() const { return getField(25, 7); }

         // 1 bit unused

         //! For Hit data: Returns ADC value (12 bit field)
         inline uint16_t getNxAdcValue() const { return getField(33, 12); }

         //! For Hit data: Returns nXYTER pileup flag (1 bit field)
         /*!
          * This flag is set when the nXYTER trigger circuit detected two hits
          * during the peak sense time interval of the slow amplitude channel.
          * This flag indicates that separation between two hits was smaller than
          * what can be handled by the amplitude channel. \sa getNxOverflow() const
          */
         inline uint8_t getNxPileup() const { return getBit(45); }

         //! For Hit data: Returns nXYTER overflow flag (1 bit field)
         /*!
          * This flag is set when a nXYTER channels detects a hit but this hit
          * can't be stored because the 4 stage channel FIFO is full. This flag
          * indicates that the hit data rate was higher than the chip can handle
          * over a longer time. \sa getNxPileup() const
          */
         inline uint8_t getNxOverflow() const { return getBit(46); }

         //! For Hit data: Returns nXYTER last-epoch flag (1 bit field)
         /*!
          * This flag is set when the hit actually belong to the previous epoch.
          * Use the getMsgStamp(uint32_t) const method to determine the
          * expanded timestamp.
          */
         inline uint8_t getNxLastEpoch() const { return getBit(47); }


         //! For Hit data: Sets nXYTER number (2 bit field)
         inline void setNxNumber(uint8_t v) { setField(6, 2, v); }

         //! For Hit data: Sets 3 most significant bits of ROC LTS (3 bit field)
         inline void setNxLtsMsb(uint8_t v) { setField(8, 3, v); }

         //! For Hit data: Sets nXYTER time stamp (14 bit field)
         inline void setNxTs(uint16_t v) { setField(11, 14, v); }

         //! For Hit data: Sets nXYTER channel number (7 bit field)
         inline void setNxChNum(uint8_t v) { setField(25, 7, v); }

         // 1 bit unused

         //! For Hit data: Sets ADC value (12 bit field)
         inline void setNxAdcValue(uint16_t v) { setField(33, 12, v); }

         //! For Hit data: Sets nXYTER pileup flag (1 bit field)
         inline void setNxPileup(uint8_t v) { setBit(45, v); }

         //! For Hit data: Sets nXYTER overflow flag (1 bit field)
         inline void setNxOverflow(uint8_t v) { setBit(46,v ); }

         //! For Hit data: Sets nXYTER last-epoch flag (1 bit field)
         inline void setNxLastEpoch(uint8_t v) { setBit(47, v); }

         //! Returns \a true is message type is #MSG_HIT (nXYTER hit data)
         inline bool isHitMsg() const { return getMessageType() == base::MSG_HIT; }

         void printData(unsigned kind = base::msg_print_Prefix | base::msg_print_Data, uint32_t epoch = 0, double localtm = 0) const;

         void printData(std::ostream& os, unsigned kind = base::msg_print_Human, uint32_t epoch = 0, double localtm = 0) const;
   };

}

#endif
