void first()
{
   base::ProcMgr::instance()->SetRawAnalysis(true);

   // default channel numbers and edges mask
   // 1 - use only rising edge, falling edge is ignore
   // 2   - falling edge enabled and fully independent from rising edge
   // 3   - falling edge enabled and uses calibration from rising edge
   // 4   - falling edge enabled and common statistic is used for calibration
   hadaq::TrbProcessor::SetDefaults(33, 0x1);

   hadaq::HldProcessor* hld = new hadaq::HldProcessor();

   hadaq::TrbProcessor* trb3_1 = new hadaq::TrbProcessor(0x8000, hld);
   trb3_1->CreateTDC(0x1000, 0x1001, 0x1002, 0x1003);
   trb3_1->SetWriteCalibrations("run1");
}



