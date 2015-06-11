#include "hadaq/AdcProcessor.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <bitset>

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
      rec.fIntegral = MakeH1("Integral","Summed integral",10000,0,10000,"integral");
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

   // start decoding the samples
   uint32_t nSample = 0; // number of msg from the same ADC channel, aka Sample
   uint32_t lastCh = 0;  // remember last used channel for dumping verbose data
   int lastTriggerEpoch = -1; // remember last trigger epoch
   for (unsigned n=0;n<len;n++) {
      AdcMessage msg(arr[n]);

      uint32_t kind = msg.getKind();
      FillH1(fKinds, kind);

      // kind==0xc is CFD data word header of modified PSA firmware
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
         FillTimingHistos(ch, integral, samplesSinceTrigger, valBeforeZeroX, valAfterZeroX);
      }
      // kind==0xd is CFD data word header of CFD firmware
      else if(kind == 0xd) {
         uint32_t ch = msg.getCh(); // has same structure as verbose data word
         if(ch>=fCh.size())
            continue;
         // there should be now 3 values after this header word
         if(n+3>=len)
            continue;
         // we should have seen some trigger epoch word
         if(lastTriggerEpoch<0)
            continue;

         // extract the epoch counter
         // upper 8bits in this word,
         // lower 16bits in following word
         const unsigned epochCounter =
               (((arr[n] >> 8) & 0xff) << 16)
               + ((arr[n+1] >> 16) & 0xffff);
         const int samplesSinceTrigger = epochCounter - lastTriggerEpoch;

         // integral and timings
         const short integral = arr[n+1] & 0xffff;
         const short valBeforeZeroX = (arr[n+2] >> 16) & 0xffff;
         const short valAfterZeroX = arr[n+2] & 0xffff;
         FillTimingHistos(ch, integral, samplesSinceTrigger, valBeforeZeroX, valAfterZeroX);
      }
      // kind==0xe is trigger epoch word of CFD firmware
      else if(kind == 0xe) {
         // the lower 24bit are the epoch counter as gray code
         std::bitset<24> trigger_epoch_gray(arr[n]);
         std::bitset<24> trigger_epoch_bin(0);
         trigger_epoch_bin[23] = trigger_epoch_gray[23];
         for(int i=22 ; i>=0 ; i--) {
            // B(i) = B(i+1) xor G(i), see Wikipedia :)
            trigger_epoch_bin[i] = trigger_epoch_bin[i+1] ^ trigger_epoch_gray[i];
         }
         lastTriggerEpoch = trigger_epoch_bin.to_ulong();
      }
      // kind==0 is verbose data word for both firmware versions
      else if(kind == 0) {
         uint32_t ch = msg.getCh();
         short int value = msg.getValue();

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
      // they are just ignored
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

void hadaq::AdcProcessor::FillTimingHistos(
      uint32_t ch,
      const int integral,
      const int samplesSinceTrigger,
      const int valBeforeZeroX,
      const int valAfterZeroX)
{
   const double samplingPeriod = 1000.0/40.0; // in nano seconds 25ns=40MHz,
   if(valBeforeZeroX<valAfterZeroX)
     return;
   const double fraction = (double)valBeforeZeroX/(valBeforeZeroX-valAfterZeroX);
   const double coarseTiming = samplingPeriod*samplesSinceTrigger;
   const double fineTiming = samplingPeriod*fraction + coarseTiming;
   FillH1(fCh[ch].fIntegral, integral);
   FillH1(fCh[ch].fCFDCoarseTiming, coarseTiming);
   FillH1(fCh[ch].fCFDFineTiming, fineTiming);
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
