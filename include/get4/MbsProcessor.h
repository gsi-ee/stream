#ifndef GET4_MBSPROCESSOR_H
#define GET4_MBSPROCESSOR_H

#include "base/StreamProc.h"

namespace get4 {

   enum { NumGet4Channels = 4,
          BinWidthPs = 50 };

   struct Get4MbsRec {
      bool used;

      base::H1handle fChannels;  //! histogram with channels
      base::H1handle fErrors;  //! errors kinds

      //base::H1handle fRisCoarseTm[NumGet4Channels]; ///< histograms of rising stamp for each channel
      //base::H1handle fFalCoarseTm[NumGet4Channels]; ///< histograms of falling stamp for each channel
      base::H1handle fRisFineTm[NumGet4Channels];   ///< histograms of rising stamp for each channel
      base::H1handle fFalFineTm[NumGet4Channels];   ///< histograms of falling stamp for each channel
      base::H1handle fTotTm[NumGet4Channels];       ///< histograms of time-over threshold for each channel

      uint64_t firstr[NumGet4Channels];
      uint64_t firstf[NumGet4Channels];
      uint64_t lastr[NumGet4Channels];
      uint64_t lastf[NumGet4Channels];

      Get4MbsRec() :
         used(false),
         fChannels(0),
         fErrors(0)
      {
         for(unsigned n=0;n<NumGet4Channels;n++) {
           // fRisCoarseTm[n] = 0;
           // fFalCoarseTm[n] = 0;
           fRisFineTm[n] = 0;
           fFalFineTm[n] = 0;
           fTotTm[n] = 0;
         }
      }

      void clear()
      {
         for(unsigned n=0;n<NumGet4Channels;n++) {
            firstr[n] = 0;
            firstf[n] = 0;
            lastr[n] = 0;
            lastf[n] = 0;
         }
      }

      uint64_t gettm(unsigned ch, bool r, bool first = true)
      {
         return r ? (first ? firstr[ch] : lastr[ch]) : (first ? firstf[ch] : lastf[ch]);
      }
   };

   struct Get4MbsRef {
      unsigned g1,ch1, g2, ch2;
      bool r1, r2;
      base::H1handle fHist;  //! histogram with channels

      Get4MbsRef() : g1(0), ch1(0), g2(0), ch2(0), r1(true), r2(true), fHist(0) {}
   };

   /** Only for Get4 debug, produce no output data */

   class MbsProcessor : public base::StreamProc {

      protected:

         base::H1handle fMsgPerGet4;   //! histogram with messages per get4
         base::H1handle fErrPerGet4;   //! histogram with errors per get4

//         base::H2handle fSlMsgPerGet4; //! histogram with slow control per GET4
//         base::H2handle fScalerPerGet4; //! histogram with slow control scaler per GET4
//         base::H2handle fDeadPerGet4; //! histogram with slow control dead time per GET4
//         base::H2handle fSpiPerGet4; //! histogram with slow control Spi per GET4
//         base::H2handle fSeuPerGet4; //! histogram with slow control SEU counter per GET4
//         base::H2handle fDllPerGet4; //! histogram with DLL flag per GET4

         std::vector<get4::Get4MbsRec> GET4;    //! vector with GET4-specific histograms

         std::vector<get4::Get4MbsRef> fRef;    //! arrays with reference between channels

         bool get4_in_use(unsigned id) { return id < GET4.size() ? GET4[id].used : false; }

      public:

         MbsProcessor(unsigned get4mask = 0x3);
         virtual ~MbsProcessor();

         bool AddRef(unsigned g2, unsigned ch2, bool r2,
                     unsigned g1, unsigned ch1, bool r1,
                     int nbins, double min, double max);

         /** Scan all messages, find reference signals */
         virtual bool FirstBufferScan(const base::Buffer& buf);
   };
}

#endif
