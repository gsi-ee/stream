#include "base/Iterator.h"

base::Iterator::Iterator(int fmt) :
   fFormat(fmt),
   fBuffer(0),
   fBufferLen(0),
   fBufferPos(0),
   fMsgSize(0),
   fEpoch(0)
{
   fMsgSize = base::Message::RawSize(fFormat);
}

base::Iterator::Iterator(const Iterator& src) :
   fFormat(src.fFormat),
   fBuffer(src.fBuffer),
   fBufferLen(src.fBufferLen),
   fBufferPos(src.fBufferPos),
   fMsgSize(src.fMsgSize),
   fEpoch(src.fEpoch)
{
}

base::Iterator::~Iterator()
{
}


void base::Iterator::setFormat(int fmt)
{
   fFormat = fmt;
   fMsgSize = base::Message::RawSize(fFormat);
}


void base::Iterator::resetEpochs()
{
   fEpoch = 0;
}


bool base::Iterator::assign(void* buf, uint32_t len)
{
   fBuffer = buf;
   fBufferLen = len;
   fBufferPos = 0;

   return len >= fMsgSize;
}
