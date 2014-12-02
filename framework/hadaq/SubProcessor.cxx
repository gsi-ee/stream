#include "hadaq/SubProcessor.h"

#include "hadaq/TrbProcessor.h"

hadaq::SubProcessor::SubProcessor(TrbProcessor* trb, const char* nameprefix, unsigned subid) :
   base::StreamProc(nameprefix, subid, false),
   fTrb(trb),
   fSeqeunceId(0),
   fNewDataFlag(false)
{
   if (trb) {
      trb->AddSub(this, subid);
      fPrintRawData = trb->IsPrintRawData();
      fCrossProcess = trb->IsCrossProcess();
      SetHistFilling(trb->HistFillLevel());
   }

   fMsgPerBrd = trb ? &trb->fMsgPerBrd : 0;
   fErrPerBrd = trb ? &trb->fErrPerBrd : 0;
   fHitsPerBrd = trb ? &trb->fHitsPerBrd : 0;
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
