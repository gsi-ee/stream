#ifndef HADAQ_TRBPROCESSOR_H
#define HADAQ_TRBPROCESSOR_H

#include "base/StreamProc.h"
//#include "base/Profiler.h"
#include "hadaq/definess.h"
#include "hadaq/TdcProcessor.h"
#include "hadaq/SubProcessor.h"

#include <vector>

namespace hadaq {

   class HldProcessor;

   /** message used for ROOT tree storage, similar to TdcMessage and AdcMessage */
   struct TrbMessage {
      bool fTrigSyncIdFound;              ///<  is sync id found
      unsigned fTrigSyncId;               ///<  sync id
      unsigned fTrigSyncIdStatus;         ///<  sync id status
      uint64_t fTrigTm;                   ///<  trigger time
      unsigned fSyncPulsePeriod;          ///<  sync pulse period
      unsigned fSyncPulseLength;          ///<  sync pulse length

      /** reset */
      void Reset()
      {
         fTrigSyncIdFound = false;
         fTrigSyncId = 0;
         fTrigTm = 0;
         fTrigSyncIdStatus = 0;
         fSyncPulsePeriod = 0;
         fSyncPulseLength = 0;
      }
   };

   /** \brief TRB processor
    *
    * \ingroup stream_hadaq_classes
    *
    * This is generic processor for data, coming from TRB board
    * Normally one requires specific sub-processor for frontend like TDC or any other
    * Idea that TrbProcessor can interpret HADAQ event/subevent structures and
    * will distribute data to sub-processors.  */

   class TrbProcessor : public base::StreamProc {

      friend class TdcProcessor;
      friend class SubProcessor;
      friend class HldProcessor;

      protected:

         HldProcessor *fHldProc{nullptr};     ///< pointer on HLD processor

         SubProcMap fMap;            ///< map of sub-processors

         unsigned fHadaqCTSId{0};       ///< identifier of CTS header in HADAQ event
         std::vector<unsigned> fHadaqHUBId;   ///< identifier of HUB header in HADQ event

         unsigned fLastTriggerId{0};    ///< last seen trigger id
         unsigned fLostTriggerCnt{0};   ///< lost trigger counts
         unsigned fTakenTriggerCnt{0};  ///< registered trigger counts

         base::H1handle fEvSize{nullptr};     ///< HADAQ event size
         unsigned fSubevHLen{0};        ///< maximal length of subevent in bytes
         unsigned fSubevHDiv{0};        ///< integer division for subevent
         base::H1handle fSubevSize{nullptr};  ///< HADAQ subevent size
         base::H1handle fLostRate{nullptr};   ///< lost rate
         base::H1handle fTrigType{nullptr};   ///< trigger type
         base::H1handle fErrBits{nullptr};    ///< error bit statistics

         base::H1handle fMsgPerBrd{nullptr};  ///<! messages per board
         base::H1handle fErrPerBrd{nullptr};  ///<! errors per board
         base::H1handle fHitsPerBrd{nullptr}; ///<! data hits per board
         base::H2handle fCalHitsPerBrd{nullptr}; ///<! calibration hits per board, used only in HADES
         base::H2handle fToTPerBrd{nullptr};  ///<! ToT values for each TDC channel


         bool fPrintRawData;         ///< true when raw data should be printed
         bool fCrossProcess;         ///< if true, cross-processing will be enabled
         int  fPrintErrCnt;          ///< number of error messages, which could be printed

         unsigned fSyncTrigMask;       ///< mask which should be applied for trigger type
         unsigned fSyncTrigValue;      ///< value from trigger type (after mask) which corresponds to sync message
         unsigned fCalibrTriggerMask;  ///< trigger mask used for calibration

         bool fUseTriggerAsSync;     ///< when true, trigger number used as sync message between TRBs

         bool fCompensateEpochReset;  ///< when true, artificially create contiguous epoch value

         bool fAutoCreate;            ///< when true, automatically crates TDC processors

         TrbMessage  fMsg;            ///< used for TTree store
         TrbMessage* pMsg{nullptr};    ///< used for TTree store

//         base::Profiler  fProfiler;   ///< profiler

         unsigned fMinTdc;         ///< minimal id of TDC
         unsigned fMaxTdc;         ///< maximal id of TDC
         std::vector<hadaq::TdcProcessor*> fTdcsVect; ///< array of TDCs

         hadaqs::RawSubevent   fLastSubevHdr; ///<! copy of last subevent header (without data)
         unsigned fCurrentRunId;           ///<! current runid
         unsigned fCurrentEventId;         ///<! current processed event id, used in log msg

         static unsigned gNumChannels;     ///< default number of channels
         static unsigned gEdgesMask;       ///< default edges mask
         static bool gIgnoreSync;          ///< ignore sync in analysis, very rare used for sync with other data sources

         static unsigned gTDCMin;          ///< min TDC id when doing autoscan
         static unsigned gTDCMax;          ///< max TDC id when doing autoscan

         static unsigned gHUBMin;          ///< min HUB id when doing autoscan
         static unsigned gHUBMax;          ///< max HUB id when doing autoscan

         /** Returns true when processor used to select trigger signal
          * TRB3 not yet able to perform trigger selection */
         virtual bool doTriggerSelection() const { return false; }

         void AccountTriggerId(unsigned id);

         /** Way to register sub-processor, like for TDC */
         void AddSub(SubProcessor* tdc, unsigned id);

         /** Scan FPGA-TDC data, distribute over sub-processors */
         virtual void ScanSubEvent(hadaqs::RawSubevent* sub, unsigned trb3runid, unsigned trb3seqid);

         void BeforeEventScan();

         void AfterEventScan();
         void AfterEventFill();

         void BuildFastTDCVector();

         virtual void CreateBranch(TTree* t);

         void EventError(const char *msg);
         void EventLog(const char *msg);

         void SetCrossProcessAll();

      public:

         TrbProcessor(unsigned brdid = 0, HldProcessor *hld = nullptr, int hfill = -1);
         virtual ~TrbProcessor();

         /** Returns instance of \ref hadaq::HldProcessor to which it belongs */
         HldProcessor *GetHLD() const { return fHldProc; }

         /** enable autocreation mode if necessary, works for single event */
         void SetAutoCreate(bool on = true) { fAutoCreate = on; }

         /** Set id of CTS sub-sub event */
         void SetHadaqCTSId(unsigned id) { fHadaqCTSId = id; }

         /** Add HUB id */
         void AddHadaqHUBId(unsigned id) { fHadaqHUBId.emplace_back(id); }

         /** Set up to 4 different HUB ids */
         void SetHadaqHUBId(unsigned id1, unsigned id2=0, unsigned id3=0, unsigned id4=0)
         {
            fHadaqHUBId.clear();
            AddHadaqHUBId(id1);
            if (id2!=0) AddHadaqHUBId(id2);
            if (id3!=0) AddHadaqHUBId(id3);
            if (id4!=0) AddHadaqHUBId(id4);
         }

         /** deprecated, keep for backward compatibility, can be ignored */
         void SetHadaqTDCId(unsigned) {}
         /** deprecated, keep for backward compatibility, can be ignored */
         void SetHadaqSUBId(unsigned) {}

         virtual void UserPreLoop();
         virtual void UserPostLoop();

         virtual void SetTriggerWindow(double left, double right);

         virtual void SetStoreKind(unsigned kind = 1);

         virtual bool FirstBufferScan(const base::Buffer& buf);

         /** Enables printing of raw data */
         void SetPrintRawData(bool on = true) { fPrintRawData = on; }

         /** Return true if printing of raw data enabled */
         bool IsPrintRawData() const { return fPrintRawData; }

         /** Set number of errors which could be printed */
         void SetPrintErrors(int cnt = 100) { fPrintErrCnt = cnt; }
         bool CheckPrintError();

         void SetCrossProcess(bool on = true);
         /** Returns true if cross-processing enabled */
         bool IsCrossProcess() const { return fCrossProcess; }

         void SetCh0Enabled(bool on = true);

         /** Set sync mask and value which, should be obtained from
          * trigger type to detect CBM sync message in CTS sub-event
          * Code is following:
          * if ((trig_type & mask) == value) syncnum = sub->LastData();  */
         void SetSyncIds(unsigned mask, unsigned value)
         {
            fSyncTrigMask = mask;
            fSyncTrigValue = value;
         }

         /** Use TRB trigger number as SYNC message.
          * By this we synchronize all TDCs on all TRB boards by trigger number */
         void SetUseTriggerAsSync(bool on = true) { fUseTriggerAsSync = on; }
         /** Returns true if trigger number should be used to sync events over all TRBs */
         bool IsUseTriggerAsSync() const { return fUseTriggerAsSync; }

         /** When enabled, artificially create contiguous epoch value */
         void SetCompensateEpochReset(bool on = true) { fCompensateEpochReset = on; }

         /** Returns number of sub-processors */
         unsigned NumSubProc() const { return fMap.size(); }

         /** Returns sub-processor by its index */
         SubProcessor* GetSubProc(unsigned indx) const
         {
            for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
               if (indx==0) return iter->second;
               indx--;
            }
            return 0;
         }

         /** Returns TDC processor according to it ID */
         TdcProcessor* GetTDC(unsigned tdcid, bool fullid = false) const
         {
            SubProcMap::const_iterator iter = fMap.find(tdcid);

            // for old analysis, where IDs are only last 8 bit
            if ((iter == fMap.end()) && !fullid) iter = fMap.find(tdcid & 0xff);

            // ignore integrated TDCs, they have upper 16bits set
            return ((iter != fMap.end()) && ((iter->first >> 16) == 0) && iter->second->IsTDC()) ? (TdcProcessor*) iter->second : 0;
         }

         /** Returns number of TDC processors */
         unsigned NumberOfTDC() const
         {
            unsigned num = 0;
            for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
               if (iter->second->IsTDC()) num++;
            }
            return num;
         }

         /** Get TDC processor by index */
         TdcProcessor* GetTDCWithIndex(unsigned indx) const
         {
            for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
               if (!iter->second->IsTDC()) continue;
               if (indx == 0) return (TdcProcessor*) iter->second;
               indx--;
            }
            return 0;
         }

         void AddBufferToTDC(
               hadaqs::RawSubevent* sub,
               hadaq::SubProcessor* tdcproc,
               unsigned ix,
               unsigned datalen);

         TdcProcessor* FindTDC(unsigned tdcid) const;

         static void SetDefaults(unsigned numch=65, unsigned edges=0x1, bool ignore_sync = true);

         static unsigned GetDefaultNumCh();

         /** Define range for TDCs, used when auto mode is enabled */
         static void SetTDCRange(unsigned min, unsigned max)
         {
            gTDCMin = min;
            gTDCMax = max;
         }

         /** Define range for HUBs, used when auto mode is enabled */
         static void SetHUBRange(unsigned min, unsigned max)
         {
            gHUBMin = min;
            gHUBMax = max;
         }

         int CreateTDC(unsigned id1, unsigned id2 = 0, unsigned id3 = 0, unsigned id4 = 0);

         /** Create TDC processor, which extracts TDC information from CTS header */
         void CreateCTS_TDC() { new hadaq::TdcProcessor(this, fHadaqCTSId, gNumChannels, gEdgesMask); }

         void DisableCalibrationFor(unsigned firstch, unsigned lastch = 0);

         void SetAutoCalibrations(long cnt = 100000);

         void SetWriteCalibrations(const char* fileprefix, bool every_time = false, bool use_linear = false);

         bool LoadCalibrations(const char* fileprefix);

         void ConfigureCalibration(const std::string& name, long period, unsigned trigmask = 0xFFFF);

         void SetCalibrTriggerMask(unsigned trigmask = 0xFFFF);

         bool CollectMissingTDCs(hadaqs::RawSubevent *sub, std::vector<unsigned> &ids);

         void ClearFastTDCVector();

         unsigned TransformSubEvent(hadaqs::RawSubevent *sub, void *tgtbuf = nullptr, unsigned tgtlen = 0, bool only_hist = false, std::vector<unsigned> *newids = nullptr);

         unsigned EmulateTransform(hadaqs::RawSubevent *sub, int dummycnt, bool only_hist = false);

         void CreatePerTDCHistos();

         /** Are there per-TDC histograms */
         bool HasPerTDCHistos() const { return (fHitsPerBrd != nullptr) && (fToTPerBrd != nullptr); }

         void ClearDAQHistos();

         /** Return reference on last subevent header */
         hadaqs::RawSubevent& GetLastSubeventHdr() { return fLastSubevHdr; }

         /** Return reference on last filled message */
         hadaq::TrbMessage &GetTrbMsg() { return fMsg; }
   };
}

#endif
