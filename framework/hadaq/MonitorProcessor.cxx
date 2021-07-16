#include "hadaq/MonitorProcessor.h"

//////////////////////////////////////////////////////////////////////////////////////////////
/// constructor

hadaq::MonitorProcessor::MonitorProcessor(unsigned brdid, HldProcessor *hld, int hfill) :
   hadaq::TrbProcessor(brdid, hld, hfill)
{
   fMonitorProcess = 3;
   pStoreVect = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Scan FPGA-TDC data, distribute over sub-processors

void hadaq::MonitorProcessor::ScanSubEvent(hadaqs::RawSubevent* sub, unsigned trb3runid, unsigned trb3seqid)
{

   memcpy((void *) &fLastSubevHdr, sub, sizeof(fLastSubevHdr));
   fCurrentRunId = trb3runid;
   fCurrentEventId = sub->GetTrigNr();

   DefFillH1(fTrigType, sub->GetTrigTypeTrb3(), 1.);

   DefFillH1(fSubevSize, sub->GetSize(), 1.);

   unsigned ix = 0, trbSubEvSize = sub->GetSize() / 4 - 4;

   bool dostore = false;

   if (IsStoreEnabled() && mgr()->HasTrigEvent()) {
      dostore = true;
      hadaq::MonitorSubEvent* subevnt = new hadaq::MonitorSubEvent();
      mgr()->AddToTrigEvent(GetName(), subevnt);
      pStoreVect = subevnt->vect_ptr();
   }

   uint32_t addr0 = 0, addr, value;

   while (ix < trbSubEvSize) {

      if (fMonitorProcess == 3)
         addr0 = sub->Data(ix++);

      addr = sub->Data(ix++);
      value = sub->Data(ix++);

      if (dostore)
         pStoreVect->emplace_back(addr0, addr, value);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// create branch

void hadaq::MonitorProcessor::CreateBranch(TTree*)
{
   if(mgr()->IsTriggeredAnalysis()) {
      pStoreVect = &fDummyVect;
      mgr()->CreateBranch(GetName(), "std::vector<hadaq::MessageMonitor>", (void**) &pStoreVect);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// store event

void hadaq::MonitorProcessor::Store(base::Event* ev)
{
   // in case of triggered analysis all pointers already set
   if (!ev || IsTriggeredAnalysis()) return;

   base::SubEvent* sub0 = ev->GetSubEvent(GetName());
   if (!sub0) return;

   if (GetStoreKind() > 0) {
      hadaq::MonitorSubEvent* sub = dynamic_cast<hadaq::MonitorSubEvent*> (sub0);
      // when subevent exists, use directly pointer on messages vector
      pStoreVect = sub ? sub->vect_ptr() : &fDummyVect;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// reset store

void hadaq::MonitorProcessor::ResetStore()
{
   pStoreVect = &fDummyVect;
}
