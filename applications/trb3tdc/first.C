void first()
{

   base::ProcMgr::instance()->SetRawAnalysis(true);

   hadaq::TdcMessage::SetFineLimits(20, 500);

   hadaq::TrbProcessor* trb3 = new hadaq::TrbProcessor(0);

   trb3->SetPrintRawData(true);

   hadaq::TdcProcessor* tdc3 = new hadaq::TdcProcessor(trb3, 3);

   // method set window for all TDCs at the same time
   //trb3->SetTriggerWindow(-4e-7, -3e-7);
}
