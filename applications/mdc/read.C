// macro reads tree with data, stored by TDC processor

void read()
{
   auto f = TFile::Open("mdc.root");
   if (!f) return;

   auto t = (TTree*) f->Get("T");
   if (!t) return;

   auto vect = new std::vector<hadaq::MdcMessage>;

   t->SetBranchAddress("MDC_10a0", &vect);

   long entry = 0, total = 0;

   while (t->GetEntry(entry) > 0) {
      entry++;

      for (unsigned n=0;n<vect->size();n++) {
         auto &msg = vect->at(n);

         int ch = msg.getCh();
         float stamp = msg.getStamp();
         float tot = msg.getToT();
         printf("   ch:%3d   tm %8.2f  tot %8.2f\n", ch, stamp, tot);
         total += 1;
      }
   }

   printf("Read %ld entries %ld messages\n", entry, total);
}
