#ifndef HADAQ_TDCMESSAGE_H
#define HADAQ_TDCMESSAGE_H

#include <stdint.h>

#include <stdio.h>

namespace hadaq {

   enum TdcMessageKind {
      tdckind_Reserved = 0x00000000,
      tdckind_Header   = 0x20000000,
      tdckind_Debug    = 0x40000000,
      tdckind_Epoch    = 0x60000000,
      tdckind_Mask     = 0xe0000000,
      tdckind_Hit      = 0x80000000,
      tdckind_Hit1     = 0xa0000000,
      tdckind_Hit2     = 0xc0000000,
      tdckind_Hit3     = 0xe0000000
   };

   enum TdcConstants {
      NumTdcChannels = 65
   };

   /** TdcMessage is wrapper for data, produced by FPGA-TDC
    * struct is used to avoid any potential overhead */

   struct TdcMessage {
      protected:
         uint32_t   fData;

      public:

         TdcMessage() : fData(0) {}

         TdcMessage(uint32_t d) : fData(d) {}

         void assign(uint32_t d) { fData=d; }

         /** Returns kind of the message
          * If used for the hit message, four different values can be returned */
         uint32_t getKind() const { return fData & tdckind_Mask; }

         bool isHitMsg() const { return fData & tdckind_Hit; }

         bool isEpochMsg() const { return getKind() == tdckind_Epoch; }
         bool isDebugMsg() const { return getKind() == tdckind_Debug; }
         bool isHeaderMsg() const { return getKind() == tdckind_Header; }
         bool isReservedMsg() const { return getKind() == tdckind_Reserved; }

         // methods for epoch

         /** Return Epoch for epoch marker, 28 bit */
         uint32_t getEpochValue() const { return fData & 0xFFFFFFF; }
         /** Get reserved bit for epoch, 1 bit */
         uint32_t getEpochRes() const { return (fData >> 28) & 0x1; }

         // methods for hit

         /** Returns hit channel ID */
         uint32_t getHitChannel() const { return (fData >> 22) & 0x7F; }

         /** Returns hit coarse time counter, 11 bit */
         uint32_t getHitTmCoarse() const { return fData & 0x7FF; }

         /** Returns hit fine time counter, 10 bit */
         uint32_t getHitTmFine() const { return (fData >> 12) & 0x3FF; }

         /** Returns time stamp, which is simple combination coarse and fine counter */
         uint32_t getHitTmStamp() const { return (getHitTmCoarse() << 10) | getHitTmFine(); }

         /** Returns hit edge 1 - rising, 0 - falling */
         uint32_t getHitEdge() const {  return (fData >> 11) & 0x1; }

         bool isHitRisingEdge() const { return getHitEdge() == 0x1; }
         bool isHitFallingEdge() const { return getHitEdge() == 0x0; }

         /** Returns hit reserved value, 2 bits */
         uint32_t getHitReserved() const { return (fData >> 29) & 0x3; }

         // methods for header

         /** Return error bits of header message */
         uint32_t getHeaderErr() const { return fData & 0xFFFF; }

         /** Return reserved bits of header message */
         uint32_t getHeaderRes() const { return (fData >> 16) & 0x1FFF; }

         void print()
         {
            printf("   0x%08x ", (unsigned) fData);

            switch (getKind()) {
               case tdckind_Reserved: printf ("reserved\n"); break;

               case tdckind_Header:   printf ("header\n"); break;
               case tdckind_Debug:    printf ("debug\n"); break;
               case tdckind_Epoch:    printf ("epoch %u\n", (unsigned) getEpochValue()); break;
               case tdckind_Hit:
               case tdckind_Hit1:
               case tdckind_Hit2:
               case tdckind_Hit3:     printf("hit ch:%4u edge:%u coarse:%4u fine:%4u\n",
                                            (unsigned) getHitChannel(), (unsigned)getHitEdge(), (unsigned)getHitTmCoarse(), (unsigned)getHitTmFine());
                                      break;
               default:
                  printf("unknown kind %u\n", (unsigned) getKind());
                  break;
            }
         }

   };

}

#endif
