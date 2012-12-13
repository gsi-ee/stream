{
   // base::ProcMgr::instance()->SetRawAnalysis(false);

   get4::Processor* proc = new get4::Processor(0, 0x5);

   // ignore all SYNC messages, do not use them for time synchronization
   proc->SetNoSyncSource();

   // use channel 0 on Get4 as reference, fill extra histos
   proc->setRefChannel(0, 0);

   // use this window to extract signals around reference
   proc->SetTriggerWindow(-1000., 1000.);
}


