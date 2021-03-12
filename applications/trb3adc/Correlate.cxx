// source streamlogin and have proper rootlogon.C
// to make Go4 stuff work
// also enable -std=c++11 for ACLIC

#include <iostream>
#include <sstream>
#include <iomanip>
#include <TFile.h>
#include <TTree.h>
#include <vector>
#include <limits>

#include <hadaq/AdcSubEvent.h>
#include <hadaq/TdcSubEvent.h>
#include <hadaq/TrbProcessor.h>

#include <my_config.h>

using namespace std;

vector<unsigned short> getAcquADCValues(
      unsigned short id,
      vector< unsigned short >* acqu_ID,
      vector< vector<unsigned short> >* acqu_Values) {
   if(acqu_ID->size() != acqu_Values->size()) {
      cerr << "Something wrong in the acqu tree, sizes don't match..." << endl;
      exit(1);
   }
   for(size_t i=0;i<acqu_ID->size();i++) {
      if(id == (*acqu_ID)[i]) {
         return (*acqu_Values)[i];
      }
   }
   // return empty if not found
   return vector<unsigned short>{};
}

struct ChannelMap_t {
   unsigned TRB3;
   unsigned ADC;
   unsigned TDC;
};

void Correlate(const char* fname = "scratch/CBTaggTAPS_9227.dat", const bool debug = false) {

#if defined(__CINT__) && !defined(__MAKECINT__)
   cout << "Needs to be compiled with ACLIC, append + to filename" << endl;
   return;
#endif

   cout << "Working on file basename " << fname << endl;

   // get some config
   map<string, my_config_t>::const_iterator it_config = my_config.find(fname+string(".hld"));
   if(it_config == my_config.end()) {
      cerr << "ERROR: Config for filename " << fname << ".hld not found" << endl;
      return;
   }
   const my_config_t& config = it_config->second;

   // build the channel map
   // the order in this vector defines the 12 logical channels
   // of this analysis, from 0-11
   const vector<ChannelMap_t> ChannelMap{
      // TRB3, ADC, TDC
      { 8, 3379, 2404},
      { 9, 3378, 2405},
      {10, 3377, 2406},
      {11, 3376, 2407},

      {24, 3371, 2412},
      {25, 3370, 2413},
      {26, 3369, 2414},
      {27, 3368, 2415},

      {28, 3375, 2408},
      {29, 3374, 2409},
      {30, 3373, 2410},
      {31, 3372, 2411},
   };

   // make some helpful vectors/maps out of it
   map<unsigned, unsigned> TRB3_IDs_map;
   vector<unsigned> ADC_IDs;
   map<unsigned, unsigned> ADC_IDs_map;
   vector<unsigned> TDC_IDs;
   map<unsigned, unsigned> TDC_IDs_map;

   for(const auto& ch : ChannelMap) {
      TRB3_IDs_map[ch.TRB3] = ADC_IDs.size(); // misuse size as counter
      ADC_IDs_map[ch.ADC] = ADC_IDs.size();
      TDC_IDs_map[ch.TDC] = ADC_IDs.size();

      // some extra lists for scanning
      ADC_IDs.emplace_back(ch.ADC);
      TDC_IDs.emplace_back(ch.TDC);
   }
   // important to sort them
   sort(ADC_IDs.begin(), ADC_IDs.end());
   sort(TDC_IDs.begin(), TDC_IDs.end());


   stringstream fname1;
   fname1 << fname << ".root";

   stringstream fname2;
   fname2 << fname << ".hld.root";


   TFile* f1 = new TFile(fname1.str().c_str());
   TTree* t1 = (TTree*)f1->Get("rawADC");
   if(t1==nullptr) {
      cerr << "Acqu data tree not found" << endl;
      exit(1);
   }

   t1->SetMakeClass(1); // very important for reading STL stuff

   TFile* f2 = new TFile(fname2.str().c_str());
   TTree* t2 = (TTree*)f2->Get("T");
   if(t2==nullptr) {
      cerr << "Go4 data tree not found" << endl;
      exit(1);
   }
   // t2->SetMakeClass(1); // Go4 stuff already has compiled classes

   Long64_t n1 = t1->GetEntries();
   Long64_t n2 = t2->GetEntries();
   // t1 contains the events from Acqu
   // t2 contains the events from Go4
   cout << "Entries: Acqu=" << n1 << " Go4=" << n2 << endl;

   vector< unsigned short >* acqu_ID = 0;             // ADC ID
   vector< vector<unsigned short> >* acqu_Values = 0; // Value
   t1->SetBranchAddress("ID",    &acqu_ID);
   t1->SetBranchAddress("Value", &acqu_Values);

   hadaq::TrbMessage* go4_trb = 0;
   t2->SetBranchAddress("TRB_8000", &go4_trb);

   vector<hadaq::TdcMessageExt>* go4_cts = 0;
   t2->SetBranchAddress("TDC_8000", &go4_cts);

   vector< hadaq::AdcMessage >* go4_adc = 0;
   t2->SetBranchAddress("ADC_0200", &go4_adc);

   vector<hadaq::TdcMessageExt>* go4_tdc = 0;
   t2->SetBranchAddress("TDC_0200", &go4_tdc);

   // setup output
   stringstream fname3;
   fname3 << fname << ".merged.root";
   TFile* f = new TFile(fname3.str().c_str(),"RECREATE");
   TTree* t = new TTree("T", "T");
   unsigned out_Channel = 0;
   unsigned out_Status  = 0;
   double out_Timing_Acqu = 0;
   double out_Integral_Acqu = 0;
   double out_Timing_TRB3 = 0;
   double out_Integral_TRB3 = 0;
   t->Branch("Channel", &out_Channel);
   t->Branch("Status", &out_Status);
   t->Branch("Timing_Acqu", &out_Timing_Acqu);
   t->Branch("Timing_TRB3", &out_Timing_TRB3);
   t->Branch("Integral_Acqu", &out_Integral_Acqu);
   t->Branch("Integral_TRB3", &out_Integral_TRB3);



   Long64_t i1 = 0;
   Long64_t i2 = 0;
   unsigned go4_EventId_last = 0;
   unsigned acqu_EventId_last = 0;
   unsigned go4_overflows = 0;
   unsigned acqu_overflows = 0;

   Long64_t matches = 0;
   while(i1 < n1 && i2 < n2) {
      t1->GetEntry(i1);
      t2->GetEntry(i2);
      // match the the serial ID

      // go4 is easily available
      if(!go4_trb->fTrigSyncIdFound) {
         cerr << "We should always have found some serial ID..." << endl;
         exit(1);
      }

      if(go4_trb->fTrigSyncIdStatus != 0xa) {
         cerr << "Found invalid TRB3 EventId status (should be 0xa): 0x"
              << hex << go4_trb->fTrigSyncIdStatus << dec << endl;
         // probably a spurious trigger without an ID, just skip it
         i2++;
         continue;
      }


      const unsigned go4_EventId = go4_trb->fTrigSyncId;
      // acqu needs searching for ADC 400 ID...
      auto EventId = getAcquADCValues(400, acqu_ID, acqu_Values);
      const unsigned acqu_EventId = EventId[0]; // convert 16bit to 32bit...

      if(acqu_EventId<acqu_EventId_last) {
         if(debug)
            cout << "Overflow Acqu: " << acqu_EventId << " < " << acqu_EventId_last << endl;
         acqu_overflows++;
      }

      if(go4_EventId<go4_EventId_last) {
         if(debug)
            cout << "Overflow Go4: " << go4_EventId << " < " << go4_EventId_last << endl;
         go4_overflows++;
      }

      go4_EventId_last = go4_EventId;
      acqu_EventId_last = acqu_EventId;

      if(abs(go4_overflows-acqu_overflows)>2) {
         cerr << "Missed one overflow, can't continue." << endl;
         exit(1);
      }

      const unsigned go4_EventId_full = go4_EventId + (go4_overflows << 16);
      const unsigned acqu_EventId_full = acqu_EventId + (acqu_overflows << 16);


      if(go4_EventId_full>acqu_EventId_full) {
         i1++;
         continue;
      }
      else if(go4_EventId_full<acqu_EventId_full) {
         i2++;
         continue;
      }

      // here the EventIDs seem to match (except for overflows)
      matches++;

      if(debug) {
         cout << "Found match=" << matches <<", i1=" << i1 << " i2=" << i2 << endl;
         // just dump events
         cout << ">>> TRB3 says:" << endl;
         for(size_t i=0;i<go4_adc->size();i++) {
            const hadaq::AdcMessage& msg = (*go4_adc)[i];
            cout << "    Ch=" << setw(4) << msg.getCh() << " Integral=" << msg.fIntegral << endl;
         }
         cout << ">>> Acqu says: " << endl;
         for(unsigned short ch=3368;ch<3383;ch++) {
            auto values = getAcquADCValues(ch, acqu_ID, acqu_Values);
            if(values.size() != 3)
               continue;
            cout << "    Ch=" << setw(4) << ch << " Integral=" << values[1] << endl;
         }

      }

      // extract the "interesting" ADC/TDC values
      size_t j_TDC = 0;
      size_t j_ADC = 0;
      size_t j_ID = 0;
      vector<double> TDC_Acqu(ChannelMap.size(), std::numeric_limits<double>::quiet_NaN());
      vector<double> ADC_Acqu(ChannelMap.size(), std::numeric_limits<double>::quiet_NaN());
      while(j_ID<acqu_ID->size()) {
         const auto id = (*acqu_ID)[j_ID];

         //cout << "j_ID="<<j_ID << " j_ADC="<<j_ADC<<endl;

         if(j_TDC<TDC_Acqu.size()) {
            const auto id_TDC = TDC_IDs[j_TDC];
            if(id<id_TDC) {
               j_ID++;
               continue;
            }
            if(id==id_TDC) {
               //cout << "Found TDC match " << id << endl;
               TDC_Acqu[TDC_IDs_map[id]] = 0.117*(short)((*acqu_Values)[j_ID][0]);
               j_TDC++;
               continue;
            }
            if(id>id_TDC) {
               j_TDC++;
               continue;
            }
         }
         else if(j_ADC<ADC_Acqu.size()) {
            const auto id_ADC = ADC_IDs[j_ADC];
            if(id<id_ADC) {
               j_ID++;
               continue;
            }
            if(id==id_ADC) {
               //cout << "Found ADC match " << id << " j_ID=" << j_ID << endl;
               ADC_Acqu[ADC_IDs_map[id]] = (*acqu_Values)[j_ID][1]; // second entry is signal
               j_ADC++;
               continue;
            }
            if(id>id_ADC) {
               j_ADC++;
               continue;
            }
         }
         else {
            break;
         }
      }


      vector<double> TDC_TRB3(ChannelMap.size(), std::numeric_limits<double>::quiet_NaN());
      vector<double> ADC_TRB3(ChannelMap.size(), std::numeric_limits<double>::quiet_NaN());


      if(go4_tdc->size() != 1 || go4_cts->size() != 1) {
         cerr << "Found hit without proper TDC information" << endl;
         exit(1);
      }
      const double tm_CTS = 1e9*(*go4_cts)[0].GetGlobalTime();
      const double tm_TDC = 1e9*(*go4_tdc)[0].GetGlobalTime();
      double ADC_phase = tm_TDC-tm_CTS;

      if(!isfinite(tm_TDC))
         cerr << "TDC not finite" << endl;
      if(!isfinite(tm_CTS))
         cerr << "CTS not finite" << endl;




      for(size_t i=0;i<go4_adc->size();i++) {
         const hadaq::AdcMessage& msg = (*go4_adc)[i];
         const auto id = msg.getCh();
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
         if(debug && abs(fineTiming)>1e8)
            cerr << "Very large fine timing found: "
                 << fineTiming
                 << " Corrected: " << (fineTiming - (1<<24)*12.5)
                 << endl;
         TDC_TRB3[TRB3_IDs_map[id]] = fineTiming;
         ADC_TRB3[TRB3_IDs_map[id]] = msg.fIntegral;
      }


      // write it finally to Tree

      if(debug)
         cout << ">>> Matched Hits:" << endl;


      for(size_t i=0;i<ADC_Acqu.size();i++) {
         const unsigned test =
               1*isfinite(ADC_Acqu[i]) +
               2*isfinite(TDC_Acqu[i]) +
               4*isfinite(ADC_TRB3[i]) +
               8*isfinite(TDC_TRB3[i]);
         if(test==0)
            continue;

         out_Channel = i;
         out_Status = test;
         out_Integral_Acqu = ADC_Acqu[i];
         out_Timing_Acqu = TDC_Acqu[i];
         out_Integral_TRB3 = ADC_TRB3[i];
         out_Timing_TRB3 = TDC_TRB3[i];

         t->Fill();

         if(debug && test==15)
            cout << "Hit Ch=" << i
                 << " TDC_Acqu=" << TDC_Acqu[i]
                 << " TDC_TRB3=" << TDC_TRB3[i]
                 << " ADC_Acqu=" << ADC_Acqu[i]
                 << " ADC_TRB3=" << ADC_TRB3[i]
                 << endl;
      }

      if(debug)
         cout << endl;
      //        if(matches>10)
      //            break;

      if(matches % 1000 == 0)
         cout << "Matched " << matches
              << " i1=" << i1
              << " i2=" << i2
              << " id1=" << acqu_EventId_full
              << " id2=" << go4_EventId_full
              << endl;
      i1++;
      i2++;
   }

   t->Write();
   f->Close();

   cout << "Wrote " << matches << " to output." << endl;

   delete t;
   delete f;

   delete t1;
   delete f1;

   delete t2;
   delete f2;
}
