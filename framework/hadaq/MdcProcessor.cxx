#include "hadaq/MdcProcessor.h"


hadaq::MdcProcessor::MdcProcessor(TrbProcessor* trb, unsigned subid) :
   hadaq::SubProcessor(trb, "MDC_%04x", subid)
{
   HitsPerBinRising = MakeH2("HitsPerBinRising", "", 33, -1, 32, 8, 0, 8);
   HitsPerBinFalling = MakeH2("HitsPerBinFalling", "", 33, -1, 32, 8, 0, 8);
   ToT = MakeH2("ToT", "", 32, 0, 32, 1500, 0.2, 600.2);
   deltaT = MakeH2("deltaT", "", 32, 0, 32, 6000, -1199.8, 1200.2);
   alldeltaT = MakeH1("alldeltaT", "", 6000, -1199.8, 1200.2);
   allToT = MakeH1("allToT", "", 1500, 0.2, 600.2);
   deltaT_ToT = MakeH2("deltaT_ToT", "", 6000, -1199.8, 1200.2, 1500, 0.2, 600.2);
   Errors = MakeH1("Errors", "", 33, -1, 32);
   for (unsigned i = 0; i < TDCCHANNELS + 1; i++) {

      char chno[16];
      sprintf(chno, "Ch%02d_t1", i);
      t1_h[i] = MakeH1(chno, chno, 6000, -1199.8, 1200.2, "ns");
      sprintf(chno, "Ch%02d_tot", i);
      tot_h[i] = MakeH1(chno, chno, 1500, 0.2, 600.2, "ns");
      sprintf(chno, "Ch%02d_potato", i);
      potato_h[i] = MakeH2(chno, chno, 600, -1199.8, 1200.2, 500, 0.2, 600.2, "t1 (ns);tot (ns)");
   }
}

bool hadaq::MdcProcessor::FirstBufferScan(const base::Buffer &buf)
{
   uint32_t *arr = (uint32_t *)buf.ptr();
   unsigned len = buf.datalen() / 4;

   uint32_t data = arr[0];
   FillH2(HitsPerBinRising, -1, data & 0x7);
   signed reference = data & 0x1FFF;

   for (unsigned n = 1; n < len; n++) {
      uint32_t data = arr[n];
      unsigned channel = data >> 27;

      if ((data >> 26) & 1) {
         FillH1(Errors, channel);
      } else {
         signed risingHit = (data >> 13) & 0x1fff;
         signed fallingHit = (data >> 0) & 0x1fff;

         float timeDiff = ((risingHit - reference)) * 0.4;
         if (timeDiff < -1250)
            timeDiff += 8192 * 0.4;
         if (timeDiff > 1250)
            timeDiff -= 8192 * 0.4;

         float timeToT = (fallingHit - risingHit) * 0.4;
         if (timeToT < 0)
            timeToT += 8192 * 0.4;

         //      if(timeDiff < -1050 || timeDiff > 1100) {
         //        printf("%i\t%i\t%f\t%i\t%f\n",risingHit,reference,timeDiff/.4,fallingHit,timeToT/.4);
         //        }

         FillH2(HitsPerBinRising, channel, risingHit & 0x7);
         FillH2(HitsPerBinFalling, channel, fallingHit & 0x7);
         FillH2(deltaT, channel, timeDiff);
         FillH1(alldeltaT, timeDiff);
         FillH2(ToT, channel, timeToT);
         FillH1(allToT, timeToT);
         FillH2(deltaT_ToT, timeDiff, timeToT);

         FillH1(tot_h[channel], timeToT);
         FillH2(potato_h[channel], timeDiff, timeToT);
         FillH1(t1_h[channel], timeDiff);
      }
   }
   return true;
}
