// this is example for


void first()
{
   // base::ProcMgr::instance()->SetRawAnalysis(true);
   base::ProcMgr::instance()->SetTriggeredAnalysis(true);

   // all new instances get this value
   base::ProcMgr::instance()->SetHistFilling(4);

   // this limits used for liner calibrations when nothing else is available
   hadaq::TdcMessage::SetFineLimits(31, 491);

   // default channel numbers and edges mask
   hadaq::TrbProcessor::SetDefaults(49, 2);

   // [min..max] range for TDC ids
   hadaq::TrbProcessor::SetTDCRange(0x900, 0x9FF);

   // [min..max] range for HUB ids
   hadaq::TrbProcessor::SetHUBRange(0x8100, 0x81FF);

   // when first argument true - TRB/TDC will be created on-the-fly
   // second parameter is function name, called after elements are created
   hadaq::HldProcessor* hld = new hadaq::HldProcessor(true, "after_create");

   // filename  prefix for calibration files and calibration mode (empty string - no file I/O)
   //  0 - only load calibration
   // -1 - accumulate data and store calibrations at the end
   // N>0 - automatic calibration after N hits in each active channel
   hld->ConfigureCalibration("", 100000);

   // only accept trigger type 0x1 when storing file
   // new hadaq::HldFilter(0x1);

   // create ROOT file store
   // base::ProcMgr::instance()->CreateStore("td.root");

   // 0 - disable store
   // 1 - std::vector<hadaq::TdcMessageExt> - includes original TDC message
   // 2 - std::vector<hadaq::MessageFloat>  - compact form, without channel 0, stamp as float (relative to ch0)
   // 3 - std::vector<hadaq::MessageDouble> - compact form, with channel 0, absolute time stamp as double
   hld->SetStoreKind(3);

}

void after_create(hadaq::HldProcessor* hld)
{
   printf("Called after all sub-components are created\n");

   if (hld==0) return;

   for (unsigned k=0;k<hld->NumberOfTRB();k++) {
      hadaq::TrbProcessor* trb = hld->GetTRB(k);
      if (trb==0) continue;

      printf("Configure %s!\n", trb->GetName());

      trb->SetPrintErrors(10);

      // use only data from trigger 0xD for calibrations
      trb->SetCalibrTrigger(0xD);
   }

   for (unsigned k=0;k<hld->NumberOfTDC();k++) {
      hadaq::TdcProcessor* tdc = hld->GetTDC(k);
      if (tdc==0) continue;

      printf("Configure %s!\n", tdc->GetName());

      // tdc->SetUseLastHit(true);
      for (unsigned nch=2;nch<tdc->NumChannels();nch++)
        tdc->SetRefChannel(nch, 1, 0xffff, 2000,  -10., 10.);
   }
}


