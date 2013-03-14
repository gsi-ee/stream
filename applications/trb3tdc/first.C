void first()
{

   base::ProcMgr::instance()->SetRawAnalysis(true);

   hadaq::TdcMessage::SetFineLimits(20, 500);

   hadaq::TrbProcessor* trb3 = new hadaq::TrbProcessor(0);

   trb3->SetPrintRawData(false);

   // create processor for hits from TDC3, only 5 channels are enabled, edges_mask
   hadaq::TdcProcessor* tdc3 = new hadaq::TdcProcessor(trb3, 3, 5, 0x1);

   for (int n=1;n<5;n++)
      tdc3->SetRefChannel(n, n+1);
   tdc3->DisableCalibrationFor(0);

//   tdc3->LoadCalibration("test.cal");
//   tdc3->SetAutoCalibration(100000);
//   tdc3->SetWriteCalibration("test.cal");

   // method set window for all TDCs at the same time
   //trb3->SetTriggerWindow(-4e-7, -3e-7);
}
