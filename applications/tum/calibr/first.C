void first()
{
   base::ProcMgr::instance()->SetRawAnalysis(true);

   // default channel numbers and edges mask
   hadaq::TrbProcessor::SetDefaults(33, 0x1);

   hadaq::HldProcessor* hld = new hadaq::HldProcessor();

   hadaq::TrbProcessor* trb3_1 = new hadaq::TrbProcessor(0x8000, hld);
   trb3_1->CreateTDC(0x1000, 0x1001, 0x1002, 0x1003);
   trb3_1->SetWriteCalibrations("run1");
}



