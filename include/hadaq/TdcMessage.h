#ifndef HADAQ_TDCMESSAGE_H
#define HADAQ_TDCMESSAGE_H

#include <cstdint>

#include <cstdio>

namespace hadaq {

   enum TdcMessageKind {
      tdckind_Trailer  = 0x00000000,
      tdckind_Header   = 0x20000000,
      tdckind_Debug    = 0x40000000,
      tdckind_Epoch    = 0x60000000,
      tdckind_Mask     = 0xe0000000,
      tdckind_Hit      = 0x80000000,  // original hit message
      tdckind_Hit1     = 0xa0000000,  // corrected hit message, instead of 0x3ff
      tdckind_Hit2     = 0xc0000000,  // fine time replaced with 5ps binning value
      tdckind_Calibr   = 0xe0000000   // calibration data for next two hits
   };

   enum TdcConstants {
      MaxNumTdcChannels = 65
   };

   enum TdcNewMessageKinds {
      newkind_TMDT     = 0x80000000,
      // with mask 3
      newkind_Mask3    = 0xE0000000,
      newkind_HDR      = 0x20000000,
      newkind_TRL      = 0x00000000,
      newkind_EPOC     = 0x60000000,
      // with mask 4
      newkind_Mask4    = 0xF0000000,
      newkind_TMDS     = 0x40000000,
      // with mask 6
      newkind_Mask6    = 0xFC000000,
      newkind_TBD      = 0x50000000,
      // with mask 8
      newkind_Mask8    = 0xFF000000,
      newkind_HSTM     = 0x54000000,
      newkind_HSTL     = 0x55000000,
      newkind_HSDA     = 0x56000000,
      newkind_HSDB     = 0x57000000,
      newkind_CTA      = 0x58000000,
      newkind_CTB      = 0x59000000,
      newkind_TEMP     = 0x5A000000,
      newkind_BAD      = 0x5B000000,
      // with mask 9
      newkind_Mask9    = 0xFF800000,
      newkind_TTRM     = 0x5C000000,
      newkind_TTRL     = 0x5C800000,
      newkind_TTCM     = 0x5D000000,
      newkind_TTCL     = 0x5D800000,
      // with mask 7
      newkind_Mask7    = 0xFE000000,
      newkind_TMDR     = 0x5E000000
   };

   /** TdcMessage is wrapper for data, produced by FPGA-TDC
    * struct is used to avoid any potential overhead */

   struct TdcMessage {
      protected:
         uint32_t   fData;

         static unsigned gFineMinValue;
         static unsigned gFineMaxValue;

      public:

         TdcMessage() : fData(0) {}

         TdcMessage(uint32_t d) : fData(d) {}

         void assign(uint32_t d) { fData = d; }

         inline uint32_t getData() const { return fData; }

         /** assign operator for the message */
         TdcMessage& operator=(const TdcMessage& src) { fData = src.fData; return *this; }

         /** Returns kind of the message
          * If used for the hit message, four different values can be returned */
         inline uint32_t getKind() const { return fData & tdckind_Mask; }

         inline bool isHit0Msg() const { return getKind() == tdckind_Hit; } // original hit message
         inline bool isHit1Msg() const { return getKind() == tdckind_Hit1; } // repaired 0x3fff message
         inline bool isHit2Msg() const { return getKind() == tdckind_Hit2; } // with replaced fine counter
         inline bool isHitMsg() const { return isHit0Msg() || isHit1Msg() || isHit2Msg(); }

         inline bool isEpochMsg() const { return getKind() == tdckind_Epoch; }
         inline bool isDebugMsg() const { return getKind() == tdckind_Debug; }
         inline bool isHeaderMsg() const { return getKind() == tdckind_Header; }
         inline bool isTrailerMsg() const { return getKind() == tdckind_Trailer; }
         inline bool isCalibrMsg() const { return getKind() == tdckind_Calibr; }

         // methods for epoch

         /** Return Epoch for epoch marker, 28 bit */
         inline uint32_t getEpochValue() const { return fData & 0xFFFFFFF; }
         /** Get reserved bit for epoch, 1 bit */
         inline uint32_t getEpochRes() const { return (fData >> 28) & 0x1; }

         // methods for hit

         /** Returns hit channel ID */
         inline uint32_t getHitChannel() const { return (fData >> 22) & 0x7F; }

         /** Returns hit coarse time counter, 11 bit */
         inline uint32_t getHitTmCoarse() const { return fData & 0x7FF; }
         inline void setHitTmCoarse(uint32_t coarse) { fData = (fData & ~0x7FF) | (coarse & 0x7FF); }

         /** Returns hit fine time counter, 10 bit */
         inline uint32_t getHitTmFine() const { return (fData >> 12) & 0x3FF; }

         /** Returns time stamp, which is simple combination coarse and fine counter */
         inline uint32_t getHitTmStamp() const { return (getHitTmCoarse() << 10) | getHitTmFine(); }

         /** Returns hit edge 1 - rising, 0 - falling */
         inline uint32_t getHitEdge() const {  return (fData >> 11) & 0x1; }

         inline bool isHitRisingEdge() const { return getHitEdge() == 0x1; }
         inline bool isHitFallingEdge() const { return getHitEdge() == 0x0; }

         void setAsHit2(uint32_t finebin);

         /** Returns hit reserved value, 2 bits */
         inline uint32_t getHitReserved() const { return (fData >> 29) & 0x3; }

         // methods for calibration message

         inline uint32_t getCalibrFine(unsigned n = 0) const { return (fData >> n*14) & 0x3fff; }
         inline void setCalibrFine(unsigned n = 0, uint32_t v = 0) { fData = (fData & ~(0x3fff << n*14)) | ((v & 0x3fff) << n*14); }

         // methods for header

         /** Return error bits of header message */
         inline uint32_t getHeaderErr() const { return fData & 0xFFFF; }

         /** Return hardware type coded in header message */
         inline uint32_t getHeaderHwType() const { return (fData >> 8) & 0xFF; }

         /** Return reserved bits of header message */
         inline uint32_t getHeaderRes() const { return (fData >> 16) & 0xFF; }

         /** Return data format: 0 - normal, 1 - double edges for each hit */
         inline uint32_t getHeaderFmt() const { return (fData >> 24) & 0xF; }

         inline bool IsVer4Header() const { return isHeaderMsg() && (getHeaderFmt() == 4); }

         // methods for debug message

         /** Return error bits of header message */
         inline uint32_t getDebugKind() const { return (fData >> 24) & 0xF; }

         /** Return reserved bits of header message */
         inline uint32_t getDebugValue() const { return fData  & 0xFFFFFF; }


         // methods for ver4 messages

         inline bool isHDR() const { return (fData & newkind_Mask3) == newkind_HDR; }
         inline bool isEPOC() const { return (fData & newkind_Mask3) == newkind_EPOC; }
         inline bool isTMDR() const { return (fData & newkind_Mask7) == newkind_TMDR; }
         inline bool isTMDT() const { return (fData & newkind_TMDT) == newkind_TMDT; }
         inline bool isTRL() const { return (fData & newkind_Mask3) == newkind_TRL; }


         /** Return Epoch for EPOC marker, 28 bit */
         inline uint32_t getEPOC() const { return fData & 0xFFFFFFF; }

         // methods for TMDT - hist message
         inline uint32_t getTMDTMode() const { return (fData >> 27) & 0xF; }
         inline uint32_t getTMDTChannel() const { return (fData >> 21) & 0x3F; }
         inline uint32_t getTMDTCoarse() const { return (fData >> 9) & 0xFFF; }
         inline uint32_t getTMDTFine() const { return fData & 0x1FF; }

         // methods for TMDR - ref channel message
         inline uint32_t getTMDRMode() const { return (fData >> 21) & 0xF; }
         inline uint32_t getTMDRCoarse() const { return (fData >> 9) & 0xFFF; }
         inline uint32_t getTMDRFine() const { return fData & 0x1FF; }




         void print(double tm = -1.);
         void print4(double tm = -1.);

         static double CoarseUnit() { return 5e-9; }

         static double CoarseUnit280() { return 1/2.8e8; }

         static double SimpleFineCalibr(unsigned fine)
         {
            if (fine<=gFineMinValue) return 0.;
            if (fine>=gFineMaxValue) return CoarseUnit();
            return (CoarseUnit() * (fine - gFineMinValue)) / (gFineMaxValue - gFineMinValue);
         }

         /** Method set static limits, which are used for simple interpolation of time for fine counter */
         static void SetFineLimits(unsigned min, unsigned max)
         {
            gFineMinValue = min;
            gFineMaxValue = max;
         }

         static unsigned GetFineMinValue() { return gFineMinValue; }
         static unsigned GetFineMaxValue() { return gFineMaxValue; }
   };

}

#endif
