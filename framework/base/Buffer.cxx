#include "base/Buffer.h"

#include <stdlib.h>
#include <string.h>

void base::Buffer::reset()
{
   if (fRec!=0) {
      if (--fRec->refcnt == 0)
         free(fRec);
      fRec = 0;
   }
}

void base::Buffer::makenew(unsigned datalen)
{
   reset();

   if (datalen==0) return;

   fRec = (RawDataRec*) malloc(sizeof(RawDataRec) + datalen);

   fRec->reset();

   fRec->refcnt = 1;

   fRec->buf = (char*) fRec + sizeof(RawDataRec);

   fRec->datalen = datalen;
}


void base::Buffer::makecopyof(void* buf, unsigned datalen)
{
   reset();

   if ((buf==0) || (datalen==0)) return;

   fRec = (RawDataRec*) malloc(sizeof(RawDataRec) + datalen);

   fRec->reset();

   fRec->refcnt = 1;

   fRec->buf = (char*) fRec + sizeof(RawDataRec);
   memcpy(fRec->buf, buf, datalen);

   fRec->datalen = datalen;
}

void base::Buffer::makereferenceof(void* buf, unsigned datalen)
{
   reset();

   if ((buf==0) || (datalen==0)) return;

   fRec = (RawDataRec*) malloc(sizeof(RawDataRec));

   fRec->reset();

   fRec->refcnt = 1;

   fRec->buf = buf;

   fRec->datalen = datalen;
}
