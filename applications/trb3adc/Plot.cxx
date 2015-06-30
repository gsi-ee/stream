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
   TH1D* h1;
   TH2D* h2;
   name = "Status";
   h1 = new TH1D(AppendCh(name,ch).c_str(),
                 AppendCh(name,ch).c_str(),
                 16, 0, 16);
   h1->SetStats(false);
   h[name] = h1;
   
   name = "TimingCorr";
   h2 = new TH2D(AppendCh(name,ch).c_str(),
                 AppendCh(name,ch).c_str(),
                 100, -350, -250,
                 100, -400, -300);
   h2->GetXaxis()->SetTitle("TRB3_Timing");
   h2->GetYaxis()->SetTitle("Acqu_Timing");
   h[name] = h2;
   
   name = "IntegralCorr";
   h2 = new TH2D(AppendCh(name,ch).c_str(),
                 AppendCh(name,ch).c_str(),
                 300, 0, 4000,
                 300, 0, 2500);
   h2->GetXaxis()->SetTitle("TRB3_Integral");
   h2->GetYaxis()->SetTitle("Acqu_Integral");
   h[name] = h2;
   
   name = "Timewalk_Acqu";
   h2 = new TH2D(AppendCh(name,ch).c_str(),
                 AppendCh(name,ch).c_str(),
                 100, 0, 2500,
                 100, -400, -300);
   h2->GetXaxis()->SetTitle("Acqu_Integral");
   h2->GetYaxis()->SetTitle("Acqu_Timing");
   h[name] = h2;
   
   name = "Timewalk_TRB3";
   h2 = new TH2D(AppendCh(name,ch).c_str(),
                 AppendCh(name,ch).c_str(),
                 100, 0, 4000,
                 100, -350, -250);
   h2->GetXaxis()->SetTitle("TRB3_Integral");
   h2->GetYaxis()->SetTitle("TRB3_Timing");
   h[name] = h2;
   
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
     ChHists[ch]["IntegralCorr"]->Fill(Integral_TRB3, Integral_Acqu);
     ChHists[ch]["Timewalk_Acqu"]->Fill(Integral_Acqu, Timing_Acqu);
     ChHists[ch]["Timewalk_TRB3"]->Fill(Integral_TRB3, Timing_TRB3);
   }
   
   map<string, TCanvas*> c;
   for(size_t ch=0;ch<ChHists.size();ch++) {
      const auto& ch_map = ChHists[ch];
      for(auto it_ch = ch_map.begin(); it_ch != ch_map.end(); it_ch++) {
         const string& name =  it_ch->first;
         // create canvas
         TCanvas* cv = nullptr;
         const auto& it_canvas = c.find(name);
         if(it_canvas == c.end()) {
            cv = new TCanvas(name.c_str(),name.c_str());
            cv->Divide(3,4);
            c[name] = cv;
         }
         else {
            cv = it_canvas->second;
         }
         cv->cd(ch+1);
         it_ch->second->Draw("colz"); // draw the histogram into canvas pad
         // output canvas if last channel reached and batch mode is on
         if(!gROOT->IsBatch())
            continue;
         if(ch==ChHists.size()-1) {
            string output = "plots.pdf";
            if(it_ch == ch_map.begin())
               cv->Print(output.append("(").c_str());
            else if(it_ch == next(ch_map.end(), -1))
               cv->Print(output.append(")").c_str());
            else
               cv->Print(output.c_str());
         }
      }   
   }   
}
