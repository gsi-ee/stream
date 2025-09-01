#include "hadaq/TrbProcessor.h"

#include <cstring>
#include <algorithm>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbIterator.h"
#include "hadaq/TdcProcessor.h"
#include "hadaq/MdcProcessor.h"
#include "hadaq/HldProcessor.h"

#include "dogma/defines.h"

#define RAWPRINT( args ...) if(IsPrintRawData()) printf( args )

unsigned hadaq::TrbProcessor::gNumChannels = 65;
unsigned hadaq::TrbProcessor::gEdgesMask = 0x1;
bool hadaq::TrbProcessor::gIgnoreSync = false;
unsigned hadaq::TrbProcessor::gTDCMin = 0x0000;
unsigned hadaq::TrbProcessor::gTDCMax = 0x0FFF;
unsigned hadaq::TrbProcessor::gHUBMin = 0x8100;
unsigned hadaq::TrbProcessor::gHUBMax = 0x81FF;
unsigned hadaq::TrbProcessor::gMDCMin = 0x0000;
unsigned hadaq::TrbProcessor::gMDCMax = 0x0000;

//////////////////////////////////////////////////////////////////////////////
/// Set defaults for the next creation of TDC processors.
///
/// \param numch provides number of channels and edges.
/// \param edges define how falling edges are used
///  - 1 - use only rising edge, falling edge is ignore
///  - 2 - falling edge enabled and fully independent from rising edge
///  - 3 - falling edge enabled and uses calibration from rising edge
///  - 4 - falling edge enabled and common statistic is used for calibration
/// see \ref hadaq::TdcProcessor::EEdgesMasks for more info
/// \param ignore_sync defines if sync messages are ignored (true by default) */

void hadaq::TrbProcessor::SetDefaults(unsigned numch, unsigned edges, bool ignore_sync)
{
   gNumChannels = numch;
   gEdgesMask = edges;
   gIgnoreSync = ignore_sync;
}

//////////////////////////////////////////////////////////////////////////////
/// Retun default number of TDC channels

unsigned hadaq::TrbProcessor::GetDefaultNumCh()
{
   return gNumChannels;
}


unsigned hadaq::TrbProcessor::GetNumCh() const
{
   return fCustomNumChannels ? fCustomNumChannels : GetDefaultNumCh();
}


//////////////////////////////////////////////////////////////////////////////
/// Constructor, one could specify histogram fill level

hadaq::TrbProcessor::TrbProcessor(unsigned brdid, HldProcessor* hldproc, int hfill, bool with_dogma) :
   base::StreamProc(with_dogma && !hldproc ? "DOGMA" : "TRB_%04X", brdid, false),
   fHldProc(hldproc),
   fMap(),
   fHadaqHUBId()
{
   if (!hldproc) {
      fDogma = with_dogma;
      mgr()->RegisterProc(this, with_dogma ? base::proc_DOGMAEvent : base::proc_TRBEvent, brdid & 0xFF);
   } else {
      hldproc->AddTrb(this, brdid);
      if (!mgr() && hldproc->mgr())
         hldproc->mgr()->AddProcessor(this);
   }

   if (hfill >= 0) SetHistFilling(hfill);

   // printf("Create TrbProcessor %s\n", GetName());

   fMinTdc = fMaxTdc = 0;
   fCurrentRunId = 0;
   fCurrentEventId = 0;

   fMsgPerBrd = nullptr;
   fErrPerBrd = nullptr;
   fHitsPerBrd = nullptr;
   fCalHitsPerBrd = nullptr;
   fToTPerBrd = nullptr;

   fEvSize = MakeH1("EvSize", "Event size", 500, 0, 50000, "bytes");
   fSubevHLen = 5000;
   fSubevHDiv = 10;
   fSubevSize = MakeH1("SubevSize", "Subevent size", fSubevHLen/fSubevHDiv, 0, fSubevHLen, "bytes");
   fLostRate = MakeH1("LostRate", "Relative number of lost packets", 1000, 0, 1., "data lost");
   fTrigType = MakeH1("TrigType", "Number of different trigger types", 16, 0, 16, "trigger;xbin:0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF");
   fErrBits =  MakeH1("ErrorBits", "Number of trbnet error bits", 32, 0, 32,
       "errorbit;xbin:OK,Coll,WMis,ChkaMis,DU,BufMis,AnsMiss,7,8,9,10,11,12,13,14,15, EvNumMis,TrgCdMis,WrLen,AnsMiss,NotFnd,PrtMiss,SevPlm,BrokEv,EthLnkErr,SEvBuFull,EthErr,TmTrgErr,28,29,30,31");

   fHadaqCTSId = 0x8000;

   fLastTriggerId = 0;
   fLostTriggerCnt = 0;
   fTakenTriggerCnt = 0;

   fSyncTrigMask = 0;
   fSyncTrigValue = 0;
   fCalibrTriggerMask = 0xFFFF;

   fUseTriggerAsSync = false;
   fCompensateEpochReset = false;

   fPrintRawData = false;
   fCrossProcess = false;
   fPrintErrCnt = 30;

   fAutoCreate = false;

   pMsg = &fMsg;

   // this is raw-scan processor, therefore no synchronization is required for it
   SetSynchronisationKind(sync_None);

   // only raw scan, data can be immediately removed
   SetRawScanOnly();

//   fProfiler.Reserve(50);
}

//////////////////////////////////////////////////////////////////////////////
/// Destructor

hadaq::TrbProcessor::~TrbProcessor()
{
}

//////////////////////////////////////////////////////////////////////////////
/// Find TDC with provided id

hadaq::TdcProcessor* hadaq::TrbProcessor::FindTDC(unsigned tdcid) const
{
   hadaq::TdcProcessor* res = GetTDC(tdcid, true);
   if (!res && fHldProc) res = fHldProc->FindTDC(tdcid);
   return res;
}

//////////////////////////////////////////////////////////////////////////////
/// Create overview histograms where each TDC corresponds one bin

void hadaq::TrbProcessor::CreatePerTDCHistos()
{
   std::vector<TdcProcessor *> tdcs;

   unsigned numtdc = NumberOfTDC();
   for (unsigned indx = 0; indx < numtdc; ++indx)
      tdcs.emplace_back(GetTDCWithIndex(indx));

   std::string lbl = "tdc;xbin:";
   unsigned cnt = 0, maxnumch = 4;
   for (auto &tdc : tdcs) {
      if (cnt++>0) lbl.append(",");
      char sbuf[50];
      snprintf(sbuf, sizeof(sbuf), "0x%04X", tdc->GetID());
      lbl.append(sbuf);
      if (tdc->NumChannels() > maxnumch)
         maxnumch = tdc->NumChannels();
   }

   while (numtdc<4) {
      if (numtdc++ > 0) lbl.append(",");
      lbl.append("----");
   }

   std::string lbl2 = lbl + ";channels";
   std::string lbltot = std::string("clear_protect;") + lbl2;

   if (!fMsgPerBrd)
      fMsgPerBrd = MakeH1("MsgPerTDC", "Number of messages per TDC", numtdc, 0, numtdc, lbl.c_str());
   if (!fErrPerBrd)
      fErrPerBrd = MakeH1("ErrPerTDC", "Number of errors per TDC", numtdc, 0, numtdc, lbl.c_str());
   if (!fHitsPerBrd)
      fHitsPerBrd = MakeH1("HitsPerTDC", "Number of data hits per TDC", numtdc, 0, numtdc, lbl.c_str());
   if (!fCalHitsPerBrd)
      fCalHitsPerBrd = MakeH2("CalHitsPerTDC", "Number of calibration data hits per TDC", numtdc, 0, numtdc, maxnumch, 0, maxnumch, lbl2.c_str());
   if (!fToTPerBrd)
      fToTPerBrd = MakeH2("ToTPerTDC", "Calibrated delay value of falling edge", numtdc, 0, numtdc, maxnumch-1, 0, maxnumch-1, lbltot.c_str());

   cnt = 0;
   for (auto &tdc : tdcs) {
      tdc->AssignPerBrdHistos(this, cnt++);
      tdc->FillToTHistogram();
   }
}

//////////////////////////////////////////////////////////////////////////////
/// Pre loop function

void hadaq::TrbProcessor::UserPreLoop()
{
   if (fMap.size() > 0)
      CreatePerTDCHistos();

   // fProfiler.MakeStatistic();
}

//////////////////////////////////////////////////////////////////////////////
/// Post loop function

void hadaq::TrbProcessor::UserPostLoop()
{
   // fProfiler.MakeStatistic();
   // printf("TRB PROFILER: %s\n", fProfiler.Format().c_str());
}

//////////////////////////////////////////////////////////////////////////////
/// Checks if error can be print out

bool hadaq::TrbProcessor::CheckPrintError()
{
   if (fPrintErrCnt<=0) return false;

   if (--fPrintErrCnt==0) {
      printf("%5s Suppress all other errors ...\n", GetName());
      return false;
   }

   return true;
}

//////////////////////////////////////////////////////////////////////////////
/// Create up to 4 TDC with pre-configured default parameters

int hadaq::TrbProcessor::CreateTDC(unsigned id1, unsigned id2, unsigned id3, unsigned id4)
{
   // overwrite default value in the beginning

   int num = 0;

   for (unsigned cnt = 0; cnt < 4; cnt++) {
      unsigned tdcid = id1;
      switch (cnt) {
         case 1: tdcid = id2; break;
         case 2: tdcid = id3; break;
         case 3: tdcid = id4; break;
         default: tdcid = id1; break;
      }
      if (tdcid == 0) 
         continue;

      if (GetTDC(tdcid, true)) {
         printf("TDC id 0x%04x already exists\n", tdcid);
         continue;
      }

      if (tdcid == fHadaqCTSId) {
         printf("TDC id 0x%04x already used as CTS id\n", tdcid);
         continue;
      }

      if (std::find(fHadaqHUBId.begin(), fHadaqHUBId.end(), tdcid) != fHadaqHUBId.end()) {
         printf("TDC id 0x%04x already used as HUB id\n", tdcid);
         continue;
      }

      auto tdc = new hadaq::TdcProcessor(this, tdcid, GetNumCh(), gEdgesMask, false, fDogma);

      tdc->SetCalibrTriggerMask(fCalibrTriggerMask);

      num++;
   }

   return num;
}

//////////////////////////////////////////////////////////////////////////////
/// Create up to 4 TDC v5 with pre-configured default parameters

int hadaq::TrbProcessor::CreateTDC5(unsigned id1, unsigned id2, unsigned id3, unsigned id4)
{
   // overwrite default value in the beginning

   int num = 0;

   for (unsigned cnt = 0; cnt < 4; cnt++) {
      unsigned tdcid = id1;
      switch (cnt) {
         case 1: tdcid = id2; break;
         case 2: tdcid = id3; break;
         case 3: tdcid = id4; break;
         default: tdcid = id1; break;
      }
      if (tdcid == 0) 
         continue;

      if (GetTDC(tdcid, true)) {
         printf("TDC id 0x%06x already exists\n", tdcid);
         continue;
      }

      if (tdcid == fHadaqCTSId) {
         printf("TDC id 0x%06x already used as CTS id\n", tdcid);
         continue;
      }

      auto tdc = new hadaq::TdcProcessor(this, tdcid, GetNumCh(), gEdgesMask, 5, true);

      tdc->SetCalibrTriggerMask(fCalibrTriggerMask);

      num++;
   }

   return num;
}

//////////////////////////////////////////////////////////////////////////////
/// Set auto calibration for all existing TDCs

void hadaq::TrbProcessor::SetAutoCalibrations(long cnt)
{
   for (auto &entry : fMap) {
      if (entry.second->IsTDC())
        ((TdcProcessor*)entry.second)->SetAutoCalibration(cnt);
   }
}

//////////////////////////////////////////////////////////////////////////////
/// Disable calibration of specified channels for all existing TDCs

void hadaq::TrbProcessor::DisableCalibrationFor(unsigned firstch, unsigned lastch)
{
   for (auto &entry : fMap) {
      if (entry.second->IsTDC())
         ((TdcProcessor*)entry.second)->DisableCalibrationFor(firstch, lastch);
   }
}

//////////////////////////////////////////////////////////////////////////////
/// Set write calibration for all existing TDCs

void hadaq::TrbProcessor::SetWriteCalibrations(const char* fileprefix, bool every_time, bool use_linear)
{
   for (auto &entry : fMap) {
      if (entry.second->IsTDC())
         ((TdcProcessor*)entry.second)->SetWriteCalibration(fileprefix, every_time, use_linear);
   }
}

//////////////////////////////////////////////////////////////////////////////
/// Load calibrations for all existing TDCs
/// as argument file prefix (without TDC id) should be specified

bool hadaq::TrbProcessor::LoadCalibrations(const char* fileprefix)
{
   bool res = true;

   for (auto &entry : fMap) {
      if (!entry.second->IsTDC()) continue;
      TdcProcessor* tdc = (TdcProcessor*) entry.second;
      if (!tdc->LoadCalibration(fileprefix)) res = false;
   }

   return res;
}

//////////////////////////////////////////////////////////////////////////////
/// Configure calibration

void hadaq::TrbProcessor::ConfigureCalibration(const std::string& name, long period, unsigned trigmask)
{
   SetCalibrTriggerMask(trigmask);

   if (period > 0) SetAutoCalibrations(period);

   if (name.length() > 0) {
      LoadCalibrations(name.c_str());
      if ((period == -1) || (period == -77))
         SetWriteCalibrations(name.c_str(), false, (period == -77));
      else if (period > 1000)
         SetWriteCalibrations(name.c_str(), true, (period % 10000 == 77));
   }
}

//////////////////////////////////////////////////////////////////////////////
/// Set trigger ids mask which should be used for calibration
/// Value 0x3FF enables calibration for all kinds of events
/// Value `(1 << 0xD)` enable calibration for 0xD trigger

void hadaq::TrbProcessor::SetCalibrTriggerMask(unsigned trigmask)
{
   fCalibrTriggerMask = trigmask;

   for (auto &entry : fMap) {
      if (!entry.second->IsTDC()) continue;
      TdcProcessor* tdc = (TdcProcessor*) entry.second;
      tdc->SetCalibrTriggerMask(fCalibrTriggerMask);
   }
}

//////////////////////////////////////////////////////////////////////////////
/// Add sub-processor (like TDC or ADC)

void hadaq::TrbProcessor::AddSub(SubProcessor* sub, unsigned id)
{
   fMap[id] = sub;
}

//////////////////////////////////////////////////////////////////////////////
/// Configure store kind

void hadaq::TrbProcessor::SetStoreKind(unsigned kind)
{
   base::StreamProc::SetStoreKind(kind);

   for (auto &entry : fMap)
      entry.second->SetStoreKind(kind);
}

//////////////////////////////////////////////////////////////////////////////
/// Enable/disable ch0 store in output event for all TDC processors

void hadaq::TrbProcessor::SetCh0Enabled(bool on)
{
   for (auto &entry : fMap) {
      if (entry.second->IsTDC())
         ((TdcProcessor*)entry.second)->SetCh0Enabled(on);
   }
}

//////////////////////////////////////////////////////////////////////////////
/// Set trigger window for all TDCs

void hadaq::TrbProcessor::SetTriggerWindow(double left, double right)
{
   base::StreamProc::SetTriggerWindow(left, right);

   for (auto &entry : fMap) {
      entry.second->SetTriggerWindow(left, right);
      entry.second->CreateTriggerHist(16, 3000, -2e-6, 2e-6);
   }
}

//////////////////////////////////////////////////////////////////////////////
/// Assign current trigger id

void hadaq::TrbProcessor::AccountTriggerId(unsigned subid)
{
   if (fLastTriggerId==0) {
      fLostTriggerCnt = 0;
      fTakenTriggerCnt = 0;
   } else {
      unsigned diff = (subid - fLastTriggerId) & 0xffff;
      if ((diff > 0x8000) || (diff==0)) {
         // ignore this, most probable it is some mistake
      } else
      if (diff==1) {
         fTakenTriggerCnt++;
      } else {
         fLostTriggerCnt+=(diff-1);
      }
   }

   fLastTriggerId = subid;

   if (fTakenTriggerCnt + fLostTriggerCnt > 1000) {
      double lostratio = 1.*fLostTriggerCnt / (fTakenTriggerCnt + fLostTriggerCnt + 0.);
      DefFillH1(fLostRate, lostratio, 1.);
      fTakenTriggerCnt = 0;
      fLostTriggerCnt = 0;
   }
}

//////////////////////////////////////////////////////////////////////////////
/// First scan of buffer - main entry point for data

bool hadaq::TrbProcessor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null())
      return false;

   if (buf().kind == base::proc_DOGMAEvent)
      return DogmaBufferScan(buf);

//   RAWPRINT("TRB3 - first scan of buffer %u\n", buf().datalen);

   // printf("%s Scan TRB buffer size %u\n", GetName(), buf().datalen);

   hadaq::TrbIterator iter(buf().buf, buf().datalen);

   hadaqs::RawEvent *ev = nullptr;

   while ((ev = iter.nextEvent()) != nullptr) {

      if (ev->GetSize() > buf().datalen+4) {
         printf("Corrupted event size %u!\n", ev->GetSize());
         return true;
      }

      if (IsPrintRawData()) ev->Dump();

      DefFillH1(fEvSize, ev->GetPaddedSize(), 1.);

      BeforeEventScan();

      while (auto sub = iter.nextSubevent()) {

         if (sub->GetSize() > buf().datalen + 4) {
            printf("Corrupted subevent size %u!\n", sub->GetSize());
            return true;
         }

         if (IsPrintRawData()) sub->Dump(true);

         // use only 16-bit in trigger number while CTS make a lot of errors in higher 8 bits
         AccountTriggerId((sub->GetTrigNr() >> 8) & 0xffff);

         ScanSubEvent(sub, ev->GetRunNr(), ev->GetSeqNr());
      }

      AfterEventScan();

      AfterEventFill();
   }

   return true;
}

//////////////////////////////////////////////////////////////////////////////
/// First scan of DOGMA buffer - main entry point for data

bool hadaq::TrbProcessor::DogmaBufferScan(const base::Buffer &buf)
{
   char *curr = (char *) buf().buf;
   unsigned len = buf().datalen;

   while (curr && (len > sizeof(dogma::DogmaEvent))) {
      auto evnt = (dogma::DogmaEvent *) curr;

      unsigned evlen = evnt->GetEventLen();
      if (evlen > len) {
         printf("Failure with dogmal event length got %u maximum %u\n", evlen, len);
         return true;
      }
      curr += evlen;
      len -= evlen;

      DefFillH1(fEvSize, evlen, 1.);

      BeforeEventScan();

      // printf("Scan DOGMA event %u size %u\n", evnt->GetSeqId(), evlen);

      auto tu = evnt->FirstSubevent();
      while (tu) {

         auto trigtype = tu->GetTrigType() & 0xf;

         auto dataid = tu->GetAddr() & 0xffffff; // enforce 24 bit for tdc
         auto datalen = tu->GetPayloadLen();

         DefFillH1(fTrigType, trigtype, 1.);

         DefFillH1(fSubevSize, tu->GetSize(), 1.);

         for (auto &entry : fMap)
            entry.second->SetNewDataFlag(false);

         auto tdcproc = GetTDC(dataid, true);

         if (tdcproc) {

            uint32_t epoch0 = tu->GetTrigTime() & 0xfffffff;
            uint32_t coarse0 = tu->GetLocalTrigTime() & 0x7ff;

            // printf("  Tu 0x%x proc %p payloadlen %u epoch0 %07x coarse0 %03x\n", dataid, tdcproc, datalen, (unsigned) epoch0, (unsigned) coarse0);

            base::Buffer buf;
            buf.makenew((datalen + 2) * 4);
            uint32_t *ptr = (uint32_t *)buf.ptr();
            *ptr++ = epoch0;
            *ptr++ = coarse0;
            for (unsigned n = 0; n < datalen; ++n)
               *ptr++ = tu->GetPayload(n);
            buf().kind = trigtype;
            buf().boardid = dataid;
            buf().format = 3; // format with epoch0/corse0 and without ref channel

            tdcproc->AddNextBuffer(buf);
            tdcproc->SetNewDataFlag(true);
         } /*
         else if (fAutoCreate && (dataid >= gTDCMin) && (dataid <= gTDCMax) &&
            (datalen > 0) && TdcMessage(tu->GetPayload(0)).isHeaderMsg()) {
               // here should be channel/edge/min/max selection based on TDC design ID
               bool ver4 = TdcMessage(tu->GetPayload(0)).IsVer4Header();
               unsigned numch = GetNumCh(), edges = gEdgesMask;

               tdcproc = new TdcProcessor(this, dataid, numch, edges, ver4);

               tdcproc->SetCalibrTriggerMask(fCalibrTriggerMask);
               tdcproc->SetStoreKind(GetStoreKind());

               mgr()->UserPreLoop(tdcproc); // while loop already running, call it once again for new processor

               char msg[1000];
               snprintf(msg, sizeof(msg), "%s: Create %s 0x%04x nch:%u edges:%u", GetName(), ver4 ? "TDC4" : "TDC", dataid, numch, edges);
               mgr()->PrintLog(msg);

               // no need provide data in autocreate mode
         } */

         tu = evnt->NextSubevent(tu);
      }

      AfterEventScan();

      AfterEventFill();
   }

   return true;
}

//////////////////////////////////////////////////////////////////////////////
/// Function called before each event scan

void hadaq::TrbProcessor::BeforeEventScan()
{
   for (auto &entry : fMap)
      entry.second->BeforeFill();
}

//////////////////////////////////////////////////////////////////////////////
/// Function called after event scan - TDCs analyze data they got

void hadaq::TrbProcessor::AfterEventScan()
{
   // scan all new data
   for (auto &entry : fMap)
      entry.second->ScanNewBuffers();
}

//////////////////////////////////////////////////////////////////////////////
/// if SetCrossProcess() enabled - fill histograms across TDCs

void hadaq::TrbProcessor::AfterEventFill()
{
   // after scan data, fill extra histograms
   if (IsCrossProcess()) {
      for (auto &entry : fMap)
         entry.second->AfterFill(&fMap);
   }
}

//////////////////////////////////////////////////////////////////////////////
/// Cal SetCrossProcess for all existing TRB processors

void hadaq::TrbProcessor::SetCrossProcessAll()
{
   if (fHldProc)
      fHldProc->SetCrossProcess(true);
   else
      SetCrossProcess(true);
}

//////////////////////////////////////////////////////////////////////////////
/// Enable Cross-processing - hits correlation between different TDCs

void hadaq::TrbProcessor::SetCrossProcess(bool on)
{
   fCrossProcess = on;
   for (auto &entry : fMap)
      entry.second->fCrossProcess = on;
}

//////////////////////////////////////////////////////////////////////////////
/// Create branch in TTree to store hadaq::TrbMessage

void hadaq::TrbProcessor::CreateBranch(TTree*)
{
   if(mgr()->IsTriggeredAnalysis())
      mgr()->CreateBranch(GetName(), "hadaq::TrbMessage", (void**)&pMsg);
}

//////////////////////////////////////////////////////////////////////////////
/// Provide buffer to sub-processor

void hadaq::TrbProcessor::AddBufferToTDC(hadaqs::RawSubevent* sub,
                                         hadaq::SubProcessor* tdcproc,
                                         unsigned ix, unsigned datalen)
{
   if (datalen == 0) {
      //if (CheckPrintError())
      //   printf("Try to add empty buffer to %s\n", tdcproc->GetName());
      return;
   }

   base::Buffer buf;

   if (gIgnoreSync && (sub->Alignment() == 4)) {
      // special case - could use data directly without copying
      buf.makereferenceof((char*)sub->RawData() + 4*ix, 4*datalen);
      buf().kind = sub->GetTrigTypeTrb3();
      buf().boardid = tdcproc->GetID();
      buf().format = sub->IsSwapped() ? 2 : 1; // special format without sync
   } else {
      buf.makenew((datalen+1)*4);
      memset(buf.ptr(), 0xff, 4); // fill dummy sync id in the begin
      sub->CopyDataTo(buf.ptr(4), ix, datalen);

      buf().kind = sub->GetTrigTypeTrb3();
      buf().boardid = tdcproc->GetID();
      buf().format = 0;
   }

   tdcproc->AddNextBuffer(buf);
   tdcproc->SetNewDataFlag(true);
}

//////////////////////////////////////////////////////////////////////////////
/// Add event-related error message - adds event info

void hadaq::TrbProcessor::EventError(const char *msg)
{
   char sbuf[1000];
   snprintf(sbuf, sizeof(sbuf), "Run: %u Ev: 0x%08x Msg: %s", fCurrentRunId, fCurrentEventId, msg);
   mgr()->AddErrLog(sbuf);
}

//////////////////////////////////////////////////////////////////////////////
/// Add event-related log message - adds event info

void hadaq::TrbProcessor::EventLog(const char *msg)
{
   char sbuf[1000];
   snprintf(sbuf, sizeof(sbuf), "Ev: %08x Msg: %s", fCurrentEventId, msg);
   mgr()->AddRunLog(sbuf);
}

//////////////////////////////////////////////////////////////////////////////
/// Scan subevent
///
/// this is first scan of subevent from TRB3 data
/// our task is statistic over all messages we will found
/// also for trigger-type 1 we should add SYNC message to each processor

void hadaq::TrbProcessor::ScanSubEvent(hadaqs::RawSubevent* sub, unsigned trb3runid, unsigned trb3seqid)
{
   memcpy((void *) &fLastSubevHdr, sub, sizeof(fLastSubevHdr));
   fCurrentRunId = trb3runid;
   fCurrentEventId = sub->GetTrigNr();

   DefFillH1(fTrigType, sub->GetTrigTypeTrb3(), 1.);

   DefFillH1(fSubevSize, sub->GetSize(), 1.);

   for (auto &entry : fMap)
      entry.second->SetNewDataFlag(false);

   unsigned ix = 0;           // cursor

   unsigned trbSubEvSize = sub->GetSize() / 4 - 4;

   if (trbSubEvSize>100000) printf("LARGE subevent %u\n", trbSubEvSize);

   fMsg.fTrigSyncIdFound = false;

   bool did_create_tdc = false;

   if ((fMinTdc == 0) && (fMaxTdc == 0) && (fTdcsVect.size() == 0) && (fMap.size() > 0)) {
      BuildFastTDCVector();
   }

   bool get_fast = fMaxTdc > fMinTdc;

   unsigned maxhublen = 0, lasthubid = 0; // if saw HUB subsubevents, control size of data inside

//   RAWPRINT("Scan TRB3 raw event 4-bytes size %u\n", trbSubEvSize);

   bool standalone_subevnt = (sub->GetDecoding() & hadaqs::EvtDecoding_AloneSubevt) != 0;

   while (ix < trbSubEvSize) {

      uint32_t data;
      unsigned datalen, dataid;

      if(standalone_subevnt && (ix == 0)) {
         data = 0; // unused
         datalen = trbSubEvSize > 2 ? trbSubEvSize - 2 : 0; // whole sub event beside last 2 words which should be 0xx5555
         dataid = sub->GetId();  // inherit ID
      } else {
         ///<! Extract data portion from the whole packet (in a loop)
         data = sub->Data(ix++);
         datalen = (data >> 16) & 0xFFFF;
         dataid = data & 0xFFFF;
      }

//      RAWPRINT("Subevent id 0x%04x len %u\n", (data & 0xFFFF), datalen);

      // ===========  this is header for TDC, build inside the TRB3 =================
//      if ((data & 0xFF00) == 0xB000) {
//         // do that ever you want
//         unsigned brdid = data & 0xFF;
//         RAWPRINT("    TRB3-TDC header: 0x%08x, TRB3-buildin TDC-FPGA brd=%u, size=%u  IGNORED\n", (unsigned) data, brdid, datalen);
//         ix+=datalen;
//         continue;
//      }

      if (std::find(fHadaqHUBId.begin(), fHadaqHUBId.end(), dataid) != fHadaqHUBId.end()) {
         RAWPRINT ("   HUB header: 0x%08x, hub=0x%04x, size=%u (ignore)\n", (unsigned) data, (unsigned) dataid, datalen);

         if (maxhublen == 0) {
            maxhublen = datalen;
         } else {
            maxhublen--; // just decrement
         }

         lasthubid = dataid;
         // ix+=datalen;  // WORKAROUND !!!

         // TODO: formally we should analyze HUB subevent as real subevent but
         // we just skip header and continue to analyze data
         continue;
      }

      if (maxhublen > 0) {
         if (datalen >= maxhublen) {
            if (CheckPrintError()) printf("error: sub-sub event %04x inside HUB %04x exceed size limit\n", dataid, lasthubid);
            datalen = maxhublen-1;
         }
         maxhublen -= (datalen+1);
      }

      ///<! ==================== CTS header and inside ================
      if (dataid == fHadaqCTSId) {
         RAWPRINT("   CTS header: 0x%x, size=%d\n", (unsigned) data, datalen);
         //hTrbTriggerCount->Fill(5);          ///<! TRB - CTS
         //hTrbTriggerCount->Fill(0);          ///<! TRB TOTAL

         data = sub->Data(ix++); datalen--;
         unsigned trigtype = (data & 0xFFFF);

         unsigned nInputs = (data >> 16) & 0xf;
         unsigned nTrigChannels = (data >> 20) & 0xf;
         unsigned bIncludeLastIdle = (data >> 25) & 0x1;
         unsigned bIncludeCounters = (data >> 26) & 0x1;
         unsigned bIncludeTimestamp = (data >> 27) & 0x1;
         unsigned nExtTrigFlag = (data >> 28) & 0x3;

         unsigned nCTSwords = nInputs*2 + nTrigChannels*2 +
                              bIncludeLastIdle*2 + bIncludeCounters*3 + bIncludeTimestamp*1;

         // corrupted data - fully ignore them
         if (nCTSwords > datalen) {
            RAWPRINT("Error: Corrupted CTS data, check system config\n");
            continue;
         }

         RAWPRINT("     CTS trigtype: 0x%04x, nExtTrigFlag: 0x%02x, datalen: %u, nCTSwords: %u\n", trigtype, nExtTrigFlag, datalen, nCTSwords);

         // we skip all the information from the CTS right now,
         // we don't need it
         ix += nCTSwords;
         datalen -= nCTSwords;
         // ix should now point to the first ETM word


         // handle the ETM module and extract syncNumber
         if(nExtTrigFlag == 0x1) {
            // ETM sends one word, is probably MBS Vulom Recv
            // this is not really tested
            data = sub->Data(ix++); datalen--;
            fMsg.fTrigSyncId = (data & 0xFFFFFF);
            fMsg.fTrigSyncIdStatus = data >> 24; // untested
            fMsg.fTrigSyncIdFound = true;
            // TODO: evaluate the upper 8 bits in data for status/error
         } else if(nExtTrigFlag==0x2) {
            // ETM sends four words, is probably a Mainz A2 recv
            data = sub->Data(ix++); datalen--;
            fMsg.fTrigSyncId = data; // full 32bits is trigger number
            // get status word
            data = sub->Data(ix++); datalen--;
            fMsg.fTrigSyncIdStatus = data;
            // word 3+4 are 0xdeadbeef i.e. not used at the moment, so skip it
            ix += 2;
            datalen -= 2;
            // success
            fMsg.fTrigSyncIdFound = true;
            //printf("EventId=%d\n",fMsg.fTrigSyncId);
         } else if(nExtTrigFlag == 0x0) {

            fMsg.Reset();

            if (sub->Data(ix) == 0xabad1dea) {
               // [1]: D[31:16] -> sync pulse number
               //      D[15:0]  -> absolute time D[47:32]
               // [2]: D[31:0]  -> absolute time D[31:0]
               // [3]: D[31:0]  -> period of sync pulse, in 10ns units
               // [4]: D[31:0]  -> length of sync pulse, in 10ns units

               fMsg.fTrigSyncIdFound = true;
               fMsg.fTrigSyncId = (sub->Data(ix+1) >> 16) & 0xffff;
               fMsg.fTrigSyncIdStatus = 0;
               fMsg.fTrigTm = (((uint64_t) (sub->Data(ix+1) & 0xffff)) << 32) | sub->Data(ix+2);
               fMsg.fSyncPulsePeriod = sub->Data(ix+3);
               fMsg.fSyncPulseLength = sub->Data(ix+4);

               ix += 5;
               datalen -= 5;
            } else {
               RAWPRINT("Error: Unknown nExtTrigFlag 0x%x in CTS subevent\n", nExtTrigFlag);
            }
         } else {
            RAWPRINT("Error: Unknown ETM in CTS header found: %x\n", nExtTrigFlag);
            // TODO: try to do some proper error recovery here...
         }

         if(fMsg.fTrigSyncIdFound)
            RAWPRINT("     Find SYNC %u\n", (unsigned) fMsg.fTrigSyncId);

         // FIXME: workaround for latest data
         if ((datalen > 1) && (sub->Data(ix) == 0x80000000)) {
            ix++;
            datalen--;
         }

         // now ix should point to the first TDC word if datalen>0
         // if not, there is no TDC present

         // This is special TDC processor for data from CTS header

         if (datalen > 0) {

            TdcProcessor* tdcproc = GetTDC(fHadaqCTSId, true);
            if (tdcproc) {
               // if TDC processor found, process such data as normal TDC data
               AddBufferToTDC(sub, tdcproc, ix, datalen);
            } else if (fAutoCreate && (datalen > 2) && TdcMessage(sub->Data(ix)).isHeaderMsg()) {

               // here should be channel/edge/min/max selection based on TDC design ID
               bool ver4 = TdcMessage(sub->Data(ix)).IsVer4Header();
               unsigned numch = GetNumCh(), edges = gEdgesMask;

               tdcproc = new TdcProcessor(this, fHadaqCTSId, numch, edges, ver4);

               tdcproc->SetCalibrTriggerMask(fCalibrTriggerMask);
               tdcproc->SetStoreKind(GetStoreKind());

               mgr()->UserPreLoop(tdcproc); // while loop already running, call it once again for new processor

               did_create_tdc = true;

               printf("%s: Create %s in CTS 0x%04x nch:%u edges:%u\n", GetName(), ver4 ? "TDC4" : "TDC", dataid, numch, edges);
            }
         }

         // don't forget to skip the words for the TDC (if any)
         ix += datalen;
         if (standalone_subevnt) break;

         continue;
      }

      TdcProcessor *tdcproc = nullptr;
      if (get_fast) {
         if ((dataid >= fMinTdc) && (dataid < fMaxTdc)) tdcproc = fTdcsVect[dataid-fMinTdc];
      } else {
         tdcproc = GetTDC(dataid, true);
      }

      ///<! ================= FPGA TDC header ========================
      if (tdcproc) {
         RAWPRINT("   FPGA-TDC header: 0x%08x, tdcid=0x%04x, size=%u\n", (unsigned) data, dataid, datalen);

         if (IsPrintRawData()) {
            TdcIterator iter;
            iter.assign(sub, ix, datalen);
            while (iter.next()) iter.printmsg();
         }

         AddBufferToTDC(sub, tdcproc, ix, datalen);

         ix += datalen;
         if (standalone_subevnt) break;
         continue; // go to next block
      }  // end of if TDC header


      ///<! ==================  Dummy header and inside ==========================
      if (dataid == 0x5555) {
         RAWPRINT("   Dummy header: 0x%x, size=%d\n", (unsigned) data, datalen);
         //hTrbTriggerCount->Fill(4);          ///<! TRB - DUMMY
         //hTrbTriggerCount->Fill(0);          ///<! TRB TOTAL
         bool first = true;
         while (datalen-- > 0) {
            ///<! In theory here must be only one word - termination package with the status
            data = sub->Data(ix++);
            RAWPRINT("      word: 0x%08x\n", (unsigned) data);

            if (first) {
               for(int bit=0; bit<32; ++bit)
                  if(data & (1 << bit))
                     DefFastFillH1(fErrBits, bit, 1.);
            }
            first = false;

            //uint32_t fSubeventStatus = data;
            //if (fSubeventStatus != 0x00000001) { bad events}
         }

         continue;
      }

      auto iter = fMap.find(dataid);
      SubProcessor* subproc = iter != fMap.end() ? iter->second : nullptr;

      ///<! ================= any other header ========================
      if (subproc) {
         RAWPRINT ("   SUB header: 0x%08x, id=0x%04x, size=%u\n", (unsigned) data, dataid, datalen);

         if(datalen == 0) {
            if (standalone_subevnt) break;
            continue;
         }

         base::Buffer buf;

         // check if this processor has some attached TDC
         unsigned  offset = 0;
         auto iter_tdc = fMap.find(dataid | 0xff0000);
         if(iter_tdc != fMap.end()) {
            // pre-scan for begin marker of non-TDC data
            for(unsigned i=0;i<datalen;i++) {
               unsigned data_ = sub->Data(ix+i);
               if(data_ >> 28 == 0x1) {
                  offset = i;
                  break;
               }
            }
            if(offset>0)
               AddBufferToTDC(sub, iter_tdc->second, ix, offset);
         }

         datalen -= offset;
         ix += offset;

         buf.makenew(datalen*4);

         sub->CopyDataTo(buf.ptr(0), ix, datalen);

         buf().kind = 0;
         buf().boardid = dataid;
         buf().format = 0;

         subproc->AddNextBuffer(buf);
         subproc->SetNewDataFlag(true);

         ix += datalen;
         if (standalone_subevnt) break;
         continue; // go to next block
      }  // end of if SUB header

      if (fAutoCreate) {
         if ((dataid >= gHUBMin) && (dataid <= gHUBMax)) {
            // suppose this is HUB
            AddHadaqHUBId(dataid);
            printf("%s: Assign HUB 0x%04x\n", GetName(), dataid);

            maxhublen = datalen;
            lasthubid = dataid;

            // continue processing
            continue;
         } else if ((dataid >= gTDCMin) && (dataid <= gTDCMax)) {

            bool is_hades = TdcProcessor::GetHadesMonitorInterval() > 0;

            // suppose this is TDC data, first word should be TDC header
            if (((datalen > 0) && TdcMessage(sub->Data(ix)).isHeaderMsg()) || ((datalen == 0) && is_hades)) {
               // here should be channel/edge/min/max selection based on TDC design ID
               bool ver4 = (datalen > 0) && TdcMessage(sub->Data(ix)).IsVer4Header();
               unsigned numch = GetNumCh(), edges = gEdgesMask;

               TdcProcessor* tdcproc = new TdcProcessor(this, dataid, numch, edges, ver4);

               tdcproc->SetCalibrTriggerMask(fCalibrTriggerMask);
               tdcproc->SetStoreKind(GetStoreKind());

               mgr()->UserPreLoop(tdcproc); // while loop already running, call it once again for new processor

               did_create_tdc = true;

               char msg[1000];
               snprintf(msg, sizeof(msg), "%s: Create %s 0x%04x nch:%u edges:%u", GetName(), ver4 ? "TDC4" : "TDC", dataid, numch, edges);
               mgr()->PrintLog(msg);

               // in auto-create mode buffers are not processed - normally it is only first event
               // AddBufferToTDC(sub, tdcproc, ix, datalen);
               ix += datalen;
               continue; // go to next block
            } else {
               if (CheckPrintError())
                  printf("sub-sub-event data with id 0x%04x does not belong to TDC\n", dataid);
            }
         } else if ((dataid >= gMDCMin) && (dataid < gMDCMax)) {
            auto mdcproc = new MdcProcessor(this, dataid);

            mdcproc->SetStoreKind(GetStoreKind());

            mgr()->UserPreLoop(mdcproc); // while loop already running, call it once again for new processor

            char msg[1000];
            snprintf(msg, sizeof(msg), "%s: Create MDC 0x%04x", GetName(), dataid);
            mgr()->PrintLog(msg);

            // in auto-create mode buffers are not processed - normally it is only first event
            ix += datalen;
            continue; // go to next block
         } else {
            printf("%s: Saw ID 0x%04x in autocreate mode\n", GetName(), dataid);
         }
      }

      RAWPRINT("Unknown header 0x%04x length %u in TRB 0x%04x subevent\n", data & 0xFFFF, datalen, GetID());
      ix += datalen;
      if (standalone_subevnt) break;
   }

   if (did_create_tdc) {
      CreatePerTDCHistos();
      ClearFastTDCVector();
   }

   // auto creation works only for one event
   if (fAutoCreate)
      fAutoCreate = false;

   if (fUseTriggerAsSync) {
      fMsg.fTrigSyncIdFound = true;
      fMsg.fTrigSyncId = trb3seqid;
      fMsg.fTrigSyncIdStatus = 0; // dummy
   }

   if (fMsg.fTrigSyncIdFound && !gIgnoreSync) {
      for (auto &entry : fMap) {
         if (entry.second->IsNewDataFlag())
            entry.second->AppendTrbSync(fMsg.fTrigSyncId);
      }
   }
}


//////////////////////////////////////////////////////////////////////////////
/// Using data structure in raw event, collect ids of missing TDCs

bool hadaq::TrbProcessor::CollectMissingTDCs(hadaqs::RawSubevent *sub, std::vector<unsigned> &ids)
{
   unsigned ix = 0; // cursor

   unsigned trbSubEvSize = (sub->GetSize() - sizeof(hadaqs::RawSubevent)) / 4;

   while (ix < trbSubEvSize) {
      ///<! Extract data portion from the whole packet (in a loop)
      uint32_t data = sub->Data(ix++);

      uint32_t dataid = data & 0xFFFF;

      uint32_t datalen = (data >> 16) & 0xFFFF;

      if (std::find(fHadaqHUBId.begin(), fHadaqHUBId.end(), dataid) != fHadaqHUBId.end()) {
         // TODO: formally we should analyze HUB subevent as real subevent but
         // we just skip header and continue to analyze data
         continue;
      }

      TdcProcessor* subproc = GetTDC(dataid, true);

      if (!subproc && (dataid != 0x5555)) {
         ids.emplace_back(dataid);
      }

      // raw data is not interesting
      ix += datalen;
   }

   return ids.size() > 0;
}

//////////////////////////////////////////////////////////////////////////////
/// Build vector of TDCs processor for fast access

void hadaq::TrbProcessor::BuildFastTDCVector()
{
   bool isany = false;
   fMinTdc = 0xffffff;
   fMaxTdc = 0;

   for (auto &entry : fMap) {
      if (entry.second->IsTDC()) {
         isany = true;
         if (entry.first > fMaxTdc) fMaxTdc = entry.first;
         if (entry.first < fMinTdc) fMinTdc = entry.first;
      }
   }

   if (!isany || (fMaxTdc-fMinTdc > 0xffff)) {
      fMinTdc = fMaxTdc = 1;
   } else {
      fMaxTdc++;
      fTdcsVect.resize(fMaxTdc - fMinTdc, nullptr);
      for (auto &entry : fMap)
         if (entry.second->IsTDC())
            fTdcsVect[entry.first - fMinTdc] = static_cast<hadaq::TdcProcessor *>(entry.second);
   }
}

//////////////////////////////////////////////////////////////////////////////
/// Clear helper vector of TDCs processor for fast access

void hadaq::TrbProcessor::ClearFastTDCVector()
{
   fTdcsVect.clear();
   fMinTdc = fMaxTdc = 0;
}

//////////////////////////////////////////////////////////////////////////////
/// Transform (calibrate) raw data
/// Creates output HLD structure, used in HADES DAQ

unsigned hadaq::TrbProcessor::TransformSubEvent(hadaqs::RawSubevent *sub, void *tgtbuf, unsigned tgtlen, bool only_hist, std::vector<unsigned> *newids)
{
//   base::ProfilerGuard grd(fProfiler, "enter");

   unsigned trig_type = sub->GetTrigTypeTrb3(), sz = sub->GetSize();

   if (only_hist && (GetID() == 0x8800)) {
      unsigned wordNr = 2;
      uint32_t bitmask = 0xff000000; /* extended mask to contain spill on/off bit*/
      uint32_t bitshift = 24;
      // above from args.c defaults
      uint32_t val = sub->Data(wordNr - 1);
      trig_type = (val & bitmask) >> bitshift;
   }

   DefFastFillH1(fTrigType, (trig_type & 0xF), 1.);

   if (sz < fSubevHLen)
      DefFastFillH1(fSubevSize, sz / fSubevHDiv, 1.);

   // JAM2018: add errorbit statistics
   uint32_t error_word = sub->GetErrBits();
   for(int bit=0; bit<32; ++bit)
      if(error_word & (1 << bit))
         DefFastFillH1(fErrBits, bit, 1.);

   // only fill histograms
   if (only_hist) return 0;

   if ((fMinTdc == 0) && (fMaxTdc == 0) && (fTdcsVect.size()==0) && (fMap.size() > 0))
      BuildFastTDCVector();

   bool get_fast = fMaxTdc > fMinTdc;

   // !!! DEBUG ONLY - just copy data
   // if (tgtbuf && tgtlen) {
   //   printf("SUBEVENT swap:%u len:%u\n", sub->IsSwapped(), sub->GetPaddedSize());
   //   memcpy((void *) tgtbuf, (void *) sub, sub->GetPaddedSize());
   //   return sub->GetPaddedSize();
   // }

//   grd.Next("hdr");

   hadaqs::RawSubevent *tgt = (hadaqs::RawSubevent *) tgtbuf;
   // copy complete header first
   if (tgt) {
      // copy header
      memcpy((void *) tgt, sub, sizeof(hadaqs::RawSubevent));
      tgtlen = (tgtlen - sizeof(hadaqs::RawSubevent)) / 4; // how many 32-bit values can be used
   }

   unsigned ix = 0, tgtix = 0; // cursor

   unsigned trbSubEvSize = (sub->GetSize() - sizeof(hadaqs::RawSubevent)) / 4;

   uint32_t *rawdata = (uint32_t *) sub->RawData();

   uint32_t data;
   unsigned datalen, id;

   bool standalone_subevnt = (sub->GetDecoding() & hadaqs::EvtDecoding_AloneSubevt) != 0;

   while (ix < trbSubEvSize) {

//      grd.Next("sub", 5);

      if (standalone_subevnt && (ix == 0)) {
         data = 0; // unused
         datalen = trbSubEvSize > 2 ? trbSubEvSize - 2 : 0; // whole subevent beside last two word which should be 0x5555
         id = sub->GetId();
      } else {
         data = rawdata[ix++];
         data = HADAQ_SWAP4(data);
         datalen = (data >> 16) & 0xFFFF;
         id = data & 0xFFFF;
      }

      if (tgt && (tgtix >= tgtlen)) {
         fprintf(stderr,"TrbProcessor::TransformSubEvent not enough space in output buffer\n");
         return 0;
      }

      if (fHadaqHUBId.size() > 0)
         if (std::find(fHadaqHUBId.begin(), fHadaqHUBId.end(), id) != fHadaqHUBId.end()) {
            // ix+=datalen;  // WORKAROUND !!!

            // copy hub header to the target
            if (tgt && !standalone_subevnt) tgt->SetData(tgtix++, data);

            // TODO: formally we should analyze HUB subevent as real subevent but
            // we just skip header and continue to analyze data
            continue;
         }

//      grd.Next("get");

      ///<! ================= FPGA TDC header ========================
      TdcProcessor *subproc = nullptr;
      if (get_fast) {
         if ((id >= fMinTdc) && (id < fMaxTdc)) subproc = fTdcsVect[id-fMinTdc];
      } else {
         subproc = GetTDC(id, true);
      }

      if (subproc) {
//         grd.Next("trans");

         if (standalone_subevnt && (ix == 0)) {
            unsigned newlen = subproc->TransformTdcData(sub, rawdata, ix, datalen, tgt, tgtix);
            if (tgt) tgtix += newlen;
         } else {
            unsigned newlen = subproc->TransformTdcData(sub, rawdata, ix, datalen, tgt, tgtix+1);
            if (tgt) {
               tgt->SetData(tgtix++, id | ((newlen & 0xffff) << 16)); // set sub-sub header
               tgtix += newlen;
            }
         }
         ix += datalen;
         if (standalone_subevnt) break;
         continue; // go to next block
      }  // end of if TDC header

      if (newids && (id != 0x5555))
         newids->emplace_back(id);

//      grd.Next("unrec", 10);

      // copy unrecognized data to the target
      if (tgt) {
         if (!standalone_subevnt || (ix != 0))
            tgt->SetData(tgtix++, data);
         memcpy(tgt->RawData(tgtix), sub->RawData(ix), datalen*4);
         tgtix += datalen;
      }

      // all other blocks are ignored
      ix += datalen;
      if (standalone_subevnt) break;
   }

//   grd.Next("finish", 15);

   if (tgt) {
      tgt->SetSize(sizeof(hadaqs::RawSubevent) + tgtix*4);
      return tgt->GetPaddedSize();
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////
/// Emulate transform (calibrate) raw data - only for debugging

unsigned hadaq::TrbProcessor::EmulateTransform(hadaqs::RawSubevent *sub, int dummycnt, bool only_hist)
{
   DefFillH1(fTrigType, 0x1, 1.);

   DefFillH1(fSubevSize, sub->GetSize(), 1.);

   DefFillH1(fErrBits, 0, 1.);

   // only fill histograms
   if (only_hist) return 0;

   for (unsigned indx=0;indx<NumberOfTDC();++indx) {
      hadaq::TdcProcessor *tdc = GetTDCWithIndex(indx);

      tdc->EmulateTransform(dummycnt);
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////
/// Clear all DAQ-related histograms

void hadaq::TrbProcessor::ClearDAQHistos()
{
   ClearH1(fEvSize);
   ClearH1(fSubevSize);
   ClearH1(fLostRate);
   ClearH1(fTrigType);
   ClearH1(fErrBits);

   ClearH1(fMsgPerBrd);
   ClearH1(fErrPerBrd);
   ClearH1(fHitsPerBrd);
   ClearH2(fCalHitsPerBrd);
   // ClearH2(fToTPerBrd); // do not clear ToT histogram
}
