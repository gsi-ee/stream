{
   nx::Processor* proc0 = new nx::Processor(0);
   proc0->SetTriggerSignal(10); // define SYNC0 as main reference time
   proc0->SetTriggerWindow(500, 1200);

   nx::Processor* proc1 = new nx::Processor(1);
   proc1->SetTriggerWindow(500, 1200);

   nx::Processor* proc2 = new nx::Processor(2);
   proc2->SetTriggerWindow(500, 1200);
}
