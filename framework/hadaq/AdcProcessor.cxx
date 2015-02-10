#include "hadaq/AdcProcessor.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbProcessor.h"

hadaq::AdcProcessor::AdcProcessor(TrbProcessor* trb, unsigned subid, unsigned numchannels) :
   SubProcessor(trb, "ADC_%04x", subid),
   fStoreVect(),
   pStoreVect(0)
{
   fChannels = 0;

   if (HistFillLevel() > 1) {
      fKinds = MakeH1("Kinds", "Messages kinds", 16, 0, 16, "kinds");
      fChannels = MakeH1("Channels", "Messages per channels", numchannels, 0, numchannels, "ch");
   }

   for (unsigned ch=0;ch<numchannels;ch++) {
      ChannelRec rec;
      SetSubPrefix("Ch", ch);
      rec.fValues = MakeH1("Values","Distribution of values (unsigned)", 1<<10, 0, 1<<10, "value");
      rec.fWaveform = MakeH2("Waveform", "Integrated Waveform", 512, 0, 512, 1<<10, 0, 1<<10, "sample;value");
      rec.fCFDIntegral = MakeH1("CFDIntegral","CFD summed integral",10000,0,10000,"integral");
      rec.fCFDFineTiming = MakeH1("CFDFineTiming","CFD fine timing",10000,0,10000,"t / ns");
      rec.fCFDCoarseTiming = MakeH1("CFDCoarseTiming","CFD coarse timing",10000,0,10000,"t / ns");
      SetSubPrefix();
      fCh.push_back(rec);
   }
}


hadaq::AdcProcessor::~AdcProcessor()
{
}


bool hadaq::AdcProcessor::FirstBufferScan(const base::Buffer& buf)
{
   unsigned len = buf.datalen()/4;
   uint32_t* arr = (uint32_t*) buf.ptr();

   // printf("First scan len %u\n", len);

   // BeforeFill(); // optional

   // use iterator only if context is important
   uint32_t nSample = 0; // number of msg from the same ADC channel, aka Sample
   uint32_t lastCh = 0;  // remember last used channel for dumping verbose data
   for (unsigned n=0;n<len;n++) {
      AdcMessage msg(arr[n]);

      uint32_t kind = msg.getKind();
      FillH1(fKinds, kind);

      // kind==0xc is CFD data word header
      if(kind == 0xc ) {
         uint32_t ch = msg.getCh(); // has same structure as verbose data word
         if(ch>=fCh.size())
            continue;
         // there should be now 4 values after this header word
         if(n+4>=len)
            continue;
         const int32_t  integral = arr[++n];
         const uint32_t samplesSinceTrigger = arr[++n];
         const int32_t  valBeforeZeroX = arr[++n];
         const int32_t  valAfterZeroX = arr[++n];
         if(valBeforeZeroX<valAfterZeroX)
            continue;
         const double samplingPeriod = 25; // in nano seconds
         const double fraction = (double)valBeforeZeroX/(valBeforeZeroX-valAfterZeroX);
         const double coarseTiming = samplingPeriod*samplesSinceTrigger;
         const double fineTiming = samplingPeriod*fraction + coarseTiming;
         FillH1(fCh[ch].fCFDIntegral, integral);
         FillH1(fCh[ch].fCFDCoarseTiming, coarseTiming);
         FillH1(fCh[ch].fCFDFineTiming, fineTiming);
      }
      // kind==0 is verbose data word
      else if(kind == 0) {
         uint32_t ch = msg.getCh();
         uint32_t value = msg.getValue();

         if(ch>=fCh.size())
            continue;

         FillH1(fChannels, ch);
         FillH1(fCh[ch].fValues, value);

         // check if msg still belongs to same ADC channel
         // catch first msg as special case
         if(n==0 || ch != lastCh) {
            lastCh = ch;
            nSample = 0;
         }
         else {
            nSample++;
         }

         FillH2(fCh[ch].fWaveform, nSample, value);
      }
      // other kinds like PSA data or compressed ADC words unsupported for now
   }

   // if (!fCrossProcess) AfterFill(); // optional


   return true;

}

bool hadaq::AdcProcessor::SecondBufferScan(const base::Buffer& buf)
{
   unsigned len = buf.datalen()/4;
   uint32_t* arr = (uint32_t*) buf.ptr();

   // printf("Second scan len %u\n", len);

   // use iterator only if context is important
   for (unsigned n=0;n<len;n++) {
      AdcMessage msg(arr[n]);

      // ignore all other kinds
      if (msg.getKind()!=1) continue;

      // we test hits, but do not allow to close events
      // this is normal procedure
      // unsigned indx = TestHitTime(0., true, false);
      unsigned indx = 0; // index 0 is event index in triggered-based analysis

      if (indx < fGlobalMarks.size()) {
         AddMessage(indx, (hadaq::AdcSubEvent*) fGlobalMarks.item(indx).subev, msg);
      }
   }

   return true;
}


void hadaq::AdcProcessor::CreateBranch(TTree* t)
{
   pStoreVect = &fStoreVect;
   mgr()->CreateBranch(t, GetName(), "std::vector<hadaq::AdcMessage>", (void**) &pStoreVect);
}

void hadaq::AdcProcessor::Store(base::Event* ev)
{
   fStoreVect.clear();

   hadaq::AdcSubEvent* sub =
         dynamic_cast<hadaq::AdcSubEvent*> (ev->GetSubEvent(GetName()));

   // when subevent exists, use directly pointer on messages vector
   if (sub!=0)
      pStoreVect = sub->vect_ptr();
   else
      pStoreVect = &fStoreVect;
}
