#include "hadaq/MdcProcessor.h"

#include <cstdio>

hadaq::MdcProcessor::MdcProcessor(TrbProcessor* trb, unsigned subid) :
   hadaq::SubProcessor(trb, "MDC_%04x", subid)
{
   SubId = subid;
   if (HistFillLevel() > 1) {
      alldeltaT = MakeH1("alldeltaT", "", 6000, -2099.8, 300.2);
      allToT = MakeH1("allToT", "", 1650, 0.2, 1100.2);
      Errors = MakeH1("Errors", "", TDCCHANNELS, -0.5, TDCCHANNELS-0.5);
      ToT = MakeH2("ToT", "", TDCCHANNELS, 0, TDCCHANNELS, 550, 0.2, 1100.2);
      deltaT = MakeH2("deltaT", "", TDCCHANNELS, 0, TDCCHANNELS, 600, -2099.8, 300.2);
      deltaT_ToT = MakeH2("deltaT_ToT", "", 600, -2099.8, 300.2, 550, 0.2, 1100.2);
      
      Hits[0] = MakeH2("Hit0","", 600, -2099.8, 300.2, 550, 0.2, 1100.2);
      Hits[1] = MakeH2("Hit1","", 600, -2099.8, 300.2, 550, 0.2, 1100.2);
//      Hits[2] = MakeH2("Hit2","", 600, -2099.8, 300.2, 550, 0.2, 1100.2);
//      Hits[3] = MakeH2("Hit3","", 600, -2099.8, 300.2, 550, 0.2, 1100.2);
//      ToTToT[0] = MakeH2("ToTToT0","", 550, 0.2, 2100.2, 550, 0.2, 2100.2);
//      ToTToT[1] = MakeH2("ToTToT1","", 600, -2099.8, 300.2, 600, -2099.8, 300.2);
//      ToTToT[2] = MakeH2("ToTToT2","", 600, -2099.8, 300.2, 600, -2099.8, 300.2);
//      ToTToT[3] = MakeH2("ToTToT3","", 1100, -2099.8, 1100.2, 1050, -2099.8, -2100.2);
//      longToT = MakeH1("longTot", "", 32, 0, 32);

   }

   if (HistFillLevel() > 2) {
      HitsPerBinRising = MakeH2("HitsPerBinRising", "", 33, -1, 32, 8, 0, 8);
      HitsPerBinFalling = MakeH2("HitsPerBinFalling", "", 33, -1, 32, 8, 0, 8);
      T0_T1 = MakeH2("T0_T1", "", 600, -2099.8, 300.2, 600, -2099.8, 300.2);
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

   uint32_t data0 = arr[0];
   FillH2(HitsPerBinRising, -1, data0 & 0x7);
   signed reference = data0 & 0x1FFF;

   int numhits = 0, numerrs = 0;
   
   int hitsperchannel[TDCCHANNELS] = {0};
//   float last_timeToT = 0;
//   float last_timeDiff = 0;
//   float last_timeDiff1 = 0;
   
//   int numlongtot = 0;
//   int haslongtot[TDCCHANNELS] = {0};
   
   for (unsigned n = 1; n < len; n++) {
      uint32_t data = arr[n];
      unsigned channel = data >> 27;

      hitsperchannel[channel]++;
      
      if ((data >> 26) & 1) {
         numerrs++;
         FillH1(Errors, channel);
      }
      numhits++;
      signed risingHit = (data >> 13) & 0x1fff;
      signed fallingHit = (data >> 0) & 0x1fff;

      float timeDiff = ((risingHit - reference)) * 0.4;
      if (timeDiff < -2000)
         timeDiff += 8192 * 0.4;
      if (timeDiff > 300)
         timeDiff -= 8192 * 0.4;



      float timeDiff1 = ((fallingHit - reference)) * 0.4;
      if (timeDiff1 < -2000)
         timeDiff1 += 8192 * 0.4;
      if (timeDiff1 > 300)
         timeDiff1 -= 8192 * 0.4;

      float timeToT = (fallingHit - risingHit) * 0.4;
      if (timeToT < 0)
         timeToT += 8192 * 0.4;

      if (dostore) {
         pStoreFloat->emplace_back(channel, timeDiff, timeToT);
      }


      FillH2(deltaT, channel, timeDiff);
      FillH1(alldeltaT, timeDiff);
      FillH2(ToT, channel, timeToT);
      FillH1(allToT, timeToT);
      FillH2(deltaT_ToT, timeDiff, timeToT);
      FillH2(T0_T1, timeDiff, timeDiff1);

      if (hChToTHld)
         SetH2Content(*hChToTHld, fHldId, channel, timeToT);

      if (hChDeltaTHld)
         SetH2Content(*hChDeltaTHld, fHldId, channel, timeDiff);

      if (HistFillLevel() > 2) {
         FillH2(HitsPerBinRising, channel, risingHit & 0x7);
         FillH2(HitsPerBinFalling, channel, fallingHit & 0x7);
         // FillH2(deltaT, channel, timeDiff);
         // FillH1(alldeltaT, timeDiff);
         // FillH2(ToT, channel, timeToT);
         // FillH1(allToT, timeToT);
         // FillH2(deltaT_ToT, timeDiff, timeToT);
         // FillH2(T0_T1, timeDiff, timeDiff1);
         // 
         // if (hChToTHld)
         //    SetH2Content(*hChToTHld, fHldId, channel, timeToT);
         // 
         // if (hChDeltaTHld)
         //    SetH2Content(*hChDeltaTHld, fHldId, channel, timeDiff);
         // 
         // if (HistFillLevel() > 2) {
         //    FillH1(tot_h[channel], timeToT);
         //    FillH2(potato_h[channel], timeDiff, timeToT);
         //    FillH1(t1_h[channel], timeDiff);
         // }
      }
//    if(channel==6) {  
//      if(hitsperchannel[channel] < 5) {
         // if(hitsperchannel[channel]==1 || haslongtot[channel])
//            FillH2(Hits[hitsperchannel[channel]-1], timeDiff, timeToT);
//         }      
/*      if(hitsperchannel[channel] == 1) {
         FillH2(Hits[0], timeDiff, timeToT);
         }
      if(channel>=1 && (hitsperchannel[channel-1] >= 1 || (arr[n+1] >> 27) == channel+1) && hitsperchannel[channel] == 1) {
         FillH2(Hits[1], timeDiff, timeToT);
         }
      if(channel>=2 && hitsperchannel[channel-2] == 0 && hitsperchannel[channel-1] == 0 && hitsperchannel[channel] == 1 && (arr[n+1] >> 27) != channel+1 && (arr[n+1] >> 27) != channel+2) {
         FillH2(Hits[2], timeDiff, timeToT);
         }      
      if(channel>=1 && hitsperchannel[channel-1] == 0 && hitsperchannel[channel] == 1 && (arr[n+1] >> 27) != channel+1) {
         FillH2(Hits[3], timeDiff, timeToT);
         } */     
//    } 
      // if(channel==13 && channel>=1 && (hitsperchannel[channel-1] >= 1) && hitsperchannel[channel] == 1) {
      //    FillH2(ToTToT[0], last_timeToT, timeToT);
      //    FillH2(ToTToT[1], last_timeDiff, timeDiff);
      //    FillH2(ToTToT[2], last_timeDiff1, timeDiff1);
      //    FillH2(ToTToT[3], timeDiff-last_timeDiff, timeDiff1-last_timeDiff1);
      //    }
         
//      if(timeToT > 150) {
//         numlongtot++;
//         haslongtot[channel]++;
//         }
         
//      last_timeToT = timeToT;  
//      last_timeDiff = timeDiff;
//      last_timeDiff1 = timeDiff1;
   }

//   FillH1(longToT,numlongtot);
   
   if (hHitsHld)
      DefFillH1(*hHitsHld, fHldId, numhits);

   if (hErrsHld)
      DefFillH1(*hErrsHld, fHldId, numerrs);

   return true;
}
