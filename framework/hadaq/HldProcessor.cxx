#include "hadaq/HldProcessor.h"

#include <string.h>
#include <stdio.h>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbIterator.h"
#include "hadaq/TrbProcessor.h"

#define RAWPRINT( args ...) if(IsPrintRawData()) printf( args )

hadaq::HldProcessor::HldProcessor() :
   base::StreamProc("HLD", 0, false),
   fMap(),
   fEventTypeSelect(0xfffff),
   fPrintRawData(false)
{
   mgr()->RegisterProc(this, base::proc_TRBEvent, 0);

   fEvType = MakeH1("EvType", "Event type", 16, 0, 16, "id");
   fEvSize = MakeH1("EvSize", "Event size", 500, 0, 50000, "bytes");
   fSubevSize = MakeH1("SubevSize", "Subevent size", 500, 0, 5000, "bytes");

   // printf("Create HldProcessor %s\n", GetName());

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

hadaq::TdcProcessor* hadaq::HldProcessor::FindTDC(unsigned tdcid) const
{
   for (TrbProcMap::const_iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      hadaq::TdcProcessor* res = iter->second->GetTDC(tdcid, true);
      if (res!=0) return res;
   }
   return 0;
}

double hadaq::HldProcessor::CheckAutoCalibration()
{
   double lvl = 1.;
   for (TrbProcMap::const_iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      double v = iter->second->CheckAutoCalibration();
      if (v<lvl) lvl = v;
   }
   return lvl;
}

void hadaq::HldProcessor::SetTriggerWindow(double left, double right)
{
   base::StreamProc::SetTriggerWindow(left, right);

   for (TrbProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetTriggerWindow(left, right);
}

void hadaq::HldProcessor::SetStoreEnabled(bool on)
{
   base::StreamProc::SetStoreEnabled(on);

   for (TrbProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetStoreEnabled(on);
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

   hadaqs::RawEvent* ev = 0;

   while ((ev = iter.nextEvent()) != 0) {

      DefFillH1(fEvType, (ev->GetId() & 0xf), 1.);

      if ((fEventTypeSelect <= 0xf) && ((ev->GetId() & 0xf) != fEventTypeSelect)) continue;

      if (IsPrintRawData()) ev->Dump();

      DefFillH1(fEvSize, ev->GetPaddedSize(), 1.);

      for (TrbProcMap::iterator diter = fMap.begin(); diter != fMap.end(); diter++)
         diter->second->BeforeEventScan();

      hadaqs::RawSubevent* sub = 0;

      while ((sub = iter.nextSubevent()) != 0) {

         if (IsPrintRawData()) sub->Dump(true);

         DefFillH1(fSubevSize, sub->GetSize(), 1.);

         // use only 16-bit in trigger number while CTS make a lot of errors in higher 8 bits
         // AccountTriggerId((sub->GetTrigNr() >> 8) & 0xffff);

         TrbProcMap::iterator iter = fMap.find(sub->GetId());

         if (iter != fMap.end())
            iter->second->ScanSubEvent(sub, ev->GetSeqNr());
      }

      for (TrbProcMap::iterator diter = fMap.begin(); diter != fMap.end(); diter++)
         diter->second->AfterEventScan();
   }

   return true;
}

unsigned hadaq::HldProcessor::TransformEvent(void* src, unsigned len, void* tgt, unsigned tgtlen)
{
   hadaq::TrbIterator iter(src, len);

   // only single event is transformed
   if (iter.nextEvent() == 0) return 0;

   if ((tgt!=0) && (tgtlen<len)) {
      fprintf(stderr,"HLD requires larger output buffer than original\n");
      return 0;
   }

   hadaqs::RawSubevent* sub = 0;

   unsigned reslen = 0;
   unsigned char* curr = (unsigned char*) tgt;
   if (tgt!=0) {
      // copy event header
      memcpy(tgt, iter.currEvent(), sizeof(hadaqs::RawEvent));
      reslen += sizeof(hadaqs::RawEvent);
      curr += sizeof(hadaqs::RawEvent);
   }

   while ((sub = iter.nextSubevent()) != 0) {
      TrbProcMap::iterator iter = fMap.find(sub->GetId());
      if (iter != fMap.end()) {
         if (curr && (tgtlen-reslen < sub->GetPaddedSize())) {
            fprintf(stderr,"not enough space for subevent in output buffer\n");
            return 0;
         }
         unsigned sublen = iter->second->TransformSubEvent(sub, curr, tgtlen - reslen);
         if (curr) {
            curr += sublen;
            reslen += sublen;
         }
      } else
      if (curr) {
         // copy subevent which cannot be recognized
         memcpy(curr, sub, sub->GetPaddedSize());
         curr += sub->GetPaddedSize();
         reslen += sub->GetPaddedSize();
      }
   }

   if (tgt==0) {
      reslen = len;
   } else {
      // set new event size
      ((hadaqs::RawEvent*) tgt)->SetSize(reslen);
   }

   if (iter.nextEvent() != 0) {
      fprintf(stderr,"HLD should transform only single event\n");
      return 0;
   }

   return reslen;
}


