
#include "base/EventProc.h"
#include "base/Event.h"
#include "hadaq/TdcSubEvent.h"
#include "hadaq/AdcSubEvent.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>

#include "TTree.h"


using namespace std;

class ADCProc : public base::EventProc {
   protected:
      
      string fTdcId;   
      string fAdcId;   
      string fTdcInCTSId;
      

      //double      fHits[16]; //!< 16 channel, last hit in every channel
      
      base::H1handle  hDiffTime; 
      base::H1handle  hAdcPhase;      
      base::H1handle  hTimeCh1; 
      base::H1handle  hTimeCh2; 
      base::H2handle  hPhaseVsCh1; 
      base::H2handle  hPhaseVsCh2; 
      base::H2handle  hTDCHitVsCh2; 
      
      const static double samplingPeriod = 12.5;
            
   public:
      ADCProc(const char* procname, unsigned id, unsigned ctsid) :
         base::EventProc(procname)
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

         hDiffTime = MakeH1("DiffTime","Timing of Ch1-Ch2", 10000, -500, 500, "t / ns");
         hAdcPhase = MakeH1("AdcPhase","Phase of external trigger to ADC clock", 1000, 0, 100, "t / ns");
         hTimeCh1 = MakeH1("TimeCh1","Timing to trigger Ch1", 10000, 0, 1000, "t / ns");
         hTimeCh2 = MakeH1("TimeCh2","Timing to trigger Ch2", 10000, 0, 1000, "t / ns");
         
         hPhaseVsCh1 = MakeH2("PhaseVsCh1","Phase vs. Ch1", 100, 40, 70, 1000, 200, 300, "phase;ch1");
         hPhaseVsCh2 = MakeH2("PhaseVsCh2","Phase vs. Ch2", 100, 40, 70, 1000, 450, 550, "phase;ch2");
         hTDCHitVsCh2 = MakeH2("TDCHitVsCh2","TDCHit vs. Ch2", 500, 0, 50, 1000, 490, 590, "phase;ch2");
         
         // enable storing already in constructor
         //SetStoreEnabled();
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
         
         // we need one hit from each TDC, 
         // and timings from two ADC channels
         // so search the subevents for this
         const double nan = std::numeric_limits<double>::quiet_NaN();
         double tm_TDC=nan, tm_CTS=nan, tm_ADC1=nan, tm_ADC2=nan;
         
         double trigger = ev->GetTriggerTime();
         if(debug)
            cout << "trigger=" << trigger << endl;
         
         for(unsigned cnt=0;cnt<adc->Size();cnt++) {
            const hadaq::AdcMessage& msg = adc->msg(cnt);
            
            unsigned chid = msg.getCh();
            double finetime = msg.fFineTiming; 
            double integral = msg.fIntegral;             
            if(debug)
               cout << "ADC ch=" << chid 
                    << " finetime=" << finetime
                    << " integral=" << integral << endl;  
            if(!isfinite(tm_ADC1)) {
               tm_ADC1 = finetime;
            }
            else if(!isfinite(tm_ADC2)) {
               tm_ADC2 = finetime;
            }
            else {
               break;
            }
         }
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
         
         if(!(isfinite(tm_TDC) && isfinite(tm_CTS) 
              && isfinite(tm_ADC1) && isfinite(tm_ADC2))) 
            return false;
         
         // convert back to ns from here
         tm_TDC  *= 1e9;
         tm_CTS  *= 1e9;
         tm_ADC1 *= 1e9;
         tm_ADC2 *= 1e9;
         
         double ADC_phase = tm_TDC-tm_CTS;
         
         // due to some unknown but fixed delay between 
         // the measurement of tm_TDC and tm_CTS, we need 
         // to correct for an ADC epoch counter "glitch" 
         // TODO: check if this threshold of 22.7ns is constant 
         // over TRB3 reboots (due to different PLL locking)
         if(tm_TDC>22.7) {
            tm_ADC1 -= samplingPeriod;
            tm_ADC2 -= samplingPeriod;
         }
         
         if(debug) {
            cout << "ADC phase from TDC: " << ADC_phase << endl;
            cout << "ADC1 timing: " << tm_ADC1 << endl;
            cout << "ADC2 timing: " << tm_ADC2 << endl;         
         }
         
         // fill some histograms
         FillH1(hAdcPhase, ADC_phase);         
         FillH1(hDiffTime, tm_ADC1-tm_ADC2);
         FillH1(hTimeCh1, tm_ADC1+ADC_phase);
         FillH1(hTimeCh2, tm_ADC2+ADC_phase);
         FillH2(hPhaseVsCh1, ADC_phase, tm_ADC1);
         FillH2(hPhaseVsCh2, ADC_phase, tm_ADC2);
         FillH2(hTDCHitVsCh2, 1e9*tm_TDC, tm_ADC2+ADC_phase);
                 
         if(debug)
            cout << endl;
         return true;
      }
};


void second() 
{
   // uncomment line to create tree for the events storage
   //base::ProcMgr::instance()->CreateStore("file.root");
   
   new ADCProc("ADCProc", 0x0200, 0x8000);
}
