#ifndef HADAQ_TRBITERATOR_H
#define HADAQ_TRBITERATOR_H

#include "hadaq/defines.h"

namespace hadaq {

   class TrbIterator {
      protected:
         void* fData;
         unsigned fDatalen;

         void* fEvCursor;
         unsigned fEvLen;

         void* fSubCursor;
         unsigned fSubLen;

      public:

         TrbIterator(void* data, unsigned datalen);
         ~TrbIterator() {}

         RawEvent* nextEvent();

         RawEvent* currEvent() const { return (RawEvent*) fEvCursor; }

         RawSubevent* nextSubevent();

         RawSubevent* currSubevent() const { return (RawSubevent*) fSubCursor; }

   };


}


#endif
