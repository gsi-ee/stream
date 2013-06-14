#include "get4/Iterator.h"

get4::Iterator::Iterator(int fmt) :
   base::Iterator(fmt),
   fEpoch(0),
   fMsg(),
   fConvRoc(),
   fConvGet4()
{
   for (unsigned n=0;n<MaxGet4;n++) fEpoch2[n] = 0;

   fConvRoc.SetTimeSystem(14+32, 1e-9);

   // this is 512 ns shift between ROC clocks and GET4 clocks
   // we decide to add 512 ns to GET4 times
   fConvGet4.SetT0(-512*20);

   // time stamp itself is 19 bits plus 24 bit of GET4 epoch
   // bining is 50 ps
   fConvGet4.SetTimeSystem(19+24, 50e-12);

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
   msg().printData(kind, fEpoch, getMsgTime());
}


void get4::Iterator::printMessages(unsigned cnt, unsigned kind)
{
   while (cnt-- > 0) {
      if (!next()) return;
      msg().printData(kind, fEpoch, getMsgTime());
   }
}
