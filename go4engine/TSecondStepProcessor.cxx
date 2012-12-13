#include "TSecondStepProcessor.h"

#include "TGo4Log.h"
#include "TH1.h"
#include "TH2.h"
#include "TObjArray.h"

#include "TGo4EventErrorException.h"

#include "go4/TStreamEvent.h"
#include "go4/TUserProcessor.h"


TString TSecondStepProcessor::fDfltSetupScript = "second.C";

TSecondStepProcessor* TSecondStepProcessor::fInstance = 0;


TSecondStepProcessor::TSecondStepProcessor(const char* name) :
   TGo4EventProcessor(name),
   fProc()
{
   if (fInstance==0) fInstance = this;

   TGo4Log::Info("Create TSecondStepProcessor %s", name);

   if (ExecuteScript(fDfltSetupScript.Data()) == -1) {
      TGo4Log::Error("Cannot setup second analysis step with %s script", fDfltSetupScript.Data());
      throw TGo4EventErrorException(this);
   }
}

TSecondStepProcessor::~TSecondStepProcessor()
{
   // delete all processors
   fProc.Delete();

   if (fInstance==this) fInstance = 0;

   TGo4Log::Info("Destroy TSecondStepProcessor");
}

void TSecondStepProcessor::AddProc(TUserProcessor* proc)
{
   if ((proc==0) || (fProc.FindObject(proc)!=0)) return;

   fProc.Add(proc);
}


Bool_t TSecondStepProcessor::BuildEvent(TGo4EventElement* outevnt)
{
//   TGo4Log::Info("Start processing!!!");

   TStreamEvent* cbmev = (TStreamEvent*) GetInputEvent();

   if (cbmev==0)
      throw TGo4EventErrorException(this);

   if (!cbmev->IsValid()) return kFALSE;

   for (Int_t n=0; n<=fProc.GetLast(); n++) {
      TUserProcessor* proc = (TUserProcessor*) fProc[n];
      proc->Process(cbmev);
   }

   outevnt->SetValid(kFALSE);

   return kFALSE;
}
