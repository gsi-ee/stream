void first()
{
   base::ProcMgr::instance()->SetTriggeredAnalysis();

   hadaq::TdcMessage::SetFineLimits(31, 421);

   hadaq::TrbProcessor* trb3 = new hadaq::TrbProcessor(0);

   // CTS subevent header id, all 16 bit
   // trb3->SetHadaqCTSId(0x8000);

   // HUB subevent header id, here high 8 bit from 16 should be specified
   // lower 8 bit are used as hub number
   trb3->SetHadaqHUBId(0x9000);

   trb3->SetPrintRawData(false);

   // set number of errors to be printed, default 1000
   trb3->SetPrintErrors(100);

   // enable cross processing only when one want to specify reference channel from other TDCs
   // in this case processing 'crosses' border of TDC and need to access data from other TDC
   // trb3->SetCrossProcess(true);

   // enable time synchronization by trigger number
   // trb3->SetUseTriggerAsSync(true);

   // we artificically made contigious value for epoch
   // trb3->SetCompensateEpochReset(true);

   // this is array with available TDCs ids
   // It is required that high 8 bits are the same.
   // These bits are used to separate TDC data from other data kinds

   int tdcmap[4] = { 0x8a00, 0x8a01, 0x8a02, 0x8a03 };
   const int chOffset = 48;

   // TDC subevent header id
   trb3->SetHadaqTDCId(tdcmap[0] & 0xFF00);

   trb3->SetHistFilling(1); // only basic histograms, all TDC channels are disabled

   for (int cnt=0;cnt<4;cnt++) {

      int tdcid = tdcmap[cnt] & 0x00FF;

      // verify prefix
      if ((tdcmap[0] & 0xFF00) != (tdcmap[cnt] & 0xFF00)) {
         fprintf(stderr, "!!!! Wrong prefix in TDC%d, do not match with TDC0  %X != %X\n", cnt, (tdcmap[cnt] & 0xFF00), (tdcmap[0] & 0xFF00));
         exit(5);
      }

      // create processor for hits from TDC
      // par1 - pointer on trb3 board
      // par2 - TDC number (lower 8 bit from subevent header
      // par3 - number of channels in TDC
      // par4 - edges mask 0x1 - rising, 0x2 - trailing, 0x3 - both edges
      hadaq::TdcProcessor* tdc = new hadaq::TdcProcessor(trb3, tdcid, 65, 0x1);

      // which hit used for reference histograms filling
      tdc->SetUseLastHit(true);

      // all reference histograms will be filled only
      // when trb3->SetHistFilling(4) will be called

      if (cnt==0) {
         int channels0[] = {chOffset+1, chOffset+2, chOffset+3, chOffset+4, 0};
         tdc->CreateHistograms( channels0 );

         tdc->SetRefChannel(chOffset+1, 0, 0xffff, 10000,  -2000., 1000., false);
         tdc->SetRefChannel(chOffset+2, 0, 0xffff, 10000,  -2000., 1000., false);

         tdc->SetRefChannel(chOffset+3, chOffset+1, 0xffff, 4800,  -20., 100., true);
         tdc->SetRefChannel(chOffset+4, chOffset+1, 0xffff, 5000,  0., 500., true);
         // IMPORTANT: for both channels references should be already specified
         tdc->SetDoubleRefChannel(chOffset+4, chOffset+3, 600, 0., 300., 300, -5., 25.);

         // this is example how to specify conditional print
         // tdc->EnableRefCondPrint(36, -3., 7., 20);
      }
      
      if (cnt==3) {
         int channels3[] = {1, 2, 3, 4, 5, 6, 0};
         tdc->CreateHistograms( channels3 );
         tdc->SetRefChannel(1, 0, 0xffff, 4000,  -2000., 2000., true);
         tdc->SetRefChannel(3, 1, 0xffff, 1200,  -20., 40., true);
         tdc->SetRefChannel(2, 1, 0xffff, 1500,  0., 300., true);
	      tdc->SetRefChannel(4, 3, 0xffff, 1500,  0., 300., true);
         tdc->SetDoubleRefChannel(3, 1000 + chOffset + 4 , 80, -10., 30., 500, 0., 500.);
         tdc->SetRefChannel(5, 1, 0xffff, 4000,  -200., 200., true);
         tdc->SetRefChannel(6, 5, 0xffff, 1500,  0., 300., true);
      }

      // next parameters are about time calibration - there are two possibilities
      // 1) automatic calibration after N hits in every enabled channel
      // 2) generate calibration on base of provided data and than use it later statically for analysis

      // disable calibration for channel #0
      // tdc->DisableCalibrationFor(0);

      // load static calibration at the beginning of the run
      tdc->LoadCalibration(Form("tdc3_%04x.cal", tdcmap[cnt]));

      // calculate and write static calibration at the end of the run
      // tdc->SetWriteCalibration(Form("tdc3_%04x.cal", tdcmap[cnt]));

      // enable automatic calibration, specify required number of hits in each channel
      //tdc->SetAutoCalibration(100000);
   }

   // method set window for all TDCs at the same time
   trb3->SetTriggerWindow(-4e-7, -0.2e-7);
}
