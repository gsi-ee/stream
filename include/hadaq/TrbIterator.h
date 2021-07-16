#ifndef HADAQ_TRBITERATOR_H
#define HADAQ_TRBITERATOR_H

#include "hadaq/definess.h"

namespace hadaq {

   /** iterator over TRB events/subevents */
   class TrbIterator {
      protected:
         void* fData;               ///< data
         unsigned fDatalen;         ///< length

         void* fEvCursor;          ///< event
         unsigned fEvLen;          ///< event length

         void* fSubCursor;         ///< subevent
         unsigned fSubLen;         ///< subevent length

      public:

         TrbIterator(void* data, unsigned datalen);
         /** destructor */
         ~TrbIterator() {}

         hadaqs::RawEvent* nextEvent();

         /** current event */
         hadaqs::RawEvent* currEvent() const { return (hadaqs::RawEvent*) fEvCursor; }

         hadaqs::RawSubevent* nextSubevent();

         /** current subevent */
         hadaqs::RawSubevent* currSubevent() const { return (hadaqs::RawSubevent*) fSubCursor; }

   };


}


#endif
