#include "hadaq/SubProcessor.h"

#include "hadaq/TrbProcessor.h"

hadaq::SubProcessor::SubProcessor(TrbProcessor *trb, const char* nameprefix, unsigned subid) :
   base::StreamProc(nameprefix, subid, false),
   fTrb(trb)
{
   if (trb) {

      std::string pref = trb->GetName();
      pref.append("/");
      pref.append(GetName());
      SetPathPrefix(pref);

      if ((mgr()==0) && (trb->mgr()!=0)) trb->mgr()->AddProcessor(this);

      trb->AddSub(this, subid);
      fPrintRawData = trb->IsPrintRawData();
      fCrossProcess = trb->IsCrossProcess();

      AssignPerBrdHistos(trb, 0);
   }
}

void hadaq::SubProcessor::AssignPerBrdHistos(TrbProcessor* trb, unsigned seqid)
{
   if (!trb) return;

   fSeqeunceId = seqid;

   SetHistFilling(trb->HistFillLevel());

   fMsgPerBrd = &trb->fMsgPerBrd;
   fErrPerBrd = &trb->fErrPerBrd;
   fHitsPerBrd = &trb->fHitsPerBrd;
}

void hadaq::SubProcessor::UserPreLoop()
{
   unsigned cnt(0);
   if (fTrb != 0)
      for (SubProcMap::const_iterator iter = fTrb->fMap.begin(); iter!=fTrb->fMap.end(); iter++) {
         if (iter->second == this) { fSeqeunceId = cnt; break; }
         cnt++;
      }
}
