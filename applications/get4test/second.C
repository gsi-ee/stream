void second() {
   TGo4Log::Info("Create get4 test classes");

   // gSystem->Load("libGet4Test");

   TGet4TestProc* p = new TGet4TestProc("Get4Test");

   for (int get4=0;get4<5;get4++)
     p->Add(0,get4); // add all channels of get4 0..5 on ROC0

   p->MakeHistos(); // make summary histograms
}
