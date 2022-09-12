#include "base/Buffer.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>

//////////////////////////////////////////////////////////////////////////////////////////////
/// reset buffer

void base::Buffer::reset()
{
   if (fRec) {
      if (--fRec->refcnt == 0)
         free(fRec);
      fRec = nullptr;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// set data length

bool base::Buffer::setdatalen(unsigned newlen)
{
   if (newlen > datalen()) return false;

   if (newlen == 0)
      reset();
   else
      fRec->datalen = newlen;

   return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// allocate new buffer

void base::Buffer::makenew(unsigned datalen)
{
   reset();

   if (datalen==0) return;

   fRec = (RawDataRec*) malloc(sizeof(RawDataRec) + datalen);
   if (!fRec) {
      printf("Buffer allocation error makenew sz %ld\n", (long) (sizeof(RawDataRec) + datalen));
      return;
   }

   fRec->reset();

   fRec->refcnt = 1;

   fRec->buf = (char*) fRec + sizeof(RawDataRec);

   fRec->datalen = datalen;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// create copy of memory

void base::Buffer::makecopyof(void* buf, unsigned datalen)
{
   reset();

   if (!buf || (datalen == 0)) return;

   fRec = (RawDataRec*) malloc(sizeof(RawDataRec) + datalen);

   fRec = (RawDataRec*) malloc(sizeof(RawDataRec));
   if (!fRec) {
      printf("Buffer allocation error makecopyof sz %ld\n", (long) (sizeof(RawDataRec) + datalen));
      return;
   }

   fRec->reset();

   fRec->refcnt = 1;

   fRec->buf = (char*) fRec + sizeof(RawDataRec);
   memcpy(fRec->buf, buf, datalen);

   fRec->datalen = datalen;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// make reference on external buffer

void base::Buffer::makereferenceof(void* buf, unsigned datalen)
{
   reset();

   if (!buf || (datalen == 0)) return;

   fRec = (RawDataRec*) malloc(sizeof(RawDataRec));
   if (!fRec) {
      printf("Buffer allocation error makereference of sz %ld\n", (long) sizeof(RawDataRec));
      return;
   }

   fRec->reset();

   fRec->refcnt = 1;

   fRec->buf = buf;

   fRec->datalen = datalen;
}
