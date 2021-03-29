#include "hadaq/TdcMessage.h"

unsigned hadaq::TdcMessage::gFineMinValue = 20;
unsigned hadaq::TdcMessage::gFineMaxValue = 500;


void hadaq::TdcMessage::setAsHit2(uint32_t finebin)
{
   fData = (fData & ~tdckind_Mask) | tdckind_Hit2; // mark as message 1
   if (finebin>=0x3ff) {
      fData |= (0x3ff << 12);
   } else {
      fData = (fData & ~(0x3ff << 12)) | (finebin << 12);
   }
}


void hadaq::TdcMessage::print(double tm)
{
   switch (getKind()) {
      case tdckind_Trailer:
         printf("     tdc trailer 0x%08x\n", (unsigned) fData);
         break;
      case tdckind_Header:
         printf("     tdc head   0x%08x\n", (unsigned) fData);
         break;
      case tdckind_Debug:
         printf("     tdc debug  kind: 0x%1x value:0x%06x\n", (unsigned) getDebugKind(), (unsigned) getDebugValue());
         break;
      case tdckind_Epoch:
         printf("     tdc epoch  0x%08x", (unsigned) fData);
         if (tm>=0) printf ("  tm:%9.2f", tm*1e9);
         printf("   epoch 0x%x\n", (unsigned) getEpochValue());
         break;
      case tdckind_Hit:
      case tdckind_Hit1:
      case tdckind_Hit2:
         printf("     tdc hit    0x%08x", (unsigned) fData);
         if (tm>=0) printf ("  tm:%9.2f", tm*1e9);
         printf("   ch %3u isrising:%u tc 0x%03x tf 0x%03x\n",
                 (unsigned) getHitChannel(), (unsigned)getHitEdge(),
                 (unsigned)getHitTmCoarse(), (unsigned)getHitTmFine());
         break;
      case tdckind_Calibr:
         printf("     tdc calibr 0x%08x", (unsigned) fData);
         printf("   fine1 %4u fine2 %4u\n",
                 (unsigned) getCalibrFine(0), (unsigned) getCalibrFine(1));
         break;
      default:
         printf("     tdc unkn   0x%08x   kind %u\n", (unsigned) fData, (unsigned) getKind());
         break;
   }
}

void hadaq::TdcMessage::print4(double)
{
   if (isHDR())
      printf("    0x%08x HDR\n", fData);
   else if(isEPOC())
      printf("    0x%08x EPOC\n", fData);
   else if(isTMDR())
      printf("    0x%08x TMDR\n", fData);
   else if(isTMDT())
      printf("    0x%08x TMDT\n", fData);
   else if (isTRL())
      printf("    0x%08x TRL\n", fData);
}
