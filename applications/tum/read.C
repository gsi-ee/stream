// macro reads tree with data, stored by TDC processor

#include "TTree.h"
#include "TFile.h"

void read() 
{
   TFile* f = TFile::Open("tree.root");
   if (f==0) return;
   
   TTree* t = (TTree*) f->Get("T");
   if (t==0) return;
   
   std::vector<hadaq::TdcMessageExt> *vect = new std::vector<hadaq::TdcMessageExt>;
   
   t->SetBranchAddress("TDC_1000", &vect);
   
   Long64_t entry(0), total(0);
   
   while (t->GetEntry(entry)>0) {
      entry++;
      
      double ch0tm(0);
      
      for (unsigned n=0;n<vect->size();n++) {
         hadaq::TdcMessageExt& ext = vect->at(n);

         int chid = ext.msg().getHitChannel();
         if (chid==0) { ch0tm = ext.GetGlobalTime(); continue; }

         double tm = ext.GetGlobalTime() + ch0tm;
         // printf("   ch:%3d tm %10.9f\n", chid, tm);
         total += 1;
      }
   }
   
   printf("Read %ld entries %ld messages\n", entry, total);
}