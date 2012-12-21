#include "nx/Iterator.h"

nx::Iterator::Iterator(int fmt) :
   base::Iterator(fmt),
   fEpoch(0),
   fMsg(),
   fConv(),
   fCorrecion(false),
   fLastNxHit(),
   fNxTmDistance(0),
   fVerifyRes(0)
{
   // we have 14 bits for stamp and 32 bits for epoch
   fConv.SetTimeSystem(14+32, 1e-9);
}

nx::Iterator::~Iterator()
{
}

void nx::Iterator::SetCorrection(bool enabled, unsigned nxmask, unsigned nx_distance)
{
   fCorrecion = enabled;

   fNxTmDistance = nx_distance;

   fLastNxHit.clear();

   if (fCorrecion) {
      unsigned numnx(0);
      while (nxmask != 0) {
         numnx++;
         nxmask = nxmask >> 1;
      }
      if (numnx < 4) numnx = 4;

      for (unsigned n=0;n<numnx;n++)
         fLastNxHit.push_back(0);
   }
}

void nx::Iterator::VerifyMessage()
{
   // try to make as simple as possible
   // if last-epoch specified, one should check FIFO fill status

   // return 1 - last epoch is ok,
   //        0 - no last epoch,
   //       -1 - last epoch err,
   //       -2 - next epoch error
   //       -3 - any other error
   //       -5 - last epoch err, MSB err

   unsigned nxid = fMsg.getNxNumber();

   uint64_t fulltm = FullTimeStamp(fEpoch, fMsg.getNxTs());

   int res = 0;

   if (fMsg.getNxLastEpoch()) {

      // do not believe that message can have last-epoch marker
      if (fMsg.getNxTs()<15800) res = -1; else
      // fifo fill status should be equal to 1 - most probable situation
      if (((fMsg.getNxLtsMsb()+7) & 0x7) != ((fMsg.getNxTs()>>11)&0x7)) res = -5; else
      // if message too far in past relative to previous
      if (fulltm + fNxTmDistance < fLastNxHit[nxid]) res = -1; else
      // we decide that lastepoch bit is set correctly
      res = 1;

   } else {

      // imperical 800 ns value
      if (fulltm + fNxTmDistance < fLastNxHit[nxid]) {
         if (fMsg.getNxTs() < 30) res = -2;
                             else res = -3;
      }
   }

   fLastNxHit[nxid] = fulltm;
   if ((res==-1) || (res==-5)) {
      fLastNxHit[nxid] += 16*1024;
      fMsg.setNxLastEpoch(0);
   }

   fVerifyRes = res;
}



void nx::Iterator::setRocNumber(uint16_t rocnum)
{
   if (fFormat == base::formatEth2)
      fMsg.setRocNumber(rocnum);
}

void nx::Iterator::printMessage(unsigned kind)
{
   msg().printData(kind, fEpoch, getMsgTime());
}


void nx::Iterator::printMessages(unsigned cnt, unsigned kind)
{
   while (cnt-- > 0) {
      if (!next()) return;
      msg().printData(kind, fEpoch, getMsgTime());
   }
}
