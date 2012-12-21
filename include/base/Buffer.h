#ifndef BASE_BUFFER_H
#define BASE_BUFFER_H

#include "base/TimeStamp.h"

namespace base {

   /** Class base::Buffer is for memory management
    * It allows to keep many references on same raw data and automatically release it  */

   struct RawDataRec {
      int           refcnt;   // number of references

      unsigned      kind;       // like ROC event, SPADIC or MBS or ..
      unsigned      boardid;    // board id
      unsigned      format;     // raw data format like optic, Ethernet for ROC and so on

      GlobalTime_t  local_tm;    // buffer head time in local scale,
      GlobalTime_t  global_tm;   // buffer head time in global time

      void*         buf;        // raw data
      unsigned      datalen;    // length of raw data

      RawDataRec() : refcnt(0), kind(0), boardid(0), format(0), local_tm(0), global_tm(0), buf(0), datalen(0) {}

      void reset()
      {
         refcnt = 0;
         kind = 0;
         boardid = 0;
         format = 0;
         local_tm = 0;
         global_tm = 0;
         buf = 0;
         datalen = 0;
      }
   };

   class Buffer {
      protected:
         RawDataRec* fRec;
      public:
         Buffer() : fRec(0) {}
         Buffer(const Buffer& src) : fRec(src.fRec) { if (fRec) fRec->refcnt++; }
         ~Buffer() { reset(); }

         Buffer& operator=(const Buffer& src)
         {
            reset();
            fRec = src.fRec;
            if (fRec) fRec->refcnt++;
            return *this;
         }

         bool null() const { return fRec==0; }

         void reset();

         RawDataRec& operator()(void) const { return *fRec; }

         RawDataRec& rec() const { return *fRec; }

         unsigned datalen() const { return fRec ? fRec->datalen : 0; }

         void* ptr(unsigned shift = 0) const { return fRec ? (shift <= fRec->datalen ? (char*)fRec->buf + shift : 0) : 0; }

         uint32_t getuint32(unsigned indx) const { return ((uint32_t*) ptr())[indx]; }


         /** Method produces empty buffer with
          * specified amount of memory */
         void makenew(unsigned datalen);

         /** Method produces buffer instance with deep copy of provided raw data
          * Means extra memory will be allocated and content of source data will be copied*/
         void makecopyof(void* buf, unsigned datalen);

         /** Method produces buffer instance with reference to provided raw data
          * Means buffer will only contain pointer of source data
          * Source data should exists until single instance of buffer is existing */
         void makereferenceof(void* buf, unsigned datalen);


   };

}

#endif
