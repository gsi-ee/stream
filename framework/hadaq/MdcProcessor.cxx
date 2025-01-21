#include "hadaq/MdcProcessor.h"


hadaq::MdcProcessor::MdcProcessor(TrbProcessor* trb, unsigned subid) :
   hadaq::SubProcessor(trb, "MDC_%04x", subid)
{
   if (HistFillLevel() > 1) {
      alldeltaT = MakeH1("alldeltaT", "", 6000, -1199.8, 1200.2);
      allToT = MakeH1("allToT", "", 1500, 0.2, 600.2);
      Errors = MakeH1("Errors", "", 33, -1, 32);
   }

   if (HistFillLevel() > 2) {
      HitsPerBinRising = MakeH2("HitsPerBinRising", "", 33, -1, 32, 8, 0, 8);
      HitsPerBinFalling = MakeH2("HitsPerBinFalling", "", 33, -1, 32, 8, 0, 8);
      ToT = MakeH2("ToT", "", 32, 0, 32, 1500, 0.2, 600.2);
      deltaT = MakeH2("deltaT", "", 32, 0, 32, 6000, -1199.8, 1200.2);
      deltaT_ToT = MakeH2("deltaT_ToT", "", 6000, -2099.8, 300.2, 1500, 0.2, 600.2);
      T0_T1 = MakeH2("T0_T1", "", 6000, -2099.8, 300.2, 6000, -2099.8, 300.2);
   }
   for (unsigned i = 0; i < TDCCHANNELS + 1; i++) {
      t1_h[i] = nullptr;
      tot_h[i] = nullptr;
      potato_h[i] = nullptr;

      if (HistFillLevel() > 3) {
         char chno[16];
         snprintf(chno, sizeof(chno), "Ch%02d_t1", i);
         t1_h[i] = MakeH1(chno, chno, 6000, -1199.8, 1200.2, "ns");
         snprintf(chno, sizeof(chno), "Ch%02d_tot", i);
         tot_h[i] = MakeH1(chno, chno, 1500, 0.2, 600.2, "ns");
         snprintf(chno, sizeof(chno), "Ch%02d_potato", i);
         potato_h[i] = MakeH2(chno, chno, 600, -1199.8, 1200.2, 500, 0.2, 600.2, "t1 (ns);tot (ns)");
      }
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Create TTree branch

void hadaq::MdcProcessor::CreateBranch(TTree*)
{
   if(GetStoreKind() > 0) {
      pStoreFloat = &fDummyFloat;
      mgr()->CreateBranch(GetName(), "std::vector<hadaq::MdcMessage>", (void**) &pStoreFloat);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Store event

void hadaq::MdcProcessor::Store(base::Event* ev)
{
   // always set to dummy
   pStoreFloat = &fDummyFloat;

   // in case of triggered analysis all pointers already set
   if (!ev /*|| IsTriggeredAnalysis()*/) return;

   auto sub0 = ev->GetSubEvent(GetName());
   if (!sub0) return;

   if (GetStoreKind() > 0) {
      auto sub = dynamic_cast<hadaq::MdcSubEvent *> (sub0);
      // when subevent exists, use directly pointer on messages vector
      if (sub) {
         pStoreFloat = sub->vect_ptr();
      }
   }
}


bool hadaq::MdcProcessor::FirstBufferScan(const base::Buffer &buf)
{
   uint32_t *arr = (uint32_t *)buf.ptr();
   unsigned len = buf.datalen() / 4;

   bool dostore = false;

   if (IsTriggeredAnalysis() && IsStoreEnabled() && mgr()->HasTrigEvent() && (GetStoreKind() > 0)) {
      dostore = true;
      auto subevnt = new hadaq::MdcSubEvent(len - 1); // expected number of messages
      mgr()->AddToTrigEvent(GetName(), subevnt);
      pStoreFloat = subevnt->vect_ptr();
   }

   uint32_t data = arr[0];
   FillH2(HitsPerBinRising, -1, data & 0x7);
   signed reference = data & 0x1FFF;

   int numhits = 0, numerrs = 0;

   for (unsigned n = 1; n < len; n++) {
      uint32_t data = arr[n];
      unsigned channel = data >> 27;

      if ((data >> 26) & 1) {
         numerrs++;
         FillH1(Errors, channel);
      } else {
         numhits++;
         signed risingHit = (data >> 13) & 0x1fff;
         signed fallingHit = (data >> 0) & 0x1fff;

         float timeDiff = ((risingHit - reference)) * 0.4;
         if (timeDiff < -2100)
            timeDiff += 8192 * 0.4;
         if (timeDiff > 300)
            timeDiff -= 8192 * 0.4;

         float timeDiff1 = ((fallingHit - reference)) * 0.4;
         if (timeDiff1 < -2100)
            timeDiff1 += 8192 * 0.4;
         if (timeDiff1 > 300)
            timeDiff1 -= 8192 * 0.4;

         float timeToT = (fallingHit - risingHit) * 0.4;
         if (timeToT < 0)
            timeToT += 8192 * 0.4;

         if (dostore) {
            pStoreFloat->emplace_back(channel, timeDiff, timeToT);
         }

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
         FillH2(T0_T1, timeDiff, timeDiff1);

         if (HistFillLevel() > 2) {
            FillH1(tot_h[channel], timeToT);
            FillH2(potato_h[channel], timeDiff, timeToT);
            FillH1(t1_h[channel], timeDiff);
         }
      }
   }

   if (hHitsHld)
      DefFillH1(*hHitsHld, fHldId, numhits);

   if (hErrsHld)
      DefFillH1(*hErrsHld, fHldId, numerrs);

   return true;
}
