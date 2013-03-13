#include "TStreamAnalysis.h"

#include <stdlib.h>

#include "TSystem.h"

#include "Go4EventServer.h"
#include "TGo4StepFactory.h"
#include "TGo4AnalysisStep.h"
#include "TGo4Version.h"
#include "TGo4Log.h"

#include "TFirstStepProcessor.h"
#include "TSecondStepProcessor.h"



//***********************************************************
TStreamAnalysis::TStreamAnalysis()
{
}
//***********************************************************

// this constructor is called by go4analysis executable
TStreamAnalysis::TStreamAnalysis(int argc, char** argv) :
   TGo4Analysis(argc, argv)
{
   if (!TGo4Version::CheckVersion(__GO4BUILDVERSION__)) {
      TGo4Log::Error("Go4 version mismatch");
      exit(-1);
   }

   TGo4Log::Info("Create TStreamAnalysis name: %s", argv[0]);
   if (argc>1) {
      TGo4Log::Info("Use initialization script: %s", argv[1]);
      TFirstStepProcessor::SetDfltScript(argv[1]);
   }


   // here should be something with macros

   TGo4StepFactory* factory = new TGo4StepFactory("Step1");
   factory->DefEventProcessor("FirstProc","TFirstStepProcessor");// object name, class name
   factory->DefInputEvent("MBS","TGo4MbsEvent"); // object name, class name
   factory->DefOutputEvent("CBM","TStreamEvent"); // object name, class name
   // factory->DefUserEventSource("TCBMUserSource"); // object name, class name

   TGo4EventSourceParameter* sourcepar = new TGo4MbsFileParameter("file.lmd");
   TGo4FileStoreParameter* storepar = new TGo4FileStoreParameter("NoOutput");
   storepar->SetOverwriteMode(kTRUE);

   TGo4AnalysisStep* step = new TGo4AnalysisStep("First", factory, sourcepar, storepar);

   step->SetSourceEnabled(kTRUE);
   step->SetStoreEnabled(kFALSE);
   step->SetProcessEnabled(kTRUE);
   step->SetErrorStopEnabled(kTRUE);

   // Now the first analysis step is set up.
   // Other steps could be created here
   AddAnalysisStep(step);


   if (gSystem->AccessPathName(TSecondStepProcessor::GetDfltScript())==0) {
      TGo4Log::Info("Create second step");

      TGo4StepFactory* factory2 = new TGo4StepFactory("Step2");
      factory2->DefEventProcessor("SecondProc","TSecondStepProcessor");// object name, class name
      factory2->DefInputEvent("CBM","TStreamEvent"); // object name, class name
      factory2->DefOutputEvent("Second","TStreamEvent"); // object name, class name
      // factory->DefUserEventSource("TCBMUserSource"); // object name, class name

//      TGo4EventSourceParameter* sourcepar = new TGo4MbsFileParameter("file.lmd");
//      TGo4FileStoreParameter* storepar = new TGo4FileStoreParameter("NoOutput");
//      storepar->SetOverwriteMode(kTRUE);

      TGo4AnalysisStep* step2 = new TGo4AnalysisStep("Second", factory2, 0, 0);

      step2->SetSourceEnabled(kTRUE);
      step2->SetStoreEnabled(kFALSE);
      step2->SetProcessEnabled(kTRUE);
      step2->SetErrorStopEnabled(kTRUE);

      AddAnalysisStep(step2);
   }

   // uncomment following line to define custom passwords for analysis server
   // DefineServerPasswords("CernOct11admin", "CernOct11ctrl", "CernOct11view");
}


Int_t TStreamAnalysis::UserPreLoop()
{
   TGo4AnalysisStep* step1 = GetAnalysisStep("First");
   TFirstStepProcessor* proc1 = dynamic_cast<TFirstStepProcessor*> (step1->GetEventProcessor());

   if (proc1) proc1->UserPreLoop();

   return TGo4Analysis::UserPreLoop();
}

Int_t TStreamAnalysis::UserPostLoop()
{
   TGo4AnalysisStep* step1 = GetAnalysisStep("First");
   TFirstStepProcessor* proc1 = dynamic_cast<TFirstStepProcessor*> (step1->GetEventProcessor());

   if (proc1) proc1->UserPostLoop();

   return TGo4Analysis::UserPostLoop();
}


//***********************************************************
TStreamAnalysis::~TStreamAnalysis()
{
   TGo4Log::Info("TStreamAnalysis: Delete instance");
}

