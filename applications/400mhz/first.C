void first()
{
   // analysis will work as triggerred -
   // after each event all data should be processed and flushed
   base::ProcMgr::instance()->SetTriggeredAnalysis(true);

   base::ProcMgr::instance()->SetHistFilling(4);

   // min/max values of
   hadaq::TdcMessage::SetFineLimits(18, 248);


   // number of fine bins
   // ToT range in ns
   hadaq::TdcProcessor::SetDefaults(300, 50);

   // default channel numbers and edges mask
   // 1 - use only rising edge, falling edge is ignored
   // 2 - falling edge enabled and fully independent from rising edge
   // 3 - falling edge enabled and uses calibration from rising edge
   // 4 - falling edge enabled and common statistic is used for calibration
   hadaq::TrbProcessor::SetDefaults(5, 2);

   hadaq::HldProcessor* hld = new hadaq::HldProcessor();
   hadaq::TrbProcessor* trb3 = new hadaq::TrbProcessor(0x8001, hld);

   // Following levels of histograms filling are supported
   //  0 - none
   //  1 - only basic statistic from TRB
   //  2 - generic statistic over TDC channels
   //  3 - basic per-channel histograms with IDs
   //  4 - per-channel histograms with references
   trb3->SetHistFilling(4);

   // indicate if raw data should be printed
//   trb3->SetPrintRawData(true);

   // set number of errors to be printed, default 1000
   trb3->SetPrintErrors(100);


   // Create TDC processors with specified ids
   trb3->CreateTDC(0x1133);

   hadaq::TdcProcessor *tdc = trb3->GetTDC(0x1133);

   // mark as 400 MHz
   // tdc->Set400Mhz();

   // or use custom frequency
   tdc->SetCustomMhz(340);


   for (unsigned nch=1;nch<5;nch++)
     tdc->SetRefChannel(nch, (nch > 1) ? 1 : 2, 0xffff, 2000,  -20., 20.);

   const char* calname = getenv("CALNAME");
   if ((calname==0) || (*calname==0)) calname = "test_";
   const char* calmode = getenv("CALMODE");
   int cnt = (calmode && *calmode) ? atoi(calmode) : -1;
   const char* caltrig = getenv("CALTRIG");
   unsigned trig = (caltrig && *caltrig) ? atoi(caltrig) : 0xFFFF;
   if (trig < 0xF) trig = 1 << trig;

   const char* uset = getenv("USETEMP");
   unsigned use_temp = 0; // 0x80000000;
   if ((uset!=0) && (*uset!=0) && (strcmp(uset,"1")==0)) use_temp = 0x80000000;

   printf("HLD configure calibration calfile:%s  cnt:%d trig:%X temp:%X\n", calname, cnt, trig, use_temp);

   // first parameter if filename  prefix for calibration files
   //     and calibration mode (empty string - no file I/O)
   // second parameter is hits count for autocalibration
   //     0 - only load calibration
   //    -1 - accumulate data and store calibrations only at the end
   //    -77 - accumulate data and produce liner calibrations only at the end
   //    >0 - automatic calibration after N hits in each active channel
   //         if value ends with 77 like 10077 linear calibration will be calculated
   // third parameter is trigger type mask used for calibration
   //   (1 << 0xD) - special 0XD trigger with internal pulser, used also for TOT calibration
   //    0x3FFF - all kinds of trigger types will be used for calibration (excluding 0xE and 0xF)
   //   0x80000000 in mask enables usage of temperature correction

   hld->ConfigureCalibration(calname, cnt, trig | use_temp);



//   trb3->SetPrintRawData(true);


   // Load calibrations for ALL TDCs
   // trb3->LoadCalibrations("/data.local1/padiwa/new_");
   // trb3->LoadCalibrations("/mami/readout/tdc3_");
   ///////  trb3->LoadCalibrations("/data/trb3/calib/new_");

   // calculate and write calibrations at the end of the run
   // trb3->SetWriteCalibrations("/data.local1/padiwa/new_");
   //////  trb3->SetWriteCalibrations("/data/trb3/calib/new_");

   // enable automatic calibrations of the channels
   //////   trb3->SetAutoCalibrations(100000);

   // method set window for all TDCs
   // trb3->SetTriggerWindow(-4e-7, -0.2e-7);

   // 0 - disable store
   // 1 - std::vector<hadaq::TdcMessageExt> - includes original TDC message
   // 2 - std::vector<hadaq::MessageFloat>  - compact form, without channel 0, stamp as float (relative to ch0)
   // 3 - std::vector<hadaq::MessageDouble> - compact form, with channel 0, absolute time stamp as double
   base::ProcMgr::instance()->SetStoreKind(0);

}
