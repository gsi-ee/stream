#ifndef HADAQ_TDCPROCESSOR_H
#define HADAQ_TDCPROCESSOR_H

#include "hadaq/SubProcessor.h"

#include "hadaq/TdcMessage.h"
#include "hadaq/TdcIterator.h"
#include "hadaq/TdcSubEvent.h"

#include <vector>
#include <cmath>
#include <string>

namespace hadaq {

   enum { FineCounterBins = 600, TotBins = 3000, ToTvalue = 20, ToThmin = 15, ToThmax = 60 };

   /** \brief TDC processor
    *
    * \ingroup stream_hadaq_classes
    *
    * This is specialized sub-processor for FPGA-TDC.
    * Normally it should be used together with TrbProcessor,
    * which the only can provide data
    * Following levels of histograms filling are working
    * - 0 - none
    * - 1 - only basic statistic from TRB
    * - 2 - generic statistic over TDC channels
    * - 3 - basic per-channel histograms with IDs
    * - 4 - per-channel histograms with references **/

   class TdcProcessor : public SubProcessor {

      friend class TrbProcessor;

      protected:

         /** TDC channel record */
         struct ChannelRec {
            unsigned refch;                ///<! reference channel for specified
            unsigned reftdc;               ///<! tdc of reference channel
            bool refabs;                   ///<! if true, absolute difference (without channel 0) will be used
            unsigned doublerefch;          ///<! double reference channel
            unsigned doublereftdc;         ///<! tdc of double reference channel
            unsigned refch_tmds;           ///<! reference channel for TMDS messages
            bool docalibr;                 ///<! if false, simple calibration will be used
            bool hascalibr = false;        ///<! indicate if channel has valid calibration (not simple linear)
            bool check_calibr;             ///<! flag used to indicate that calibration was checked
            base::H1handle fRisingFine;    ///<! histogram of all fine counters
            base::H1handle fRisingMult;    ///<! number of hits per event
            base::H1handle fRisingRef;     ///<! histogram of time diff to ref channel
            base::C1handle fRisingRefCond; ///<! condition to print raw events
            base::H1handle fRisingCalibr;  ///<! histogram of channel calibration function
            base::H1handle fRisingPCalibr; ///<! histogram of prevented channel calibration function
            base::H2handle fRisingRef2D;   ///<! histogram
            base::H1handle fRisingRefRef;  ///<! difference of two ref times, connected with double ref
            base::H2handle fRisingDoubleRef; ///<! correlation with diff time from other channel
            base::H1handle fRisingTmdsRef; ///<! histogram of time diff to ref channel for TMDS message
            base::H1handle fFallingFine;   ///<! histogram of all fine counters
            base::H1handle fFallingMult;   ///<! number of hits per event
            base::H1handle fTot;           ///<! histogram of time-over-threshold measurement
            base::H1handle fTot0D;         ///<! TOT from 0xD trigger (used for shift calibration)
            base::H1handle fFallingCalibr; ///<! histogram of channel calibration function
            base::H1handle fFallingPCalibr; ///<! histogram of channel calibration function
            int rising_cnt;                ///<! number of rising hits in last event
            int falling_cnt;               ///<! number of falling hits in last event
            double rising_hit_tm;          ///<! leading edge time, used in correlation analysis. can be first or last time
            double rising_last_tm;         ///<! last leading edge time
            bool rising_new_value;         ///<! used to calculate TOT and avoid errors after single leading and double trailing edge
            double rising_ref_tm;          ///<! rising ref time
            double rising_tmds;            ///<! first detected rising time from TMDS
            unsigned rising_coarse;        ///<! rising coarse
            unsigned rising_fine;          ///<! rising fine
            unsigned last_rising_fine;     ///<! last rising fine
            unsigned last_falling_fine;    ///<! last falling fine
            long all_rising_stat;          ///<! all rising stat
            long all_falling_stat;         ///<! all falling stat
            std::vector<uint32_t> rising_stat;  ///<! rising stat
            std::vector<float> rising_calibr;   ///<! rising calibr
            std::vector<uint32_t> falling_stat;  ///<! falling stat
            std::vector<float> falling_calibr;   ///<! falling calibr
            float last_tot;                 ///<! last tot
            long tot0d_cnt;                 ///<! counter of tot0d statistic for calibration
            long tot0d_misscnt;             ///<! counter of tot which misses histogram rnage
            std::vector<uint32_t> tot0d_hist;  ///<! histogram used for TOT calibration, allocated only when required
            float tot_shift;                ///<! calibrated tot shift
            float tot_dev;                  ///<! tot shift deviation after calibration
            float time_shift_per_grad;      ///<! delay in channel (ns/C), caused by temperature change
            float trig0d_coef;              ///<! scaling coefficient, applied when build calibration from 0xD trigger (reserved)
            int rising_cond_prnt;           ///<! rising condition print
            float calibr_quality_rising;    ///<! quality of last calibration 0. is nothing
            float calibr_quality_falling;    ///<! quality of last calibration 0. is nothing
            long calibr_stat_rising;        ///<! accumulated statistic during last calibration
            long calibr_stat_falling;       ///<! accumulated statistic during last calibration

            /** constructor */
            ChannelRec() :
               refch(0xffffff),
               reftdc(0xffffffff),
               refabs(false),
               doublerefch(0xffffff),
               doublereftdc(0xffffff),
               refch_tmds(0xffffff),
               docalibr(true),
               hascalibr(false),
               check_calibr(false),
               fRisingFine(nullptr),
               fRisingMult(nullptr),
               fRisingRef(nullptr),
               fRisingRefCond(nullptr),
               fRisingCalibr(nullptr),
               fRisingPCalibr(nullptr),
               fRisingRef2D(nullptr),
               fRisingRefRef(nullptr),
               fRisingDoubleRef(nullptr),
               fRisingTmdsRef(nullptr),
               fFallingFine(nullptr),
               fFallingMult(nullptr),
               fTot(nullptr),
               fTot0D(nullptr),
               fFallingCalibr(nullptr),
               fFallingPCalibr(nullptr),
               rising_cnt(0),
               falling_cnt(0),
               rising_hit_tm(0.),
               rising_last_tm(0),
               rising_new_value(false),
               rising_ref_tm(0.),
               rising_tmds(0.),
               rising_coarse(0),
               rising_fine(0),
               last_rising_fine(0),
               last_falling_fine(0),
               all_rising_stat(0),
               all_falling_stat(0),
               rising_stat(),
               rising_calibr(),
               falling_stat(),
               falling_calibr(),
               last_tot(0.),
               tot0d_cnt(0),
               tot0d_misscnt(0),
               tot0d_hist(),
               tot_shift(0.),
               tot_dev(0.),
               time_shift_per_grad(0.),
               trig0d_coef(0.),
               rising_cond_prnt(-1),
               calibr_quality_rising(-1.),
               calibr_quality_falling(-1.),
               calibr_stat_rising(0),
               calibr_stat_falling(0)
            {
            }

            /** create calibration structures */
            void CreateCalibr(unsigned numfine, double coarse_unit = -1.)
            {
               rising_stat.resize(numfine);
               falling_stat.resize(numfine);

               FillCalibr(numfine, coarse_unit);
            }

            /** Initialize claibration with default values */
            void FillCalibr(unsigned numfine, double coarse_unit = -1.)
            {
               for (unsigned n = 0; n < numfine; n++)
                  falling_stat[n] = rising_stat[n] = 0;

               SetLinearCalibr(hadaq::TdcMessage::GetFineMinValue(), hadaq::TdcMessage::GetFineMaxValue(), coarse_unit);
            }

            /** Assign linear calibration */
            void SetLinearCalibr(unsigned finemin, unsigned finemax, double coarse_unit = -1)
            {
               rising_calibr.resize(5,0);
               falling_calibr.resize(5,0);

               // to support old API where factor for default coarse unit was possible to specify
               // normally exact value of coarse unit is specified, therefore values <0 and >0.01 are very special
               if (coarse_unit < 0)
                  coarse_unit = hadaq::TdcMessage::CoarseUnit();
               else if (coarse_unit > 0.01)
                  coarse_unit = coarse_unit * hadaq::TdcMessage::CoarseUnit();

               rising_calibr[0] = 2; // 2 values
               rising_calibr[1] = finemin;
               rising_calibr[2] = 0.;
               rising_calibr[3] = finemax;
               rising_calibr[4] = coarse_unit;

               for (int n = 0; n < 5; ++n)
                  falling_calibr[n] = rising_calibr[n];
            }

            /** Release memory used by calibration structures */
            void ReleaseCalibr()
            {
               rising_stat.clear();
               rising_calibr.clear();
               falling_stat.clear();
               falling_calibr.clear();
            }

            /** Create ToT histogram  */
            void CreateToTHist()
            {
               tot0d_hist.resize(TotBins);
               for (unsigned n=0;n<TotBins;n++)
                  tot0d_hist[n] = 0;
            }

            /** Release memory for ToT histogram  */
            void ReleaseToTHist()
            {
               tot0d_hist.clear();

            }
         };

         bool fVersion4 = false;         ///< if version4 TDC is analyzed
         bool fDogma = false;           ///< used with dogma readout

         TdcIterator fIter1;         ///<! iterator for the first scan
         TdcIterator fIter2;         ///<! iterator for the second scan

         base::H1handle fChannels{nullptr};   ///<! histogram with messages per channel
         base::H1handle fHits{nullptr};       ///<! histogram with hits per channel
         base::H1handle fErrors{nullptr};     ///<! histogram with errors per channel
         base::H1handle fUndHits{nullptr};    ///<! histogram with undetected hits per channel
         base::H1handle fCorrHits{nullptr};   ///<! histogram with corrected hits per channel
         base::H1handle fMsgsKind{nullptr};   ///<! messages kinds
         base::H2handle fAllFine{nullptr};    ///<! histogram of all fine counters
         base::H2handle fAllCoarse{nullptr};  ///<! histogram of all coarse counters
         base::H2handle fRisingCalibr{nullptr};///<! histogram with all rising calibrations
         base::H2handle fFallingCalibr{nullptr}; ///<! histogram with all falling calibrations
         base::H2handle fRisingPCalibr{nullptr};///<! histogram with prevented all rising calibrations
         base::H2handle fFallingPCalibr{nullptr}; ///<! histogram with prevented all falling calibrations
         base::H1handle fHitsRate{nullptr};    ///<! histogram with data rate
         base::H1handle fTotShifts{nullptr};  ///<! histogram with all TOT shifts
         base::H1handle fTempDistr{nullptr};   ///<! temperature distribution

         base::H2handle fhRaisingFineCalibr{nullptr}; ///<! histogram of calibrated raising fine counter vs channel
         base::H2handle fhTotVsChannel{nullptr}; ///<! histogram of ToT vs channel
         base::H1handle fhTotMoreCounter{nullptr}; ///<! histogram of counter with ToT >20 ns per channel
         base::H1handle fhTotMinusCounter{nullptr}; ///<! histogram of counter with ToT < 0 ns per channel

         base::H2handle fhSigmaTotVsChannel{nullptr}; ///<! JAM histogram of calibration ToT difference (rms) from expected Tot, vs channel

         base::H2handle fhRisingPrevDiffVsChannel{nullptr}; ///<! JAM histogram time difference (ns) between subsequent rising hits vs channel. Suggested by Jan Michel

         unsigned fHldId{0};               ///<! sequence number of processor in HLD
         base::H1handle *fHitsPerHld{nullptr};   ///<! hits per TDC - from HLD
         base::H1handle *fErrPerHld{nullptr};    ///<! errors per TDC - from HLD
         base::H2handle *fChHitsPerHld{nullptr}; ///<! hits per TDC channel - from HLD
         base::H2handle *fChErrPerHld{nullptr};  ///<! errors per TDC channel - from HLD
         base::H2handle *fChCorrPerHld{nullptr};  ///<! corrections per TDC channel - from HLD
         base::H2handle *fQaFinePerHld{nullptr};  ///<! QA fine counter per TDC channel - from HLD
         base::H2handle *fQaToTPerHld{nullptr};  ///<! QA ToT per TDC channel - from HLD
         base::H2handle *fQaEdgesPerHld{nullptr};  ///<! QA Edges per TDC channel - from HLD
         base::H2handle *fQaErrorsPerHld{nullptr};  ///<! QA Errors per TDC channel - from HLD

         // JAM 8-10-2021 something new for calibration monitor:
         base::H2handle *fToTPerTDCChannel{nullptr};  ///< HADAQ ToT per TDC channel, real values
         base::H2handle *fShiftPerTDCChannel{nullptr};  ///< HADAQ calibrated shift per TDC channel, real values
         base::H1handle *fExpectedToTPerTDC{nullptr};  ///< HADAQ expected ToT per TDC  used for calibration
         base::H2handle *fDevPerTDCChannel{nullptr};  ///< HADAQ ToT deviation per TDC channel from calibration
         base::H2handle *fTPreviousPerTDCChannel{nullptr};
         base::H2handle *fToTCountPerTDCChannel{nullptr};  ///< HADAQ number of evaluated ToTs per TDC channel JAM 12/2023

         unsigned                 fNumChannels;       ///<! number of channels
         unsigned                 fNumFineBins;       ///<! number of fine-counter bins
         std::vector<ChannelRec>  fCh;                ///<! full description for each channels
         float                    fCalibrTemp;        ///<! temperature when calibration was performed
         float                    fCalibrTempCoef;    ///<! coefficient to scale calibration curve (real value -1)
         bool                     fCalibrUseTemp;     ///<! when true, use temperature adjustment for calibration
         unsigned                 fCalibrTriggerMask; ///<! mask with enabled for trigger events ids, default all

         bool                     fToTdflt;        ///<! indicate if default setting used, which can be adjusted after seeing first event
         double                   fToTvalue;       ///<! ToT of 0xd trigger
         double                   fToThmin;        ///<! histogram min
         double                   fToThmax;        ///<! histogram max
         double                   fTotUpperLimit;  ///<! upper limit for ToT range check
         int                      fTotStatLimit;   ///<! how much statistic required for ToT calibration
         double                   fTotRMSLimit;    ///<! maximal RMS valus for complete calibration

         long   fCalibrAmount;        ///<! current accumulated calibr data
         double fCalibrProgress;      ///<! current progress in calibration
         std::string fCalibrStatus;   ///<! calibration status
         double fCalibrQuality;       ///<! calibration quality:
                                      //  0         - not exists
                                      //  0..0.3    - bad (red color)
                                      //  0.3..0.7  - poor (yellow color)
                                      //  0.7..0.8  - accumulating (blue color)
                                      //  0.8..1.0  - ok (green color)

         float                    fTempCorrection; ///<! correction for temperature sensor
         float                    fCurrentTemp;    ///<! current measured temperature
         unsigned                 fDesignId;       ///<! design ID, taken from status message
         double                   fCalibrTempSum0; ///<! sum0 used to check temperature during calibration
         double                   fCalibrTempSum1; ///<! sum1 used to check temperature during calibration
         double                   fCalibrTempSum2; ///<! sum2 used to check temperature during calibration


         std::vector<hadaq::TdcMessageExt>  fDummyVect; ///<! dummy empty vector
         std::vector<hadaq::TdcMessageExt> *pStoreVect{nullptr}; ///<! pointer on store vector

         std::vector<hadaq::MessageFloat> fDummyFloat;  ///<! vector with compact messages
         std::vector<hadaq::MessageFloat> *pStoreFloat = nullptr; ///<! pointer on store vector
         hadaq::TdcSubEventFloat          *pEventFloat = nullptr; ///<! pointer on current event

         std::vector<hadaq::MessageDouble> fDummyDouble;  ///<! vector with compact messages
         std::vector<hadaq::MessageDouble> *pStoreDouble = nullptr; ///<! pointer on store vector

         /** EdgeMask defines how TDC calibration for falling edge is performed
          * 0,1 - use only rising edge, falling edge is ignore
          * 2   - falling edge enabled and fully independent from rising edge
          * 3   - falling edge enabled and uses calibration from rising edge
          * 4   - falling edge enabled and common statistic is used for calibration */
         unsigned    fEdgeMask;        ///<! which channels to analyze, analyzes trailing edges when more than 1
         long        fCalibrCounts;    ///<! indicates minimal number of counts in each channel required to produce calibration
         bool        fAutoCalibr;      ///<! when true, perform auto calibration
         bool        fAutoCalibrOnce;  ///<! when true, auto calibration will be executed once
         int         fAllCalibrMode;   ///<! use all data for calibrations, used with DABC -1 - disabled, 0 - off, 1 - on
         int         fAllTotMode;      ///<! ToT calibration mode -1 - disabled, 0 - accumulate stat for channels, 1 - accumulate stat for ToT
         int         fAllDTrigCnt;     ///<! number of 0xD triggers

         std::string fWriteCalibr;    ///<! file which should be written at the end of data processing
         bool        fWriteEveryTime; ///<! write calibration every time automatic calibration performed
         bool        fUseLinear;      ///<! create linear calibrations for the channel
         int         fLinearNumPoints; ///<! number of linear points

         bool      fEveryEpoch;       ///<! if true, each hit must be supplied with epoch

         bool      fUseLastHit;       ///<! if true, last hit will be used in reference calculations

         bool      fUseNativeTrigger;  ///<! if true, TRB3 trigger is used as event time

         bool      fCompensateEpochReset; ///<! if true, compensates epoch reset

         unsigned  fCompensateEpochCounter;  ///<! counter to compensate epoch reset

         bool      fCh0Enabled;           ///<! when true, channel 0 stored in output event

         TdcMessage fLastTdcHeader;      ///<! copy of last TDC header
         TdcMessage fLastTdcTrailer;      ///<! copy of last TDC trailer

         long      fRateCnt;             ///<! counter used for rate calculation
         double    fLastRateTm = -1.;    ///<! last ch0 time when rate was calculated
         double    fRef0Time = 0.;       ///<! absolute ref time, set once when first channel0 time will be seen

         unsigned  fSkipTdcMessages;     ///<! number of first messages, skipped from analysis
         bool      fIsCustomMhz{false};  ///<! is custom Mhz mode
         double    fCustomMhz;           ///<! new design Mhz
         bool      fPairedChannels;      ///<! special mode when rising/falling edges splitted on different channels
         bool      fModifiedFallingEdges; ///<! true when file taken with HADES and falling edges may be modified,

         std::vector<std::string> fCalibrLog; ///<! error log messages during calibration

         /** Returns true when processor used to select trigger signal
          * TDC not yet able to perform trigger selection */
         bool doTriggerSelection() const override { return false; }

         static unsigned gNumFineBins;   ///<! default value for number of bins in histograms for fine bins
         static unsigned gTotRange;      ///<! default range for TOT histogram
         static unsigned gHist2dReduce;  ///<! reduce factor for points in 2D histogram
         static unsigned gErrorMask;     ///<! mask for errors to display
         static bool gAllHistos;         ///<! when true, all histos for all channels created simultaneously
         static double gTrigDWindowLow;  ///<! low limit of time stamps for 0xD trigger used for calibration
         static double gTrigDWindowHigh; ///<! high limit of time stamps for 0xD trigger used for calibration
         static bool gUseDTrigForRef;    ///<! when true, use special triggers for ref calculations
         static bool gUseAsDTrig;        ///<! when true, all events are analyzed as 0xD trigger
         static int gHadesMonitorInterval; ///<! how often special HADES monitoring procedure called
         static bool gHadesReducedMonitor; ///<! if true suppress some unrequired histograms for hades tdc calib mon
         static int gTotStatLimit;         ///<! how much statistic required for ToT calibration
         static double gTotRMSLimit;       ///<! allowed RMS value
         static int gDefaultLinearNumPoints;      ///<! number of points when linear calibration is used
         static bool gIgnoreCalibrMsgs;    ///<! ignore calibration messages
         static bool gStoreCalibrTables;   ///<! when enabled, store calibration tables for v4 TDC
         static bool gPreventFineCalibration;  ///<! when enabled, not produce calibration but just fill extra histograms
         static int gTimeRefKind;          ///<! which time used as reference for time stamps

         void AppendTrbSync(uint32_t syncid) override;

         /** This is maximum disorder time for TDC messages
          * TODO: derive this value from sub-items */
         double MaximumDisorderTm() const override { return 2e-6; }

         bool DoBufferScan(const base::Buffer &buf, bool isfirst);
         bool DoBuffer4Scan(const base::Buffer &buf, bool isfirst);

         double DoTestToT(int iCh);
         double DoTestErrors(int iCh);
         double DoTestEdges(int iCh);
         double DoTestFineTimeH2(int iCh, base::H2handle h);
         double DoTestFineTime(double hRebin[], int nBinsRebin, int nEntries);

         void BeforeFill() override;
         void AfterFill(SubProcMap* = nullptr) override;

         long CheckChannelStat(unsigned ch);

         double CalibrateChannel(unsigned nch, bool rising, const std::vector<uint32_t> &statistic, std::vector<float> &calibr, bool use_linear = false, bool preliminary = false);
         void CopyCalibration(const std::vector<float> &calibr, base::H1handle hcalibr, unsigned ch = 0, base::H2handle h2calibr = nullptr);

         bool CalibrateTot(unsigned ch, std::vector<uint32_t> &hist, float &tot_shift, float &tot_dev, float cut = 0.);

         bool CheckPrintError();

         bool SetChannelPrefix(unsigned ch, unsigned level = 3);

         bool CreateChannelHistograms(unsigned ch);

         double TestCanCalibrate(bool fillhist = false, std::string *status = nullptr);

         bool PerformAutoCalibrate();

         void ClearChannelStat(unsigned ch);

         float ExtractCalibr(const std::vector<float> &func, unsigned bin);

         /** extract calibration value */
         inline float ExtractCalibrDirect(const std::vector<float> &func, unsigned bin)
         {
            if (func.size() > 100) return func[bin];

            // here only two-point linear approximation is supported
            // later one can extend approximation for N-points linear function
            if (bin <= func[1]) return func[2];
            int pnt = func.size() - 2;  // 3 for simple linear point
            if (bin >= func[pnt]) return func[pnt+1];

            if (pnt > 3) {
               int segm = std::ceil((bin - func[1]) / (func[pnt] - func[1]) * (func[0] - 1)); // segment in linear approx
               pnt = 1 + segm*2; // first segment should have pnt = 3, second segment pnt = 5 and so on
            }

            return func[pnt-1] + (bin - func[pnt-2]) / (func[pnt] - func[pnt-2]) * (func[pnt+1] - func[pnt-1]);
         }

         void CreateV4CalibrTable(unsigned ch, uint32_t *table);
         void SetTable(uint32_t *table, unsigned addr, uint32_t value);

         void FindFMinMax(const std::vector<float> &func, int nbin, int &fmin, int &fmax);

         void CreateBranch(TTree*) override;

         void AddError(unsigned code, const char *args, ...);

      public:

         /** error codes */
         enum EErrors {
            errNoHeader,       ///< no header found
            errChId,           ///< wrong channel id
            errEpoch,          ///< wrong/missing epoch
            errFine,           ///< bad fine counter
            err3ff,            ///< 0x3ff error
            errCh0,            ///< missing channel 0
            errMismatchDouble, ///<
            errUncknHdr,       ///< unknown header
            errDesignId,       ///< mismatch in design id
            errMisc            ///< all other errors
         };

         /** edges mask */
         enum EEdgesMasks {
            edge_Rising = 1,        ///< process only rising edge
            edge_BothIndepend = 2,  ///< process rising and falling edges independent
            edge_ForceRising  = 3,  ///< use rising edge calibration for falling
            edge_CommonStatistic = 4  ///< accumulate common statistic for both
         };

         TdcProcessor(TrbProcessor* trb, unsigned tdcid, unsigned numchannels = MaxNumTdcChannels, unsigned edge_mask = 1, bool ver4 = false, bool dogma = false);
         virtual ~TdcProcessor();

         static void SetMaxBoardId(unsigned);

         static void SetDefaults(unsigned numfinebins = 600, unsigned totrange = 100, unsigned hist2dreduced = 10);

         static void SetErrorMask(unsigned mask = 0xffffffffU);

         static void SetAllHistos(bool on = true);

         static void SetIgnoreCalibrMsgs(bool on = true);

         static void SetHadesMonitorInterval(int tm = -1);
         static int GetHadesMonitorInterval();

         static void SetHadesReducedMonitoring(bool on=true);
         static bool IsHadesReducedMonitoring();

         static void SetUseDTrigForRef(bool on = true);

         static void SetTriggerDWindow(double low = -25, double high = 50);

         static void SetToTCalibr(int minstat = 100, double rms = 0.15);

         static void SetDefaultLinearNumPoints(int cnt = 2);

         static void SetUseAsDTrig(bool on = true);

         static void SetStoreCalibrTables(bool on = true);

         static void SetPreventFineCalibration(bool on = true);

         static void SetTimeRefKind(int kind = -1);

         /** Set number of TDC messages, which should be skipped from subevent before analyzing it */
         void SetSkipTdcMessages(unsigned cnt = 0) { fSkipTdcMessages = cnt; }

         void Set400Mhz(bool on = true);

         void SetCustomMhz(float freq = 400.);

         /** When file taken with HADES and falling edges may be modified while ToT offset can be negative
          *  Resulting fine-time offset also can be negative and therefore coarse message counter is modified
          *  Enable flag to be able recognize and repair such shifts.
          *  Works only if ToT offset is not less than -1 ns, otherwise overflows too many and algorithm may not work */
         void SetModifiedFallingEdges(bool on = true) { fModifiedFallingEdges = on; }

         /** Set paired channels mode
          * In such case odd channels are normal rising edge but even channels are falling edge of previous one */
         void SetPairedChannels(bool on = true) { fPairedChannels = on; }
         /** Get paired channels mode */
         bool GetPairedChannels() const { return fPairedChannels; }

         /** Set minimal counts number for ToT calibration */
         void SetTotStatLimit(int minstat = 100) { fTotStatLimit = minstat; }
         /** Set maximal allowed RMS for ToT histogram in ns */
         void SetTotRMSLimit(double rms = 0.15) { fTotRMSLimit = rms; }

         /** Returns number of TDC channels */
         inline unsigned NumChannels() const { return fNumChannels; }
         /** Returns number of fine TDC bins */
         inline unsigned NumFineBins() const { return fNumFineBins; }
         /** Returns true if processing rising edge */
         inline bool DoRisingEdge() const { return true; }
         /** Returns true if processing falling */
         inline bool DoFallingEdge() const { return fEdgeMask > 1; }
         /** Returns value of edge mask */
         inline unsigned GetEdgeMask() const { return fEdgeMask; }

         inline bool IsVersion4() const { return fVersion4; }

         /** Returns calibration progress */
         double GetCalibrProgress() const { return fCalibrProgress; }
         /** Returns calibration status */
         std::string GetCalibrStatus() const { return fCalibrStatus; }
         /** Returns calibration quality */
         double GetCalibrQuality() const { return fCalibrQuality; }
         /** Acknowledge calibration quality */
         void AcknowledgeCalibrQuality(double lvl = 1.)
         {
            if (fCalibrQuality < lvl) {
               fCalibrQuality = lvl;
               fCalibrProgress = 1.;
               fCalibrStatus = "Ready";
            }
         }
         /** Take all messages from calibration log */
         std::vector<std::string> TakeCalibrLog()
         {
            std::vector<std::string> res;
            std::swap(res, fCalibrLog);
            return res;
         }

         /** Get number of indexed histograms */
         int GetNumHist() const { return 9; }

         /** Get histogram name by index */
         const char* GetHistName(int k) const
         {
            switch(k) {
               case 0: return "RisingFine";
               case 1: return "RisingCoarse";
               case 2: return "RisingRef";
               case 3: return "FallingFine";
               case 4: return "FallingCoarse";
               case 5: return "Tot";
               case 6: return "RisingMult";
               case 7: return "FallingMult";
               case 8: return "RisingTmdsRef";
            }
            return "";
         }

         /** Get histogram by index and channel id */
         base::H1handle GetHist(unsigned ch, int k = 0)
         {
            if (ch >= NumChannels())
               return nullptr;
            switch (k) {
               case 0: return fCh[ch].fRisingFine;
               case 1: return nullptr;
               case 2: return fCh[ch].fRisingRef;
               case 3: return fCh[ch].fFallingFine;
               case 4: return nullptr;
               case 5: return fCh[ch].fTot;
               case 6: return fCh[ch].fRisingMult;
               case 7: return fCh[ch].fFallingMult;
               case 8: return fCh[ch].fRisingTmdsRef;
            }
            return nullptr;
         }

         void CreateHistograms(int *arr = nullptr);

         /** Assign per HLD histos */
         void AssignPerHldHistos(unsigned id, base::H1handle *hHits, base::H1handle *hErrs,
                                              base::H2handle *hChHits, base::H2handle *hChErrs,
                                              base::H2handle *hChCorr,
                                              base::H2handle *hQaFine, base::H2handle *hQaToT,
                                              base::H2handle *hQaEdges, base::H2handle *hQaErrors,
                                              base::H2handle *hTot, base::H2handle *hShift,
                                              base::H1handle *hExpTot, base::H2handle *hDev, base::H2handle *hTPrev, base::H2handle *hTotCount)
         {
            fHldId = id;
            fHitsPerHld = hHits;
            fErrPerHld = hErrs;
            fChHitsPerHld = hChHits;
            fChErrPerHld = hChErrs;
            fChCorrPerHld = hChCorr;
            fQaFinePerHld = hQaFine;
            fQaToTPerHld = hQaToT;
            fQaEdgesPerHld = hQaEdges;
            fQaErrorsPerHld = hQaErrors;
            fToTPerTDCChannel = hTot;
            fShiftPerTDCChannel = hShift;
            fExpectedToTPerTDC = hExpTot;
            fDevPerTDCChannel = hDev;
            fTPreviousPerTDCChannel = hTPrev;
            fToTCountPerTDCChannel = hTotCount;
         }

         /** Set calibration trigger type(s)
          * One could specify up-to 4 different trigger types, for instance 0x1 and 0xD
          * First argument could be use to enable all triggers (0xFFFF, default)
          * or none of the triggers (-1) */
         void SetCalibrTrigger(int typ1 = 0xFFFF, unsigned typ2 = 0, unsigned typ3 = 0, unsigned typ4 = 0)
         {
            fCalibrTriggerMask = 0;
            if (typ1<0) return;
            if (typ1 >= 0xFFFF) { fCalibrTriggerMask = 0xFFFF; return; }
            if (typ1 && (typ1<=0xF)) fCalibrTriggerMask |= (1 << typ1);
            if (typ2 && (typ2<=0xF)) fCalibrTriggerMask |= (1 << typ2);
            if (typ3 && (typ3<=0xF)) fCalibrTriggerMask |= (1 << typ3);
            if (typ4 && (typ4<=0xF)) fCalibrTriggerMask |= (1 << typ4);
         }

         /** Set calibration trigger mask directly, 1bit per each trigger type
          * if bit 0x80000000 configured, calibration will use temperature correction */
         void SetCalibrTriggerMask(unsigned trigmask)
         {
            fCalibrTriggerMask = trigmask & 0x3FFF;
            fCalibrUseTemp = (trigmask & 0x80000000) != 0;
         }

         /** Set temperature coefficient, which is applied to calibration curves
          * Typical value is about 0.0044 */
         void SetCalibrTempCoef(float coef)
         {
            fCalibrTempCoef = coef;
         }

         /** Set shift for the channel time stamp, which is added with temperature change */
         void SetChannelTempShift(unsigned ch, float shift_per_grad)
         {
            if (ch < fCh.size()) fCh[ch].time_shift_per_grad = shift_per_grad;
         }

         /** Set channel TOT shift in nano-seconds, typical value is around 30 ns */
         void SetChannelTotShift(unsigned ch, float tot_shift)
         {
            if (ch < fCh.size()) fCh[ch].tot_shift = tot_shift;
         }

         /** Returns channel TOT shift in nano-seconds */
         double GetChannelTotShift(unsigned ch) const
         {
            return (ch < fCh.size()) ? fCh[ch].tot_shift : 0;
         }

         void DisableCalibrationFor(unsigned firstch, unsigned lastch = 0);

         void SetRefChannel(unsigned ch, unsigned refch, unsigned reftdc = 0xffff, int npoints = 5000, double left = -10., double right = 10., bool twodim = false);


         void SetRefTmds(unsigned ch, unsigned refch, int npoints, double left, double right);

         bool SetDoubleRefChannel(unsigned ch1, unsigned ch2,
                                  int npx = 200, double xmin = -10., double xmax = 10.,
                                  int npy = 200, double ymin = -10., double ymax = 10.);

         void CreateRateHisto(int np = 1000, double xmin = 0., double xmax = 1e5);

         double GetTdcCoarseUnit() const;

         /** Configure upper limit for ToT */
         void SetTotUpperLimit(double lmt = 20) { fTotUpperLimit = lmt; }

         /** Get configured upper limit for ToT */
         double GetTotUpperLimit() const { return fTotUpperLimit; }

         bool EnableRefCondPrint(unsigned ch, double left = -10, double right = 10, int numprint = 0);

         /** If set, each hit must be supplied with epoch message */
         void SetEveryEpoch(bool on) { fEveryEpoch = on; }
         /** Return true if each hit must be supplied with epoch message */
         bool IsEveryEpoch() const { return fEveryEpoch; }

         bool IsRegularChannel0() const;

         void SetLinearCalibration(unsigned nch, unsigned finemin=30, unsigned finemax=500);

         /** configure auto calibration */
         void SetAutoCalibration(long cnt = 100000) { fCalibrCounts = cnt % 1000000000L; fAutoCalibrOnce = (cnt>1000000000L); fAutoCalibr = (cnt >= 0); }

         /** Configure mode, when calibration should be start/stop explicitly */
         void UseExplicitCalibration() { fAllCalibrMode = 0; }

         /** Return explicit calibr mode, -1 - off, 0 - normal data processing, 1 - accumulating calibration */
         int GetExplicitCalibrationMode() { return fAllCalibrMode; }

         void BeginCalibration(long cnt);

         void CompleteCalibration(bool dummy = false, const std::string &filename = "", const std::string &subdir = "");

         bool LoadCalibration(const std::string& fprefix);

         /** When specified, calibration will be written to the file
          * If every_time == true, write every time when automatic calibration performed, otherwise only at the end */
         void SetWriteCalibration(const std::string &fprefix, bool every_time = false, bool use_linear = false)
         {
            fWriteCalibr = fprefix;
            fWriteEveryTime = every_time;
            fUseLinear = use_linear;
         }

         /** Enable linear calibrations */
         void SetUseLinear(bool on = true) { fUseLinear = on; }

         /** Returns true is linear calibrations are configured */
         bool IsUseLinear() const { return fUseLinear; }

         /** Set number of points in linear calibrations */
         void SetLinearNumPoints(int cnt = 2) { fLinearNumPoints = (cnt < 2) ? 2 : ((cnt > 100) ? 100 : cnt); }
         /** Return number of points in linear calibrations */
         int GetLinearNumPoints() const { return fLinearNumPoints; }

         void SetToTRange(double tot_0xd, double hmin, double hmax);

         void ConfigureToTByHwType(unsigned hwtype);

         /** When enabled, last hit time in the channel used for reference time calculations
          * By default, first hit time is used
          * Special case is reference to channel 0 - here all hits will be used */
         void SetUseLastHit(bool on = true) { fUseLastHit = on; }

         /** Returns true if last hit used in reference histogram calculations */
         bool IsUseLastHist() const { return fUseLastHit; }

         /** Return last TDC header, seen by the processor */
         const TdcMessage& GetLastTdcHeader() const { return fLastTdcHeader; }

         /** Return last TDC header, seen by the processor */
         const TdcMessage& GetLastTdcTrailer() const { return fLastTdcTrailer; }

         void UserPostLoop() override;

         /** Get ref histogram for specified channel */
         base::H1handle GetChannelRefHist(unsigned ch, bool = true)
            { return ch < fCh.size() ? fCh[ch].fRisingRef : nullptr; }

         /** Clear ref histogram for specified channel */
         void ClearChannelRefHist(unsigned ch, bool rising = true)
            { ClearH1(GetChannelRefHist(ch, rising)); }

         /** Enable/disable store of channel 0 in output event */
         void SetCh0Enabled(bool on = true) { fCh0Enabled = on; }

         /** Scan all messages, find reference signals
          * if returned false, buffer has error and must be discarded */
         bool FirstBufferScan(const base::Buffer& buf) override
         {
            return fVersion4 ? DoBuffer4Scan(buf, true) : DoBufferScan(buf, true);
         }

         /** Scan buffer for selecting messages inside trigger window */
         bool SecondBufferScan(const base::Buffer& buf) override
         {
            return fVersion4 ? DoBuffer4Scan(buf, false) : DoBufferScan(buf, false);
         }

         void IncCalibration(unsigned ch, bool rising, unsigned fine, unsigned value);

         void ProduceCalibration(bool clear_stat = true, bool use_linear = false, bool dummy = false, bool preliminary = false);

         /** Access value of temperature during calibration.
          * Used to adjust all kind of calibrations afterwards */
         float GetCalibrTemp() const { return fCalibrTemp; }

         /** Set temperature used for calibration */
         void SetCalibrTemp(float v) { fCalibrTemp = v; }

         void StoreCalibration(const std::string& fname, unsigned fileid = 0);

         float GetCalibrFunc(unsigned ch, bool isrising, unsigned bin)
         {
            return ExtractCalibrDirect(isrising ? fCh[ch].rising_calibr : fCh[ch].falling_calibr, bin);
         }

         void Store(base::Event*) override;

         void ResetStore() override;

         unsigned TransformTdcData(hadaqs::RawSubevent* sub, uint32_t *rawdata, unsigned indx, unsigned datalen, hadaqs::RawSubevent* tgt = nullptr, unsigned tgtindx = 0);

         void EmulateTransform(int dummycnt);

         void DoHadesHistAnalysis();

         void FillToTHistogram();

         static TdcProcessor *CreateFromCalibr(TrbProcessor *trb, const std::string &fname);
   };

}

#endif
