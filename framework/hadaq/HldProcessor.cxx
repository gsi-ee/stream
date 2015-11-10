#include "hadaq/HldProcessor.h"

#include <string.h>
#include <stdio.h>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbIterator.h"
#include "hadaq/TrbProcessor.h"

#define RAWPRINT( args ...) if(IsPrintRawData()) printf( args )

hadaq::HldProcessor::HldProcessor(bool auto_create, const char* after_func) :
   base::StreamProc("HLD", 0, false),
   fMap(),
   fEventTypeSelect(0xfffff),
   fPrintRawData(false),
   fAutoCreate(auto_create),
   fAfterFunc(after_func),
   fCalibrName(),
   fCalibrPeriod(-111),
   fMsg(),
   pMsg(0)
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

hadaq::TrbProcessor* hadaq::HldProcessor::FindTRB(unsigned trbid) const
{
   TrbProcMap::const_iterator iter = fMap.find(trbid);
   return iter == fMap.end() ? 0 : iter->second;
}

unsigned hadaq::HldProcessor::NumberOfTRB() const
{
   return fMap.size();
}

hadaq::TrbProcessor* hadaq::HldProcessor::GetTRB(unsigned indx) const
{
   for (TrbProcMap::const_iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      if (indx==0) return iter->second;
      indx--;
   }
   return 0;
}

unsigned hadaq::HldProcessor::NumberOfTDC() const
{
   unsigned num = 0;
   for (TrbProcMap::const_iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      num += iter->second->NumberOfTDC();
   return num;
}

hadaq::TdcProcessor* hadaq::HldProcessor::GetTDC(unsigned indx) const
{
   for (TrbProcMap::const_iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      unsigned num = iter->second->NumberOfTDC();
      if (indx < num) return iter->second->GetTDCWithIndex(indx);
      indx -= num;
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

void hadaq::HldProcessor::ConfigureCalibration(const std::string& name, long period)
{
   fCalibrName = name;
   fCalibrPeriod = period;
   for (TrbProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->ConfigureCalibration(name, period);
}


void hadaq::HldProcessor::SetTriggerWindow(double left, double right)
{
   base::StreamProc::SetTriggerWindow(left, right);

   for (TrbProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetTriggerWindow(left, right);
}

void hadaq::HldProcessor::SetStoreKind(unsigned kind)
{
   base::StreamProc::SetStoreKind(kind);

   for (TrbProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetStoreKind(kind);
}


void hadaq::HldProcessor::SetPrintRawData(bool on)
{
   fPrintRawData = on;
   for (TrbProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetPrintRawData(on);
}

void hadaq::HldProcessor::CreateBranch(TTree* t)
{
   if(mgr()->IsTriggeredAnalysis()) {
      pMsg = &fMsg;
      mgr()->CreateBranch(t, GetName(), "hadaq::HldMessage", (void**)&pMsg);
   }
}

bool hadaq::HldProcessor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

//   RAWPRINT("TRB3 - first scan of buffer %u\n", buf().datalen);

//   printf("Scan TRB buffer size %u\n", buf().datalen);

   hadaq::TrbIterator iter(buf().buf, buf().datalen);

   hadaqs::RawEvent* ev = 0;

   unsigned evcnt = 0;

   while ((ev = iter.nextEvent()) != 0) {

      evcnt++;

      DefFillH1(fEvType, (ev->GetId() & 0xf), 1.);

      fMsg.trig_type = ev->GetId() & 0xf;

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
         else
         if (fAutoCreate) {
            TrbProcessor* trb = new TrbProcessor(sub->GetId(), this);
            trb->SetAutoCreate(true);
            trb->SetHadaqCTSId(sub->GetId());
            trb->SetStoreKind(GetStoreKind());

            mgr()->UserPreLoop(trb); // while loop already running, call it once again for new processor

            printf("Create TRB 0x%04x procmgr %p lvl %d \n", sub->GetId(), base::ProcMgr::instance(), trb->HistFillLevel());

            // in auto mode only TDC processors should be created
            trb->ScanSubEvent(sub, ev->GetSeqNr());

            trb->ConfigureCalibration(fCalibrName, fCalibrPeriod);

            trb->SetAutoCreate(false); // do not create TDCs after first event
         }
      }

      for (TrbProcMap::iterator diter = fMap.begin(); diter != fMap.end(); diter++)
         diter->second->AfterEventScan();

      if (fAutoCreate) {
         fAutoCreate = false; // with first event
         if (fAfterFunc.length()>0) mgr()->CallFunc(fAfterFunc.c_str(), this);
      }
   }

   if (mgr()->IsTriggeredAnalysis()) {
      if (evcnt>1)
         fprintf(stderr, "event count %u bigger than 1 - not work for triggered analysis\n", evcnt);
      mgr()->AddToTrigEvent(GetName(), new hadaq::HldSubEvent(fMsg));
   }

   return true;
}

void hadaq::HldProcessor::Store(base::Event* ev)
{
   if (ev!=0) {
      // only for stream analysis use special handling when many events could be produced at once

      hadaq::HldSubEvent* sub =
         dynamic_cast<hadaq::HldSubEvent*> (ev->GetSubEvent(GetName()));

      // when subevent exists, use directly pointer on message
      if (sub!=0)
         pMsg = &(sub->fMsg);
      else
         pMsg = &fMsg;
   }
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

void hadaq::HldProcessor::UserPreLoop()
{
   if (!fAutoCreate && (fAfterFunc.length()>0))
      mgr()->CallFunc(fAfterFunc.c_str(), this);
}


