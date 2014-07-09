void first() {

   base::ProcMgr::instance()->SetRawAnalysis(true);

   // this is just for MBS readout testing

   get4::MbsProcessor* proc = new get4::MbsProcessor();

   //          (get4, ch, isrising) - (get4, ch, isrising),  nbins, min, max
   proc->AddRef(0, 0, false, 0, 0, true, 500, 0., 1000.);
}


