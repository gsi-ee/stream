#ifndef HADAQ_TRBPROCESSOR_H
#define HADAQ_TRBPROCESSOR_H

#include "base/StreamProc.h"

// #include "base/Profiler.h"

#include "hadaq/definess.h"

#include "hadaq/TdcProcessor.h"

#include "hadaq/SubProcessor.h"

#include <vector>

namespace hadaq {

   class HldProcessor;

   // used for ROOT tree storage, similar to TdcMessage and AdcMessage
   struct TrbMessage {
      bool fTrigSyncIdFound;
      unsigned fTrigSyncId;
      unsigned fTrigSyncIdStatus;
      uint64_t fTrigTm;
      unsigned fSyncPulsePeriod;
      unsigned fSyncPulseLength;

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

   /** This is generic processor for data, coming from TRB board
    * Normally one requires specific sub-processor for frontend like TDC or any other
    * Idea that TrbProcessor can interpret HADAQ event/subevent structures and
    * will distribute data to sub-processors.  */

   class TrbProcessor : public base::StreamProc {

      friend class TdcProcessor;
      friend class SubProcessor;
      friend class HldProcessor;

      protected:

         HldProcessor *fHldProc;     ///< pointer on HLD processor

         SubProcMap fMap;            ///< map of sub-processors

         unsigned fHadaqCTSId;       ///< identifier of CTS header in HADAQ event
         std::vector<unsigned> fHadaqHUBId;   ///< identifier of HUB header in HADQ event

         unsigned fLastTriggerId;    ///< last seen trigger id
         unsigned fLostTriggerCnt;   ///< lost trigger counts
         unsigned fTakenTriggerCnt;  ///< registered trigger counts

         base::H1handle fEvSize;     ///< HADAQ event size
         unsigned fSubevHLen;        ///< maximal length of subevent in bytes
         unsigned fSubevHDiv;        ///< integer division for subevent
         base::H1handle fSubevSize;  ///< HADAQ subevent size
         base::H1handle fLostRate;   ///< lost rate
         base::H1handle fTrigType;   ///< trigger type
         base::H1handle fErrBits;    ///< error bit statistics

         base::H1handle fMsgPerBrd;  //! messages per board - from TRB
         base::H1handle fErrPerBrd;  //! errors per board - from TRB
         base::H1handle fHitsPerBrd; //! data hits per board - from TRB

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
         TrbMessage* pMsg;            ///< used for TTree store

//         base::Profiler  fProfiler;   ///< profiler

         unsigned fMinTdc{0};         ///< minimal id of TDC
         unsigned fMaxTdc{0};         ///< maximal id of TDC
         std::vector<TdcProcessor*> fTdcsVect; ///< array of TDCs

         hadaqs::RawSubevent   fLastSubevHdr; //! copy of last subevent header (without data)

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

         void SetAutoCreate(bool on = true) { fAutoCreate = on; }

         void AccountTriggerId(unsigned id);

         /** Way to register sub-processor, like for TDC */
         void AddSub(SubProcessor* tdc, unsigned id);

         /** Scan FPGA-TDC data, distribute over sub-processors */
         void ScanSubEvent(hadaqs::RawSubevent* sub, unsigned trb3eventid);

         void BeforeEventScan();

         void AfterEventScan();

         virtual void CreateBranch(TTree* t);

      public:

         TrbProcessor(unsigned brdid = 0, HldProcessor* hld = 0, int hfill = -1);
         virtual ~TrbProcessor();

         void SetHadaqCTSId(unsigned id) { fHadaqCTSId = id; }
         void SetHadaqHUBId(unsigned id1, unsigned id2=0, unsigned id3=0, unsigned id4=0)
         {
            fHadaqHUBId.clear();
            fHadaqHUBId.push_back(id1);
            if (id2!=0) fHadaqHUBId.push_back(id2);
            if (id3!=0) fHadaqHUBId.push_back(id3);
            if (id4!=0) fHadaqHUBId.push_back(id4);
         }
         void AddHadaqHUBId(unsigned id) { fHadaqHUBId.push_back(id); }

         void SetHadaqTDCId(unsigned) {} // keep for backward compatibility, can be ignored
         void SetHadaqSUBId(unsigned) {} // keep for backward compatibility, can be ignored

         virtual void UserPreLoop();
         virtual void UserPostLoop();

         /** Set trigger window not only for itself, bit for all subprocessors */
         virtual void SetTriggerWindow(double left, double right);

         /** Enable/disable store for TRB and all TDC processors */
         virtual void SetStoreKind(unsigned kind = 1);

         /** Scan all messages, find reference signals */
         virtual bool FirstBufferScan(const base::Buffer& buf);

         void SetPrintRawData(bool on = true) { fPrintRawData = on; }
         bool IsPrintRawData() const { return fPrintRawData; }

         void SetPrintErrors(int cnt = 100) { fPrintErrCnt = cnt; }
         bool CheckPrintError();

         void SetCrossProcess(bool on = true);
         bool IsCrossProcess() const { return fCrossProcess; }

         /** Enable/disable ch0 store in output event for all TDC processors */
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
         bool IsUseTriggerAsSync() const { return fUseTriggerAsSync; }

         /** When enabled, artificially create contiguous epoch value */
         void SetCompensateEpochReset(bool on = true) { fCompensateEpochReset = on; }

         unsigned NumSubProc() const { return fMap.size(); }
         SubProcessor* GetSubProc(unsigned n) const
         {
            for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
               if (n==0) return iter->second;
               n--;
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

         unsigned NumberOfTDC() const
         {
            unsigned num = 0;
            for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
               if (iter->second->IsTDC()) num++;
            }
            return num;
         }

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

         /** Search TDC in current TRB or in the top HLD */
         TdcProcessor* FindTDC(unsigned tdcid) const;

         /** Set defaults for the next creation of TDC processors.
          * One provides number of channels and edges.
          * Also one can enable usage of sync message - by default sync messages are ignored.
          * 1 - use only rising edge, falling edge is ignore
          * 2 - falling edge enabled and fully independent from rising edge
          * 3 - falling edge enabled and uses calibration from rising edge
          * 4 - falling edge enabled and common statistic is used for calibration */
         static void SetDefaults(unsigned numch=65, unsigned edges=0x1, bool ignore_sync = true);

         static void SetTDCRange(unsigned min, unsigned max)
         {
            gTDCMin = min;
            gTDCMax = max;
         }

         static void SetHUBRange(unsigned min, unsigned max)
         {
            gHUBMin = min;
            gHUBMax = max;
         }

         /** Create up-to 4 TDCs processors with specified IDs */
         int CreateTDC(unsigned id1, unsigned id2 = 0, unsigned id3 = 0, unsigned id4 = 0);

         /** Create TDC processor, which extracts TDC information from CTS header */
         void CreateCTS_TDC() { new hadaq::TdcProcessor(this, fHadaqCTSId, gNumChannels, gEdgesMask); }

         /** Disable calibration of specified channels in all TDCs */
         void DisableCalibrationFor(unsigned firstch, unsigned lastch = 0);

         /** Mark automatic calibrations for all TDCs */
         void SetAutoCalibrations(long cnt = 100000);

         /** Specify to produce and write calibrations at the end of data processing */
         void SetWriteCalibrations(const char* fileprefix, bool every_time = false, bool use_linear = false);

         /** Load TDC calibrations, as argument file prefix (without TDC id) should be specified
          * One also could specify coefficient to scale calibration (koef >= 1) */
         bool LoadCalibrations(const char* fileprefix);

         /** Central method to configure way how calibrations will be performed */
         void ConfigureCalibration(const std::string& name, long period, unsigned trigmask = 0xFFFF);

         /** Set calibration trigger type for all TDCs */
         void SetCalibrTriggerMask(unsigned trigmask = 0xFFFF);

         /** Calibrate hits in subevent */
         unsigned TransformSubEvent(hadaqs::RawSubevent *sub, void *tgtbuf = 0, unsigned tgtlen = 0, bool only_hist = false);

         /** Just for emulation of TDC calibrations */
         unsigned EmulateTransform(hadaqs::RawSubevent *sub, int dummycnt, bool only_hist = false);

         /** Create TDCs using IDs from subevent */
         bool CreateMissingTDC(hadaqs::RawSubevent *sub, const std::vector<uint64_t> &mintdc, const std::vector<uint64_t> &maxtdc, int numch, int edges);

         /** Create TDC-specific histograms */
         void CreatePerTDCHistos();

         hadaqs::RawSubevent& GetLastSubeventHdr() { return fLastSubevHdr; }
   };
}

#endif
