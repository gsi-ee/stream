#include "get4/Iterator.h"

get4::Iterator::Iterator(int fmt) :
   base::Iterator(fmt),
   fMsg()
{
   for (unsigned n=0;n<MaxGet4;n++) fEpoch2[n] = 0;
}

get4::Iterator::~Iterator()
{
}

void get4::Iterator::setRocNumber(uint16_t rocnum)
{
   if (fFormat == base::formatEth2)
      fMsg.setRocNumber(rocnum);
}

void get4::Iterator::printMessage(unsigned kind)
{
   msg().printData(kind, getMsgEpoch());
}


void get4::Iterator::printMessages(unsigned cnt, unsigned kind)
{
   while (cnt-- > 0) {
      if (!next()) return;
      msg().printData(kind, getMsgEpoch());
   }
}
