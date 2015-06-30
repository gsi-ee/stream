
#include "base/EventProc.h"
#include "base/Event.h"
#include "hadaq/TdcSubEvent.h"
#include "hadaq/AdcSubEvent.h"
#include "TGo4Analysis.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>

#include "TTree.h"

#include <my_config.h>

using namespace std;

class ADCProc : public base::EventProc {
protected:
   
   string fTdcId;   
   string fAdcId;   
   string fTdcInCTSId;
   
   
   base::H1handle  hAdcPhase;      
 
   struct hChannel_t {
       base::H1handle Timing;
       base::H2handle TDCHitVsTiming;
   };
   
   typedef map<unsigned, hChannel_t> hChannels_t;
   hChannels_t hChannels;
   
   const my_config_t config;
   
public:
   ADCProc(const char* procname, unsigned id, unsigned ctsid, const my_config_t& cfg) :
      base::EventProc(procname),
      config(cfg)
   {
      
      stringstream ss;
      ss << hex;
      
      ss << "ADC_" << setw(4) << setfill('0') << id;
      fAdcId = ss.str();
      ss.str("");
      
      ss << "TDC_" << setw(4) << setfill('0') << id;
      fTdcId = ss.str();
      ss.str("");
      
      ss << "TDC_" << setw(4) << setfill('0') << ctsid;
      fTdcInCTSId = ss.str();
      ss.str("");
      
      cout << "Create " << GetName() << " with " 
           << fAdcId << "/" << fTdcId 
           << " with ref " << fTdcInCTSId << endl;
      
      hAdcPhase = MakeH1("AdcPhase","Phase of external trigger to ADC clock", 1000, 0, 100, "t / ns");
      // enable storing already in constructor
      SetStoreEnabled();
   }
   
   
   virtual void CreateBranch(TTree* t)
   {
      //t->Branch(GetName(), fHits, "hits[16]/D");
   }
   
   virtual bool Process(base::Event* ev) 
   {
      const bool debug = false;
      
      
      hadaq::AdcSubEvent* adc = 
            dynamic_cast<hadaq::AdcSubEvent*> (ev->GetSubEvent(fAdcId));
      
      hadaq::TdcSubEvent* tdc = 
            dynamic_cast<hadaq::TdcSubEvent*> (ev->GetSubEvent(fTdcId));
      
      hadaq::TdcSubEvent* cts = 
            dynamic_cast<hadaq::TdcSubEvent*> (ev->GetSubEvent(fTdcInCTSId));
      
      if(adc==0 || tdc==0 || cts==0)
         return false;
      
      
      
      // we may need one hit from each TDC, 
      // so search the subevents for this
      const double nan = std::numeric_limits<double>::quiet_NaN();
      double tm_TDC=nan, tm_CTS=nan;
      
      double trigger = ev->GetTriggerTime();
      if(debug)
         cout << "trigger=" << trigger << endl;
      
      for(unsigned cnt=0;cnt<tdc->Size();cnt++) {
         const hadaq::TdcMessageExt& ext = tdc->msg(cnt);
         unsigned chid = ext.msg().getHitChannel();
         double tm = ext.GetGlobalTime();
         if(debug)
            cout << "TDC ch=" << chid << " time=" << tm << endl;  
         if(chid==1) {
            tm_TDC = tm;
            break;
         }
      }
      for(unsigned cnt=0;cnt<cts->Size();cnt++) {
         const hadaq::TdcMessageExt& ext = cts->msg(cnt);
         unsigned chid = ext.msg().getHitChannel();
         double tm = ext.GetGlobalTime();
         if(debug)
            cout << "CTS ch=" << chid << " time=" << tm << endl;  
         if(chid==1) {
            tm_CTS = tm;
            break;
         }
      }
      
      if(!(isfinite(tm_TDC) && isfinite(tm_CTS))) 
         return false;
      
      // convert back to ns from here
      tm_TDC  *= 1e9;
      tm_CTS  *= 1e9;
      
      double ADC_phase = tm_TDC-tm_CTS;
      FillH1(hAdcPhase, ADC_phase);
      
      
      //cout << adc->Size() << endl;
      
      for(unsigned cnt=0;cnt<adc->Size();cnt++) {
         const hadaq::AdcMessage& msg = adc->msg(cnt);
         unsigned chid = msg.getCh();
         SetSubPrefix("Ch", chid);
         hChannels_t::iterator it = hChannels.find(chid);
         hChannel_t hCh;
         if(it == hChannels.end()) {
            hCh.Timing = MakeH1("Timing","Timing to Trigger", 1000, -500, -100, "T' / ns");
            hCh.TDCHitVsTiming = MakeH2("TDCHitVsTiming", "TDCHit vs. Timing", 200, 14, 30, 300, -500, -100, "#delta_{2} / ns;T' / ns");
            hChannels[chid] = hCh;
         }
         else {
            hCh = it->second;
         }
         double fineTiming = 1e9*msg.fFineTiming;
         fineTiming += ADC_phase;
         
         // due to some unknown but fixed delay between 
         // the measurement of tm_TDC and tm_CTS, we need 
         // to correct for an ADC epoch counter "glitch"
         // the treshold does not seem to be constant...
         // so this bug should actually be fixed in hardware 
         if(tm_TDC>config.AdcEpochCounterFixThreshold) {
             fineTiming -= config.SamplingPeriod;
         }
         
         if(debug)
             cout << "ADC ch=" << chid
                  << " timing=" << fineTiming
                  << endl;
         
         FillH1(hCh.Timing, fineTiming);
         FillH2(hCh.TDCHitVsTiming, tm_TDC, fineTiming);
         
      }
            
      if(debug)
         cout << endl;
      return true;
   }
};


void second() 
{
   
   // search for filename specific config
   string inputFilename = TGo4Analysis::Instance()->GetInputFileName(); 
   map<string, my_config_t>::const_iterator it_config = my_config.find(inputFilename);
   if(it_config == my_config.end()) {
      cerr << "ERROR: Config for filename " << inputFilename << " not found, returning" << endl;
      return;
   }
   
   // create outputfile
   const string outputFilename = inputFilename+string(".root"); 
   base::ProcMgr::instance()->CreateStore(outputFilename.c_str());
   
   // setup the ADCProc
   new ADCProc("ADCProc", 0x0200, 0x8000, it_config->second);
}
