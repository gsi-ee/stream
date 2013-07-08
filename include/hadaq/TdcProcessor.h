#ifndef HADAQ_TDCPROCESSOR_H
#define HADAQ_TDCPROCESSOR_H

#include "base/StreamProc.h"

#include <map>

#include "hadaq/defines.h"
#include "hadaq/TdcMessage.h"
#include "hadaq/TdcIterator.h"

namespace hadaq {

   class TrbProcessor;
   class TdcProcessor;

   typedef std::map<unsigned,TdcProcessor*> SubProcMap;


   enum { FineCounterBins = 600 };

   /** This is specialized sub-processor for FPGA-TDC.
    * Normally it should be used together with TrbProcessor,
    * which the only can provide data  */

   class TdcProcessor : public base::StreamProc {

      friend class TrbProcessor;

      protected:

         enum { rising_edge = 1, falling_edge = 2 };

         struct ChannelRec {
            unsigned refch;                //! reference channel for specified
            unsigned reftdc;               //! tdc of reference channel,
            bool docalibr;                 //! if false, simple calibration will be used
            base::H1handle fRisingFine;    //! histogram of all fine counters
            base::H1handle fRisingCoarse;  //! histogram of all coarse counters
            base::H1handle fRisingRef;     //! histogram of all coarse counters
            base::H1handle fRisingCoarseRef; //! histogram
            base::H2handle fRisingRef2D; //! histogram
            base::H1handle fRisingCalibr;  //! histogram of channel calibration function
            base::H1handle fFallingFine;   //! histogram of all fine counters
            base::H1handle fFallingCoarse; //! histogram of all coarse counters
            base::H1handle fFallingRef;    //! histogram of all coarse counters
            base::H1handle fFallingCoarseRef; //! histogram
            base::H1handle fFallingCalibr; //! histogram of channel calibration function
            double first_rising_tm;
            double first_falling_tm;
            unsigned first_rising_coarse;
            unsigned first_falling_coarse;
            unsigned first_rising_fine;
            unsigned first_falling_fine;
            long all_rising_stat;
            long all_falling_stat;
            long rising_stat[FineCounterBins];
            float rising_calibr[FineCounterBins];
            long falling_stat[FineCounterBins];
            float falling_calibr[FineCounterBins];


            ChannelRec() :
               refch(0xffffff),
               reftdc(0xffffffff),
               docalibr(true),
               fRisingFine(0),
               fRisingCoarse(0),
               fRisingRef(0),
               fRisingCoarseRef(0),
               fRisingRef2D(0),
               fRisingCalibr(0),
               fFallingFine(0),
               fFallingCoarse(0),
               fFallingRef(0),
               fFallingCoarseRef(0),
               fFallingCalibr(0),
               first_rising_tm(0.),
               first_falling_tm(0.),
               first_rising_coarse(0),
               first_falling_coarse(0),
               first_rising_fine(0),
               first_falling_fine(0),
               all_rising_stat(0),
               all_falling_stat(0)
            {
               for (unsigned n=0;n<FineCounterBins;n++) {
                  rising_stat[n] = 0;
                  rising_calibr[n] = hadaq::TdcMessage::SimpleFineCalibr(n);
                  falling_stat[n] = 0;
                  falling_calibr[n] = hadaq::TdcMessage::SimpleFineCalibr(n);
               }
            }
         };


         TdcIterator fIter1;         //! iterator for the first scan
         TdcIterator fIter2;         //! iterator for the second scan

         base::H1handle fMsgPerBrd;  //! messages per board
         base::H1handle fErrPerBrd;  //! errors per board
         base::H1handle fHitsPerBrd; //! data hits per board

         base::H1handle fChannels;   //! histogram with messages per channel
         base::H1handle fErrors;     //! histogram with errors per channel
         base::H1handle fUndHits;    //! histogram with undetected hits per channel
         base::H1handle fMsgsKind;   //! messages kinds
         base::H1handle fAllFine;    //! histogram of all fine counters
         base::H1handle fAllCoarse;  //! histogram of all coarse counters

         std::vector<ChannelRec>  fCh; //! histogram for individual channels

         bool        fNewDataFlag;    //! flag used by TRB processor to indicate if new data was added
         unsigned    fEdgeMask;       //! fill histograms 1 - rising, 2 - falling, 3 - both
         long        fAutoCalibration;//! indicates minimal number of counts in each channel required to produce calibration

         std::string fWriteCalibr;    //! file which should be written at the end of data processing

         bool      fPrintRawData;     //! if true, raw data will be printed

         bool      fEveryEpoch;       //! if true, each hit must be supplied with epoch

         bool      fCrossProcess;     //! if true, AfterFill will be called by Trb processor

         /** Returns true when processor used to select trigger signal
          * TDC not yet able to perform trigger selection */
         virtual bool doTriggerSelection() const { return false; }

         static unsigned fMaxBrdId;     //! maximum board id
         static unsigned fFineMinValue; //! minimum value for fine counter, used for simple linear approximation
         static unsigned fFineMaxValue; //! maximum value for fine counter, used for simple linear approximation

         void SetNewDataFlag(bool on) { fNewDataFlag = on; }
         bool IsNewDataFlag() const { return fNewDataFlag; }

         /** Method will be called by TRB processor if SYNC message was found
          * One should change 4 first bytes in the last buffer in the queue */
         void AppendTrbSync(uint32_t syncid);

         /** This is maximum disorder time for TDC messages
          * TODO: derive this value from sub-items */
         virtual double MaximumDisorderTm() const { return 2e-6; }

         /** Scan all messages, find reference signals */
         bool DoBufferScan(const base::Buffer& buf, bool isfirst);

         /** These methods used to fill different raw histograms during first scan */
         void BeforeFill();
         void AfterFill(SubProcMap* subproc = 0);

         void CalibrateChannel(unsigned nch, long* statistic, float* calibr);
         void CopyCalibration(float* calibr, base::H1handle hcalibr);
         void ProduceCalibration(bool clear_stat);
         void StoreCalibration(const std::string& fname);

      public:

         TdcProcessor(TrbProcessor* trb, unsigned tdcid, unsigned numchannels = MaxNumTdcChannels, unsigned edge_mask = 1);
         virtual ~TdcProcessor();

         static void SetMaxBoardId(unsigned max) { fMaxBrdId = max; }

         inline unsigned NumChannels() const { return fCh.size(); }
         inline bool DoRisingEdge() const { return fEdgeMask & rising_edge; }
         inline bool DoFallingEdge() const { return fEdgeMask & falling_edge; }

         int GetNumHist() const { return 6; }

         const char* GetHistName(int k) const {
            switch(k) {
               case 0: return "RisingFine";
               case 1: return "RisingCoarse";
               case 2: return "RisingRef";
               case 3: return "FallingFine";
               case 4: return "FallingCoarse";
               case 5: return "FallingRef";
            }
            return "";
         }

         base::H1handle GetHist(unsigned ch, int k = 0) {
            if (ch>=NumChannels()) return 0;
            switch (k) {
               case 0: return fCh[ch].fRisingFine;
               case 1: return fCh[ch].fRisingCoarse;
               case 2: return fCh[ch].fRisingRef;
               case 3: return fCh[ch].fFallingFine;
               case 4: return fCh[ch].fFallingCoarse;
               case 5: return fCh[ch].fFallingRef;
            }
            return 0;
         }

         void DisableCalibrationFor(unsigned firstch, unsigned lastch = 0);

         /** Set reference signal for the TDC channel ch
          * \param refch   specifies number of reference channel
          * \param reftdc  specifies tdc id, used for ref channel.
          * Default (0xffffffff) same TDC will be used
          * To be able use other TDCs, one should enable TTrbProcessor::SetCrossProcess(true);   */
         void SetRefChannel(unsigned ch, unsigned refch, unsigned reftdc = 0xffffffff);

         /** If set, each hit must be supplied with epoch message */
         void SetEveryEpoch(bool on) { fEveryEpoch = on; }
         bool IsEveryEpoch() const { return fEveryEpoch; }

         void SetAutoCalibration(long cnt = 100000) { fAutoCalibration = cnt; }

         bool LoadCalibration(const std::string& fname);

         void SetWriteCalibration(const std::string& fname) { fWriteCalibr = fname; }

         void SetPrintRawData(bool on = true) { fPrintRawData = on; }
         bool IsPrintRawData() const { return fPrintRawData; }

         virtual void UserPreLoop();

         virtual void UserPostLoop();


         /** Scan all messages, find reference signals
          * if returned false, buffer has error and must be discarded */
         virtual bool FirstBufferScan(const base::Buffer& buf)
         {
            return DoBufferScan(buf, true);
         }

         /** Scan buffer for selecting them inside trigger */
         virtual bool SecondBufferScan(const base::Buffer& buf)
         {
            return DoBufferScan(buf, false);
         }
   };

}

#endif
