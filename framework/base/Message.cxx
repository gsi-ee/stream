#include "base/Message.h"

#include <stdio.h>
#include <string.h>

bool base::Message::convertFromOld(void* src)
{
   data = 0;

   uint8_t* arr = (uint8_t*) src;

   setMessageType((arr[0] >> 5) & 0x7);
   setRocNumber((arr[0] >> 2) & 0x7);

   switch (getMessageType()) {
      case MSG_HIT:
         setNxNumber(arr[0] & 0x3);
         setNxLtsMsb((arr[1] >> 5) & 0x7);
         setNxTs(((arr[1] & 0x1f) << 9) | (arr[2] << 1) | (arr[3] >> 7));
         setNxChNum(arr[3] & 0x7f);
         setNxAdcValue(((arr[4] & 0x7f) << 5) | ((arr[5] & 0xf8) >> 3));
         setNxPileup((arr[5] >> 2) & 0x1);
         setNxOverflow((arr[5] >> 1) & 0x1);
         setNxLastEpoch(arr[5] & 0x1);
         break;

      case MSG_EPOCH:
         setEpochNumber((arr[1] << 24) | (arr[2] << 16) | (arr[3] << 8) | arr[4]);
         setEpochMissed(arr[5]);
         break;

      case MSG_SYNC:
         setSyncChNum(arr[0] & 0x3);
         setSyncEpochLSB(arr[1] >> 7);
         setSyncTs(((arr[1] & 0x7f) << 7) | ((arr[2] & 0xfc) >> 1));
         setSyncData(((arr[2] & 0x3) << 22) | (arr[3] << 14) | (arr[4] << 6) | (arr[5] >> 2));
         setSyncStFlag(arr[5] & 0x3);
         break;

      case MSG_AUX:
         setAuxChNum(((arr[0] & 0x3) << 5) | ((arr[1] & 0xf8) >> 3));
         setAuxEpochLSB((arr[1] & 0x4) >> 2);
         setAuxTs(((arr[1] & 0x3) << 12) | (arr[2] << 4) | ((arr[3] & 0xe0) >> 4));
         setAuxFalling((arr[3] >> 4) & 1);
         setAuxOverflow((arr[3] >> 3) & 1);
         break;

      case MSG_SYS:
         setSysMesType(arr[1]);
         setSysMesData((arr[2] << 24) | (arr[3] << 16) | (arr[4] << 8) | arr[5]);
         break;

      default:
         return false;
   }

   return true;
}


bool base::Message::convertToOld(void* tgt)
{
   uint8_t* data = (uint8_t*) tgt;
   for (int n=0;n<6;n++) data[n] = 0;

   data[0] = ((getMessageType() & 0x7) << 5) | ((getRocNumber() & 0x7) << 2);

   switch (getMessageType()) {
      case MSG_HIT:
         data[0] = data[0] | (getNxNumber() & 0x3);
         data[1] = ((getNxTs() >> 9) & 0x1f) | ((getNxLtsMsb() << 5) & 0xe0);
         data[2] = (getNxTs() >> 1);
         data[3] = ((getNxTs() << 7) & 0x80) | (getNxChNum() & 0x7f);
         data[4] = (getNxAdcValue() >> 5) & 0x7f;
         data[5] = ((getNxAdcValue() << 3) & 0xf8) |
                    (getNxLastEpoch() & 0x1) |
                    ((getNxOverflow() & 0x1) << 1) |
                    ((getNxPileup() & 0x1) << 2);
         break;

      case MSG_EPOCH:
         data[1] = (getEpochNumber() >> 24) & 0xff;
         data[2] = (getEpochNumber() >> 16) & 0xff;
         data[3] = (getEpochNumber() >> 8) & 0xff;
         data[4] = getEpochNumber() & 0xff;
         data[5] = getEpochMissed();
         break;

      case MSG_SYNC:
         data[0] = data[0] | (getSyncChNum() & 0x3);
         data[1] = (getSyncEpochLSB() << 7) | ((getSyncTs() >> 7) & 0x7f);
         data[2] = ((getSyncTs() << 1) & 0xfc) | ((getSyncData() >> 22) & 0x3);
         data[3] = (getSyncData() >> 14) & 0xff;
         data[4] = (getSyncData() >> 6) & 0xff;
         data[5] = ((getSyncData() << 2) & 0xfc) | (getSyncStFlag() & 0x3);
         break;

      case MSG_AUX:
         data[0] = data[0] | ((getAuxChNum() >> 5) & 0x3);
         data[1] = ((getAuxChNum() << 3) & 0xf8) |
                    ((getAuxEpochLSB() << 2) & 0x4) |
                    ((getAuxTs() >> 12) & 0x3);
         data[2] = (getAuxTs() >> 4) & 0xff;
         data[3] = ((getAuxTs() << 4) & 0xe0) |
                    (getAuxFalling() << 4) |
                    (getAuxOverflow() << 3);
         break;

      case MSG_SYS:
         data[1] = getSysMesType();
         data[2] = (getSysMesData() >> 24) & 0xff;
         data[3] = (getSysMesData() >> 16) & 0xff;
         data[4] = (getSysMesData() >> 8) & 0xff;
         data[5] = getSysMesData() & 0xff;
         break;

      default:
         return false;
   }

   return true;
}


//----------------------------------------------------------------------------
//! Returns expanded and adjusted time of message (in ns)
//! epoch should correspond to the message type - epoch2 for Get4, epoch for all others
//! When converting from GET4 (156Mhz) to SYNC (250 Mhz) clock, one should take into account
//! that initial value of 250 MHz timestamp counter is 0x80 or 512 ns. Therefore such offset
//! should be applied during conversion

uint64_t base::Message::getMsgFullTime(uint32_t epoch) const
{
   switch (getMessageType()) {
      case MSG_HIT:
         return FullTimeStamp(getNxLastEpoch() ? epoch - 1 : epoch, getNxTs());
      case MSG_EPOCH:
         return FullTimeStamp(getEpochNumber(), 0);
      case MSG_SYNC:
         return FullTimeStamp((getSyncEpochLSB() == (epoch & 0x1)) ? epoch : epoch - 1, getSyncTs());
      case MSG_AUX:
         return FullTimeStamp((getAuxEpochLSB() == (epoch & 0x1)) ? epoch : epoch - 1, getAuxTs());
      case MSG_EPOCH2:
         return FullTimeStamp2(getEpoch2Number(), 0) / 20 + 512;
      case MSG_GET4:
         return FullTimeStamp2(epoch, getGet4Ts()) / 20 + 512;
      case MSG_SYS:
         return FullTimeStamp(epoch, 0);

   }
   return 0;
}

//----------------------------------------------------------------------------
//! Returns expanded and adjusted time of message in double (in ns)
//! epoch should correspond to the message type - epoch2 for Get4, epoch for all others

double base::Message::getMsgFullTimeD(uint32_t epoch) const
{
   switch (getMessageType()) {
      case MSG_EPOCH2:
         return FullTimeStamp2(getEpoch2Number(), 0) / 20. + 512.;
      case MSG_GET4:
         return FullTimeStamp2(epoch, getGet4Ts()) / 20. + 512.;
   }
   return getMsgFullTime(epoch);
}


//----------------------------------------------------------------------------
//! Returns the time difference between two expanded time stamps

uint64_t base::Message::CalcDistance(uint64_t start, uint64_t stop)
{
   if (start>stop) {
      stop += 0x3FFFFFFFFFFFLLU;
      if (start>stop) {
         printf("Epochs overflow error in CalcDistance\n");
         return 0;
      }
   }

   return stop - start;
}


//----------------------------------------------------------------------------
//! Returns the time difference between two expanded time stamps

double base::Message::CalcDistanceD(double start, double stop)
{
   if (start>stop) {
      stop += 0x3FFFFFFFFFFFLLU;
      if (start>stop) {
         printf("Epochs overflow error in CalcDistanceD\n");
         return 0.;
      }
   }

   return stop - start;
}



//----------------------------------------------------------------------------
//! Print message in human readable format to \a cout.
/*!
 * Prints a one line representation of the message in to \a cout.
 * See printData(std::ostream&, unsigned, uint32_t) const for full
 * documentation.
 */

void base::Message::printData(unsigned kind, uint32_t epoch) const
{
  printData(std::cout, kind, epoch);
}

//----------------------------------------------------------------------------
//! Print message in binary or human readable format to a stream.
/*!
 * Prints a one line representation of the message in to stream \a os.
 * The parameter \a kind is mask with 4 bits
 * \li roc::msg_print_Prefix (1) - ROC number and message type
 * \li roc::msg_print_Data   (2) - print all message specific data fields
 * \li roc::msg_print_Hex    (4) - print data as hex dump
 * \li roc::msg_print_Human  (8) - print in human readable format
 *
 * If bit msg_print_Human in \a kind is not set, raw format
 * output is generated. All data fields are shown in hexadecimal.
 * This is the format of choice when chasing hardware problems at the bit level.
 *
 * If bit msg_print_Human is set, a more human readable output is generated.
 * The timestamp is shown as fully extended and adjusted time as
 * returned by the getMsgFullTime(uint32_t) const method.
 * All data fields are represented in decimal.
 *
 * \param os output stream
 * \param kind mask determing output format
 * \param epoch current epoch number (from last epoch message)
 *
 * Typical message output in human format looks like
\verbatim
Msg:7 Roc:1 SysType: 1 Nx:0 Data:        0 : DAQ started
Msg:7 Roc:1 SysType: 6 Nx:0 Data:        0 : FIFO reset
Msg:2 Roc:1 EPO @    0.536870912 Epo:     32768 0x00008000 Miss:   0
Msg:0 Roc:0 NOP (raw 80:40:82:0F:00:00)
Msg:2 Roc:1 EPO @    0.646627328 Epo:     39467 0x00009a2b Miss:   0
Msg:1 Roc:1 HIT @    0.646614333 Nx:2 Chn: 12 Ts: 3389-e( 8) Adc:2726 Pu:0 Of:0
Msg:1 Roc:1 HIT @    0.646630717 Nx:2 Chn: 13 Ts: 3389  ( 0) Adc:2745 Pu:0 Of:0
Msg:2 Roc:1 EPO @    0.805306368 Epo:     49152 0x0000c000 Miss:   0
Msg:3 Roc:1 SYN @    0.805306408 Chn:2 Ts:   40   Data:   49152 0x00c000 Flag:0
Msg:7 Roc:1 SysType: 2 Nx:0 Data:        0 : DAQ finished
\endverbatim
 *
 * Typical message output in binary format looks like
\verbatim
Msg:7 Roc:1 SysType: 1 Nx:0 Data:        0 : DAQ started
Msg:7 Roc:1 SysType: 6 Nx:0 Data:        0 : FIFO reset
Msg:2 Roc:1 Epoch:00008000 Missed:00
Msg:1 Roc:1 Nx:2 Chn:0d Ts:3ec9 Last:1 Msb:7 Adc:a22 Pup:0 Oflw:0
Msg:1 Roc:1 Nx:2 Chn:0e Ts:3ec9 Last:0 Msb:7 Adc:a18 Pup:0 Oflw:0
Msg:0 Roc:0 NOP (raw 80:40:82:0F:00:00)
Msg:2 Roc:1 Epoch:00010000 Missed:00
Msg:3 Roc:1 SyncChn:2 EpochLSB:0 Ts:0028 Data:010000 Flag:0
Msg:7 Roc:1 SysType: 2 Nx:0 Data:        0 : DAQ finished
\endverbatim
 */

void base::Message::printData(std::ostream& os, unsigned kind, uint32_t epoch) const
{
   char buf[256];

   if (kind & msg_print_Hex) {
      uint8_t* arr = (uint8_t*) &data;
      snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X ",
               arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
      os << buf;
   }

   if (kind & msg_print_Human) {
      double timeInSec = getMsgFullTimeD(epoch)/1.e9;
      int fifoFill = 0;

      switch (getMessageType()) {
         case MSG_HIT:
            fifoFill = getNxLtsMsb() - ((getNxTs()>>11)&0x7);
            if (getNxLastEpoch()) fifoFill += 8;
            snprintf(buf, sizeof(buf),
                  "Msg:%u Roc:%u ", getMessageType(), getRocNumber());
            os << buf;
            snprintf(buf, sizeof(buf),
                  "HIT  @%15.9f Nx:%d Chn:%3d Ts:%5d%s(%2d) Adc:%4d Pu:%d Of:%d",
                  timeInSec, getNxNumber(), getNxChNum(), getNxTs(),
                  (getNxLastEpoch() ? "-e" : "  "),
                  fifoFill, getNxAdcValue(), getNxPileup(), getNxOverflow());
            os << buf << std::endl;
            break;
         case MSG_EPOCH:
            snprintf(buf, sizeof(buf),
                  "Msg:%u Roc:%u ", getMessageType(), getRocNumber());
            os << buf;
            snprintf(buf, sizeof(buf),
                  "EPO  @%15.9f Epo:%10u 0x%08x Miss: %3d%c",
                  timeInSec, getEpochNumber(), getEpochNumber(),
                  getEpochMissed(), (getEpochMissed()==0xff) ? '+' : ' ');
            os << buf << std::endl;
            break;
         case MSG_SYNC:
            snprintf(buf, sizeof(buf),
                  "Msg:%u Roc:%u ", getMessageType(), getRocNumber());
            os << buf;
            snprintf(buf, sizeof(buf),
                  "SYN  @%15.9f Chn:%d Ts:%5d%s Data:%8d 0x%06x Flag:%d",
                  timeInSec, getSyncChNum(), getSyncTs(),
                  ((getSyncEpochLSB() != (epoch&0x1)) ? "-e" : "  "),
                  getSyncData(), getSyncData(), getSyncStFlag());
            os << buf << std::endl;
            break;
         case MSG_AUX:
            snprintf(buf, sizeof(buf),
                  "Msg:%u Roc:%u ", getMessageType(), getRocNumber());
            os << buf;
            snprintf(buf, sizeof(buf),
                  "AUX  @%15.9f Chn:%d Ts:%5d%s Falling:%d Overflow:%d",
                  timeInSec, getAuxChNum(), getAuxTs(),
                  ((getAuxEpochLSB() != (epoch&0x1)) ? "-e" : "  "),
                  getAuxFalling(), getAuxOverflow());
            os << buf << std::endl;
            break;
         case MSG_EPOCH2:
            snprintf(buf, sizeof(buf),
                  "Msg:%u Roc:%u ", getMessageType(), getRocNumber());
            os << buf;
            snprintf(buf, sizeof(buf),
                  "EPO2 @%17.11f Get4:%2d Epoche2:%10u 0x%08x StampTime:%2d Sync:%x Dataloss:%x Epochloss:%x Epochmissmatch:%x", 
                        timeInSec, getEpoch2ChipNumber(), getEpoch2Number(), getEpoch2Number(), 
                        getEpoch2StampTime(), getEpoch2Sync(), getEpoch2DataLost(), getEpoch2EpochLost(), getEpoch2EpochMissmatch());
            os << buf << std::endl;
            break;
         case MSG_GET4:
            snprintf(buf, sizeof(buf),
                  "Msg:%u Roc:%u ", getMessageType(), getRocNumber());
            os << buf;
            snprintf(buf, sizeof(buf),
                  "Get4 @%17.11f Get4:%2d Chn:%3d Edge:%1d Ts:%7d CRC8:%3d",
                  timeInSec, getGet4Number(), getGet4ChNum(), getGet4Edge(), getGet4Ts(), getGet4CRC() );
            os << buf << std::endl;
            break;
         default:
            kind = kind & ~msg_print_Human;
            if (kind==0) kind = msg_print_Prefix | msg_print_Data;
      }

      // return, if message was correctly printed in human-readable form
      if (kind & msg_print_Human) return;
   }

   if (kind & msg_print_Prefix) {
      snprintf(buf, sizeof(buf), "Msg:%u Roc:%u ", getMessageType(), getRocNumber());
      os << buf;
   }

   if (kind & msg_print_Data) {
      uint8_t* arr = (uint8_t*) &data;
      switch (getMessageType()) {
        case MSG_NOP:
           snprintf(buf, sizeof(buf), "NOP (raw %02X:%02X:%02X:%02X:%02X:%02X)",
                    arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
            break;
         case MSG_HIT:
            snprintf(buf, sizeof(buf), "Nx:%1x Chn:%02x Ts:%04x Last:%1x Msb:%1x Adc:%03x Pup:%1x Oflw:%1x",
                    getNxNumber(), getNxChNum(), getNxTs(), getNxLastEpoch(),
                    getNxLtsMsb(), getNxAdcValue(), getNxPileup(),
                    getNxOverflow());
            break;
         case MSG_EPOCH:
            snprintf(buf, sizeof(buf), "Epoch:%08x Missed:%02x",
                    getEpochNumber(), getEpochMissed());
            break;
         case MSG_SYNC:
            snprintf(buf, sizeof(buf), "SyncChn:%1x EpochLSB:%1x Ts:%04x Data:%06x Flag:%1x",
                    getSyncChNum(), getSyncEpochLSB(), getSyncTs(),
                    getSyncData(), getSyncStFlag());
            break;
         case MSG_AUX:
            snprintf(buf, sizeof(buf), "AuxChn:%02x EpochLSB:%1x Ts:%04x Falling:%1x Overflow:%1x",
                    getAuxChNum(), getAuxEpochLSB(), getAuxTs(),
                    getAuxFalling(), getAuxOverflow());
            break;
         case MSG_EPOCH2:
            snprintf(buf, sizeof(buf), "Get4:0x%02x Epoche2:0x%08x StampTime:0x%x Sync:%x Dataloss:%x Epochloss:%x Epochmissmatch:%x", 
                     getEpoch2ChipNumber(), getEpoch2Number(), getEpoch2StampTime(), getEpoch2Sync(), getEpoch2DataLost(), getEpoch2EpochLost(), getEpoch2EpochMissmatch());
            break;
         case MSG_GET4:
            snprintf(buf, sizeof(buf), "Get4:0x%02x Chn:%1x Edge:%1x Ts:0x%05x CRC8:0x%02x",
                         getGet4Number(), getGet4ChNum(), getGet4Edge(), getGet4Ts(), getGet4CRC() );
            break;
         case MSG_SYS: {
            char sysbuf[256];

            switch (getSysMesType()) {
              case SYSMSG_DAQ_START:
                 snprintf(sysbuf, sizeof(sysbuf), "DAQ started");
                 break;
              case SYSMSG_DAQ_FINISH:
                 snprintf(sysbuf, sizeof(sysbuf), "DAQ finished");
                 break;
              case SYSMSG_NX_PARITY: {
                   uint32_t data = getSysMesData();
                   uint32_t nxb3_flg = (data>>31) & 0x01;
                   uint32_t nxb3_val = (data>>24) & 0x7f;
                   uint32_t nxb2_flg = (data>>23) & 0x01;
                   uint32_t nxb2_val = (data>>16) & 0x7f;
                   uint32_t nxb1_flg = (data>>15) & 0x01;
                   uint32_t nxb1_val = (data>>8 ) & 0x7f;
                   uint32_t nxb0_flg = (data>>7 ) & 0x01;
                   uint32_t nxb0_val = (data    ) & 0x7f;
                   snprintf(sysbuf, sizeof(sysbuf),"Nx:%1x %1d%1d%1d%1d:%02x:%02x:%02x:%02x nX parity error", getNxNumber(), 
                            nxb3_flg, nxb2_flg, nxb1_flg, nxb0_flg,
                            nxb3_val, nxb2_val, nxb1_val, nxb0_val);
                 }
                 break;
              case SYSMSG_SYNC_PARITY:
                 snprintf(sysbuf, sizeof(sysbuf), "SYNC parity error ");
                 break;
              case SYSMSG_DAQ_RESUME:
                 snprintf(sysbuf, sizeof(sysbuf), "DAQ resume after high/low water");
                 break;
              case SYSMSG_FIFO_RESET:
                 snprintf(sysbuf, sizeof(sysbuf), "FIFO reset");
                 break;
              case SYSMSG_USER: {
                 const char* subtyp = "";
                 if (getSysMesData()==SYSMSG_USER_CALIBR_ON) subtyp = "Calibration ON"; else
                 if (getSysMesData()==SYSMSG_USER_CALIBR_OFF) subtyp = "Calibration OFF"; else
                 if (getSysMesData()==SYSMSG_USER_RECONFIGURE) subtyp = "Reconfigure";
                 snprintf(sysbuf, sizeof(sysbuf), "User message 0x%08x %s", getSysMesData(), subtyp);
                 break;
              }
              case SYSMSG_PACKETLOST:
                 snprintf(sysbuf, sizeof(sysbuf), "Packet lost");
                 break;
              case SYSMSG_GET4_EVENT: {
                 uint32_t data           = getSysMesData();
                 uint32_t get4_pattern   = getField(16, 6); //(data>>16) & 0x3f;
                 uint32_t get4_eventType = getBit(22);      //(data>>7)  & 0xfff;
                 uint32_t get4_TS        = getField(23,12); //(data>>6)  & 0x1;
                 uint32_t get4_chip      = getField(40, 8); // data      & 0xff;
                 if(get4_eventType==1)
                    snprintf(sysbuf, sizeof(sysbuf),
                          "Get4:0x%02x TS:0x%03x Pattern:0x%02x - GET4 External Sync Event", get4_chip, get4_TS, get4_pattern);
                 else
                    snprintf(sysbuf, sizeof(sysbuf),
                          "Get4:0x%02x TS:0x%03x ErrCode:0x%02x - GET4 Error Event", get4_chip, get4_TS, get4_pattern);
                 break;
              }
              case SYSMSG_CLOSYSYNC_ERROR:
                 snprintf(sysbuf, sizeof(sysbuf), "Closy synchronization error");
                 break;
              case SYSMSG_TS156_SYNC:
                 snprintf(sysbuf, sizeof(sysbuf), "156.25MHz timestamp reset");
                 break;
              default:
                 snprintf(sysbuf, sizeof(sysbuf), "unknown system message type ");
            }

            snprintf(buf, sizeof(buf), "SysType:%2x Data:%8x : %s", getSysMesType(), getSysMesData(), sysbuf);

            break;
         }
         default:
           snprintf(buf, sizeof(buf), "Error - unexpected MessageType: %1x", getMessageType());
      }
   }

   os << buf << std::endl;
}

uint32_t base::Message::RawSize(int fmt)
{
   switch (fmt) {
      case formatEth1:   return 6;
      case formatOptic1: return 8;
      case formatEth2:   return 6;
      case formatOptic2: return 8;
      case formatNormal: return 8;
   }
   return 8;
}

bool base::Message::assign(void* src, int fmt)
{
   switch (fmt) {
      case formatEth1:
         convertFromOld(src);
         return true;
      case formatOptic1: 
         convertFromOld((uint8_t*) src + 2);
         setRocNumber((*((uint8_t*) src) << 8) | *((uint8_t*) src + 1) );
         return true;
      case formatEth2:
         memcpy(&data, src, 6);
         return true;
      case formatOptic2:
         memcpy(&data, (uint8_t*) src + 2, 6);
         setRocNumber(*((uint16_t*) src));
         return true;
      case formatNormal:
         memcpy(&data, src, 8);
         return true;
   }

   return false;
}

bool base::Message::copyto(void* tgt, int fmt)
{
   switch (fmt) {
      case formatEth1:
         convertToOld(tgt);
         return true;
      case formatOptic1:
         convertToOld((uint8_t*) tgt + 2);
         *((uint16_t*) tgt) = getRocNumber();
         return true;
      case formatEth2:
         memcpy(tgt, &data, 6);
         return true;
      case formatOptic2:
         memcpy((uint8_t*) tgt + 2, &data, 6);
         *((uint16_t*) tgt) = getRocNumber();
         return true;
      case formatNormal:
         memcpy(tgt, &data, 8);
         return true;
   }

   return false;

}
