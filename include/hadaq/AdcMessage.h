#ifndef HADAQ_ADCMESSAGE_H
#define HADAQ_ADCMESSAGE_H

#include <stdint.h>

#include <stdio.h>

namespace hadaq {

   /** AdcMessage is wrapper for data, produced by FPGA-TDC
    * struct is used to avoid any potential overhead */

   struct AdcMessage {
      protected:
         uint32_t   fData;

      public:

         AdcMessage() : fData(0) {}

         AdcMessage(uint32_t d) : fData(d) {}

         void assign(uint32_t d) { fData=d; }

         /** Returns kind of the message */
         uint32_t getKind() const { return fData >> 28; }

         // dummy compare to keep sorting in subevent
         bool operator<(const AdcMessage &rhs) const
            { return (fData < rhs.fData); }

         uint32_t getAdcId() const { return (fData >> 20) & 0xf; }
         uint32_t getAdcCh() const { return (fData >> 16) & 0xf; }


         // and so on
   };

}

#endif
