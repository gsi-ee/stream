#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include <TH2D.h>
#include <TH1D.h>
#include <TCanvas.h>

#include <vector>
#include <map>
#include <iostream>
#include <sstream>

#include <TROOT.h>

using namespace std;

typedef map<string, TH1*> ChHist_t;


string AppendCh(const string& s, size_t ch) {
   stringstream ss;
   ss << "Ch" << ch << "_" << s;
   return ss.str();
}

ChHist_t MakeChHist(size_t ch) {
   ChHist_t h;
   string name;
   name = "Status";
   h[name] = new TH1D(AppendCh(name,ch).c_str(),
                      AppendCh(name,ch).c_str(),
                      16, 0, 16);
   name = "TimingCorr";
   h[name] = new TH2D(AppendCh(name,ch).c_str(),
                      AppendCh(name,ch).c_str(),
                      500, -500, -200,
                      500, -500, -200);
   return h;
}

void Plot(const char* fname = "scratch/CBTaggTAPS_9227.dat") {
   stringstream fname_;
   fname_ << fname << ".merged.root";
   TFile* f = new TFile(fname_.str().c_str());
   TTree* t = (TTree*)f->Get("T");
   
   if(t==nullptr) {
      cerr << "Tree T not found in file " << fname_.str() << endl;
      return;
   }
   
   const map<unsigned, string> Status2String = {
      {0, "Empty"},
      {1, "ADC_Acqu"},
      {2, "TDC_Acqu"},
      {3, "No TRB3"},
      {4, "ADC_TRB3"},
      {5, "ADC"},
      {6, "TDC_Acqu ADC_TRB3"},
      {7, "No TDC_TRB3"},
      {8, "TDC_TRB3"},
      {9, "TDC_TRB3 ADC_Acqu"},
      {10, "TDC"},
      {11, "No ADC_TRB3"},
      {12, "No Acqu"},
      {13, "No TDC_Acqu"},
      {14, "No ADC_Acqu"},
      {15, "Complete"},
   };
   
   // setup branches
   unsigned Channel;
   unsigned Status;
   double Timing_TRB3;
   double Timing_Acqu;
   double Integral_TRB3;
   double Integral_Acqu;
   t->SetBranchAddress("Channel", &Channel);
   t->SetBranchAddress("Status", &Status);
   t->SetBranchAddress("Timing_TRB3", &Timing_TRB3);
   t->SetBranchAddress("Timing_Acqu", &Timing_Acqu);
   t->SetBranchAddress("Integral_TRB3", &Integral_TRB3);
   t->SetBranchAddress("Integral_Acqu", &Integral_Acqu);
   
   // prepare channel-wise histograms
   gROOT->cd();
   vector<ChHist_t> ChHists(12);
   for(size_t ch=0;ch<ChHists.size();ch++) {
      ChHists[ch] = MakeChHist(ch);
      // pre fill bins
      for(const auto& it_map : Status2String)
         ChHists[ch]["Status"]->Fill(it_map.second.c_str(), 0);
   }
   
   
   for(Long64_t i=0;i<t->GetEntries();i++) {
     t->GetEntry(i);
     const unsigned& ch = Channel;
     ChHists[ch]["Status"]->Fill(Status2String.at(Status).c_str(), 1);
     ChHists[ch]["TimingCorr"]->Fill(Timing_TRB3, Timing_Acqu);
     
   }
   
   map<string, TCanvas*> c;
   for(size_t ch=0;ch<ChHists.size();ch++) {
      for(auto& it_ch : ChHists[ch]) {
         const string& name =  it_ch.first;
         // create canvas
         auto it_canvas = c.find(name);
         if(it_canvas == c.end()) {
            c[name] = new TCanvas(name.c_str());
            c[name]->Divide(3,4);
         }
         c[name]->cd(ch+1);
         (it_ch.second)->Draw("colz");
      }   
   }   
}
