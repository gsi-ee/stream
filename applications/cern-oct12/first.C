void first() {
   base::ProcMgr::instance()->SetRawAnalysis(false);

   nx::Processor::SetDisorderTm(5000);
   nx::Processor::SetLastEpochCorr(true);

   nx::Processor* proc0 = new nx::Processor(0);
   proc0->SetTriggerSignal(10); // define SYNC0 as main reference time
   proc0->SetTriggerWindow(500, 1200);

   nx::Processor* proc1 = new nx::Processor(1);
   proc1->SetTriggerWindow(500, 1200);

   nx::Processor* proc2 = new nx::Processor(2);
   proc2->SetTriggerWindow(500, 1200);

//   nx::Processor* proc3 = new nx::Processor(3);
//   proc3->SetTriggerWindow(500, 1200);

   nx::Processor* proc4 = new nx::Processor(4);
   proc4->SetTriggerWindow(500, 1200);

   nx::Processor* proc5 = new nx::Processor(5);
   proc5->SetTriggerWindow(500, 1200);

   nx::Processor* proc6 = new nx::Processor(6);
   proc6->SetTriggerWindow(500, 1200);


   hadaq::TrbProcessor* trb3 = new hadaq::TrbProcessor(0);

   hadaq::TdcProcessor* tdc1 = new hadaq::TdcProcessor(trb3, 1);
   hadaq::TdcProcessor* tdc2 = new hadaq::TdcProcessor(trb3, 2);
   hadaq::TdcProcessor* tdc6 = new hadaq::TdcProcessor(trb3, 6);
   hadaq::TdcProcessor* tdc7 = new hadaq::TdcProcessor(trb3, 7);

   // method set window for all TDCs at the same time
   trb3->SetTriggerWindow(-400, -300);
}
