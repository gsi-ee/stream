void first()
{

   base::ProcMgr::instance()->SetRawAnalysis(true);

   hadaq::TdcMessage::SetFineLimits(31, 500);

   hadaq::TrbProcessor* trb3 = new hadaq::TrbProcessor(0);

   // CTS subevent header id, all 16 bit
   // trb3->SetHadaqCTSId(0x8000);

   // HUB subevent header id, here high 8 bit from 16 should be specified
   // lower 8 bit are used as hub number
   trb3->SetHadaqHUBId(0x9000);

   trb3->SetPrintRawData(false);

   // enable cross processing only when one want to specify reference channel from other TDCs
   // in this case processing 'crosses' border of TDC and need to access data from other TDC
   // trb3->SetCrossProcess(true);

   // this is array with available TDCs ids
   // Full id, which is appeared in raw data, combined from value,
   // provided in SetHadaqTDCId() and individual ID of each TDC
   // For instance, if SetHadaqTDCId(0x8000) was called and tdcid=0x07,
   // header in HADAQ raw data will have id 0x8007

   int tdcmap[3] = { 0xC002, 0xC005, 0xD007 };

   // TDC subevent header id, here high 8 bit from 16 should be specified
   // lower 8 bit are used as tdc number
   // Be aware that all TDCs have same prevfix
   trb3->SetHadaqTDCId(tdcmap[0] & 0xFF00);

   for (int cnt=0;cnt<3;cnt++) {

      int tdcid = tdcmap[cnt] & 0x00FF;

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

      // specify reference channel for any other channel -
      // will appear as additional histogram with time difference between channels
      for (int n=1;n<65;n++)
         tdc->SetRefChannel(n, n+2);
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
      // tdc->LoadCalibration(Form("test_%d.cal", tdcid));

      // calculate and write static calibration at the end of the run
      // tdc->SetWriteCalibration(Form("test_%d.cal", tdcid));

      // enable automatic calibration, specify required number of hits in each channel
      tdc->SetAutoCalibration(100000);

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

}
