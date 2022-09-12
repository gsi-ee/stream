#ifndef BASE_BUFFER_H
#define BASE_BUFFER_H

#include "base/TimeStamp.h"

namespace base {

   /** Internal raw data for base::Buffer */

   struct RawDataRec {
      int           refcnt{0};   ///< number of references

      unsigned      kind{0};       ///< like ROC event, SPADIC or MBS or ..
      unsigned      boardid{0};    ///< board id
      unsigned      format{0};     ///< raw data format like optic, Ethernet for ROC and so on

      GlobalTime_t  local_tm{0};    ///< buffer head time in local scale,
      GlobalTime_t  global_tm{0};   ///< buffer head time in global time

      void*         buf{nullptr};        ///< raw data
      unsigned      datalen{0};    ///< length of raw data

      unsigned      user_tag{0};   ///< arbitrary data, can be used for any additional data

      /** constructor */
      RawDataRec() : refcnt(0), kind(0), boardid(0), format(0), local_tm(0), global_tm(0), buf(nullptr), datalen(0), user_tag(0) {}

      /** reset */
      void reset()
      {
         refcnt = 0;
         kind = 0;
         boardid = 0;
         format = 0;
         local_tm = 0;
         global_tm = 0;
         buf = nullptr;
         datalen = 0;
         user_tag = 0;
      }
   };

   /** Memory management class
    *
    * \ingroup stream_core_classes
    *
    * It allows to keep many references on same raw data and automatically release it  */

   class Buffer {
      protected:
         RawDataRec* fRec;         ///< data
      public:
         /** constructor */
         Buffer() : fRec(nullptr) {}
         /** constructor */
         Buffer(const Buffer& src) : fRec(src.fRec) { if (fRec) fRec->refcnt++; }
         /** destructor */
         ~Buffer() { reset(); }

         /** assign operator */
         Buffer& operator=(const Buffer& src)
         {
            reset();
            fRec = src.fRec;
            if (fRec) fRec->refcnt++;
            return *this;
         }

         /** returns true if empty */
         bool null() const { return fRec==nullptr; }

         void reset();

         /** access operator */
         RawDataRec& operator()(void) const { return *fRec; }

         /** access operator */
         RawDataRec& rec() const { return *fRec; }

         /** Returns length of memory buffer */
         unsigned datalen() const { return fRec ? fRec->datalen : 0; }

         /** Change length of memory buffer - only can be reduced */
         bool setdatalen(unsigned newlen);

         /** return pointer with shift */
         void* ptr(unsigned shift = 0) const { return fRec ? (shift <= fRec->datalen ? (char*)fRec->buf + shift : nullptr) : nullptr; }

         /** get uint32_t at given index */
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
