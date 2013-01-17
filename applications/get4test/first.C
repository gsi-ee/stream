{
   // base::ProcMgr::instance()->SetRawAnalysis(false);

   get4::Processor* proc = new get4::Processor(0, 0x30);

   // ignore all SYNC messages, do not use them for time synchronization
   proc->SetNoSyncSource();

   // ignore all 250 MHz messages for timing, useful when clocks are not synchronized
   proc->SetIgnore250Mhz();

   // use channel 0 on Get4 as reference
   proc->setRefChannel(4, 0);

   // use this window to extract signals around reference
   proc->SetTriggerWindow(-1e-6, 1e-6);

//   double msgtm = 3.607101958; // 18 hits
//   double msgtm = 3.611034118; // 22 hits
//   double msgtm = 11.744051718; // 17 hits
//   double msgtm = 70.065848839; // SYNC in between of GET4 messages

//   proc->SetPrint(1000, msgtm - 1e-5, msgtm + 1e-5);
}


