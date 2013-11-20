#include "hadaq/TdcMessage.h"

unsigned hadaq::TdcMessage::gFineMinValue = 20;
unsigned hadaq::TdcMessage::gFineMaxValue = 500;

void hadaq::TdcMessage::print(double tm)
{
   switch (getKind()) {
      case tdckind_Reserved:
         printf("     tdc reserv 0x%08x\n", (unsigned) fData);
         break;

      case tdckind_Header:
         printf("     tdc head   0x%08x\n", (unsigned) fData);
         break;
      case tdckind_Debug:
         printf("     tdc debug  0x%08x\n", (unsigned) fData);
         break;
      case tdckind_Epoch:
         printf("     tdc epoch  0x%08x", (unsigned) fData);
         if (tm>=0) printf ("  tm:%9.2f", tm*1e9);
         printf("   epoch 0x%x\n", (unsigned) getEpochValue());
         break;
      case tdckind_Hit:
      case tdckind_Hit1:
      case tdckind_Hit2:
      case tdckind_Hit3:
         printf("     tdc hit    0x%08x", (unsigned) fData);
         if (tm>=0) printf ("  tm:%9.2f", tm*1e9);
         printf("   ch %3u isrising:%u tc 0x%03x tf 0x%03x\n",
                 (unsigned) getHitChannel(), (unsigned)getHitEdge(),
                 (unsigned)getHitTmCoarse(), (unsigned)getHitTmFine());
         break;
      default:
         printf("     tdc unkn   0x%08x   kind %u\n", (unsigned) fData, (unsigned) getKind());
         break;
   }
}
