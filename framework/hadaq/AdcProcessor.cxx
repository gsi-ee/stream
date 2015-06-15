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
   TdcProcessor(trb, subid, 2, 0, "ADC_%04X"), // TDC channels 2, edgeMask=0 means just rising edge
   fSamplingPeriod(samplingPeriod),
   fStoreVect(),
   pStoreVect(0)
{
  
   fChannels = 0;
   
   if (HistFillLevel() > 1) {
      fKinds = MakeH1("ADCKinds", "Messages kinds", 16, 0, 16, "kinds");
      fChannels = MakeH1("ADCChannels", "Messages per channels", numchannels, 0, numchannels, "ch");
      if(HistFillLevel() > 3) {
         fADCPhase = MakeH1("ADCPhase", "ADC Clock phase to trigger", 3000, 0, 3*fSamplingPeriod, "phase / ns");
      }
   }
   

   for (unsigned ch=0;ch<numchannels;ch++) {
      const ChannelRec rec; 
      fCh.push_back(rec);
   }
   // histograms are created on demand
   // fCh.size() equals now numchannels...
}


hadaq::AdcProcessor::~AdcProcessor()
{
}

void hadaq::AdcProcessor::SetDiffChannel(unsigned ch, int diffch)
{
   fCh[ch].fDiffCh = diffch;
}

void fitxy(const vector<double>& x_, const vector<double>& y_, 
           const vector<double>& sigy, 
           double& a, double& b) {
   double S   = 0;
   double Sx  = 0;
   double Sy  = 0;
   double Sxx = 0;
   double Sxy = 0;
   for(size_t i=0;i<y_.size();i++) {
      const double s = sigy[i];
      const double s2 = s*s;
      const double x  = x_[i];
      const double y  = y_[i];
      S   += 1/s2;
      Sx  += x/s2;
      Sy  += y/s2;
      Sxx += x*x/s2;
      Sxy += x*y/s2;
   }
   const double D = S*Sxx-Sx*Sx;
   a = (Sxx*Sy - Sx*Sxy)/D;
   b = (S*Sxy-Sx*Sy)/D;
}

double getfraction(const vector<short>& edges) {
   const size_t n = edges.size();
   vector<double> x_(n), y_(n), sx(n), sy(n);
   for(size_t i=0; i<y_.size(); i++) {
      x_[i] = i;
      y_[i] = edges[i];
      sx[i] = 0.5;
      sy[i] = 1; //0.1+0.01*edges[i];
   }
   double a, b;
   fitxy(x_, y_, sy, a, b);
   //fitxye(x_,y_,sx,sy,a,b);
   return -a/b;
}

bool hadaq::AdcProcessor::FirstBufferScan(const base::Buffer& buf)
{
   unsigned len = buf.datalen()/4;
   uint32_t* arr = (uint32_t*) buf.ptr();
   
   // search for ADC marker (also contains epoch counter of trigger hit)
   // in order to split the whole buffer into TDC part and ADC part
   unsigned ADC_trigger_epoch;
   unsigned ADC_offset = 0;
   for (unsigned n=0;n<len;n++) {
      if(arr[n] >> 28 == 0x1) {
         ADC_offset = n;
         ADC_trigger_epoch = arr[n] & 0xfffffff; // lower 28 bits are epoch
         break;
      }
   }
   // marker not found or no TDC data found, skip this buffer 
   if(ADC_offset==0) {
      return false;
   }
   
   // decode the TDC stuff
   base::Buffer TDC_buf;
   TDC_buf.makereferenceof(buf.ptr(), 4*ADC_offset);
   if(!TdcProcessor::FirstBufferScan(TDC_buf))
      return false;
   TdcProcessor::SecondBufferScan(TDC_buf);
   
   // obtain TDC fine timing for two channels
   const TdcProcessor::ChannelRec& tdc_trigger  = TdcProcessor::fCh.at(0);
   const TdcProcessor::ChannelRec& tdc_adcclock = TdcProcessor::fCh.at(1);  
   // then calculate the ADC clock phase to the trigger signal
   double adc_phase = 1.0e9*(tdc_adcclock.rising_hit_tm-tdc_trigger.rising_hit_tm);
   if(HistFillLevel()>3) 
      FillH1(fADCPhase, adc_phase);
   
  

   // start decoding the ADC data
   uint32_t nSample = 0; // number of msg from the same ADC channel, aka Sample
   uint32_t lastCh = 0;  // remember last used channel for dumping verbose data

   
   const char* subprefix = "ChADC";
   
   vector< vector<short> > raw_samples;
   raw_samples.resize(fCh.size());
   
   for (unsigned n=ADC_offset+1;n<len;n++) {
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

         ChannelRec& r = fCh[ch]; // helpful shortcut
         SetSubPrefix(subprefix, ch);
        
         // extract the epoch counter
         // upper 8bits in this word,
         // lower 16bits in following word
         const unsigned epochCounter =
               (((arr[n] >> 8) & 0xff) << 16)
               + ((arr[n+1] >> 16) & 0xffff);
         const int samplesSinceTrigger = epochCounter - ADC_trigger_epoch; // TODO: detect 24bit overflow
         if(r.fCoarseTiming==0)
            r.fCoarseTiming = MakeH1("CoarseTiming","Coarse timing to external trigger",10000,0,1000,"t / ns");;
         FillH1(r.fCoarseTiming, samplesSinceTrigger*fSamplingPeriod);

         // integral is in the lower 16bits
         const short integral = arr[n+1] & 0xffff;
         if(r.fIntegral==0)
            r.fIntegral = MakeH1("Integral","Summed integral",10000,0,10000,"integral");
         FillH1(r.fIntegral, integral);
         
         // CFD timing from last 32bit word
         const short valBeforeZeroX = (arr[n+2] >> 16) & 0xffff;
         const short valAfterZeroX = arr[n+2] & 0xffff;
         const double fraction = (double)valBeforeZeroX/(valBeforeZeroX-valAfterZeroX);
         const double fineTiming = (samplesSinceTrigger + fraction)*fSamplingPeriod + adc_phase;
         
         r.fTiming = fineTiming;
         if(r.fFineTiming==0)
            r.fFineTiming = MakeH1("FineTiming","Fine timing to external trigger",10000,0,1000,"t / ns");
         FillH1(r.fFineTiming, r.fTiming);
         
         if(r.fBeforeVsFrac==0)
            r.fBeforeVsFrac = MakeH2("BeforeVsFrac","First vs. Fraction",1000,-500,500,1000,0,fSamplingPeriod,"before;fraction");
         FillH2(r.fBeforeVsFrac,valBeforeZeroX,fraction*fSamplingPeriod);
         
         if(r.fAfterVsFrac==0)
            r.fAfterVsFrac = MakeH2("AfterVsFrac","Second vs. Fraction",1000,-500,500,1000,0,fSamplingPeriod,"after;fraction");
         FillH2(r.fAfterVsFrac,valAfterZeroX,fraction*fSamplingPeriod);
         
         if(r.fPhaseVsBefore==0)
            r.fPhaseVsBefore = MakeH2("PhaseVsBefore","Phase vs. Before",1000,0,3*fSamplingPeriod,1000,-500,500,"phase;before");
         FillH2(r.fPhaseVsBefore,adc_phase,valBeforeZeroX);
         
         if(r.fPhaseVsAfter==0) 
            r.fPhaseVsAfter = MakeH2("PhaseVsAfter","Phase vs. After",1000,0,3*fSamplingPeriod,1000,-500,500,"phase;after");
         FillH2(r.fPhaseVsAfter,adc_phase,valAfterZeroX);
         
         if(r.fPhaseVsFrac==0)
            r.fPhaseVsFrac = MakeH2("PhaseVsFrac","Phase vs. Fraction",1000,0,3*fSamplingPeriod,1000,0,fSamplingPeriod,"phase;fraction");
         FillH2(r.fPhaseVsFrac,adc_phase,fraction*fSamplingPeriod);
         
         if(r.fPhaseVsEpoch==0)
            r.fPhaseVsEpoch = MakeH2("PhaseVsEpoch","Phase vs. Epoch",1000,0,3*fSamplingPeriod,100,0,100,"phase;epoch");
         FillH2(r.fPhaseVsEpoch,adc_phase,samplesSinceTrigger);
         
         if(r.fSamples==0)
            r.fSamples = MakeH2("Samples","Samples of the zero crossing",2,0,2,1000,-500,500,"crossing;value");
         FillH2(r.fSamples, 0, valBeforeZeroX);
         FillH2(r.fSamples, 1, valAfterZeroX);
     
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

         SetSubPrefix(subprefix, ch);
         
         if(HistFillLevel() > 1)
            FillH1(fChannels, ch);
         
         ChannelRec& r = fCh[ch]; // helpful shortcut         
         
         if(r.fValues == 0)
            r.fValues = MakeH1("Values","Distribution of values (unsigned)", 1<<10, 0, 1<<10, "value");
         FillH1(r.fValues, value);

         // check if msg still belongs to same ADC channel
         // catch first msg as special case
         if(n==0 || ch != lastCh) {
            lastCh = ch;
            nSample = 0;
         }
         else {
            nSample++;
         }

         if(r.fWaveform==0)
            r.fWaveform = MakeH2("Waveform", "Integrated Waveform", 512, 0, 512, 1<<11, -(1<<10), 1<<10, "sample;value");
         FillH2(r.fWaveform, nSample, value);
         
         raw_samples[ch].push_back(value);
         
         SetSubPrefix();         
      }
      // other kinds like PSA data or compressed ADC words unsupported for now
      // they are just ignored
   }

   const size_t ch_ = 24;
   if(raw_samples[ch_].size()>100) {
      ChannelRec& c = fCh[ch_];
      if(c.fPhaseVsSample==0)
         c.fPhaseVsSample = MakeH2("PhaseVsSample","Phase vs. Sample",1000,0,3*fSamplingPeriod,500,-5,5,"phase;sample");
      vector<short>::iterator i = raw_samples[ch_].begin();
      vector<short> edges;
      edges.assign(i+30,i+33);
      //edges.assign(i+29,i+34);
      const double fraction = getfraction(edges);
      FillH2(c.fPhaseVsSample,adc_phase,fraction);
   }
   
   for(size_t ch=0;ch<fCh.size();ch++) {
      ChannelRec& c = fCh[ch];
      if(c.fDiffCh<0)
         continue;
      
      const double diff = c.fTiming - fCh[c.fDiffCh].fTiming;
      
      const double pos = std::fmod((c.fTiming + fCh[c.fDiffCh].fTiming)/2, 2*fSamplingPeriod);
      const double neg = std::fmod((c.fTiming - fCh[c.fDiffCh].fTiming)/2, 2*fSamplingPeriod);
      
      
      if(!isfinite(diff))
         continue;
      
      SetSubPrefix(subprefix, ch);
      
      if(c.fDiffTiming == 0)
         c.fDiffTiming = MakeH1("DiffTiming","Timing difference",10000,-500,500,"t / ns");
      FillH1(c.fDiffTiming, diff);
      
      if(c.fPhaseVsPos==0)
         c.fPhaseVsPos = MakeH2("PhaseVsPos","Phase vs. Pos",1000,0,3*fSamplingPeriod,1000,0,2*fSamplingPeriod,"phase;pos");
      FillH2(c.fPhaseVsPos,adc_phase,pos);
      
      if(c.fPhaseVsNeg==0)
         c.fPhaseVsNeg = MakeH2("PhaseVsNeg","Phase vs. Pos",1000,0,3*fSamplingPeriod,1000,0,2*fSamplingPeriod,"phase;neg");
      FillH2(c.fPhaseVsNeg,adc_phase,neg);
      
      SetSubPrefix(); 
   }
   
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
