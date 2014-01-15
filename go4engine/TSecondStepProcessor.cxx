#include "TSecondStepProcessor.h"

#include "TGo4Log.h"
#include "TH1.h"
#include "TH2.h"
#include "TObjArray.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TString.h"

#include "TGo4EventErrorException.h"

#include "base/ProcMgr.h"

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

   if (gSystem->Getenv("STREAMSYS")==0) {
      TGo4Log::Error("STREAMSYS variable not defined");
      throw TGo4EventErrorException(this);
   }

   gROOT->ProcessLine(".include $STREAMSYS/include");

   if (ExecuteScript(TString::Format("%s+", fDfltSetupScript.Data()).Data()) == -1) {
      TGo4Log::Error("Cannot setup second analysis step with %s script in ACLiC mode", fDfltSetupScript.Data());
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

   TStreamEvent* event = (TStreamEvent*) GetInputEvent();

   if ((event==0) || (base::ProcMgr::instance()==0))
      throw TGo4EventErrorException(this);

   if (!event->IsValid()) return kFALSE;

   base::ProcMgr::instance()->ProcessEvent(event);

   outevnt->SetValid(kFALSE);

   return kFALSE;
}
