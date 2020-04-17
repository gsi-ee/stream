#include "hadaq/TrbIterator.h"

#include <cstdio>

hadaq::TrbIterator::TrbIterator(void* data, unsigned datalen) :
   fData(data),
   fDatalen(datalen),
   fEvCursor(0),
   fSubCursor(0),
   fSubLen(0)
{
}


hadaqs::RawEvent* hadaq::TrbIterator::nextEvent()
{
   if (fEvCursor == 0) {

      fEvCursor = fData;
      fEvLen = fDatalen;

   } else {
      hadaqs::RawEvent* prev = (hadaqs::RawEvent*) fEvCursor;

      unsigned fulllen = prev->GetPaddedSize();

      if (fulllen >= fEvLen) {
         if (fulllen > fEvLen)
            printf("hadaqs::RawEvent length mismatch %u %u\n", fulllen, fEvLen);
         fEvCursor = 0;
         fEvLen = 0;
      } else {
         fEvCursor = ((uint8_t*) fEvCursor) + fulllen;
         fEvLen -= fulllen;
      }
   }

   if ((fEvCursor!=0) && (fEvLen!=0) && (fEvLen < sizeof(hadaqs::RawEvent))) {
      printf("Strange hadaqs::RawEvent length %u minumum %u\n", (unsigned) fEvLen, (unsigned) sizeof(hadaqs::RawEvent));
      fEvCursor = 0;
      fEvLen = 0;
   }

   fSubCursor = 0;
   fSubLen = 0;

   return (hadaqs::RawEvent*) fEvCursor;
}

hadaqs::RawSubevent* hadaq::TrbIterator::nextSubevent()
{
   hadaqs::RawEvent* ev = currEvent();

   if (ev==0) return 0;

   if (fSubCursor == 0) {
      fSubCursor = ((uint8_t*) ev) + sizeof(hadaqs::RawEvent);

      fSubLen = ev->GetPaddedSize();

      if (fSubLen >= sizeof(hadaqs::RawEvent)) {
         fSubLen -= sizeof(hadaqs::RawEvent);
      } else {
         printf("Wrong hadaqs::RawEvent length %u header size %u\n", (unsigned) ev->GetSize(), (unsigned) sizeof(hadaqs::RawEvent));
         fSubLen = 0;
      }
   } else {

      hadaqs::RawSubevent* sub = (hadaqs::RawSubevent*) fSubCursor;

      unsigned fulllen = sub->GetPaddedSize();

//      printf ("Shift to next subevent size %u  fulllen %u\n", fulllen, fSubLen);

      if (fulllen >= fSubLen) {
         if (fulllen > fSubLen)
            printf("Mismatch in subevent length %u %u\n", fulllen, fSubLen);

         fSubLen = 0;
      } else {
         fSubCursor = ((uint8_t*) fSubCursor) + fulllen;
         fSubLen -= fulllen;

         if (fSubLen < sizeof(hadaqs::RawSubevent)) {
            fSubLen = 0;
         } else {

            unsigned align = ((uint8_t*) fSubCursor - (uint8_t*) fEvCursor) % 8;

            if (align != 0) {
               printf("Align problem %u != 0 of subevent relative to buffer begin\n", align);
            }
         }
      }
   }

   if ((fSubLen==0) || ((fSubLen!=0) && (fSubLen < sizeof(hadaqs::RawSubevent)))) {
      // printf("Strange hadaq::Subevent length %u\n", fSubLen);
      fSubCursor = 0;
      fSubLen = 0;
   }

   return (hadaqs::RawSubevent*) fSubCursor;
}

