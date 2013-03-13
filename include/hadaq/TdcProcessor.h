#ifndef HADAQ_TDCPROCESSOR_H
#define HADAQ_TDCPROCESSOR_H

#include "base/StreamProc.h"

#include "hadaq/defines.h"

#include "hadaq/TdcMessage.h"
#include "hadaq/TdcIterator.h"

namespace hadaq {

   class TrbProcessor;

   enum { FineCounterBins = 1024 };

   /** This is specialized sub-processor for FPGA-TDC.
    * Normally it should be used together with TrbProcessor,
    * which the only can provide data  */

   class TdcProcessor : public base::StreamProc {

      friend class TrbProcessor;

      protected:
         struct ChannelRec {
            bool disabled;                 //! if disabled, channel will be ignored
            unsigned refch;                //! reference channel for specified
            bool docalibr;                 //! if flase, simple calibration will be used
            base::H1handle fRisingFine;    //! histogram of all fine counters
            base::H1handle fRisingCoarse;  //! histogram of all coarse counters
            base::H1handle fRisingRef;     //! histogram of all coarse counters
            base::H1handle fRisingCalibr;  //! histogram of channel calibration function
            base::H1handle fFallingFine;   //! histogram of all fine counters
            base::H1handle fFallingCoarse; //! histogram of all coarse counters
            base::H1handle fFallingRef;    //! histogram of all coarse counters
            base::H1handle fFallingCalibr; //! histogram of channel calibration function
            double last_rising_tm;
            double last_falling_tm;
            long rising_stat[FineCounterBins];
            float rising_calibr[FineCounterBins];
            long falling_stat[FineCounterBins];
            float falling_calibr[FineCounterBins];


            ChannelRec() :
               disabled(false),
               refch(0),
               docalibr(true),
               fRisingFine(0),
               fRisingCoarse(0),
               fRisingRef(0),
               fRisingCalibr(0),
               fFallingFine(0),
               fFallingCoarse(0),
               fFallingRef(0),
               fFallingCalibr(0),
               last_rising_tm(0.),
               last_falling_tm(0.)
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

         base::H1handle fChannels;  //! histogram with messages per channel
         base::H1handle fMsgsKind;  //! messages kinds
         base::H1handle fAllFine;   //! histogram of all fine counters
         base::H1handle fAllCoarse; //! histogram of all coarse counters

         ChannelRec fCh[NumTdcChannels]; //! histogram for individual channles

         bool   fNewDataFlag;         //! flag used by TRB processor to indicate if new data was added
         bool   fAllHistos;           //! fill all possible histograms

         std::string fWriteCalibr;    //! file which should be written at the end of data processing

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
         void FillHistograms();
         void AfterFill();

         void CalibrateChannel(long* statistic, float* calibr);
         void CopyCalibration(float* calibr, base::H1handle hcalibr);
         void ProduceCalibration();
         void StoreCalibration(const std::string& fname);


      public:

         TdcProcessor(TrbProcessor* trb, unsigned tdcid, unsigned numchannels = NumTdcChannels, bool all_histos = false);
         virtual ~TdcProcessor();

         static void SetMaxBoardId(unsigned max) { fMaxBrdId = max; }

         void SetRefChannel(unsigned ch, unsigned refch);

         void SetWriteCalibr(const std::string& fname) { fWriteCalibr = fname; }

         bool LoadCalibration(const std::string& fname);

         void DisableChannels(int firstch, int lastch = -1);

         void DisableCalibrationFor(int firstch, int lastch = -1);

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
