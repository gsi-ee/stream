#include "hadaq/TdcMessage.h"

unsigned hadaq::TdcMessage::gFineMinValue = 20;
unsigned hadaq::TdcMessage::gFineMaxValue = 500;

//////////////////////////////////////////////////////////////////////////////////////////////
/// set as hit2

void hadaq::TdcMessage::setAsHit2(uint32_t finebin)
{
   fData = (fData & ~tdckind_Mask) | tdckind_Hit2; // mark as message 1
   if (finebin>=0x3ff) {
      fData |= (0x3ff << 12);
   } else {
      fData = (fData & ~(0x3ff << 12)) | (finebin << 12);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// print message

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

//////////////////////////////////////////////////////////////////////////////////////////////
/// print v4 message

void hadaq::TdcMessage::print4(uint32_t &ttype)
{
   if (isHDR()) {
      printf("    0x%08x HDR version:%u.%u type:0x%x trigger:%u\n", fData, getHDRMajor(), getHDRMinor(), getHDRTType(), getHDRTrigger());
      ttype = getHDRTType();
   } else if(isEPOC())
      printf("    0x%08x EPOC 0x%07x\n", fData, getEPOC());
   else if(isTMDR())
      printf("    0x%08x TMDR mode:0x%x coarse:%u fine:%u\n", fData, getTMDRMode(), getTMDRCoarse(), getTMDRFine());
   else if(isTMDT())
      printf("    0x%08x TMDT mode:0x%x ch:%u coarse:%u fine:%u\n", fData, getTMDTMode(), getTMDTChannel(), getTMDTCoarse(), getTMDTFine());
   else if (isTRL()) {
      switch (ttype) {
         case 0x4:
         case 0x6:
         case 0x7:
         case 0x8:
         case 0x9:
         case 0xE: {
            printf("    0x%08x TRLB eflags:0x%x maxdc:%u tptime:%u freq:%5.3fMHz\n", fData, getTRLBEflags(), getTRLBMaxdc(), getTRLBTptime(), getTRLBFreq()*1e-2);
            break;
         }
         case 0xC: {
            printf("    0x%08x TRLC cpc:0x%x ccs:0x%x ccdiv:%u freq:%5.3fMHz\n", fData, getTRLCCpc(), getTRLCCcs(), getTRLCCcdiv(), getTRLCFreq()*1e-2);
            break;
         }
         case 0x0:
         case 0x1:
         case 0x2:
         case 0xf:
         default: {
            printf("    0x%08x TRLA platform:0x%x version:%u.%u.%u numch:%u\n", fData, getTRLAPlatformId(), getTRLAMajor(), getTRLAMinor(), getTRLASub(), getTRLANumCh());
         }
      }
   }
}
