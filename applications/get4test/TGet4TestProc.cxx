#include "TGet4TestProc.h"

#include "TGo4Log.h"

#include "get4/SubEvent.h"

#include "go4/TStreamEvent.h"

TGet4TestProc::TGet4TestProc() :
   TUserProcessor()
{
}

TGet4TestProc::TGet4TestProc(const char* name) :
   TUserProcessor(name)
{

}

void TGet4TestProc::Process(TStreamEvent* ev)
{
    get4::SubEvent* sub0 = dynamic_cast<get4::SubEvent*> (ev->GetSubEvent("ROC0"));

    if (sub0!=0)
       TGo4Log::Info("Find GET4 data for ROC0 size %u", sub0->fExtMessages.size());
    else
       TGo4Log::Error("Not found GET4 data for ROC0");
}
