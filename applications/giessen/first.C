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



   hadaq::TrbProcessor* trb3 = new hadaq::TrbProcessor(0x8000, hld);
   trb3->SetHistFilling(4);
   trb3->CreateTDC(0x0c00, 0x0c01, 0x0c03);
   // trb3->LoadCalibrations("/data.local1/padiwa/new_");

   trb3 = new hadaq::TrbProcessor(0x8002, hld);
   trb3->SetHistFilling(4);
   trb3->CreateTDC(0x0c10, 0x0c11, 0x0c12, 0x0c13);
   // trb3->LoadCalibrations("/data.local1/padiwa/new_");

   trb3 = new hadaq::TrbProcessor(0x8003, hld);
   trb3->SetHistFilling(4);
   trb3->CreateTDC(0x0c20, 0x0c21, 0x0c22, 0x0c23);
   // trb3->LoadCalibrations("/data.local1/padiwa/new_");

   trb3 = new hadaq::TrbProcessor(0x8004, hld);
   trb3->SetHistFilling(4);
   trb3->CreateTDC(0x0c30, 0x0c31, 0x0c32, 0x0c33 ) ;
   // trb3->LoadCalibrations("/data.local1/padiwa/new_");

   // indicate if raw data should be printed
   hld->SetPrintRawData(false);

   // method set window for all TRBs/TDCs
   // hld->SetTriggerWindow(-4e-7, -0.2e-7);
}
