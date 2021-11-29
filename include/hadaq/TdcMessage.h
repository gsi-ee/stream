#ifndef HADAQ_TDCMESSAGE_H
#define HADAQ_TDCMESSAGE_H

#include <cstdint>

#include <cstdio>

namespace hadaq {

   /** TDC messages kind */
   enum TdcMessageKind {
      tdckind_Trailer  = 0x00000000,  ///< trailer
      tdckind_Header   = 0x20000000,  ///< header
      tdckind_Debug    = 0x40000000,  ///< debug
      tdckind_Epoch    = 0x60000000,  ///< epoch
      tdckind_Mask     = 0xe0000000,  ///< mask
      tdckind_Hit      = 0x80000000,  ///< original hit message
      tdckind_Hit1     = 0xa0000000,  ///< corrected hit message, instead of 0x3ff
      tdckind_Hit2     = 0xc0000000,  ///< fine time replaced with 5ps binning value
      tdckind_Calibr   = 0xe0000000   ///< calibration data for next two hits
   };

   /** special constants */
   enum TdcConstants {
      MaxNumTdcChannels = 65 ///< maximal number of TDC channels
   };

   /** TDC v4 messages kind */
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

   /** \brief TDC message
     *
     * \ingroup stream_hadaq_classes
     *
     * TdcMessage is wrapper for data, produced by FPGA-TDC
     * struct is used to avoid any potential overhead */

   struct TdcMessage {
      protected:
         uint32_t   fData;  ///< message raw data

         static unsigned gFineMinValue;  ///< default fine min
         static unsigned gFineMaxValue;  ///< default fine max

      public:

         /** constructor */
         TdcMessage() : fData(0) {}

         /** constructor */
         TdcMessage(uint32_t d) : fData(d) {}

         /** assign raw data to message */
         void assign(uint32_t d) { fData = d; }

         /** get message raw data */
         inline uint32_t getData() const { return fData; }

         /** assign operator for the message */
         TdcMessage& operator=(const TdcMessage& src) { fData = src.fData; return *this; }

         /** Returns kind of the message
          * If used for the hit message, four different values can be returned */
         inline uint32_t getKind() const { return fData & tdckind_Mask; }
         /** is original hit message */
         inline bool isHit0Msg() const { return getKind() == tdckind_Hit; }
         /** is repaired 0x3fff message */
         inline bool isHit1Msg() const { return getKind() == tdckind_Hit1; }
         /** is hit message with replaced (calibrated) fine counter */
         inline bool isHit2Msg() const { return getKind() == tdckind_Hit2; }
         /** is any of hit message */
         inline bool isHitMsg() const { return isHit0Msg() || isHit1Msg() || isHit2Msg(); }

         /** is epoch message */
         inline bool isEpochMsg() const { return getKind() == tdckind_Epoch; }
         /** is debug message */
         inline bool isDebugMsg() const { return getKind() == tdckind_Debug; }
         /** is header message */
         inline bool isHeaderMsg() const { return getKind() == tdckind_Header; }
         /** is trailer message */
         inline bool isTrailerMsg() const { return getKind() == tdckind_Trailer; }
         /** is calibration message */
         inline bool isCalibrMsg() const { return getKind() == tdckind_Calibr; }

         // methods for epoch

         /** Return Epoch for epoch marker, 28 bit */
         inline uint32_t getEpochValue() const { return fData & 0xFFFFFFF; }
         /** Get reserved bit for epoch, 1 bit */
         inline uint32_t getEpochRes() const { return (fData >> 28) & 0x1; }

         // methods for hit

         /** Returns hit channel ID */
         inline uint32_t getHitChannel() const { return (fData >> 22) & 0x7F; }
         /** Set hit channel */
         inline void setHitChannel(uint32_t chid) { fData = (fData & ~(0x7F << 22)) | ((chid & 0x7F) << 22); }

         /** Returns hit coarse time counter, 11 bit */
         inline uint32_t getHitTmCoarse() const { return fData & 0x7FF; }
         /** Set hit coarse time counter, 11 bit */
         inline void setHitTmCoarse(uint32_t coarse) { fData = (fData & ~0x7FF) | (coarse & 0x7FF); }

         /** Returns hit fine time counter, 10 bit */
         inline uint32_t getHitTmFine() const { return (fData >> 12) & 0x3FF; }

         /** Returns time stamp, which is simple combination coarse and fine counter */
         inline uint32_t getHitTmStamp() const { return (getHitTmCoarse() << 10) | getHitTmFine(); }

         /** Returns hit edge 1 - rising, 0 - falling */
         inline uint32_t getHitEdge() const {  return (fData >> 11) & 0x1; }
         /** Set hit edge */
         inline void setHitEdge(uint32_t edge) { fData = (fData & ~(0x1 << 11)) | ((edge & 0x1) << 11); }

         /** Is rising edge */
         inline bool isHitRisingEdge() const { return getHitEdge() == 0x1; }
         /** Is falling edge */
         inline bool isHitFallingEdge() const { return getHitEdge() == 0x0; }

         void setAsHit2(uint32_t finebin);

         /** Returns hit reserved value, 2 bits */
         inline uint32_t getHitReserved() const { return (fData >> 29) & 0x3; }

         // methods for calibration message

         /** get calibration for fine counter */
         inline uint32_t getCalibrFine(unsigned n = 0) const { return (fData >> n*14) & 0x3fff; }
         /** set calibration for fine counter */
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

         /** is V4 header */
         inline bool IsVer4Header() const { return isHeaderMsg() && (getHeaderFmt() == 4); }

         // methods for debug message

         /** Return error bits of header message */
         inline uint32_t getDebugKind() const { return (fData >> 24) & 0xF; }

         /** Return reserved bits of header message */
         inline uint32_t getDebugValue() const { return fData  & 0xFFFFFF; }


         // methods for ver4 messages

         /** is v4 HDR message */
         inline bool isHDR() const { return (fData & newkind_Mask3) == newkind_HDR; }
         /** is v4 EPOC message */
         inline bool isEPOC() const { return (fData & newkind_Mask3) == newkind_EPOC; }
         /** is v4 TMDR message */
         inline bool isTMDR() const { return (fData & newkind_Mask7) == newkind_TMDR; }
         /** is v4 TMDT message */
         inline bool isTMDT() const { return (fData & newkind_TMDT) == newkind_TMDT; }
         /** is v4 TMDS message */
         inline bool isTMDS() const { return (fData & newkind_Mask4) == newkind_TMDS; }
         /** is v4 TRL message */
         inline bool isTRL() const { return (fData & newkind_Mask3) == newkind_TRL; }


         // method for HDR
         /** HDR major */
         inline uint32_t getHDRMajor() const { return (fData >> 24) & 0xF; }
         /** HDR minor */
         inline uint32_t getHDRMinor() const { return (fData >> 20) & 0xF; }
         /** HDR type */
         inline uint32_t getHDRTType() const { return (fData >> 16) & 0xF; }
         /** HDR trigger */
         inline uint32_t getHDRTrigger() const { return (fData & 0xFFFF); }


         /** Return Epoch for EPOC marker, 28 bit */
         inline uint32_t getEPOC() const { return fData & 0xFFFFFFF; }
         /** Is EPOC marker error */
         inline bool getEPOCError() const { return (fData & 0x10000000) != 0; }

         // methods for TMDT - hist message
         /** TMDT mode */
         inline uint32_t getTMDTMode() const { return (fData >> 27) & 0xF; }
         /** TMDT channel */
         inline uint32_t getTMDTChannel() const { return (fData >> 21) & 0x3F; }
         /** TMDT coarse */
         inline uint32_t getTMDTCoarse() const { return (fData >> 9) & 0xFFF; }
         /** TMDT fine */
         inline uint32_t getTMDTFine() const { return fData & 0x1FF; }

         // methods for TMDR - ref channel message
         /** TMDR mode */
         inline uint32_t getTMDRMode() const { return (fData >> 21) & 0xF; }
         /** TMDR coarse */
         inline uint32_t getTMDRCoarse() const { return (fData >> 9) & 0xFFF; }
         /** TMDR fine */
         inline uint32_t getTMDRFine() const { return fData & 0x1FF; }

         // methods for TMDS - sampling TDC message
         /** TMDS channel */
         inline uint32_t getTMDSChannel() const { return (fData >> 21) & 0x7F; }
         /** TMDS coarse */
         inline uint32_t getTMDSCoarse() const { return (fData >> 9) & 0xFFF; }
         /** TMDS pattern */
         inline uint32_t getTMDSPattern() const { return fData & 0x1FF; }

         // methods for TRLA
         /** TRLA platform id  */
         inline uint32_t getTRLAPlatformId() const { return (fData  >> 20) & 0xff; }
         /** TRLA major */
         inline uint32_t getTRLAMajor() const { return (fData >> 16) & 0xf; }
         /** TRLA minor */
         inline uint32_t getTRLAMinor() const { return (fData >> 12) & 0xf; }
         /** TRLA sub */
         inline uint32_t getTRLASub() const { return (fData >> 8) & 0xf; }
         /** TRLA numch */
         inline uint32_t getTRLANumCh() const { return (fData & 0x7F) + 1; }

         // methods for TRLB
         /** TRLB eflags */
         inline uint32_t getTRLBEflags() const { return (fData >> 24) & 0xF; }
         /** TRLB maxdc */
         inline uint32_t getTRLBMaxdc() const { return (fData >> 20) & 0xF; }
         /** TRLB tptime */
         inline uint32_t getTRLBTptime() const { return (fData >> 16) & 0xF; }
         /** TRLB freq */
         inline uint32_t getTRLBFreq() const { return (fData & 0xFFFF); }

         // methods for TRLC
         /** TRLC cpc */
         inline uint32_t getTRLCCpc() const { return (fData >> 24) & 0x7; }
         /** TRLC ccs */
         inline uint32_t getTRLCCcs() const { return (fData >> 20) & 0xF; }
         /** TRLC ccdiv */
         inline uint32_t getTRLCCcdiv() const { return (fData >> 16) & 0xF; }
         /** TRLC freq */
         inline uint32_t getTRLCFreq() const { return (fData & 0xFFFF); }

         void print(double tm = -1.);
         void print4(uint32_t &ttype);

         /** default coarse unit for 200 MHz */
         static double CoarseUnit() { return 5e-9; }

         /** default coarse unit for 280 MHz */
         static double CoarseUnit280() { return 1/2.8e8; }

         /** get simple linear calibration for fine counter */
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

         /** get pre-configured min fine counter*/
         static unsigned GetFineMinValue() { return gFineMinValue; }
         /** get pre-configured max fine counter*/
         static unsigned GetFineMaxValue() { return gFineMaxValue; }
   };

}

#endif
