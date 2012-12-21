#ifndef BASE_ITERATOR_H
#define BASE_ITERATOR_H

#include "base/Message.h"

namespace base {

   class Iterator {
      protected:
         int           fFormat;       // format identifier
         void*         fBuffer;       // assigned buffer
         uint32_t      fBufferLen;    // length of assigned buffer
         uint32_t      fBufferPos;    // current position
         uint32_t      fMsgSize;      // size of single message
         uint32_t      fEpoch;        // current epoch

      public:
         Iterator(int fmt = formatNormal);

         ~Iterator();

         void setFormat(int fmt);
         int getFormat() const { return fFormat; }

         uint32_t getMsgSize() const { return fMsgSize; }

         bool assign(void* buf, uint32_t len);

         /** Return epoch value for current message */
         inline uint32_t getMsgEpoch() const { return fEpoch; }

         // returns true is last message was extracted from the buffer
         inline bool islast() const { return  fBufferPos >= fBufferLen; }

         /** Return last epoch value */
         uint32_t getLastEpoch() const { return fEpoch; }
   };
}


#endif
