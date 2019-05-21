#include "get4/Message.h"

#include <stdio.h>

//----------------------------------------------------------------------------
//! Print message in human readable format to \a cout.
/*!
 * Prints a one line representation of the message in to \a cout.
 * See printData(std::ostream&, unsigned, uint32_t) const for full
 * documentation.
 */

void get4::Message::printData(unsigned kind, uint32_t epoch, double localtm) const
{
  printData(std::cout, kind, epoch, localtm);
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
 * returned by the get4::Iterator::getMsgTime() const method.
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

void get4::Message::printData(std::ostream& os, unsigned kind, uint32_t epoch, double timeInSec) const
{
   char buf[512];

   if (kind & base::msg_print_Hex) {
      uint8_t* arr = (uint8_t*) &data;
      snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X ",
               arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
      os << buf;
   }

   if (kind & base::msg_print_Human) {
      switch (getMessageType()) {
         case base::MSG_EPOCH:
            snprintf(buf, sizeof(buf),
                  "Msg:%u Roc:%u ", getMessageType(), getRocNumber());
            os << buf;
            snprintf(buf, sizeof(buf),
                  "EPO  @%15.9f Epo:%10u 0x%08x Miss: %3d%c",
                  timeInSec, getEpochNumber(), getEpochNumber(),
                  getEpochMissed(), (getEpochMissed()==0xff) ? '+' : ' ');
            os << buf << std::endl;
            break;
         case base::MSG_SYNC:
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
         case base::MSG_AUX:
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
         case base::MSG_EPOCH2:
            snprintf(buf, sizeof(buf),
                  "Msg:%u Roc:%u ", getMessageType(), getRocNumber());
            os << buf;
            snprintf(buf, sizeof(buf),
                  "EPO2 @%17.11f Get4:%2d Epoche2:%10u 0x%08x StampTime:%2d Sync:%x Dataloss:%x Epochloss:%x Epochmissmatch:%x",
                        timeInSec, getEpoch2ChipNumber(), getEpoch2Number(), getEpoch2Number(),
                        getEpoch2StampTime(), getEpoch2Sync(), getEpoch2DataLost(), getEpoch2EpochLost(), getEpoch2EpochMissmatch());
            os << buf << std::endl;
            break;
         case base::MSG_GET4:
            snprintf(buf, sizeof(buf),
                  "Msg:%u Roc:%u ", getMessageType(), getRocNumber());
            os << buf;
            snprintf(buf, sizeof(buf),
                  "Get4 @%17.11f Get4:%2d Chn:%3d Edge:%1d Ts:%7d CRC8:%3d",
                  timeInSec, getGet4Number(), getGet4ChNum(), getGet4Edge(), getGet4Ts(), getGet4CRC() );
            os << buf << std::endl;
            break;
         case base::MSG_SYS:
            if (isGet4V10R32()) {
               snprintf(buf, sizeof(buf), "Msg:%X Roc:%u ", 8+getGet4V10R32MessageType(), getRocNumber());

               os << buf;

               switch (getGet4V10R32MessageType()) {
                  case MSG_GET4_EPOCH:
                     snprintf(buf, sizeof(buf), "EP32 @%17.11f 32bit:%10u 0x%08x get4:%u sync:%u",
                           timeInSec,
                           (unsigned) getGet4V10R32EpochNumber(),
                           (unsigned) getGet4V10R32EpochNumber(),
                           (unsigned) getGet4V10R32ChipId(),
                           (unsigned) getGet4V10R32SyncFlag());
                     break;
                  case MSG_GET4_SLOWCTRL:
                     snprintf(buf, sizeof(buf), "SC32 get4:%u chn:0x%x edge:0x%x typ:0x%x data:0x%04x",
                           (unsigned) getGet4V10R32ChipId(),
                           (unsigned) getGet4V10R32SlChan(), (unsigned) getGet4V10R32SlEdge(),
                           (unsigned) getGet4V10R32SlType(), (unsigned) getGet4V10R32SlData());
                     break;
                  case MSG_GET4_ERROR:
                     snprintf(buf, sizeof(buf), "ER32 get4:%u chn:0x%x edge:0x%x data:0x%04x",
                           (unsigned) getGet4V10R32ChipId(),
                           (unsigned) getGet4V10R32ErrorChan(),
                           (unsigned) getGet4V10R32ErrorEdge(), (unsigned) getGet4V10R32ErrorData());
                     break;
                  case MSG_GET4_HIT:
                     snprintf(buf, sizeof(buf), "HT32 @%17.11f get4:%u chn:0x%x ts:0x%04x fine:0x%03x tot:0x%3x dll:%x",
                           timeInSec,
                           (unsigned) getGet4V10R32ChipId(),
                           (unsigned) getGet4V10R32HitChan(),
                           (unsigned) getGet4V10R32HitTs(),
                           (unsigned) getGet4V10R32HitFt(),
                           (unsigned) getGet4V10R32HitTot(),
                           (unsigned) getGet4V10R32HitDllFlag());
                     break;
                }
              os << buf << std::endl;
            } else {
               kind = kind & ~base::msg_print_Human;
               if (kind==0) kind = base::msg_print_Prefix | base::msg_print_Data;
            }
            break;
         default:

            kind = kind & ~base::msg_print_Human;
            if (kind==0) kind = base::msg_print_Prefix | base::msg_print_Data;
      }

      // return, if message was correctly printed in human-readable form
      if (kind & base::msg_print_Human) return;
   }

   if (kind & base::msg_print_Prefix) {
      snprintf(buf, sizeof(buf), "Msg:%u Roc:%u ", getMessageType(), getRocNumber());
      os << buf;
   }

   if (kind & base::msg_print_Data) {
      uint8_t* arr = (uint8_t*) &data;
      switch (getMessageType()) {
        case base::MSG_NOP:
           snprintf(buf, sizeof(buf), "NOP (raw %02X:%02X:%02X:%02X:%02X:%02X)",
                    arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
            break;
         case base::MSG_EPOCH:
            snprintf(buf, sizeof(buf), "Epoch:%08x Missed:%02x",
                    getEpochNumber(), getEpochMissed());
            break;
         case base::MSG_SYNC:
            snprintf(buf, sizeof(buf), "SyncChn:%1x EpochLSB:%1x Ts:%04x Data:%06x Flag:%1x",
                    getSyncChNum(), getSyncEpochLSB(), getSyncTs(),
                    getSyncData(), getSyncStFlag());
            break;
         case base::MSG_AUX:
            snprintf(buf, sizeof(buf), "AuxChn:%02x EpochLSB:%1x Ts:%04x Falling:%1x Overflow:%1x",
                    getAuxChNum(), getAuxEpochLSB(), getAuxTs(),
                    getAuxFalling(), getAuxOverflow());
            break;
         case base::MSG_EPOCH2:
            snprintf(buf, sizeof(buf), "Get4:0x%02x Epoche2:0x%08x StampTime:0x%x Sync:%x Dataloss:%x Epochloss:%x Epochmissmatch:%x",
                     getEpoch2ChipNumber(), getEpoch2Number(), getEpoch2StampTime(), getEpoch2Sync(), getEpoch2DataLost(), getEpoch2EpochLost(), getEpoch2EpochMissmatch());
            break;
         case base::MSG_GET4:
            snprintf(buf, sizeof(buf), "Get4:0x%02x Chn:%1x Edge:%1x Ts:0x%05x CRC8:0x%02x",
                         getGet4Number(), getGet4ChNum(), getGet4Edge(), getGet4Ts(), getGet4CRC() );
            break;
         case base::MSG_SYS: {
            char sysbuf[256];

            if (isGet4V10R32()) {
               switch (getGet4V10R32MessageType()) {
                  case MSG_GET4_EPOCH:
                     snprintf(sysbuf, sizeof(sysbuf), "Epoch2:0x%08x sync:%u", (unsigned)  getGet4V10R32EpochNumber(), (unsigned) getGet4V10R32SyncFlag());
                     break;
                  case MSG_GET4_SLOWCTRL:
                     snprintf(sysbuf, sizeof(sysbuf), "Slow control chn:0x%x edge:0x%x typ:0x%x data:0x%04x",
                           (unsigned) getGet4V10R32SlChan(), (unsigned) getGet4V10R32SlEdge(), (unsigned) getGet4V10R32SlType(), (unsigned) getGet4V10R32SlData());
                     break;
                  case MSG_GET4_ERROR:
                     snprintf(sysbuf, sizeof(sysbuf), "Error chn:0x%x edge:0x%x data:0x%04x",
                           (unsigned) getGet4V10R32ErrorChan(), (unsigned) getGet4V10R32ErrorEdge(), (unsigned) getGet4V10R32ErrorData());
                     break;
                  case MSG_GET4_HIT:
                     snprintf(sysbuf, sizeof(sysbuf), "HIT chn:0x%x ts:0x%04x fine:0x%03x tot:0x%3x dll:%x",
                           (unsigned) getGet4V10R32HitChan(),
                           (unsigned) getGet4V10R32HitTs(),
                           (unsigned) getGet4V10R32HitFt(),
                           (unsigned) getGet4V10R32HitTot(),
                           (unsigned) getGet4V10R32HitDllFlag());
                     break;
               }

               snprintf(buf, sizeof(buf), "Get4:0x%02x 32-bit %s", (unsigned) getGet4V10R32ChipId(), sysbuf);

            } else {

               switch (getSysMesType()) {
                  case base::SYSMSG_DAQ_START:
                     snprintf(sysbuf, sizeof(sysbuf), "DAQ started");
                     break;
                  case base::SYSMSG_DAQ_FINISH:
                     snprintf(sysbuf, sizeof(sysbuf), "DAQ finished");
                     break;
                  case base::SYSMSG_SYNC_PARITY:
                     snprintf(sysbuf, sizeof(sysbuf), "SYNC parity error ");
                     break;
                  case base::SYSMSG_DAQ_RESUME:
                     snprintf(sysbuf, sizeof(sysbuf), "DAQ resume after high/low water");
                     break;
                  case base::SYSMSG_FIFO_RESET:
                     snprintf(sysbuf, sizeof(sysbuf), "FIFO reset");
                     break;
                  case base::SYSMSG_USER: {
                     const char* subtyp = "";
                     if (getSysMesData()==base::SYSMSG_USER_CALIBR_ON) subtyp = "Calibration ON"; else
                        if (getSysMesData()==base::SYSMSG_USER_CALIBR_OFF) subtyp = "Calibration OFF"; else
                           if (getSysMesData()==base::SYSMSG_USER_RECONFIGURE) subtyp = "Reconfigure";
                     snprintf(sysbuf, sizeof(sysbuf), "User message 0x%08x %s", getSysMesData(), subtyp);
                     break;
                  }
                  case base::SYSMSG_PACKETLOST:
                     snprintf(sysbuf, sizeof(sysbuf), "Packet lost");
                     break;
                  case base::SYSMSG_GET4_EVENT: {
                     // uint32_t data           = getSysMesData();
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
                  case base::SYSMSG_CLOSYSYNC_ERROR:
                     snprintf(sysbuf, sizeof(sysbuf), "Closy synchronization error");
                     break;
                  case base::SYSMSG_TS156_SYNC:
                     snprintf(sysbuf, sizeof(sysbuf), "156.25MHz timestamp reset");
                     break;
                  default:
                     snprintf(sysbuf, sizeof(sysbuf), "unknown system message type ");
               }

               snprintf(buf, sizeof(buf), "SysType:%2x Data:%8x : %s", getSysMesType(), getSysMesData(), sysbuf);
            }

            break;
         }
         default:
           snprintf(buf, sizeof(buf), "Error - unexpected MessageType: %1x", getMessageType());
      }
   }

   os << buf << std::endl;
}

