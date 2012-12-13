#ifndef TSTREAMEVENT_H
#define TSTREAMEVENT_H

#include <TGo4EventElement.h>

#include "base/Event.h"

class TStreamEvent : public TGo4EventElement,
                     public base::Event
{
   public:

      TStreamEvent();
      TStreamEvent(const char* name);
      virtual ~TStreamEvent();

      ClassDef(TStreamEvent,1)
};

#endif
