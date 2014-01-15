void first()
{
   base::ProcMgr::instance()->SetRawAnalysis(false);

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
   trb3->SetUseTriggerAsSync(true);

   // we artificically made contigious value for epoch
   trb3->SetCompensateEpochReset(true);

   // this is array with available TDCs ids
   // It is required that high 8 bits are the same.
   // These bits are used to separate TDC data from other data kinds

   int tdcmap[4] = { 0x8a00, 0x8a01, 0x8a02, 0x8a03 };
   const int chOffset = 48;

   // TDC subevent header id
   trb3->SetHadaqTDCId(tdcmap[0] & 0xFF00);

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

      tdc->SetUseLastHit(true);

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

  


      // specify reference channel for any other channel -
      // will appear as additional histogram with time difference between channels
      // for (int n=2;n<65;n=n+2)
      //   tdc->SetRefChannel(n, n-1);

      // one also able specify reference from other TDCs
      // but one should enable CrossProcessing for trb3
      // Here we set as reference channel 0 on tdc 1
      // tdc->SetRefChannel(0, 0, 1);

      // for old FPGA code one should have epoch for each hit, no longer necessary
      // tdc->SetEveryEpoch(true);


      // next parameters are about time calibration - there are two possibilities
      // 1) automatic calibration after N hits in every enabled channel
      // 2) generate calibration on base of provided data and than use it later statically for analysis

      // disable calibration for channel #0
      tdc->DisableCalibrationFor(0);

      // load static calibration at the beginning of the run
      tdc->LoadCalibration(Form("tdc3_%04x.cal", tdcmap[cnt]));

      // calculate and write static calibration at the end of the run
      // tdc->SetWriteCalibration(Form("tdc3_%04x.cal", tdcmap[cnt]));

      // enable automatic calibration, specify required number of hits in each channel
      //tdc->SetAutoCalibration(100000);

      // this will be required only when second analysis step will be introduced
      // and one wants to select only hits around some trigger signal for that analysis

      // method set window for all TDCs at the same time
      //trb->SetTriggerWindow(-4e-7, -3e-7);

#ifdef __GO4ANAMACRO__
      int numx = 1;
      int numy = 1;
      while ((numx * numy) < tdc->NumChannels()) {
         if (numx==numy) numx++; else numy++;
      }

      TGo4Picture** pic = new TGo4Picture*[tdc->GetNumHist()];
      int* piccnt = new int[tdc->GetNumHist()];
      for (int k=0;k<tdc->GetNumHist();k++) {
         pic[k] = new TGo4Picture(Form("TDC%d_%s",tdcid, tdc->GetHistName(k)), Form("All %s", tdc->GetHistName(k)));
         pic[k]->SetDivision(numy,numx);
         piccnt[k] = 0;
      }
      for (int n=0;n<tdc->NumChannels();n++) {
         int x = n % numx;
         int y = n / numx;
         for (int k=0;k<tdc->GetNumHist();k++) {
            TObject* obj = (TObject*) tdc->GetHist(n, k);
            if (obj) piccnt[k]++;
            pic[k]->Pic(y,x)->AddObject(obj);
         }
      }
      for (int k=0;k<tdc->GetNumHist();k++) {
         if (piccnt[k] > 0) go4->AddPicture(pic[k]);
                      else delete pic[k];
      }
      delete[] pic;
      delete[] piccnt;

#endif

   }

   // method set window for all TDCs at the same time
   trb3->SetTriggerWindow(-4e-7, -1e-9);

   base::StreamProc::SetBufsQueueCapacity(100);


   // This is a method to regularly invoke macro, where arbitrary action can be performed
   // One could specify period in seconds or function will be called for every event processed

   // new THookProc("my_hook();", 2.5);
}



// this is example of hook function
// here one gets access to all tdc processors and obtains Mean and RMS
// value on the first channel for each TDC and prints on the display

void my_hook()
{
   hadaq::TrbProcessor* trb3 = base::ProcMgr::instance()->FindProc("TRB0");
   if (trb3==0) return;

   printf("Do extra work NUM %u\n", trb3->NumSubProc());

   for (unsigned ntdc=0;ntdc<trb3->NumSubProc();ntdc++) {

      hadaq::TdcProcessor* tdc = trb3->GetSubProc(ntdc);
      if (tdc==0) continue;

      TH1* hist = (TH1*) tdc->GetChannelRefHist(1);

      printf("  TDC%u mean:%5.2f rms:%5.2f\n", tdc->GetBoardId(), hist->GetMean(), hist->GetRMS());

      tdc->ClearChannelRefHist(1);
   }

}
