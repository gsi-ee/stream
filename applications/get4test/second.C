{
   TGo4Log::Info("Create get4 test classes");

   // gSystem->Load("libGet4Test");

   TGet4TestProc* p = new TGet4TestProc("Get4Test");

   p->Add(0,4); // add all channels of get4 4

   p->Add(0,5); // add all channels of get4 5

   p->MakeHistos(); // make summary histograms
}
