#include "hadaq/HldProcessor.h"

#include <string.h>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbIterator.h"
#include "hadaq/TrbProcessor.h"

#define RAWPRINT( args ...) if(IsPrintRawData()) printf( args )

hadaq::HldProcessor::HldProcessor() :
   base::StreamProc("HLD", 0, false),
   fMap(),
   fPrintRawData(false)
{
   mgr()->RegisterProc(this, base::proc_TRBEvent, 0);

   fEvSize = MakeH1("EvSize", "Event size", 500, 0, 5000, "bytes");
   fSubevSize = MakeH1("SubevSize", "Subevent size", 500, 0, 5000, "bytes");

   printf("Create HldProcessor %s\n", GetName());

   // this is raw-scan processor, therefore no synchronization is required for it
   SetSynchronisationKind(sync_None);

   // only raw scan, data can be immediately removed
   SetRawScanOnly();
}

hadaq::HldProcessor::~HldProcessor()
{
}

void hadaq::HldProcessor::AddTrb(TrbProcessor* trb, unsigned id)
{
   fMap[id] = trb;
}


void hadaq::HldProcessor::SetTriggerWindow(double left, double right)
{
   base::StreamProc::SetTriggerWindow(left, right);

   for (TrbProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetTriggerWindow(left, right);
}

void hadaq::HldProcessor::SetPrintRawData(bool on)
{
   fPrintRawData = on;
   for (TrbProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetPrintRawData(on);
}


bool hadaq::HldProcessor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

//   RAWPRINT("TRB3 - first scan of buffer %u\n", buf().datalen);

//   printf("Scan TRB buffer size %u\n", buf().datalen);

   hadaq::TrbIterator iter(buf().buf, buf().datalen);

   hadaq::RawEvent* ev = 0;

   while ((ev = iter.nextEvent()) != 0) {

      if (IsPrintRawData()) ev->Dump();

      FillH1(fEvSize, ev->GetSize());

      hadaq::RawSubevent* sub = 0;

      while ((sub = iter.nextSubevent()) != 0) {

         if (IsPrintRawData()) sub->Dump(true);

         FillH1(fSubevSize, sub->GetSize());

         // use only 16-bit in trigger number while CTS make a lot of errors in higher 8 bits
         // AccountTriggerId((sub->GetTrigNr() >> 8) & 0xffff);

         TrbProcMap::iterator iter = fMap.find(sub->GetId());

         if (iter != fMap.end())
            iter->second->ScanSubEvent(sub, ev->GetSeqNr());
      }

      for (TrbProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
         iter->second->AfterEventScan();
   }

   return true;
}

