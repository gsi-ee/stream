#ifndef HADAQ_TRBPROCESSOR_H
#define HADAQ_TRBPROCESSOR_H

#include "base/StreamProc.h"

#include "hadaq/definess.h"

#include "hadaq/TdcProcessor.h"

#include "hadaq/SubProcessor.h"

namespace hadaq {

   class HldProcessor;


   /** This is generic processor for data, coming from TRB board
    * Normally one requires specific sub-processor for frontend like TDC or any other
    * Idea that TrbProcessor can interpret HADAQ event/subevent structures and
    * will distribute data to sub-processors.  */

   class TrbProcessor : public base::StreamProc {

      friend class TdcProcessor;
      friend class SubProcessor;
      friend class HldProcessor;

      protected:

         HldProcessor* fHldProc;     ///< pointer on HLD processor

         SubProcMap fMap;            ///< map of sub-processors

         unsigned fHadaqCTSId;       ///< identifier of CTS header in HADAQ event
         unsigned fHadaqHUBId;       ///< identifier of HUB header in HADQ event

         unsigned fLastTriggerId;    ///< last seen trigger id
         unsigned fLostTriggerCnt;   ///< lost trigger counts
         unsigned fTakenTriggerCnt;  ///< registered trigger counts

         base::H1handle fEvSize;     ///< HADAQ event size
         base::H1handle fSubevSize;  ///< HADAQ subevent size
         base::H1handle fLostRate;   ///< lost rate

         base::H1handle fMsgPerBrd;  //! messages per board - from TRB
         base::H1handle fErrPerBrd;  //! errors per board - from TRB
         base::H1handle fHitsPerBrd; //! data hits per board - from TRB

         bool fPrintRawData;         ///< true when raw data should be printed
         bool fCrossProcess;         ///< if true, cross-processing will be enabled
         int  fPrintErrCnt;          ///< number of error messages, which could be printed

         unsigned fSyncTrigMask;     ///< mask which should be applied for trigger type
         unsigned fSyncTrigValue;    ///< value from trigger type (after mask) which corresponds to sync message

         bool fUseTriggerAsSync;     ///< when true, trigger number used as sync message between TRBs

         bool fCompensateEpochReset;  ///< when true, artificially create contiguous epoch value

         static unsigned gNumChannels;     ///< default number of channels
         static unsigned gEdgesMask;      ///< default edges mask

         /** Returns true when processor used to select trigger signal
          * TRB3 not yet able to perform trigger selection */
         virtual bool doTriggerSelection() const { return false; }

         void AccountTriggerId(unsigned id);

         /** Way to register sub-processor, like for TDC */
         void AddSub(SubProcessor* tdc, unsigned id);

         /** Scan FPGA-TDC data, distribute over sub-processors */
         void ScanSubEvent(hadaqs::RawSubevent* sub, unsigned trb3eventid);

         void AfterEventScan();

      public:

         TrbProcessor(unsigned brdid = 0, HldProcessor* hld = 0);
         virtual ~TrbProcessor();

         void SetHadaqCTSId(unsigned id) { fHadaqCTSId = id; }
         void SetHadaqHUBId(unsigned id) { fHadaqHUBId = id; }
         void SetHadaqTDCId(unsigned) {} // keep for backward compatibility, can be ignored
         void SetHadaqSUBId(unsigned) {} // keep for backward compatibility, can be ignored

         virtual void UserPreLoop();

         /** Set trigger window not only for itself, bit for all subprocessors */
         virtual void SetTriggerWindow(double left, double right);

         /** Enable/disable store for TRB and all TDC processors */
         virtual void SetStoreEnabled(bool on = true);

         /** Scan all messages, find reference signals */
         virtual bool FirstBufferScan(const base::Buffer& buf);

         void SetPrintRawData(bool on = true) { fPrintRawData = on; }
         bool IsPrintRawData() const { return fPrintRawData; }

         void SetPrintErrors(int cnt = 100) { fPrintErrCnt = cnt; }
         bool CheckPrintError();

         void SetCrossProcess(bool on = true) { fCrossProcess = on; }
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

            if (iter == fMap.end()) return 0;
            return iter->second->IsTDC() ? (TdcProcessor*) iter->second : 0;
         }

         /** Search TDC in current TRB or in the top HLD */
         TdcProcessor* FindTDC(unsigned tdcid) const;

         static void SetDefaults(unsigned numch=65, unsigned edges=0x1);

         /** Create up-to 4 TDCs processors with specified IDs */
         void CreateTDC(unsigned id1, unsigned id2 = 0, unsigned id3 = 0, unsigned id4 = 0);

         /** Create TDC processor, which extracts TDC information from CTS header */
         void CreateCTS_TDC() { CreateTDC(fHadaqCTSId); }

         /** Disable calibration of specified channels in all TDCs */
         void DisableCalibrationFor(unsigned firstch, unsigned lastch = 0);

         /** Mark automatic calibrations for all TDCs */
         void SetAutoCalibrations(long cnt = 100000);

         /** Specify to produce and write calibrations at the end of data processing */
         void SetWriteCalibrations(const char* fileprefix, bool every_time = false);

         /** Load TDC calibrations, as argument file prefix (without TDC id) should be specified
          * One also could specify coefficient to scale calibration (koef >= 1) */
         bool LoadCalibrations(const char* fileprefix, double koef = 1.);

         /** Calibrate hits in subevent */
         void TransformSubEvent(hadaqs::RawSubevent* sub);

         /** Checks if all TDC can perform auto calibration and do it,
          * return progress from 0 to 1 */
         double CheckAutoCalibration();

   };
}

#endif
