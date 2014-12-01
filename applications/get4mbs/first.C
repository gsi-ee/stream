void first() {

   base::ProcMgr::instance()->SetRawAnalysis(true);

   // this is just for MBS readout testing

   // create processor  get4mask, 32bit mode, tot multiplier
   get4::MbsProcessor* proc = new get4::MbsProcessor(0x1, true, 8);

   // proc->SetAutoCalibr(100000);

   // proc->LoadCalibration("mist.cal");

   // proc->SetWriteCalibr("mist.cal");

   //   (get4, ch, isrising) - (get4, ch, isrising),  nbins, min, max
   proc->AddRef(0, 0, true,   0, 1, true,   200, -1000., 9000.);
   proc->AddRef(0, 0, true,   0, 2, true,   200, -1000., 9000.);
   proc->AddRef(0, 0, true,   0, 3, true,   200, -1000., 9000.);

   //proc->AddRef(0, 0, true,   1, 1, true,   200, -1000., 9000.);
   //proc->AddRef(0, 0, true,   1, 2, true,   200, -1000., 9000.);
   //proc->AddRef(0, 0, true,   1, 3, true,   200, -1000., 9000.);

}


