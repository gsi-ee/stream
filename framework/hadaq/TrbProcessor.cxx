#include "hadaq/TrbProcessor.h"

#include <string.h>
#include <algorithm>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbIterator.h"
#include "hadaq/TdcProcessor.h"
#include "hadaq/HldProcessor.h"

#define RAWPRINT( args ...) if(IsPrintRawData()) printf( args )

unsigned hadaq::TrbProcessor::gNumChannels = 65;
unsigned hadaq::TrbProcessor::gEdgesMask = 0x1;
bool hadaq::TrbProcessor::gIgnoreSync = false;
unsigned hadaq::TrbProcessor::gTDCMin = 0x0000;
unsigned hadaq::TrbProcessor::gTDCMax = 0x0FFF;
unsigned hadaq::TrbProcessor::gHUBMin = 0x8100;
unsigned hadaq::TrbProcessor::gHUBMax = 0x81FF;

void hadaq::TrbProcessor::SetDefaults(unsigned numch, unsigned edges, bool ignore_sync)
{
   gNumChannels = numch;
   gEdgesMask = edges;
   gIgnoreSync = ignore_sync;
}


hadaq::TrbProcessor::TrbProcessor(unsigned brdid, HldProcessor* hldproc, int hfill) :
   base::StreamProc("TRB_%04X", brdid, false),
   fHldProc(hldproc),
   fMap(),
   fHadaqHUBId()
{
   if (hldproc==0) {
      mgr()->RegisterProc(this, base::proc_TRBEvent, brdid & 0xFF);
   } else {
      hldproc->AddTrb(this, brdid);
      if ((mgr()==0) && (hldproc->mgr()!=0)) hldproc->mgr()->AddProcessor(this);
   }

   if (hfill >= 0) SetHistFilling(hfill);

   // printf("Create TrbProcessor %s\n", GetName());

   fMsgPerBrd = 0;
   fErrPerBrd = 0;
   fHitsPerBrd = 0;

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

hadaq::TrbProcessor::~TrbProcessor()
{
}

hadaq::TdcProcessor* hadaq::TrbProcessor::FindTDC(unsigned tdcid) const
{
   hadaq::TdcProcessor* res = GetTDC(tdcid, true);
   if ((res==0) && (fHldProc!=0)) res = fHldProc->FindTDC(tdcid);
   return res;
}

void hadaq::TrbProcessor::CreatePerTDCHistos()
{
   unsigned numtdc = fMap.size();
   if (numtdc<4) numtdc = 4;
   std::string lbl = "tdc;xbin:";
   unsigned cnt = 0;
   for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (cnt++>0) lbl.append(",");
      char sbuf[50];
      sprintf(sbuf, "0x%04X", iter->second->GetID());
      lbl.append(sbuf);
   }

   while (cnt++<numtdc)
      lbl.append(",----");

   if (!fMsgPerBrd)
      fMsgPerBrd = MakeH1("MsgPerTDC", "Number of messages per TDC", numtdc, 0, numtdc, lbl.c_str());
   if (!fErrPerBrd)
      fErrPerBrd = MakeH1("ErrPerTDC", "Number of errors per TDC", numtdc, 0, numtdc, lbl.c_str());
   if (!fHitsPerBrd)
      fHitsPerBrd = MakeH1("HitsPerTDC", "Number of data hits per TDC", numtdc, 0, numtdc, lbl.c_str());

   cnt = 0;
   for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++)
      iter->second->AssignPerBrdHistos(this, cnt++);

}

void hadaq::TrbProcessor::UserPreLoop()
{
   if (fMap.size()>0)
      CreatePerTDCHistos();

   // fProfiler.MakeStatistic();
}

void hadaq::TrbProcessor::UserPostLoop()
{
   // fProfiler.MakeStatistic();
   // printf("TRB PROFILER: %s\n", fProfiler.Format().c_str());
}


bool hadaq::TrbProcessor::CheckPrintError()
{
   if (fPrintErrCnt<=0) return false;

   if (--fPrintErrCnt==0) {
      printf("%5s Suppress all other errors ...\n", GetName());
      return false;
   }

   return true;
}

int hadaq::TrbProcessor::CreateTDC(unsigned id1, unsigned id2, unsigned id3, unsigned id4)
{
   // overwrite default value in the beginning

   int num = 0;

   for (unsigned cnt=0;cnt<4;cnt++) {
      unsigned tdcid = id1;
      switch (cnt) {
         case 1: tdcid = id2; break;
         case 2: tdcid = id3; break;
         case 3: tdcid = id4; break;
         default: tdcid = id1; break;
      }
      if (tdcid==0) continue;

      if (GetTDC(tdcid, true)!=0) {
         printf("TDC id 0x%04x already exists\n", tdcid);
         continue;
      }

      if (tdcid==fHadaqCTSId) {
         printf("TDC id 0x%04x already used as CTS id\n", tdcid);
         continue;
      }

      if (std::find(fHadaqHUBId.begin(), fHadaqHUBId.end(), tdcid) != fHadaqHUBId.end()) {
         printf("TDC id 0x%04x already used as HUB id\n", tdcid);
         continue;
      }

      hadaq::TdcProcessor *tdc = new hadaq::TdcProcessor(this, tdcid, gNumChannels, gEdgesMask);

      tdc->SetCalibrTriggerMask(fCalibrTriggerMask);

      num++;
   }

   return num;
}


void hadaq::TrbProcessor::SetAutoCalibrations(long cnt)
{
   for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (iter->second->IsTDC())
        ((TdcProcessor*)iter->second)->SetAutoCalibration(cnt);
   }
}


void hadaq::TrbProcessor::DisableCalibrationFor(unsigned firstch, unsigned lastch)
{
   for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (iter->second->IsTDC())
         ((TdcProcessor*)iter->second)->DisableCalibrationFor(firstch, lastch);
   }
}


void hadaq::TrbProcessor::SetWriteCalibrations(const char* fileprefix, bool every_time, bool use_linear)
{
   for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (!iter->second->IsTDC()) continue;
      TdcProcessor* tdc = (TdcProcessor*) iter->second;

      tdc->SetWriteCalibration(fileprefix, every_time, use_linear);
   }
}


bool hadaq::TrbProcessor::LoadCalibrations(const char* fileprefix)
{
   bool res = true;

   for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (!iter->second->IsTDC()) continue;
      TdcProcessor* tdc = (TdcProcessor*) iter->second;
      if (!tdc->LoadCalibration(fileprefix)) res = false;
   }

   return res;
}

void hadaq::TrbProcessor::ConfigureCalibration(const std::string& name, long period, unsigned trigmask)
{
   SetCalibrTriggerMask(trigmask);

   if (period > 0) SetAutoCalibrations(period);

   if (name.length() > 0) {
      LoadCalibrations(name.c_str());
      if ((period == -1) || (period == -77)) SetWriteCalibrations(name.c_str(), false, (period == -77));
   }
}

void hadaq::TrbProcessor::SetCalibrTriggerMask(unsigned trigmask)
{
   fCalibrTriggerMask = trigmask;

   for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (!iter->second->IsTDC()) continue;
      TdcProcessor* tdc = (TdcProcessor*) iter->second;
      tdc->SetCalibrTriggerMask(fCalibrTriggerMask);
   }
}

void hadaq::TrbProcessor::AddSub(SubProcessor* tdc, unsigned id)
{
   fMap[id] = tdc;
}

void hadaq::TrbProcessor::SetStoreKind(unsigned kind)
{
   base::StreamProc::SetStoreKind(kind);

   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetStoreKind(kind);
}

void hadaq::TrbProcessor::SetCh0Enabled(bool on)
{
   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      if (iter->second->IsTDC())
         ((TdcProcessor*)iter->second)->SetCh0Enabled(on);
   }
}

void hadaq::TrbProcessor::SetTriggerWindow(double left, double right)
{
   base::StreamProc::SetTriggerWindow(left, right);

   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      iter->second->SetTriggerWindow(left, right);
      iter->second->CreateTriggerHist(16, 3000, -2e-6, 2e-6);
   }
}


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


bool hadaq::TrbProcessor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

//   RAWPRINT("TRB3 - first scan of buffer %u\n", buf().datalen);

//   printf("Scan TRB buffer size %u\n", buf().datalen);


   hadaq::TrbIterator iter(buf().buf, buf().datalen);

   hadaqs::RawEvent  *ev = 0;

   while ((ev = iter.nextEvent()) != 0) {

      if (ev->GetSize() > buf().datalen+4) {
         printf("Corrupted event size %u!\n", ev->GetSize()); return true;
      }

      if (IsPrintRawData()) ev->Dump();

      DefFillH1(fEvSize, ev->GetPaddedSize(), 1.);

      BeforeEventScan();

      hadaqs::RawSubevent* sub = 0;
      unsigned subcnt(0);

      while ((sub = iter.nextSubevent()) != 0) {

         if (sub->GetSize() > buf().datalen+4) {
            printf("Corrupted subevent size %u!\n", sub->GetSize()); return true;
         }

         if (IsPrintRawData()) sub->Dump(true);

         // use only 16-bit in trigger number while CTS make a lot of errors in higher 8 bits
         AccountTriggerId((sub->GetTrigNr() >> 8) & 0xffff);

         ScanSubEvent(sub, ev->GetSeqNr());

         subcnt++;
      }

      AfterEventScan();
   }

   return true;
}


void hadaq::TrbProcessor::BeforeEventScan()
{
   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->BeforeFill();
}


void hadaq::TrbProcessor::AfterEventScan()
{
   // we scan in all TDC processors newly add buffers
   // this is required to
   if (IsCrossProcess()) {
      for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
         iter->second->ScanNewBuffers();

      for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
         iter->second->AfterFill(&fMap);
   }
}


void hadaq::TrbProcessor::SetCrossProcess(bool on)
{
   fCrossProcess = on;
   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->fCrossProcess = on;
}


void hadaq::TrbProcessor::CreateBranch(TTree*)
{
   if(mgr()->IsTriggeredAnalysis()) {
      mgr()->CreateBranch(GetName(), "hadaq::TrbMessage", (void**)&pMsg);
   }
}

void hadaq::TrbProcessor::AddBufferToTDC(hadaqs::RawSubevent* sub,
                                         hadaq::SubProcessor* tdcproc,
                                         unsigned ix, unsigned datalen)
{
   if (datalen==0) {
      if (CheckPrintError())
         printf("Try to add empty buffer to %s\n", tdcproc->GetName());
      return;
   }

   base::Buffer buf;

   if (gIgnoreSync && (sub->Alignment()==4)) {
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

void hadaq::TrbProcessor::ScanSubEvent(hadaqs::RawSubevent* sub, unsigned trb3eventid)
{
   // this is first scan of subevent from TRB3 data
   // our task is statistic over all messages we will found
   // also for trigger-type 1 we should add SYNC message to each processor

   memcpy((void *) &fLastSubevHdr, sub, sizeof(fLastSubevHdr));

   DefFillH1(fTrigType, sub->GetTrigTypeTrb3(), 1.);

   DefFillH1(fSubevSize, sub->GetSize(), 1.);

   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetNewDataFlag(false);

   unsigned ix = 0;           // cursor

   unsigned trbSubEvSize = sub->GetSize() / 4 - 4;

   if (trbSubEvSize>100000) printf("LARGE subevent %u\n", trbSubEvSize);

   fMsg.fTrigSyncIdFound = false;

   bool did_create_tdc = false;

   unsigned maxhublen = 0, lasthubid = 0; // if saw HUB subsubevents, control size of data inside

//   RAWPRINT("Scan TRB3 raw event 4-bytes size %u\n", trbSubEvSize);

   while (ix < trbSubEvSize) {
      //! Extract data portion from the whole packet (in a loop)
      uint32_t data = sub->Data(ix++);

      unsigned datalen = (data >> 16) & 0xFFFF;
      unsigned dataid = data & 0xFFFF;

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

         if (maxhublen==0) {
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

      if (maxhublen>0) {
         if (datalen >= maxhublen) {
            if (CheckPrintError()) printf("error: sub-sub event %04x inside HUB %04x exceed size limit\n", dataid, lasthubid);
            datalen = maxhublen-1;
         }
         maxhublen -= (datalen+1);
      }

      //! ==================== CTS header and inside ================
      if (dataid == fHadaqCTSId) {
         RAWPRINT("   CTS header: 0x%x, size=%d\n", (unsigned) data, datalen);
         //hTrbTriggerCount->Fill(5);          //! TRB - CTS
         //hTrbTriggerCount->Fill(0);          //! TRB TOTAL

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

         RAWPRINT("     CTS trigtype: 0x%04x, nExtTrigFlag: 0x%02x, datalen: %u, nCTSwords: %u\n", trigtype, nExtTrigFlag, datalen, nCTSwords);

         // we skip all the information from the CTS right now,
         // we don't need it
         ix += nCTSwords;
         datalen -= nCTSwords;
         // ix should now point to the first ETM word


         // handle the ETM module and extract syncNumber
         if(nExtTrigFlag==0x1) {
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
         } else if(nExtTrigFlag==0x0) {

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
               RAWPRINT("Error: Unknown value in CTS header found: %x\n", sub->Data(ix));
            }
         } else {
            RAWPRINT("Error: Unknown ETM in CTS header found: %x\n", nExtTrigFlag);
            // TODO: try to do some proper error recovery here...
         }

         if(fMsg.fTrigSyncIdFound)
            RAWPRINT("     Find SYNC %u\n", (unsigned) fMsg.fTrigSyncId);

         // now ix should point to the first TDC word if datalen>0
         // if not, there is no TDC present

         // This is special TDC processor for data from CTS header
         TdcProcessor* tdcproc = GetTDC(fHadaqCTSId, true);
         if ((tdcproc!=0) && (datalen>0)) {
            // if TDC processor found, process such data as normal TDC data
            AddBufferToTDC(sub, tdcproc, ix, datalen);
         }

         // don't forget to skip the words for the TDC (if any)
         ix += datalen;

         continue;
      }

      TdcProcessor* tdcproc = GetTDC(dataid);
      //! ================= FPGA TDC header ========================
      if (tdcproc != 0) {
         RAWPRINT("   FPGA-TDC header: 0x%08x, tdcid=0x%04x, size=%u\n", (unsigned) data, dataid, datalen);

         if (IsPrintRawData()) {
            TdcIterator iter;
            iter.assign(sub, ix, datalen);
            while (iter.next()) iter.printmsg();
         }

         AddBufferToTDC(sub, tdcproc, ix, datalen);

         ix+=datalen;

         continue; // go to next block
      }  // end of if TDC header


      //! ==================  Dummy header and inside ==========================
      if (dataid == 0x5555) {
         RAWPRINT("   Dummy header: 0x%x, size=%d\n", (unsigned) data, datalen);
         //hTrbTriggerCount->Fill(4);          //! TRB - DUMMY
         //hTrbTriggerCount->Fill(0);          //! TRB TOTAL
         while (datalen-- > 0) {
            //! In theory here must be only one word - termination package with the status
            data = sub->Data(ix++);
            RAWPRINT("      word: 0x%08x\n", (unsigned) data);
            //uint32_t fSubeventStatus = data;
            //if (fSubeventStatus != 0x00000001) { bad events}
         }

         continue;
      }

      SubProcMap::const_iterator iter = fMap.find(dataid);
      SubProcessor* subproc = iter != fMap.end() ? iter->second : 0;

      //! ================= any other header ========================
      if (subproc!=0) {
         RAWPRINT ("   SUB header: 0x%08x, id=0x%04x, size=%u\n", (unsigned) data, dataid, datalen);

         if(datalen==0)
            continue;

         base::Buffer buf;

         // check if this processor has some attached TDC
         size_t offset = 0;
         SubProcMap::const_iterator iter_tdc = fMap.find(dataid | 0xff0000);
         if(iter_tdc != fMap.end()) {
            // pre-scan for begin marker of non-TDC data
            for(size_t i=0;i<datalen;i++) {
               const uint32_t data_ = sub->Data(ix+i);
               if(data_ >> 28 == 0x1) {
                  offset = i;
                  break;
               }
            }
            if(offset>0) {
               AddBufferToTDC(sub, iter_tdc->second, ix, offset);
            }
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

         ix+=datalen;

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
         } else
         if ((dataid >= gTDCMin) && (dataid <= gTDCMax)) {
            // suppose this is TDC data, first word should be TDC header
            TdcMessage msg(sub->Data(ix));

            if (msg.isHeaderMsg() || hadaq::TdcProcessor::IsDRICHReapir()) {
               unsigned numch = gNumChannels, edges = gEdgesMask;
               // here should be channel/edge/min/max selection based on TDC design ID

               TdcProcessor* tdcproc = new TdcProcessor(this, dataid, numch, edges);
               tdcproc->SetCalibrTriggerMask(fCalibrTriggerMask);
               tdcproc->SetStoreKind(GetStoreKind());

               mgr()->UserPreLoop(tdcproc); // while loop already running, call it once again for new processor

               did_create_tdc = true;

               printf("%s: Create TDC 0x%04x nch:%u edges:%u\n", GetName(), dataid, numch, edges);

               // in auto-create mode buffers are not processed - normally it is only first event
               // AddBufferToTDC(sub, tdcproc, ix, datalen);
               ix+=datalen;
               continue; // go to next block
            } else {
               if (CheckPrintError()) printf("sub-sub-event data with id 0x%04x does not belong to TDC\n", dataid);
            }
         } else {
            printf("%s: Saw ID 0x%04x in autocreate mode\n", GetName(), dataid);
         }
      }


      RAWPRINT("Unknown header 0x%04x length %u in TRB 0x%04x subevent\n", data & 0xFFFF, datalen, GetID());
      ix+=datalen;
   }

   if (did_create_tdc) CreatePerTDCHistos();

   if (fUseTriggerAsSync) {
      fMsg.fTrigSyncIdFound = true;
      fMsg.fTrigSyncId = trb3eventid;
      fMsg.fTrigSyncIdStatus = 0; // dummy
   }

   if (fMsg.fTrigSyncIdFound && !gIgnoreSync) {
      for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
         if (iter->second->IsNewDataFlag())
            iter->second->AppendTrbSync(fMsg.fTrigSyncId);
      }
   }
}

bool hadaq::TrbProcessor::CreateMissingTDC(hadaqs::RawSubevent *sub, const std::vector<uint64_t> &mintdc, const std::vector<uint64_t> &maxtdc, int numch, int edges)
{
   bool isany = false;

   unsigned ix(0); // cursor

   unsigned trbSubEvSize = (sub->GetSize() - sizeof(hadaqs::RawSubevent)) / 4;

   while (ix < trbSubEvSize) {
      //! Extract data portion from the whole packet (in a loop)
      uint32_t data = sub->Data(ix++);

      uint32_t dataid = data & 0xFFFF;

      uint32_t datalen = (data >> 16) & 0xFFFF;

      if (std::find(fHadaqHUBId.begin(), fHadaqHUBId.end(), dataid) != fHadaqHUBId.end()) {
         // TODO: formally we should analyze HUB subevent as real subevent but
         // we just skip header and continue to analyze data
         continue;
      }

      //! ================= FPGA TDC header ========================
      TdcProcessor* subproc = GetTDC(dataid, true);

      bool match = false;

      for (unsigned n=0;n<mintdc.size();++n)
         if ((dataid>=mintdc[n]) && (dataid<maxtdc[n])) match = true;

      if (!subproc && match && (dataid != 0x5555)) {
         subproc = new hadaq::TdcProcessor(this, dataid, numch, edges);
         isany = true;
      }  // end of if TDC header

      // all other blocks are ignored
      ix+=datalen;
   }

   return isany;
}


unsigned hadaq::TrbProcessor::TransformSubEvent(hadaqs::RawSubevent* sub, void* tgtbuf, unsigned tgtlen, bool only_hist)
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
   for(int bit=0; bit<32;++bit)
   {
     uint32_t mask = (bit << 1);
     if((error_word & mask) == mask) DefFastFillH1(fErrBits, bit, 1.);
   }

   // only fill histograms
   if (only_hist) return 0;

   if ((fMinTdc == 0) && (fMaxTdc == 0) && (fTdcsVect.size()==0) && (fMap.size() > 0)) {
      bool isany = false;
      fMinTdc = 0xffffff;

      for (auto &&entry : fMap) {
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
         for (auto &&entry : fMap)
            if (entry.second->IsTDC())
               fTdcsVect[entry.first - fMinTdc] = static_cast<hadaq::TdcProcessor*>(entry.second);
      }
   }

   bool get_fast = fMaxTdc > fMinTdc;

   // !!! DEBUG ONLY - just copy data
   // if (tgtbuf && tgtlen) {
   //   printf("SUBEVENT swap:%u len:%u\n", sub->IsSwapped(), sub->GetPaddedSize());
   //   memcpy((void *) tgtbuf, (void *) sub, sub->GetPaddedSize());
   //   return sub->GetPaddedSize();
   // }

//   grd.Next("hdr");

   hadaqs::RawSubevent* tgt = (hadaqs::RawSubevent*) tgtbuf;
   // copy complete header first
   if (tgt!=0) {
      // copy header
      memcpy((void *) tgt, sub, sizeof(hadaqs::RawSubevent));
      tgtlen = (tgtlen - sizeof(hadaqs::RawSubevent)) / 4; // how many 32-bit values can be used
   }

   unsigned ix(0), tgtix(0); // cursor

   unsigned trbSubEvSize = (sub->GetSize() - sizeof(hadaqs::RawSubevent)) / 4;

   while (ix < trbSubEvSize) {

//      grd.Next("sub", 5);

      //! Extract data portion from the whole packet (in a loop)
      uint32_t data = sub->Data(ix++);

      if (tgt && (tgtix >= tgtlen)) {
         fprintf(stderr,"TrbProcessor::TransformSubEvent not enough space in output buffer\n");
         return 0;
      }

      unsigned datalen = (data >> 16) & 0xFFFF;
      unsigned id = data & 0xFFFF;

      if (fHadaqHUBId.size() > 0)
         if (std::find(fHadaqHUBId.begin(), fHadaqHUBId.end(), id) != fHadaqHUBId.end()) {
            // ix+=datalen;  // WORKAROUND !!!

            // copy hub header to the target
            if (tgt) tgt->SetData(tgtix++, data);

            // TODO: formally we should analyze HUB subevent as real subevent but
            // we just skip header and continue to analyze data
            continue;
         }

//      grd.Next("get");

      //! ================= FPGA TDC header ========================
      TdcProcessor *subproc = nullptr;
      if (get_fast) {
         if ((id >= fMinTdc) && (id < fMaxTdc)) subproc = fTdcsVect[id-fMinTdc];
      } else {
         subproc = GetTDC(id, true);
      }

      if (subproc) {
//         grd.Next("trans");
         unsigned newlen = subproc->TransformTdcData(sub, ix, datalen, tgt, tgtix+1);
         if (tgt) {
            tgt->SetData(tgtix++, id | ((newlen & 0xffff) << 16));
            tgtix += newlen;
         }
         ix += datalen;
         continue; // go to next block
      }  // end of if TDC header

//      grd.Next("unrec", 10);

      // copy unrecognized data to the target
      if (tgt) {
         tgt->SetData(tgtix++, data);
         memcpy(tgt->RawData(tgtix), sub->RawData(ix), datalen*4);
         tgtix+=datalen;
      }

      // all other blocks are ignored
      ix+=datalen;
   }

//   grd.Next("finish", 15);

   if (tgt) {
      tgt->SetSize(sizeof(hadaqs::RawSubevent) + tgtix*4);
      return tgt->GetPaddedSize();
   }

   return 0;
}


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

