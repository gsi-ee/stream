#ifndef HADAQ_TDCPROCESSOR_H
#define HADAQ_TDCPROCESSOR_H

#include "hadaq/SubProcessor.h"

#include "hadaq/TdcMessage.h"
#include "hadaq/TdcIterator.h"
#include "hadaq/TdcSubEvent.h"

namespace hadaq {

   enum { FineCounterBins = 600, TotBins = 3000, TotLeft = 50, TotRight = 80 };

   /** This is specialized sub-processor for FPGA-TDC.
    * Normally it should be used together with TrbProcessor,
    * which the only can provide data
    * Following levels of histograms filling are working
    *  0 - none
    *  1 - only basic statistic from TRB
    *  2 - generic statistic over TDC channels
    *  3 - basic per-channel histograms with IDs
    *  4 - per-channel histograms with references
    **/

   class TdcProcessor : public SubProcessor {

      friend class TrbProcessor;

      protected:

         enum { rising_edge = 1, falling_edge = 2 };

         struct ChannelRec {
            unsigned refch;                //! reference channel for specified
            unsigned reftdc;               //! tdc of reference channel
            bool refabs;                   //! if true, absolute difference (without channel 0) will be used
            unsigned doublerefch;          //! double reference channel
            unsigned doublereftdc;         //! tdc of double reference channel
            bool docalibr;                 //! if false, simple calibration will be used
            bool hascalibr;                //! indicate if channel has valid calibration (not simple linear)
            base::H1handle fRisingFine;    //! histogram of all fine counters
            base::H1handle fRisingCoarse;  //! histogram of all coarse counters
            base::H1handle fRisingMult;    //! number of hits per event
            base::H1handle fRisingRef;     //! histogram of all coarse counters
            base::C1handle fRisingRefCond; //! condition to print raw events
            base::H1handle fRisingCoarseRef; //! histogram
            base::H1handle fRisingCalibr;  //! histogram of channel calibration function
            base::H2handle fRisingRef2D;   //! histogram
            base::H2handle fRisingDoubleRef; //! correlation with dif time from other channel
            base::H1handle fFallingFine;   //! histogram of all fine counters
            base::H1handle fFallingCoarse; //! histogram of all coarse counters
            base::H1handle fFallingMult;   //! number of hits per event
            base::H1handle fTot;           //! histogram of time-over-threshold measurement
            base::H1handle fTot0D;         //! TOT from 0xD trigger (used for shift calibration)
            base::H1handle fFallingCoarseRef; //! histogram
            base::H1handle fFallingCalibr; //! histogram of channel calibration function
            int rising_cnt;                //! number of rising hits in last event
            int falling_cnt;               //! number of falling hits in last event
            double rising_hit_tm;          //! leading edge time, used in correlation analysis. can be first or last time
            double rising_last_tm;         //! last leading edge time
            bool rising_new_value;         //! used to calculate TOT and avoid errors after single leading and double trailing edge
            double rising_ref_tm;
            unsigned rising_coarse;
            unsigned rising_fine;
            unsigned last_rising_fine;
            unsigned last_falling_fine;
            long all_rising_stat;
            long all_falling_stat;
            long rising_stat[FineCounterBins];
            float rising_calibr[FineCounterBins];
            long falling_stat[FineCounterBins];
            float falling_calibr[FineCounterBins];
            float last_tot;
            long tot0d_cnt;                 //! counter of tot0d statistic for calibration
            long tot0d_hist[TotBins];       //! histogram for TOT calibration
            float tot_shift;                //! calibrated tot shift
            float time_shift_per_grad;      //! delay in channel (ns/C), caused by temperature change
            float trig0d_coef;              //! scaling coefficient, applied when build calibration from 0xD trigger (reserved)
            int rising_cond_prnt;

            ChannelRec() :
               refch(0xffffff),
               reftdc(0xffffffff),
               refabs(false),
               doublerefch(0xffffff),
               doublereftdc(0xffffff),
               docalibr(true),
               hascalibr(false),
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
               fTot(0),
               fTot0D(0),
               fFallingCoarseRef(0),
               fFallingCalibr(0),
               rising_cnt(0),
               falling_cnt(0),
               rising_hit_tm(0.),
               rising_last_tm(0),
               rising_new_value(false),
               rising_ref_tm(0.),
               rising_coarse(0),
               rising_fine(0),
               last_rising_fine(0),
               last_falling_fine(0),
               all_rising_stat(0),
               all_falling_stat(0),
               last_tot(0.),
               tot0d_cnt(0),
               tot_shift(0.),
               time_shift_per_grad(0.),
               trig0d_coef(0.),
               rising_cond_prnt(-1)
            {
               for (unsigned n=0;n<FineCounterBins;n++) {
                  falling_stat[n] = rising_stat[n] = 0;
                  falling_calibr[n] = rising_calibr[n] = hadaq::TdcMessage::SimpleFineCalibr(n);
               }
               for (unsigned n=0;n<TotBins;n++) {
                  tot0d_hist[n] = 0;
               }
            }
         };

         TdcIterator fIter1;         //! iterator for the first scan
         TdcIterator fIter2;         //! iterator for the second scan

         base::H1handle fChannels;   //! histogram with messages per channel
         base::H1handle fHits;       //! histogram with hits per channel
         base::H1handle fErrors;     //! histogram with errors per channel
         base::H1handle fUndHits;    //! histogram with undetected hits per channel
         base::H1handle fMsgsKind;   //! messages kinds
         base::H2handle fAllFine;    //! histogram of all fine counters
         base::H2handle fAllCoarse;  //! histogram of all coarse counters
         base::H2handle fRisingCalibr;//! histogram with all rising calibrations
         base::H2handle fFallingCalibr; //! histogram all rising calibrations
         base::H1handle fHitsRate;    //! histogram with data rate
         base::H1handle fTotShifts;  //! histogram with all tot shifts
         base::H1handle fTempDistr;   //! temperature distribution

         unsigned                 fNumChannels; //! number of channels
         std::vector<ChannelRec>  fCh; //! histogram for individual channels
         float                    fCalibrTemp;  //! temperature when calibration was performed
         float                    fCalibrTempCoef; //! coefficient to scale calibration curve (real value -1)
         bool                     fCalibrUseTemp;  //! when true, use temperature adjustment for calibration
         unsigned                 fCalibrTriggerMask; //! mask with enabled for trigger events ids, default all

         double fCalibrProgress;      //! progress of auto calibration
         std::string fCalibrStatus;   //! calibration status

         float                    fCurrentTemp;  //! current measured temperature
         unsigned                 fDesignId;     //! design ID, taken from status message
         double                   fCalibrTempSum0; //! sum0 used to check temperature during calibration
         double                   fCalibrTempSum1; //! sum1 used to check temperature during calibration
         double                   fCalibrTempSum2; //! sum2 used to check temperature during calibration

         std::vector<hadaq::TdcMessageExt>  fDummyVect; //! dummy empty vector
         std::vector<hadaq::TdcMessageExt> *pStoreVect; //! pointer on store vector

         std::vector<hadaq::MessageFloat> fStoreFloat;  //! vector with compact messages
         std::vector<hadaq::MessageFloat> *pStoreFloat; //! pointer on store vector

         std::vector<hadaq::MessageDouble> fStoreDouble;  //! vector with compact messages
         std::vector<hadaq::MessageDouble> *pStoreDouble; //! pointer on store vector

         enum { edge_None = 0, edge_Rising = 1, edge_BothIndepend = 2, edge_ForceRising  = 3, edge_CommonStatistic = 4 };

         /** EdgeMask defines how TDC calibration for falling edge is performed
          * 0,1 - use only rising edge, falling edge is ignore
          * 2   - falling edge enabled and fully independent from rising edge
          * 3   - falling edge enabled and uses calibration from rising edge
          * 4   - falling edge enabled and common statistic is used for calibration */
         unsigned    fEdgeMask;       //! which channels to analyze, analyzes trailing edges when more than 1
         long        fAutoCalibration;//! indicates minimal number of counts in each channel required to produce calibration

         std::string fWriteCalibr;    //! file which should be written at the end of data processing
         bool fWriteEveryTime;        //! write calibration every time automatic calibration performed

         bool      fEveryEpoch;       //! if true, each hit must be supplied with epoch

         bool      fUseLastHit;       //! if true, last hit will be used in reference calculations

         bool      fUseNativeTrigger;  //! if true, TRB3 trigger is used as event time

         bool      fCompensateEpochReset; //! if true, compensates epoch reset

         unsigned  fCompensateEpochCounter;  //! counter to compensate epoch reset

         bool      fCh0Enabled;           //! when true, channel 0 stored in output event

         TdcMessage fLastTdcHeader;      //! copy of last TDC header

         long      fRateCnt;             //! counter used for rate calculation
         double    fLastRateTm;          //! last ch0 time when rate was calculated

         /** Returns true when processor used to select trigger signal
          * TDC not yet able to perform trigger selection */
         virtual bool doTriggerSelection() const { return false; }

         static unsigned gNumFineBins;  //! default value for number of bins in histograms for fine bins
         static unsigned gTotRange;  //! default range for TOT histogram
         static bool gAllHistos;     //! when true, all histos for all channels created simultaneosely

         /** Method will be called by TRB processor if SYNC message was found
          * One should change 4 first bytes in the last buffer in the queue */
         virtual void AppendTrbSync(uint32_t syncid);

         /** This is maximum disorder time for TDC messages
          * TODO: derive this value from sub-items */
         virtual double MaximumDisorderTm() const { return 2e-6; }

         /** Scan all messages, find reference signals */
         bool DoBufferScan(const base::Buffer& buf, bool isfirst);

         /** These methods used to fill different raw histograms during first scan */
         virtual void BeforeFill();
         virtual void AfterFill(SubProcMap* = 0);

         bool CalibrateChannel(unsigned nch, long* statistic, float* calibr);
         void CopyCalibration(float* calibr, base::H1handle hcalibr, unsigned ch = 0, base::H2handle h2calibr = 0);

         bool CalibrateTot(unsigned ch, long* hist, float& tot_shift, float cut = 0.);

         bool CheckPrintError();

         bool CreateChannelHistograms(unsigned ch);

         /** Check if automatic calibration can be performed - enough statistic is accumulated */
         double TestCanCalibrate();

         /** Perform automatic calibration of channels */
         bool PerformAutoCalibrate();

         /** Extract calibration value */
         float ExtractCalibr(float* func, unsigned bin);

         virtual void CreateBranch(TTree*);

      public:

         TdcProcessor(TrbProcessor* trb, unsigned tdcid, unsigned numchannels = MaxNumTdcChannels, unsigned edge_mask = 1);
         virtual ~TdcProcessor();

         static void SetMaxBoardId(unsigned) { }

         static void SetDefaults(unsigned numfinebins=600, unsigned totrange = 100);

         static void SetAllHistos(bool on = true);

         inline unsigned NumChannels() const { return fNumChannels; }
         inline bool DoRisingEdge() const { return true; }
         inline bool DoFallingEdge() const { return fEdgeMask > 1; }

         double GetCalibrProgress() const { return fCalibrProgress; }
         std::string GetCalibrStatus() const { return fCalibrStatus; }

         int GetNumHist() const { return 8; }

         const char* GetHistName(int k) const {
            switch(k) {
               case 0: return "RisingFine";
               case 1: return "RisingCoarse";
               case 2: return "RisingRef";
               case 3: return "FallingFine";
               case 4: return "FallingCoarse";
               case 5: return "Tot";
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
               case 5: return fCh[ch].fTot;
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

         /** Set calibration trigger type(s)
          * One could specify up-to 4 different trigger types, for instance 0x1 and 0xD
          * First argument could be use to enable all triggers (0xFFFF, default)
          * or none of the triggers (-1) */
         void SetCalibrTrigger(int typ1 = 0xFFFF, unsigned typ2 = 0, unsigned typ3 = 0, unsigned typ4 = 0) {
            fCalibrTriggerMask = 0;
            if (typ1<0) return;
            if (typ1 >= 0xFFFF) { fCalibrTriggerMask = 0xFFFF; return; }
            if ((typ1>=0) && (typ1<=0xF)) fCalibrTriggerMask |= (1 << typ1);
            if ((typ2>=0) && (typ2<=0xF)) fCalibrTriggerMask |= (1 << typ2);
            if ((typ3>=0) && (typ3<=0xF)) fCalibrTriggerMask |= (1 << typ3);
            if ((typ4>=0) && (typ4<=0xF)) fCalibrTriggerMask |= (1 << typ4);
         }

         /** Set calibration trigger mask directly, 1bit per each trigger type
          * if bit 0x80000000 configured, calibration will use temperature correction */
         void SetCalibrTriggerMask(unsigned trigmask) {
            fCalibrTriggerMask = trigmask & 0x3FFF;
            fCalibrUseTemp = (trigmask & 0x80000000) != 0;
         }

         /** Set temperature coefficient, which is applied to calibration curves
          * Typical value is about 0.0044 */
         void SetCalibrTempCoef(float coef) {
            fCalibrTempCoef = coef;
         }

         /** Set shift for the channel time stamp, which is added with temperature change */
         void SetChannelTempShift(unsigned ch, float shift_per_grad) {
            if (ch < fCh.size()) fCh[ch].time_shift_per_grad = shift_per_grad;
         }

         /** Disable calibration for specified channels */
         void DisableCalibrationFor(unsigned firstch, unsigned lastch = 0);

         /** Set reference signal for the TDC channel ch
          * \param refch   specifies number of reference channel
          * \param reftdc  specifies tdc id, used for ref channel.
          * Default (0xffff) same TDC will be used
          * If redtdc contains 0x70000 (like 0x7c010), than direct difference without channel 0 will be calculated
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

         /** Create rate histogram to count hits per second (excluding channel 0) */
         void CreateRateHisto(int np = 1000, double xmin = 0., double xmax = 1e5);

         /** Enable print of TDC data when time difference to ref channel belong to specified interval
          * Work ONLY when reference channel 0 is used.
          * One could set maximum number of events to print
          * In any case one should first set reference channel  */
         bool EnableRefCondPrint(unsigned ch, double left = -10, double right = 10, int numprint = 0);

         /** If set, each hit must be supplied with epoch message */
         void SetEveryEpoch(bool on) { fEveryEpoch = on; }
         bool IsEveryEpoch() const { return fEveryEpoch; }

         void SetAutoCalibration(long cnt = 100000) { fAutoCalibration = cnt; }

         bool LoadCalibration(const std::string& fprefix);

         /** When specified, calibration will be written to the file
          * If every_time == true, when every time when automatic calibration performed, otherwise only at the end */
         void SetWriteCalibration(const std::string& fprefix, bool every_time = false)
            { fWriteCalibr = fprefix; fWriteEveryTime = every_time; }

         /** When enabled, last hit time in the channel used for reference time calculations
          * By default, first hit time is used
          * Special case is reference to channel 0 - here all hits will be used */
         void SetUseLastHit(bool on = true) { fUseLastHit = on; }

         bool IsUseLastHist() const { return fUseLastHit; }

         /** Return last TDC header, seen by the processor */
         const TdcMessage& GetLastTdcHeader() const { return fLastTdcHeader; }

         virtual void UserPostLoop();

         base::H1handle GetChannelRefHist(unsigned ch, bool = true)
            { return ch < fCh.size() ? fCh[ch].fRisingRef : 0; }

         void ClearChannelRefHist(unsigned ch, bool rising = true)
            { ClearH1(GetChannelRefHist(ch, rising)); }

         /** Enable/disable store of channel 0 in output event */
         void SetCh0Enabled(bool on = true) { fCh0Enabled = on; }

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

         /** For expert use - artificially set calibration statistic */
         void IncCalibration(unsigned ch, bool rising, unsigned fine, unsigned value);

         /** For expert use - artificially produce calibration */
         void ProduceCalibration(bool clear_stat = true);

         /** For expert use - store calibration in the file */
         void StoreCalibration(const std::string& fname);

         virtual void Store(base::Event*);

         /** Method transform TDC data, if output specified, use it otherwise change original data */
         unsigned TransformTdcData(hadaqs::RawSubevent* sub, unsigned indx, unsigned datalen, hadaqs::RawSubevent* tgt = 0, unsigned tgtindx = 0);
   };

}

#endif
