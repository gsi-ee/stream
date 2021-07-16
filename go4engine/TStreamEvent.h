#ifndef TSTREAMEVENT_H
#define TSTREAMEVENT_H

#include "TGo4EventElement.h"

#include "base/Event.h"

/** Envelope for \ref base::Event event in go4 */

class TStreamEvent : public TGo4EventElement,
                     public base::Event
{
   public:

      TStreamEvent() : TGo4EventElement(), base::Event() {}

      TStreamEvent(const char* name)  : TGo4EventElement(name), base::Event() {}

      virtual ~TStreamEvent() {}

      ClassDef(TStreamEvent,1)
};

#endif
