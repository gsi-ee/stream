void first() {

   base::ProcMgr::instance()->SetRawAnalysis(true);

   // this is just for MBS readout testing

   get4::MbsProcessor* proc = new get4::MbsProcessor();

   //          (get4, ch, isrising) - (get4, ch, isrising),  nbins, min, max
   proc->AddRef(0, 0, true, 0, 2, true, 100, -1000., 9000.);
}


