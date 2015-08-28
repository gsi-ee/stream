
void first()
{

   // base::ProcMgr::instance()->SetRawAnalysis(true);

   base::ProcMgr::instance()->SetTriggeredAnalysis();

   // this limits used for liner calibrations when nothing else is available
   hadaq::TdcMessage::SetFineLimits(31, 511);

   // default channel numbers and edges mask
   hadaq::TrbProcessor::SetDefaults(33, 0x1);

   hadaq::HldProcessor* hld = new hadaq::HldProcessor();

   // About time calibration - there are two possibilities
   // 1) automatic calibration after N hits in every enabled channel.
   //     Just use SetAutoCalibrations method for this
   // 2) generate calibration on base of provided data and than use it later statically for analysis
   //     Than one makes special run with SetWriteCalibrations() enabled.
   //     Later one reuse such calibrations enabling only LoadCalibrations() call

   hadaq::TrbProcessor* trb3_1 = new hadaq::TrbProcessor(0x8000, hld);
   trb3_1->SetHistFilling(4);
   trb3_1->SetCrossProcess(true);
   trb3_1->CreateTDC(0x1000, 0x1001, 0x1002, 0x1003);
   // enable automatic calibration, specify required number of hits in each channel
   // trb3_1->SetAutoCalibrations(50000);
   // calculate and write static calibration at the end of the run
   // trb3_1->SetWriteCalibrations("run1");
   // load static calibration at the beginning of the run
   trb3_1->LoadCalibrations("run1");

   //hadaq::TrbProcessor* trb3_2 = new hadaq::TrbProcessor(0x8001, hld);
   //trb3_2->SetHistFilling(4);
   //trb3_2->SetCrossProcess(true);
   //trb3_2->CreateTDC(0x1010, 0x1011, 0x1012, 0x1013);
   //trb3_2->SetAutoCalibrations(50000);
   //trb3_2->SetWriteCalibrations("run1");
   //trb3_2->LoadCalibrations("run1");

   base::ProcMgr::instance()->CreateStore("tree.root");

   // this is array with available TDCs ids
   int tdcmap[4] = { 0x1000, 0x1001, 0x1002, 0x1003 };

   // TDC subevent header id

   for (int cnt=0;cnt<4;cnt++) {

      hadaq::TdcProcessor* tdc = hld->FindTDC(tdcmap[cnt]);
      if (tdc==0) continue;

      tdc->SetStoreEnabled();

      // enable storage of first channel, which contains absolute time
      // all other channels times are relative to the first channel
      tdc->SetCh0Enabled(true);

      // specify reference channel
      for (int ch=2;ch<=32;ch++)
         tdc->SetRefChannel(ch, ch-1, 0xffff, 2000,  -10., 90., true);

     // specify reference channel from other TDC
     //if (cnt > 0) {
     //    tdc->SetRefChannel(0, 0, 0xc000, 2000,  -10., 10., true);
     //     tdc->SetRefChannel(5, 5, 0xc000, 2000,  -10., 10., true);
     //     tdc->SetRefChannel(7, 7, 0xc000, 2000,  -10., 10., true);
     // }

      // for old FPGA code one should have epoch for each hit, no longer necessary
      // tdc->SetEveryEpoch(true);

      // When enabled, time of last hit will be used for reference calculations
      // tdc->SetUseLastHit(true);
   }
}



