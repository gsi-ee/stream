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
#include <TEventList.h>
#include <TEntryList.h>

using namespace std;

typedef map<string, TH1*> ChHist_t;


string PrependCh(const string& s, size_t ch) {
   stringstream ss;
   ss << "Ch" << ch << "_" << s;
   return ss.str();
}

struct axis_t {
   Int_t Bins;
   Double_t Low;
   Double_t High;
   string Label;
};

TH2D* MakeH2(const string& titlename, 
             const axis_t& x_axis,
             const axis_t& y_axis) {
   TH2D* h;
   h = new TH2D(titlename.c_str(),
                titlename.c_str(),
                x_axis.Bins, x_axis.Low, x_axis.High,
                y_axis.Bins, y_axis.Low, y_axis.High
                );
   h->GetXaxis()->SetTitle(x_axis.Label.c_str());
   h->GetYaxis()->SetTitle(y_axis.Label.c_str());
   return h;
}
       
TH1D* MakeH1(const string& titlename, 
             const axis_t& x_axis,
             const string& y_axislabel = "") {
   TH1D* h;
   h = new TH1D(titlename.c_str(),
                titlename.c_str(),
                x_axis.Bins, x_axis.Low, x_axis.High);
   h->GetXaxis()->SetTitle(x_axis.Label.c_str());
   h->GetYaxis()->SetTitle(y_axislabel.c_str());
   return h;
}


ChHist_t MakeChHist(size_t ch) {
   ChHist_t h;
   

   const axis_t timing_acqu{200, -400, -300, "Timing_Acqu / ns"};   
   const axis_t timing_trb3{200, -350, -250, "Timing_TRB3 / ns"};
   const axis_t integral_acqu{300, 0, 2500, "Integral_Acqu"};   
   const axis_t integral_trb3{300, 0, 4000, "Integral_TRB3"};
   
   
   h["Status"] = MakeH1(PrependCh("Status", ch), {16, 0, 16, ""});
   h["Status"]->SetStats(false);
   
   h["Timing_Acqu"] = MakeH1(PrependCh("Timing_Acqu", ch), timing_acqu);
   h["Timing_TRB3"] = MakeH1(PrependCh("Timing_TRB3", ch), timing_trb3);
   
   h["Integral_Acqu"] = MakeH1(PrependCh("Integral_Acqu", ch), integral_acqu);
   h["Integral_TRB3"] = MakeH1(PrependCh("Integral_TRB3", ch), integral_trb3);   
   
   h["TimingCorr"] = MakeH2(PrependCh("TimingCorr",ch), 
                            timing_trb3,   timing_acqu);   
   h["IntegralCorr"] = MakeH2(PrependCh("IntegralCorr",ch), 
                              integral_trb3, integral_acqu);
   
   h["Timewalk_Acqu"] = MakeH2(PrependCh("Timewalk_Acqu",ch), 
                               integral_acqu, timing_acqu);
   h["Timewalk_TRB3"] = MakeH2(PrependCh("Timewalk_TRB3",ch), 
                               integral_trb3, timing_trb3);
   
   return h;
}

void Plot(const char* cut = "", 
          const char* fname = "scratch/CBTaggTAPS_9227.dat") {
   stringstream fname_;
   fname_ << fname << ".merged.root";
   TFile* f = new TFile(fname_.str().c_str());
   TTree* t = (TTree*)f->Get("T");
   
   if(t==nullptr) {
      cerr << "Tree T not found in file " << fname_.str() << endl;
      return;
   }
   
   // make some eventlist from the given cut
   t->Draw(">>eventlist",cut);
   TEventList* eventlist = (TEventList*)gDirectory->Get("eventlist");
   cout << "Cut reduced events from " << t->GetEntries() << " to " << eventlist->GetN() << endl;
   t->SetEventList(eventlist);
   TEntryList* entrylist = t->GetEntryList(); 
   
   
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
   
   
   for(Long64_t i=0;i<entrylist->GetN();i++) {
     t->GetEntry(entrylist->Next());
     const unsigned& ch = Channel;
     ChHists[ch]["Status"]->Fill(Status2String.at(Status).c_str(), 1);
     ChHists[ch]["Timing_Acqu"]->Fill(Timing_Acqu);
     ChHists[ch]["Timing_TRB3"]->Fill(Timing_TRB3);    
     ChHists[ch]["Integral_Acqu"]->Fill(Integral_Acqu);
     ChHists[ch]["Integral_TRB3"]->Fill(Integral_TRB3);    
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
         TH1* h = it_ch->second;
         // append cut expression to title
         string s_cut = cut;
         if(s_cut != "") {
            string title = h->GetTitle();
            title.append(" {");
            title.append(s_cut);
            title.append("}");
            h->SetTitle(title.c_str());
         }
         h->Draw("colz"); // draw the histogram into canvas pad
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
