void second() {
   TGo4Log::Info("Create get4 test classes");

   // gSystem->Load("libGet4Test");

   TGet4TestProc* p = new TGet4TestProc("Get4Test");

   p->Add(0,0); // add all channels of get4 0

   p->Add(0,2); // add all channels of get4 2

   p->MakeHistos(); // make summary histograms
}
