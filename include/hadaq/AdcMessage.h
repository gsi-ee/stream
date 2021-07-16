#ifndef HADAQ_ADCMESSAGE_H
#define HADAQ_ADCMESSAGE_H

#include <cstdint>
#include <cstdio>
#include <limits>

namespace hadaq {

   /** AdcMessage is wrapper for data, produced by FPGA
    * struct is used to avoid any potential overhead */

   struct AdcMessage {
      protected:
         uint32_t   fData;  ///< data

      public:
         double fFineTiming;   ///< fine timing
         double fIntegral;     ///< integral

         /** constructor */
         AdcMessage() : fData(0),
            fFineTiming(std::numeric_limits<double>::quiet_NaN()),
            fIntegral(std::numeric_limits<double>::quiet_NaN())
         {}

         /** constructor */
         AdcMessage(uint32_t d) : fData(d) {}

         //void assign(uint32_t d) { fData=d; }

         /** Returns kind of the message */
         uint32_t getKind() const { return fData >> 28; }

         /** dummy compare to keep sorting in subevent */
         bool operator<(const AdcMessage &rhs) const
            { return (fData < rhs.fData); }

         /** ADC index, 0..11 */
         uint32_t getAdcId() const { return (fData >> 20) & 0xf; }
         /** ADC channel, 0..3 */
         uint32_t getAdcCh() const { return (fData >> 16) & 0xf; }
         /** logical ADC channel, 0..47 */
         uint32_t getCh() const { return 4*getAdcId()+getAdcCh(); }

         /** the value of the ADC channel
          * handle the sign bit correctly for debug signals from CFD firmware
          * 10bit unsigned words should not be affected by this */
         short int getValue() const { return fData & 0xffff; }
   };

}

#endif
