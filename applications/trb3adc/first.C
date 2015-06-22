#ifndef __CINT__
#include <hadaq/AdcProcessor.h>
#endif

void first()
{
   // analysis will work as triggerred -
   // after each event all data should be processed and flushed
   base::ProcMgr::instance()->SetTriggeredAnalysis();
   //base::ProcMgr::instance()->SetRawAnalysis();

   hadaq::HldProcessor* hld = new hadaq::HldProcessor();

   hadaq::TrbProcessor* trb3 = new hadaq::TrbProcessor(0x8000, hld);
   trb3->CreateCTS_TDC();
   //trb3->SetUseTriggerAsSync(); // use TRB3 CTS trigger number, not from ETM
   trb3->SetHistFilling(4);
   //trb3->SetPrintRawData();
   
   new hadaq::AdcProcessor(trb3, 0x0200);
   //adc->SetDiffChannel(8, 24);
   //adc->SetDiffChannel(24, 8);


   // uncomment these line to enable store of all data in the tree
   hld->SetStoreEnabled(true);

   // create store - typically done in second.C, should be called only once
   //base::ProcMgr::instance()->CreateStore("file.root");
}
