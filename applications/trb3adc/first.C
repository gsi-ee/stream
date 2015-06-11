void first()
{
   // analysis will work as triggerred -
   // after each event all data should be processed and flushed
   // base::ProcMgr::instance()->SetTriggeredAnalysis();
   base::ProcMgr::instance()->SetRawAnalysis(true);

   hadaq::HldProcessor* hld = new hadaq::HldProcessor();

   hadaq::TrbProcessor* trb3 = new hadaq::TrbProcessor(0x8000, hld);
   trb3->SetHistFilling(4);
   //trb3->SetPrintRawData();
   
   hadaq::AdcProcessor* adc = new hadaq::AdcProcessor(trb3, 0x0200);
   adc->SetDiffChannel(12, 13);
   adc->SetDiffChannel(13, 12);


   // uncomment these line to enable store of all ADC data in the tree
   //hld->SetStoreEnabled(true);

   // create store - typically done in second.C, should be called only once
   // base::ProcMgr::instance()->CreateStore("file.root");
}
