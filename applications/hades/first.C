void first()
{
   // analysis will work as triggerred -
   // after each event all data should be processed and flushed
   base::ProcMgr::instance()->SetTriggeredAnalysis();

   hadaq::TdcMessage::SetFineLimits(31, 421);

   hadaq::HldProcessor* hld = new hadaq::HldProcessor();

   // Following levels of histograms filling are supported
   //  0 - none
   //  1 - only basic statistic from TRB
   //  2 - generic statistic over TDC channels
   //  3 - basic per-channel histograms with IDs
   //  4 - per-channel histograms with references
   // trb3->SetHistFilling(4);

   // Load calibrations for ALL TDCs
   /// trb3->LoadCalibrations("/data.local1/padiwa/new_");

   // calculate and write calibrations at the end of the run
   //trb3->SetWriteCalibrations("/data.local1/padiwa/new_");

   // enable automatic calibrations of the channels
   //trb3->SetAutoCalibrations(100000);


   hadaq::TrbProcessor* trb3 = new hadaq::TrbProcessor(0x8880, hld);
   trb3->SetHistFilling(4);
   trb3->CreateTDC(0x5000, 0x5001, 0x5002, 0x5003);
   trb3->SetCh0Enabled(true);
   //trb3->SetWriteCalibrations("calibr_");
   trb3->LoadCalibrations("calibr_");

   trb3 = new hadaq::TrbProcessor(0x8890, hld);
   trb3->SetHistFilling(4);
   trb3->CreateTDC(0x5010, 0x5011, 0x5012, 0x5013);
   trb3->SetCh0Enabled(true);
   //trb3->SetWriteCalibrations("calibr_");
   trb3->LoadCalibrations("calibr_");

   // indicate if raw data should be printed
   hld->SetPrintRawData(false);

   // method set window for all TRBs/TDCs
   // hld->SetTriggerWindow(-4e-7, -0.2e-7);

   // uncomment these line to enable store of all TDC data in the tree
   // hld->SetStoreEnabled(true);

   // create store - typically done in second.C, should be called only once
   // base::ProcMgr::instance()->CreateStore("file.root");

}
