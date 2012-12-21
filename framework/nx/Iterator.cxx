#include "nx/Iterator.h"

nx::Iterator::Iterator(int fmt) :
   base::Iterator(fmt),
   fMsg()
{
}

nx::Iterator::~Iterator()
{
}

void nx::Iterator::setRocNumber(uint16_t rocnum)
{
   if (fFormat == base::formatEth2)
      fMsg.setRocNumber(rocnum);
}

void nx::Iterator::printMessage(unsigned kind)
{
   msg().printData(kind, getMsgEpoch());
}


void nx::Iterator::printMessages(unsigned cnt, unsigned kind)
{
   while (cnt-- > 0) {
      if (!next()) return;
      msg().printData(kind, getMsgEpoch());
   }
}
