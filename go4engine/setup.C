void setup()
{
   base::ProcMgr::instance()->SetRawAnalysis(true);

   get4::Processor* proc = new get4::Processor(0, 0x5);
   // use channel 0 on Get4 as reference, fill extra histos
   proc->setRefChannel(0, 0);
}
