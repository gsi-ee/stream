#include "base/Iterator.h"

///////////////////////////////////////////////////////////////////////////
/// constructor

base::Iterator::Iterator(int fmt) :
   fFormat(fmt),
   fBuffer(0),
   fBufferLen(0),
   fBufferPos(0),
   fMsgSize(0)
{
   fMsgSize = base::Message::RawSize(fFormat);
}

///////////////////////////////////////////////////////////////////////////
/// destructor

base::Iterator::~Iterator()
{
}

///////////////////////////////////////////////////////////////////////////
/// set format

void base::Iterator::setFormat(int fmt)
{
   fFormat = fmt;
   fMsgSize = base::Message::RawSize(fFormat);
}

///////////////////////////////////////////////////////////////////////////
/// assign buffer

bool base::Iterator::assign(void* buf, uint32_t len)
{
   fBuffer = buf;
   fBufferLen = len;
   fBufferPos = 0;

   return len >= fMsgSize;
}
