#include "go4/TStreamEvent.h"

TStreamEvent::TStreamEvent() :
   TGo4EventElement(),
   base::Event()
{
}

TStreamEvent::TStreamEvent(const char* name):
   TGo4EventElement(name),
   base::Event()
{
}

TStreamEvent::~TStreamEvent()
{
}

