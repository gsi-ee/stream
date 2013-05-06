void first()
{

   base::ProcMgr::instance()->SetRawAnalysis(true);

   hadaq::TdcMessage::SetFineLimits(20, 500);

   hadaq::TrbProcessor* trb3 = new hadaq::TrbProcessor(0);

   // CTS subevent header id, all 16 bit
//   trb3->SetHadaqCTSId(0x8000);

   // TDC subevent header id, here high 8 bit from 16 should be specified
   // lower 8 bit are used as tdc number
   trb3->SetHadaqTDCId(0x800);

   trb3->SetPrintRawData(true);

   // create processor for hits from TDC
   // par1 - pointer on trb3 board
   // par2 - TDC number (lower 8 bit from subevent header
   // par3 - number of channels in TDC
   // par4 - edges mask 0x1 - rising, 0x2 - trailing, 0x3 - both edges
   hadaq::TdcProcessor* tdc3 = new hadaq::TdcProcessor(trb3, 1, 65, 0x1);

   // specify reference channel for any other channel -
   // will appear as additional histogram with time difference between channels
   for (int n=1;n<4;n++)
      tdc3->SetRefChannel(n, n+1);


   // for old FPGA code one should have epoch for each hit, no longer necessary
//   tdc3->SetEveryEpoch(true);


   // next parameters are about time calibration - there are two possibilities
   // 1) automatic calibration after N hits in every enabled channel
   // 2) generate calibration on base of provided data and than use it later statically for analysis


   // disable calibration for channel #0
   tdc3->DisableCalibrationFor(0);

    // load static calibration at the beginning of the run
//   tdc3->LoadCalibration("test3.cal");

    // calculate and write static calibration at the end of the run
//   tdc3->SetWriteCalibration("test3.cal");

   // enable automatic calibration, specify required number of hits in each channel
//   tdc3->SetAutoCalibration(100000);


   // this will be required only when second analysis step will be introduced
   // and one wants to select only hits around some trigger signal for that analysis

   // method set window for all TDCs at the same time
   //trb3->SetTriggerWindow(-4e-7, -3e-7);
}
