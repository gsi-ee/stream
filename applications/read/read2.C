// macro reads tree with data, stored by TDC processor

#include "TTree.h"
#include "TFile.h"

void read2() 
{
   TFile* f = TFile::Open("td.root");
   if (f==0) return;
   
   TTree* t = (TTree*) f->Get("T");
   if (t==0) return;
   
   std::vector<hadaq::MessageFloat> *vect = new std::vector<hadaq::MessageFloat>;
   
   t->SetBranchAddress("TDC_C004", &vect);
   
   Long64_t entry(0), total(0);
   
   while (t->GetEntry(entry++)>0) {
      
      printf("Entry %ld size %u\n", entry, vect->size());
      
      for (unsigned n=0;n<vect->size();n++) {
         hadaq::MessageFloat &msg = vect->at(n);

         int chid = msg.getCh();
         int edge = msg.getEdge();
         float tm = msg.stamp;

         printf("   ch:%3d tm %10.9f\n", chid, tm);
         total += 1;
      }
      if (entry>20) break;
   }
   
   printf("Read %ld entries %ld messages\n", entry, total);
}