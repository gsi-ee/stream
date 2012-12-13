#include "go4/TUserProcessor.h"

#include "TGo4Log.h"

#include "TSecondStepProcessor.h"

TUserProcessor::TUserProcessor() :
   TGo4EventProcessor()
{
}

TUserProcessor::TUserProcessor(const char* name) :
   TGo4EventProcessor(name)
{
   TGo4Log::Info("Create TUserProcessor %s", name);

   if (TSecondStepProcessor::Instance())
      TSecondStepProcessor::Instance()->AddProc(this);
   else
      TGo4Log::Error("No TSecondStepProcessor instance to register processor");
}

TUserProcessor::~TUserProcessor()
{
   TGo4Log::Info("Destroy TUserProcessor %s", GetName());
}
