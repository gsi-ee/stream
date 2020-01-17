#include "hadaq/AdcProcessor.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <bitset>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbProcessor.h"
#include <cmath>

using namespace std;

vector<double> hadaq::AdcProcessor::storage;

hadaq::AdcProcessor::AdcProcessor(TrbProcessor* trb, unsigned subid, unsigned numchannels, double samplingPeriod) :
   SubProcessor(trb, "ADC_%04X", subid),
   fSamplingPeriod(samplingPeriod),
   fStoreVect(),
   pStoreVect(nullptr)
{
   // Subprocessors need unique subids,
   // thus we attach a TDC with some special subid to it
   new TdcProcessor(trb, subid + 0xff0000, 2, 0);

   fKinds = nullptr;
   fChannels = nullptr;

   if (HistFillLevel() > 1) {
      fKinds = MakeH1("ADCKinds", "Messages kinds", 16, 0, 16, "kinds");
      fChannels = MakeH1("ADCChannels", "Messages per channels", numchannels, 0, numchannels, "ch");
   }

   // create records for channels
   for (unsigned ch=0;ch<numchannels;ch++)
      fCh.emplace_back();

   // histograms are created on demand
   // fCh.size() equals now numchannels...
}


hadaq::AdcProcessor::~AdcProcessor()
{
}

bool hadaq::AdcProcessor::FirstBufferScan(const base::Buffer& buf)
{
   unsigned len = buf.datalen()/4;
   if(len<1) {
      return false;
   }
   return true;
}

bool hadaq::AdcProcessor::SecondBufferScan(const base::Buffer& buf)
{
   unsigned len = buf.datalen()/4;
   uint32_t* arr = (uint32_t*) buf.ptr();


   // printf("Second scan len %u\n", len);


   // first word contains trigger epoch counter
   const unsigned ADC_trigger_epoch = arr[0] & 0xffffff;

   // start decoding the ADC data
   uint32_t nSample = 0; // number of msg from the same ADC channel, aka Sample
   uint32_t lastCh = 0;  // remember last used channel for dumping verbose data

   const char* subprefix = "Ch";

   for (unsigned n=1;n<len;n++) {
      AdcMessage msg(arr[n]);

      uint32_t kind = msg.getKind();
      FillH1(fKinds, kind);

      // kind==0xd is CFD data word header of CFD firmware
      if(kind == 0xd) {
         uint32_t ch = msg.getCh(); // has same structure as verbose data word
         if(ch>=fCh.size())
            continue;
         // there should be now 2 values after this header word
         const unsigned expected_len = 2;
         if(n+expected_len>len)
            continue;

         FillH1(fChannels, ch);
         ChannelRec& r = fCh[ch]; // helpful shortcut
         SetSubPrefix(subprefix, ch);

         // extract the epoch counter
         // upper 8bits in this word,
         // lower 16bits in following word
         const unsigned epochCounter =
               (((arr[n] >> 8) & 0xff) << 16)
               + ((arr[n+1] >> 16) & 0xffff);
         const int samplesSinceTrigger = epochCounter - ADC_trigger_epoch; // TODO: detect 24bit overflow
         if(r.fHCoarseTiming==0)
            r.fHCoarseTiming = MakeH1("CoarseTiming","Coarse timing to external trigger",10000,0,1000,"t / ns");;
         FillH1(r.fHCoarseTiming, 1e9*samplesSinceTrigger*fSamplingPeriod);

         // integral is in the lower 16bits
         const short integral = arr[n+1] & 0xffff;
         if(r.fHIntegral==0)
            r.fHIntegral = MakeH1("Integral","Summed integral",10000,0,10000,"integral");
         FillH1(r.fHIntegral, integral);

         // CFD timing from last 32bit word
         const short valBeforeZeroX = (arr[n+2] >> 16) & 0xffff;
         const short valAfterZeroX = arr[n+2] & 0xffff;
         const double fraction = (double)valBeforeZeroX/(valBeforeZeroX-valAfterZeroX);
         const double fineTiming = (samplesSinceTrigger + fraction)*fSamplingPeriod;

         if(r.fHFineTiming==0)
            r.fHFineTiming = MakeH1("FineTiming","Fine timing to external trigger",10000,0,1000,"t / ns");
         FillH1(r.fHFineTiming, 1e9*fineTiming);

         if(r.fHSamples==0)
            r.fHSamples = MakeH2("Samples","Samples of the zero crossing",2,0,2,1000,-500,500,"crossing;value");
         FillH2(r.fHSamples, 0, valBeforeZeroX);
         FillH2(r.fHSamples, 1, valAfterZeroX);

         unsigned indx = 0; // index 0 is event index in triggered-based analysis

         if (indx < fGlobalMarks.size()) {
            msg.fFineTiming = fineTiming;
            msg.fIntegral = integral;
            AddMessage(indx, (hadaq::AdcSubEvent*) fGlobalMarks.item(indx).subev, msg);
         }


         SetSubPrefix();
         // don't forget to move forward
         n += expected_len;
      }
      // kind==0 is waveform debug data word
      else if(kind == 0) {
         uint32_t ch = msg.getCh();
         short int value = msg.getValue();

         if(ch>=fCh.size())
            continue;

         FillH1(fChannels, ch);

         SetSubPrefix(subprefix, ch);

         if(HistFillLevel() > 1)
            FillH1(fChannels, ch);

         ChannelRec& r = fCh[ch]; // helpful shortcut

         if(r.fHValues == 0)
            r.fHValues = MakeH1("Values","Distribution of values (unsigned)", 1<<10, 0, 1<<10, "value");
         FillH1(r.fHValues, value);

         // check if msg still belongs to same ADC channel
         // catch first msg as special case
         if(n==0 || ch != lastCh) {
            lastCh = ch;
            nSample = 0;
         }
         else {
            nSample++;
         }

         if(r.fHWaveform==0)
            r.fHWaveform = MakeH2("Waveform", "Integrated Waveform", 512, 0, 512, 1<<11, -(1<<10), 1<<10, "sample;value");
         FillH2(r.fHWaveform, nSample, value);

         SetSubPrefix();
      }
      // other kinds like PSA data or compressed ADC words unsupported for now
      // they are just ignored
   }

   return true;
}


void hadaq::AdcProcessor::CreateBranch(TTree*)
{
   pStoreVect = &fStoreVect;
   mgr()->CreateBranch(GetName(), "std::vector<hadaq::AdcMessage>", (void**) &pStoreVect);
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

void hadaq::AdcProcessor::ResetStore()
{
   pStoreVect = &fStoreVect;
}

