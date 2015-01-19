#ifndef HADAQ_TRBITERATOR_H
#define HADAQ_TRBITERATOR_H

#include "hadaq/definess.h"

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

         hadaqs::RawEvent* nextEvent();

         hadaqs::RawEvent* currEvent() const { return (hadaqs::RawEvent*) fEvCursor; }

         hadaqs::RawSubevent* nextSubevent();

         hadaqs::RawSubevent* currSubevent() const { return (hadaqs::RawSubevent*) fSubCursor; }

   };


}


#endif
