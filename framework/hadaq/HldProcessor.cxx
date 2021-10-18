#include "hadaq/HldProcessor.h"

#include <cstring>
#include <cstdio>
#include <cmath>
#include <time.h>
#include <iostream>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbIterator.h"
#include "hadaq/TrbProcessor.h"

#define RAWPRINT( args ...) if(IsPrintRawData()) printf( args )

////////////////////////////////////////////////////////////////////////////////////
/// constructor
/// \param auto_create enables automatic TRB and TDC creation based on setting,
///    see \ref hadaq::TrbProcessor::SetTDCRange and \ref hadaq::TrbProcessor::SetHUBRange
/// \param after_func function name which will be called after all TRBs and TDCs are created

hadaq::HldProcessor::HldProcessor(bool auto_create, const char* after_func) :
   base::StreamProc("HLD", 0, false),
   fMap(),
   fEventTypeSelect(0xfffff),
   fPrintRawData(false),
   fAutoCreate(auto_create),
   fAfterFunc(after_func==0 ? "" : after_func),
   fCalibrName(),
   fCalibrPeriod(-111),
   fCalibrTriggerMask(0xFFFF),
   fMsg(),
   pMsg(0),
   fLastEvHdr(),
   fLastHadesTm(0)
{
   mgr()->RegisterProc(this, base::proc_TRBEvent, 0);

   fEvType = MakeH1("EvType", "Event type", 16, 0, 16, "id");
   fEvSize = MakeH1("EvSize", "Event size", 500, 0, 50000, "bytes");
   fSubevSize = MakeH1("SubevSize", "Subevent size", 500, 0, 5000, "bytes");

   fHitsPerTDC = nullptr;   // HADAQ hits per TDC
   fErrPerTDC = nullptr;    // HADAQ errors per TDC
   fHitsPerTDCChannel = nullptr; // HADAQ hits per TDC channel
   fErrPerTDCChannel = nullptr;  // HADAQ errorsvper TDC channel
   fCorrPerTDCChannel = nullptr; // corrections
   fQaFinePerTDCChannel = nullptr;  // HADAQ QA fine time per TDC channel
   fQaToTPerTDCChannel = nullptr;  // HADAQ QA ToT per TDC channel
   fQaEdgesPerTDCChannel = nullptr;  // HADAQ QA edges per TDC channel
   fQaErrorsPerTDCChannel = nullptr;  // HADAQ QA errors per TDC channel
   fQaSummary = nullptr; // HADAQ QA summary histogramm
   
    fToTPerTDCChannel = nullptr;  ///< HADAQ ToT per TDC channel, real values
    fShiftPerTDCChannel = nullptr;  ///< HADAQ calibrated shift per TDC channel, real values
    fExpectedToTPerTDC = nullptr;  ///< HADAQ expected ToT per TDC sed for calibration
    fDevPerTDCChannel = nullptr;  ///< HADAQ ToT deviation per TDC channel from calibration
          
   
   
   
   // printf("Create HldProcessor %s\n", GetName());

   // this is raw-scan processor, therefore no synchronization is required for it
   SetSynchronisationKind(sync_None);

   // only raw scan, data can be immediately removed
   SetRawScanOnly();
}

////////////////////////////////////////////////////////////////////////////////////
/// destructor

hadaq::HldProcessor::~HldProcessor()
{
}

////////////////////////////////////////////////////////////////////////////////////////
/// Adds TRB processor

void hadaq::HldProcessor::AddTrb(TrbProcessor* trb, unsigned id)
{
   fMap[id] = trb;
}

////////////////////////////////////////////////////////////////////////////////////////
/// Return number of TRBs

unsigned hadaq::HldProcessor::NumberOfTRB() const
{
   return fMap.size();
}

////////////////////////////////////////////////////////////////////////////////////////
/// Get TRB by index

hadaq::TrbProcessor* hadaq::HldProcessor::GetTRB(unsigned indx) const
{
   for (TrbProcMap::const_iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      if (indx==0) return iter->second;
      indx--;
   }
   return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
/// Find TRB by id

hadaq::TrbProcessor* hadaq::HldProcessor::FindTRB(unsigned trbid) const
{
   TrbProcMap::const_iterator iter = fMap.find(trbid);
   return iter == fMap.end() ? 0 : iter->second;
}


////////////////////////////////////////////////////////////////////////////////////////
/// Return number of TDCs in all TRBs

unsigned hadaq::HldProcessor::NumberOfTDC() const
{
   unsigned num = 0;
   for (TrbProcMap::const_iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      num += iter->second->NumberOfTDC();
   return num;
}

////////////////////////////////////////////////////////////////////////////////////////
/// Get TDC by index

hadaq::TdcProcessor* hadaq::HldProcessor::GetTDC(unsigned indx) const
{
   for (TrbProcMap::const_iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      unsigned num = iter->second->NumberOfTDC();
      if (indx < num) return iter->second->GetTDCWithIndex(indx);
      indx -= num;
   }

   return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
/// Find TDC by id

hadaq::TdcProcessor* hadaq::HldProcessor::FindTDC(unsigned tdcid) const
{
   for (TrbProcMap::const_iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      hadaq::TdcProcessor* res = iter->second->GetTDC(tdcid, true);
      if (res!=0) return res;
   }
   return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
/// Configure calibration modes
/// \param fileprefix is filename prefix for calibration files (empty string - no file I/O)
/// \param period is hits count for autocalibration
///   - 0 - only load calibration
///   - -1 - accumulate data and store calibrations only at the end
///   - -77 - accumulate data and produce liner calibrations only at the end
///   - >0 - automatic calibration after N hits in each active channel
///         if value ends with 77 like 10077 linear calibration will be calculated
/// \param trigmask is trigger type mask used for calibration
///  - (1 << 0xD) - special 0XD trigger with internal pulser, used also for TOT calibration
///  - 0x3FFF - all kinds of trigger types will be used for calibration (excluding 0xE and 0xF)
///  - 0x80000000 in mask enables usage of temperature correction

void hadaq::HldProcessor::ConfigureCalibration(const std::string& fileprefix, long period, unsigned trigmask)
{
   fCalibrName = fileprefix;
   fCalibrPeriod = period;
   fCalibrTriggerMask = trigmask;
   for (TrbProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->ConfigureCalibration(fileprefix, period, trigmask);
}

////////////////////////////////////////////////////////////////////////////////////////
/// Set trigger window not only for itself, but for all subprocessors

void hadaq::HldProcessor::SetTriggerWindow(double left, double right)
{
   base::StreamProc::SetTriggerWindow(left, right);

   for (TrbProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetTriggerWindow(left, right);
}

////////////////////////////////////////////////////////////////////////////////////////
/// Set store kind for all sub processors

void hadaq::HldProcessor::SetStoreKind(unsigned kind)
{
   base::StreamProc::SetStoreKind(kind);

   for (TrbProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetStoreKind(kind);
}

////////////////////////////////////////////////////////////////////////////////////////
/// Configure printing raw data in all sub-processors

void hadaq::HldProcessor::SetPrintRawData(bool on)
{
   fPrintRawData = on;
   for (TrbProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetPrintRawData(on);
}


////////////////////////////////////////////////////////////////////////////////////////
/// Create branch in TTree to store provided data - \ref hadaq::HldMessage

void hadaq::HldProcessor::CreateBranch(TTree*)
{
   if(mgr()->IsTriggeredAnalysis()) {
      pMsg = &fMsg;
      mgr()->CreateBranch(GetName(), "hadaq::HldMessage", (void**)&pMsg);
   }
}

////////////////////////////////////////////////////////////////////////////////////////
/// Perform scan of data in the buffer
/// Central entry point for all analysis

bool hadaq::HldProcessor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

//   RAWPRINT("TRB3 - first scan of buffer %u\n", buf().datalen);

   hadaq::TrbIterator iter(buf().buf, buf().datalen);

   hadaqs::RawEvent* ev = nullptr;

   unsigned evcnt = 0;

   while ((ev = iter.nextEvent()) != nullptr) {

      evcnt++;

      DefFillH1(fEvType, (ev->GetId() & 0xf), 1.);

      memcpy((void *)&fLastEvHdr, ev, sizeof(fLastEvHdr));

      fMsg.trig_type = ev->GetId() & 0xf;
      fMsg.run_nr = ev->GetRunNr();
      fMsg.seq_nr = ev->GetSeqNr();

      if ((fEventTypeSelect <= 0xf) && ((ev->GetId() & 0xf) != fEventTypeSelect)) continue;

      if (IsPrintRawData()) ev->Dump();

      DefFillH1(fEvSize, ev->GetPaddedSize(), 1.);

      for (TrbProcMap::iterator diter = fMap.begin(); diter != fMap.end(); diter++)
         diter->second->BeforeEventScan();

      hadaqs::RawSubevent* sub = nullptr;

      while ((sub = iter.nextSubevent()) != nullptr) {

         if (IsPrintRawData()) sub->Dump(true);

         DefFillH1(fSubevSize, sub->GetSize(), 1.);

         // use only 16-bit in trigger number while CTS make a lot of errors in higher 8 bits
         // AccountTriggerId((sub->GetTrigNr() >> 8) & 0xffff);

         TrbProcMap::iterator iter = fMap.find(sub->GetId());

         if (iter != fMap.end())
            iter->second->ScanSubEvent(sub, fMsg.run_nr, fMsg.seq_nr);
         else
         if (fAutoCreate) {
            TrbProcessor* trb = new TrbProcessor(sub->GetId(), this);
            trb->SetAutoCreate(true);
            trb->SetHadaqCTSId(sub->GetId());
            trb->SetStoreKind(GetStoreKind());

            mgr()->UserPreLoop(trb); // while loop already running, call it once again for new processor

            printf("Create TRB 0x%04x procmgr %p hlvl %d \n", sub->GetId(), trb->mgr(), trb->HistFillLevel());

            // in auto mode only TDC processors should be created
            trb->ScanSubEvent(sub, fMsg.run_nr, fMsg.seq_nr);

            trb->ConfigureCalibration(fCalibrName, fCalibrPeriod, fCalibrTriggerMask);

            trb->SetAutoCreate(false); // do not create TDCs after first event
         }
      }

      for (TrbProcMap::iterator diter = fMap.begin(); diter != fMap.end(); diter++)
         diter->second->AfterEventScan();

      for (TrbProcMap::iterator diter = fMap.begin(); diter != fMap.end(); diter++)
         diter->second->AfterEventFill();

      if (fAutoCreate) {
         fAutoCreate = false; // with first event
         if (fAfterFunc.length()>0) mgr()->CallFunc(fAfterFunc.c_str(), this);
         CreatePerTDCHisto();
      }
   }

   if (mgr()->IsTriggeredAnalysis() && mgr()->HasTrigEvent()) {
      if (evcnt>1)
         fprintf(stderr, "event count %u bigger than 1 - not work for triggered analysis\n", evcnt);
      mgr()->AddToTrigEvent(GetName(), new hadaq::HldSubEvent(fMsg));
   }

   if (hadaq::TdcProcessor::GetHadesMonitorInterval() > 0) {
      auto tm = ::time(NULL);
      if (fLastHadesTm <= 0) fLastHadesTm = tm;
      if (tm - fLastHadesTm > hadaq::TdcProcessor::GetHadesMonitorInterval()) {
         fLastHadesTm = tm;
         for (auto &item : fMap) {
            unsigned num = item.second->NumberOfTDC();
            for (unsigned indx=0;indx<num;++indx)
               item.second->GetTDCWithIndex(indx)->DoHadesHistAnalysis();
         }
         DoHadesHistSummary();
      }
   }

   return true;
}

////////////////////////////////////////////////////////////////////////////////////////
/// Fill QA summary histograms

void hadaq::HldProcessor::DoHadesHistSummary()
{
    if (fQaFinePerTDCChannel == nullptr || fQaToTPerTDCChannel == nullptr || fQaEdgesPerTDCChannel == nullptr ||
            fQaErrorsPerTDCChannel == nullptr || fQaSummary == nullptr) return;

    int nBinsX = 0, nBinsY = 0;
    GetH2NBins(fQaFinePerTDCChannel, nBinsX, nBinsY);
    int nBadFine = 0, nBadToT = 0, nBadEdges = 0, nBadErrors = 0;
    for (int x = 0; x < nBinsX; x++){
        for (int y = 0; y < nBinsY; y++){
            double cFine = GetH2Content(fQaFinePerTDCChannel, x, y);
            if (cFine > 0. && cFine < 50.) nBadFine++;

            double cToT = GetH2Content(fQaToTPerTDCChannel, x, y);
            if (cToT > 0. && cToT < 50.) nBadToT++;

            double cEdges = GetH2Content(fQaEdgesPerTDCChannel, x, y);
            if (cEdges > 0. && cEdges < 50.) nBadEdges++;

            double cErrors = GetH2Content(fQaErrorsPerTDCChannel, x, y);
            double cErrors1;
            double cErrors2 = std::modf(cErrors, &cErrors1);
            if (cErrors > 0. && cErrors < 50. && cErrors2 >= 0.99) nBadErrors++;
        }
    }
    SetH1Content(fQaSummary, 0, nBadFine);
    SetH1Content(fQaSummary, 1, nBadToT);
    SetH1Content(fQaSummary, 2, nBadEdges);
    SetH1Content(fQaSummary, 3, nBadErrors);
}

////////////////////////////////////////////////////////////////////////////////////////
/// Call store event

void hadaq::HldProcessor::Store(base::Event* ev)
{
   if (ev) {
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

////////////////////////////////////////////////////////////////////////////////////////
/// Reset store

void hadaq::HldProcessor::ResetStore()
{
   pMsg = &fMsg;
}

////////////////////////////////////////////////////////////////////////////////////////
/// Function to transform HLD event, used for TDC calibrations

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

////////////////////////////////////////////////////////////////////////////////////////
/// Executing preliminary function before entering event loop

void hadaq::HldProcessor::UserPreLoop()
{
   if (!fAutoCreate && (fAfterFunc.length()>0))
      mgr()->CallFunc(fAfterFunc.c_str(), this);

   if (!fAutoCreate) CreatePerTDCHisto();
}

////////////////////////////////////////////////////////////////////////////////////////
/// Create summary histos where each bin corresponds to single TDC

void hadaq::HldProcessor::CreatePerTDCHisto()
{
   if (fErrPerTDC) return;

   std::vector<TdcProcessor *> tdcs;

   for (auto &item : fMap) {
      unsigned num = item.second->NumberOfTDC();
      for (unsigned indx = 0; indx < num; ++indx)
         tdcs.emplace_back(item.second->GetTDCWithIndex(indx));
   }

   if (tdcs.empty()) return;

   std::string lbl = "xbin:";
   unsigned cnt = 0;
   for (auto &tdc : tdcs) {
      if (cnt++>0) lbl.append(",");
      char sbuf[50];
      snprintf(sbuf, sizeof(sbuf), "0x%04X", tdc->GetID());
      lbl.append(sbuf);
   }

   std::string opt1 = lbl + ";fill:2;tdc"; // opt1 += ";hmin:0;hmax:1000";

   if (!fHitsPerTDC)
      fHitsPerTDC = MakeH1("HitsPerTDC", "Number of hits per TDC", tdcs.size(), 0, tdcs.size(), opt1.c_str());

   if (!fErrPerTDC)
      fErrPerTDC = MakeH1("ErrPerTDC", "Number of errors per TDC", tdcs.size(), 0, tdcs.size(), opt1.c_str());

   std::string opt2 = lbl + ";opt:colz,pal70;tdc;channels";

   if (!fHitsPerTDCChannel)
      fHitsPerTDCChannel = MakeH2("HitsPerChannel", "Number of hits per TDC channel",
                                  tdcs.size(), 0, tdcs.size(),
                                  TrbProcessor::GetDefaultNumCh(), 0, TrbProcessor::GetDefaultNumCh(),
                                  opt2.c_str());

   if (!fErrPerTDCChannel)
      fErrPerTDCChannel = MakeH2("ErrPerChannel", "Number of errors per TDC channel",
                                 tdcs.size(), 0, tdcs.size(),
                                 TrbProcessor::GetDefaultNumCh(), 0, TrbProcessor::GetDefaultNumCh(),
                                 opt2.c_str());

   if (!fCorrPerTDCChannel)
      fCorrPerTDCChannel = MakeH2("CorrPerChannel", "Number of corrected hits per TDC channel",
            tdcs.size(), 0, tdcs.size(),
            TrbProcessor::GetDefaultNumCh(), 0, TrbProcessor::GetDefaultNumCh(),
            opt2.c_str());
      
      // JAM 2021

       if (!fToTPerTDCChannel)
          fToTPerTDCChannel = MakeH2("ToTPerChannel", "ToT per TDC channel",
                              tdcs.size(), 0, tdcs.size(),
                                   TrbProcessor::GetDefaultNumCh(), 0, TrbProcessor::GetDefaultNumCh(),
                                   opt2.c_str());
       
        if (!fShiftPerTDCChannel)
          fShiftPerTDCChannel = MakeH2("ShiftPerChannel", "Calibrated time shift of falling edge per TDC channel",
                              tdcs.size(), 0, tdcs.size(),
                                   TrbProcessor::GetDefaultNumCh(), 0, TrbProcessor::GetDefaultNumCh(),
                                   opt2.c_str());
        
        if (!fExpectedToTPerTDC)
          fExpectedToTPerTDC = MakeH1("ExpectedToT", "Expected ToT used for calibration per TDC", tdcs.size(), 0, tdcs.size(), opt1.c_str());
          
        if (!fDevPerTDCChannel)
          fDevPerTDCChannel = MakeH2("DevPerChannel", "Deviation of Tot from calibration per TDC channel",
                              tdcs.size(), 0, tdcs.size(),
                                   TrbProcessor::GetDefaultNumCh(), 0, TrbProcessor::GetDefaultNumCh(),
                                   opt2.c_str());
       
      

   if (hadaq::TdcProcessor::GetHadesMonitorInterval() > 0) {
       if (!fQaFinePerTDCChannel)
          fQaFinePerTDCChannel = MakeH2("QaFinePerChannel", "QA fine time per TDC channel",
                              tdcs.size(), 0, tdcs.size(),
                                   TrbProcessor::GetDefaultNumCh(), 0, TrbProcessor::GetDefaultNumCh(),
                                   opt2.c_str());

       if (!fQaToTPerTDCChannel)
          fQaToTPerTDCChannel = MakeH2("QAToTPerChannel", "QA ToT per TDC channel",
                              tdcs.size(), 0, tdcs.size(),
                                   TrbProcessor::GetDefaultNumCh(), 0, TrbProcessor::GetDefaultNumCh(),
                                   opt2.c_str());
       if (!fQaEdgesPerTDCChannel)
          fQaEdgesPerTDCChannel = MakeH2("QaEdgesPerChannel", "QA edges per TDC channel",
                              tdcs.size(), 0, tdcs.size(),
                                   TrbProcessor::GetDefaultNumCh(), 0, TrbProcessor::GetDefaultNumCh(),
                                   opt2.c_str());

       if (!fQaErrorsPerTDCChannel)
          fQaErrorsPerTDCChannel = MakeH2("QaErrorsPerChannel", "QA errors per TDC channel",
                              tdcs.size(), 0, tdcs.size(),
                                   TrbProcessor::GetDefaultNumCh(), 0, TrbProcessor::GetDefaultNumCh(),
                                   opt2.c_str());

       if (!fQaSummary)
          fQaSummary = MakeH1("QaSummary", "QA summary", 4, -0.5, 3.5, "QA histogram;# bad channels");
       
// too late here?       
//         if (!fToTPerTDCChannel)
//           fToTPerTDCChannel = MakeH2("ToTPerChannel", "ToT per TDC channel",
//                               tdcs.size(), 0, tdcs.size(),
//                                    TrbProcessor::GetDefaultNumCh(), 0, TrbProcessor::GetDefaultNumCh(),
//                                    opt2.c_str());
//        
//         if (!fShiftPerTDCChannel)
//           fShiftPerTDCChannel = MakeH2("ShiftPerChannel", "Calibrated time shift of falling edge per TDC channel",
//                               tdcs.size(), 0, tdcs.size(),
//                                    TrbProcessor::GetDefaultNumCh(), 0, TrbProcessor::GetDefaultNumCh(),
//                                    opt2.c_str());
//         
//         if (!fExpectedToTPerTDC)
//           fExpectedToTPerTDC = MakeH1("ExpectedToT", "Expected ToT used for calibration per TDC", tdcs.size(), 0, tdcs.size(), opt1.c_str());
//           
//         if (!fDevPerTDCChannel)
//           fDevPerTDCChannel = MakeH2("DevPerChannel", "Deviation of Tot from calibration per TDC channel",
//                               tdcs.size(), 0, tdcs.size(),
//                                    TrbProcessor::GetDefaultNumCh(), 0, TrbProcessor::GetDefaultNumCh(),
//                                    opt2.c_str());
       
   }
   cnt = 0;
   for (auto &tdc : tdcs)
      tdc->AssignPerHldHistos(cnt++, &fHitsPerTDC, &fErrPerTDC, &fHitsPerTDCChannel, &fErrPerTDCChannel, &fCorrPerTDCChannel,
          &fQaFinePerTDCChannel, &fQaToTPerTDCChannel, &fQaEdgesPerTDCChannel, &fQaErrorsPerTDCChannel,
          
           &fToTPerTDCChannel, &fShiftPerTDCChannel, &fExpectedToTPerTDC,  &fDevPerTDCChannel);
}


////////////////////////////////////////////////////////////////////////////////////////
/// Enable cross-processing of data
/// Let produce time correlation between different TRBs

void hadaq::HldProcessor::SetCrossProcess(bool on)
{
   for (TrbProcMap::iterator diter = fMap.begin(); diter != fMap.end(); diter++)
      diter->second->SetCrossProcess(on);
}
