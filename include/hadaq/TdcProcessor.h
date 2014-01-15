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
            unsigned reftdc;               //! tdc of reference channel
            unsigned doublerefch;          //! double reference channel
            unsigned doublereftdc;         //! tdc of double reference channel
            bool docalibr;                 //! if false, simple calibration will be used
            base::H1handle fRisingFine;    //! histogram of all fine counters
            base::H1handle fRisingCoarse;  //! histogram of all coarse counters
            base::H1handle fRisingMult;    //! number of hits per event
            base::H1handle fRisingRef;     //! histogram of all coarse counters
            base::C1handle fRisingRefCond; //! condition to print raw events
            base::H1handle fRisingCoarseRef; //! histogram
            base::H1handle fRisingCalibr;  //! histogram of channel calibration function
            base::H2handle fRisingRef2D;   //! histogram
            base::H2handle fRisingDoubleRef; //! correlation with dif time from other chanel
            base::H1handle fFallingFine;   //! histogram of all fine counters
            base::H1handle fFallingCoarse; //! histogram of all coarse counters
            base::H1handle fFallingMult;   //! number of hits per event
            base::H1handle fFallingRef;    //! histogram of all coarse counters
            base::C1handle fFallingRefCond; //! condition to print raw events
            base::H1handle fFallingCoarseRef; //! histogram
            base::H1handle fFallingCalibr; //! histogram of channel calibration function
            int rising_cnt;                //! number of rising hits in last event
            int falling_cnt;               //! number of falling hits in last event
            double rising_hit_tm;
            double rising_ref_tm;
            double falling_hit_tm;
            unsigned rising_coarse;
            unsigned falling_coarse;
            unsigned rising_fine;
            unsigned falling_fine;
            long all_rising_stat;
            long all_falling_stat;
            long rising_stat[FineCounterBins];
            float rising_calibr[FineCounterBins];
            long falling_stat[FineCounterBins];
            float falling_calibr[FineCounterBins];
            int rising_cond_prnt;
            int falling_cond_prnt;

            ChannelRec() :
               refch(0xffffff),
               reftdc(0xffffffff),
               doublerefch(0xffffff),
               doublereftdc(0xffffff),
               docalibr(true),
               fRisingFine(0),
               fRisingCoarse(0),
               fRisingMult(0),
               fRisingRef(0),
               fRisingRefCond(0),
               fRisingCoarseRef(0),
               fRisingCalibr(0),
               fRisingRef2D(0),
               fRisingDoubleRef(0),
               fFallingFine(0),
               fFallingCoarse(0),
               fFallingMult(0),
               fFallingRef(0),
               fFallingRefCond(0),
               fFallingCoarseRef(0),
               fFallingCalibr(0),
               rising_cnt(0),
               falling_cnt(0),
               rising_hit_tm(0.),
               rising_ref_tm(0.),
               falling_hit_tm(0.),
               rising_coarse(0),
               falling_coarse(0),
               rising_fine(0),
               falling_fine(0),
               all_rising_stat(0),
               all_falling_stat(0),
               rising_cond_prnt(-1),
               falling_cond_prnt(-1)
            {
               for (unsigned n=0;n<FineCounterBins;n++) {
                  rising_stat[n] = 0;
                  rising_calibr[n] = hadaq::TdcMessage::SimpleFineCalibr(n);
                  falling_stat[n] = 0;
                  falling_calibr[n] = hadaq::TdcMessage::SimpleFineCalibr(n);
               }
            }
         };

         TrbProcessor* fTrb;         //! pointer on TRB processor

         TdcIterator fIter1;         //! iterator for the first scan
         TdcIterator fIter2;         //! iterator for the second scan

         base::H1handle fMsgPerBrd;  //! messages per board - from TRB
         base::H1handle fErrPerBrd;  //! errors per board - from TRB
         base::H1handle fHitsPerBrd; //! data hits per board - from TRB

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

         bool      fUseLastHit;       //! if true, last hit will be used in reference calculations

         bool      fUseNativeTrigger;  //! if true, TRB3 trigger is used as event time

         bool      fCompensateEpochReset; //! if true, compensates epoch reset

         unsigned  fCompensateEpochCounter;  //! counter to compensate epoch reset

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

         bool CheckPrintError();

         bool CreateChannelHistograms(unsigned ch);

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
               case 6: return "RisingMult";
               case 7: return "FallingMult";
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
               case 6: return fCh[ch].fRisingMult;
               case 7: return fCh[ch].fFallingMult;
            }
            return 0;
         }

         /** Create basic histograms for specified channels.
          * If array not specified, histograms for all channels are created.
          * In array last element must be 0 or out of channel range. Call should be like:
          * int channels[] = {33, 34, 35, 36, 0};
          * tdc->CreateHistograms( channels ); */
         void CreateHistograms(int *arr = 0);

         void DisableCalibrationFor(unsigned firstch, unsigned lastch = 0);

         /** Set reference signal for the TDC channel ch
          * \param refch   specifies number of reference channel
          * \param reftdc  specifies tdc id, used for ref channel.
          * Default (0xffffffff) same TDC will be used
          * To be able use other TDCs, one should enable TTrbProcessor::SetCrossProcess(true);
          * If left-right range are specified, ref histograms are created.
          * If twodim==true, 2-D histogram which will accumulate correlation between
          * time difference to ref channel and:
          *   fine_counter (shift 0 ns)
          *   fine_counter of ref channel (shift -1 ns)
          *   coarse_counter/4 (shift -2 ns) */
         void SetRefChannel(unsigned ch, unsigned refch, unsigned reftdc = 0xffff, int npoints = 5000, double left = -10., double right = 10., bool twodim = false);

         /** Fill double-reference histogram
          * Required that for both channels references are specified via SetRefChannel() command.
          * If ch2 > 1000, than channel from other TDC can be used. tdcid = (ch2 - 1000) / 1000 */
         bool SetDoubleRefChannel(unsigned ch1, unsigned ch2,
                                  int npx = 200, double xmin = -10., double xmax = 10.,
                                  int npy = 200, double ymin = -10., double ymax = 10.);

         /** Enable print of TDC data when time difference to ref channel belong to specified interval
          * Work ONLY when reference channel 0 is used.
          * One could set maximum number of events to print
          * In any case one should first set reference channel  */
         bool EnableRefCondPrint(unsigned ch, double left = -10, double right = 10, int numprint = 0);

         /** If set, each hit must be supplied with epoch message */
         void SetEveryEpoch(bool on) { fEveryEpoch = on; }
         bool IsEveryEpoch() const { return fEveryEpoch; }

         void SetAutoCalibration(long cnt = 100000) { fAutoCalibration = cnt; }

         bool LoadCalibration(const std::string& fname);

         void SetWriteCalibration(const std::string& fname) { fWriteCalibr = fname; }

         void SetPrintRawData(bool on = true) { fPrintRawData = on; }
         bool IsPrintRawData() const { return fPrintRawData; }

         /** When enabled, last hit time in the channel used for reference time calculations
          * By default, first hit time is used
          * Special case is reference to channel 0 - here all hits will be used */
         void SetUseLastHit(bool on = true) { fUseLastHit = on; }

         bool IsUseLastHist() const { return fUseLastHit; }

         virtual void UserPreLoop();

         virtual void UserPostLoop();

         base::H1handle GetChannelRefHist(unsigned ch, bool rising = true)
            { return ch < fCh.size() ? (rising ? fCh[ch].fRisingRef : fCh[ch].fFallingRef) : 0; }

         void ClearChannelRefHist(unsigned ch, bool rising = true)
            { ClearH1(GetChannelRefHist(ch, rising)); }

         /** Scan all messages, find reference signals
          * if returned false, buffer has error and must be discarded */
         virtual bool FirstBufferScan(const base::Buffer& buf)
         {
            return DoBufferScan(buf, true);
         }

         /** Scan buffer for selecting messages inside trigger window */
         virtual bool SecondBufferScan(const base::Buffer& buf)
         {
            return DoBufferScan(buf, false);
         }
   };

}

#endif
