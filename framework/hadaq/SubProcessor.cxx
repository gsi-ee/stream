#include "hadaq/SubProcessor.h"

#include "hadaq/TrbProcessor.h"

//////////////////////////////////////////////////////////////////////////////////////////////
/// constructor

hadaq::SubProcessor::SubProcessor(TrbProcessor *trb, const char* nameprefix, unsigned subid) :
   base::StreamProc(nameprefix, subid, false),
   fTrb(trb),
   fSeqeunceId(0),
   fIsTDC(false),
   fMsgPerBrd(nullptr),
   fErrPerBrd(nullptr),
   fHitsPerBrd(nullptr),
   fCalHitsPerBrd(nullptr),
   fToTPerBrd(nullptr),
   fNewDataFlag(false),
   fPrintRawData(false),
   fCrossProcess(false)
{
   if (trb) {
      std::string pref = trb->GetName();
      pref.append("/");
      pref.append(GetName());
      SetPathPrefix(pref);

      if (!mgr() && trb->mgr()) trb->mgr()->AddProcessor(this);

      trb->AddSub(this, subid);
      fPrintRawData = trb->IsPrintRawData();
      fCrossProcess = trb->IsCrossProcess();

      AssignPerBrdHistos(trb, 0);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// assign overview histograms

void hadaq::SubProcessor::AssignPerBrdHistos(TrbProcessor* trb, unsigned seqid)
{
   if (!trb) return;

   fSeqeunceId = seqid;

   SetHistFilling(trb->HistFillLevel());

   fMsgPerBrd = &trb->fMsgPerBrd;
   fErrPerBrd = &trb->fErrPerBrd;
   fHitsPerBrd = &trb->fHitsPerBrd;
   fCalHitsPerBrd = &trb->fCalHitsPerBrd;
   fToTPerBrd = &trb->fToTPerBrd;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// pre loop

void hadaq::SubProcessor::UserPreLoop()
{
   unsigned cnt(0);
   if (fTrb)
      for (auto &&item : fTrb->fMap) {
         if (item.second == this) { fSeqeunceId = cnt; break; }
         cnt++;
      }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// get HLD processor

hadaq::HldProcessor *hadaq::SubProcessor::GetHLD() const
{
   return fTrb ? fTrb->GetHLD() : nullptr;
}

