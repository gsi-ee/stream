void first()
{

   base::ProcMgr::instance()->SetRawAnalysis(true);

   hadaq::TdcMessage::SetFineLimits(20, 500);

   hadaq::TrbProcessor* trb3 = new hadaq::TrbProcessor(0);

   trb3->SetPrintRawData(false);

   hadaq::TdcProcessor* tdc3 = new hadaq::TdcProcessor(trb3, 3, true);

   for (int n=1;n<127;n++)
      tdc3->SetRefChannel(n, n+1);

   // method set window for all TDCs at the same time
   //trb3->SetTriggerWindow(-4e-7, -3e-7);
}
