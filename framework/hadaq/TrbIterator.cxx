#include "hadaq/TrbIterator.h"

#include <stdio.h>

hadaq::TrbIterator::TrbIterator(void* data, unsigned datalen) :
   fData(data),
   fDatalen(datalen),
   fEvCursor(0),
   fSubCursor(0),
   fSubLen(0)
{
}


hadaq::RawEvent* hadaq::TrbIterator::nextEvent()
{
   if (fEvCursor == 0) {

      fEvCursor = fData;
      fEvLen = fDatalen;

   } else {
      hadaq::RawEvent* prev = (hadaq::RawEvent*) fEvCursor;

      unsigned fulllen = prev->GetPaddedSize();

//      printf("nextEvent fEvLen = %u evlen = %u\n", fEvLen, fulllen);

      if (fulllen >= fEvLen) {
         if (fulllen > fEvLen)
            printf("hadaq::RawEvent length mismatch %u %u\n", fulllen, fEvLen);
         fEvCursor = 0;
         fEvLen = 0;
      } else {
         fEvCursor = ((uint8_t*) fEvCursor) + fulllen;
         fEvLen -= fulllen;
      }
   }

   if ((fEvCursor!=0) && (fEvLen!=0) && (fEvLen < sizeof(hadaq::RawEvent))) {
      printf("Strange hadaq::RawEvent length %u minumum %u\n", (unsigned) fEvLen, (unsigned) sizeof(hadaq::RawEvent));
      fEvCursor = 0;
      fEvLen = 0;
   }

   fSubCursor = 0;
   fSubLen = 0;

   return (hadaq::RawEvent*) fEvCursor;
}

hadaq::RawSubevent* hadaq::TrbIterator::nextSubevent()
{
   RawEvent* ev = currEvent();

   if (ev==0) return 0;

   if (fSubCursor == 0) {
      fSubCursor = ((uint8_t*) ev) + sizeof(hadaq::RawEvent);

      fSubLen = ev->GetPaddedSize();

      if (fSubLen >= sizeof(hadaq::RawEvent)) {
         fSubLen -= sizeof(hadaq::RawEvent);
      } else {
         printf("Wrong hadaq::RawEvent length %u header size %u\n", (unsigned) ev->GetSize(), (unsigned) sizeof(hadaq::RawEvent));
         fSubLen = 0;
      }
   } else {

      hadaq::RawSubevent* sub = (hadaq::RawSubevent*) fSubCursor;

      unsigned fulllen = sub->GetPaddedSize();

//      printf ("Shift to next subevent size %u  fulllen %u\n", fulllen, fSubLen);

      if (fulllen >= fSubLen) {
         if (fulllen > fSubLen)
            printf("Mismatch in subevent length %u %u\n", fulllen, fSubLen);

         fSubLen = 0;
      } else {
         fSubCursor = ((uint8_t*) fSubCursor) + fulllen;
         fSubLen -= fulllen;

         if (fSubLen < sizeof(hadaq::RawSubevent)) {
            fSubLen = 0;
         } else {

            unsigned align = ((uint8_t*) fSubCursor - (uint8_t*) fEvCursor) % 8;

            if (align != 0) {
               printf("Align problem %u != 0 of subevent relative to buffer begin\n", align);
            }
         }
      }
   }

   if ((fSubLen==0) || ((fSubLen!=0) && (fSubLen < sizeof(hadaq::RawSubevent)))) {
      // printf("Strange hadaq::Subevent length %u\n", fSubLen);
      fSubCursor = 0;
      fSubLen = 0;
   }

   return (hadaq::RawSubevent*) fSubCursor;
}

