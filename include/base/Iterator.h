#ifndef BASE_ITERATOR_H
#define BASE_ITERATOR_H

#include "base/Message.h"

namespace base {

   /** iterator over raw data messages */

   class Iterator {
      protected:
         int           fFormat;       // format identifier
         void*         fBuffer;       // assigned buffer
         uint32_t      fBufferLen;    // length of assigned buffer
         uint32_t      fBufferPos;    // current position
         uint32_t      fMsgSize;      // size of single message

      public:
         Iterator(int fmt = formatNormal);

         ~Iterator();

         void setFormat(int fmt);
         int getFormat() const { return fFormat; }

         /** get message size */
         uint32_t getMsgSize() const { return fMsgSize; }

         bool assign(void* buf, uint32_t len);

         /** returns true is last message was extracted from the buffer */
         inline bool islast() const { return  fBufferPos >= fBufferLen; }

         /** Expanded timestamp for 250 MHz * 14 bit epochs */
         inline static uint64_t FullTimeStamp(uint32_t epoch, uint16_t stamp)
            { return ((uint64_t) epoch << 14) | (stamp & 0x3fff); }
   };
}


#endif
