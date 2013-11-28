#ifndef HADAQ_TRBPROCESSOR_H
#define HADAQ_TRBPROCESSOR_H

#include "base/StreamProc.h"

#include "hadaq/defines.h"

#include "hadaq/TdcProcessor.h"

namespace hadaq {


   /** This is generic processor for data, coming from TRB board
    * Normally one requires specific sub-processor for frontend like TDC or any other
    * Idea that TrbProcessor can interpret HADAQ event/subevent structures and
    * will distribute data to sub-processors.  */

   class TrbProcessor : public base::StreamProc {

      friend class TdcProcessor;

      protected:

         SubProcMap fMap;            ///< map of sub-processors

         unsigned fHadaqCTSId;       ///< identifier of CTS header in HADAQ event
         unsigned fHadaqHUBId;       ///< identifier of HUB header in HADQ event
         unsigned fHadaqTDCId;       ///< identifier of TDC header in HADQ event

         unsigned fLastTriggerId;    ///< last seen trigger id
         unsigned fLostTriggerCnt;   ///< lost trigger counts
         unsigned fTakenTriggerCnt;  ///< registered trigger counts

         base::H1handle fEvSize;     ///< HADAQ event size
         base::H1handle fSubevSize;  ///< HADAQ subevent size
         base::H1handle fLostRate;   ///< lost rate

         base::H1handle fTdcDistr;   ///< distribution of data over TDCs

         base::H1handle fMsgPerBrd;  //! messages per board - from TRB
         base::H1handle fErrPerBrd;  //! errors per board - from TRB
         base::H1handle fHitsPerBrd; //! data hits per board - from TRB

         bool fPrintRawData;         ///< true when raw data should be printed
         bool fCrossProcess;         ///< if true, cross-processing will be enabled
         int  fPrintErrCnt;          ///< number of error messages, which could be printed

         /** Returns true when processor used to select trigger signal
          * TRB3 not yet able to perform trigger selection */
         virtual bool doTriggerSelection() const { return false; }

         void AccountTriggerId(unsigned id);

         /** Way to register sub-processor, like for TDC */
         void AddSub(TdcProcessor* tdc, unsigned id);

         /** Scan FPGA-TDC data, distribute over sub-processors */
         void ScanSubEvent(hadaq::RawSubevent* sub);

      public:

         TrbProcessor(unsigned brdid);
         virtual ~TrbProcessor();

         void SetHadaqCTSId(unsigned id) { fHadaqCTSId = id; }
         void SetHadaqHUBId(unsigned id) { fHadaqHUBId = id; }
         void SetHadaqTDCId(unsigned id) { fHadaqTDCId = id; }

         /** Set trigger window not only for itself, bit for all subprocessors */
         virtual void SetTriggerWindow(double left, double right);

         /** Scan all messages, find reference signals */
         virtual bool FirstBufferScan(const base::Buffer& buf);

         void SetPrintRawData(bool on = true) { fPrintRawData = on; }
         bool IsPrintRawData() const { return fPrintRawData; }

         void SetPrintErrors(int cnt = 100) { fPrintErrCnt = cnt; }
         bool CheckPrintError();

         void SetCrossProcess(bool on = true) { fCrossProcess = on; }
         bool IsCrossProcess() const { return fCrossProcess; }


         unsigned NumSubProc() const { return fMap.size(); }
         TdcProcessor* GetSubProc(unsigned n) const
         {
            for (SubProcMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
               if (n==0) return iter->second;
               n--;
            }
            return 0;
         }

   };
}

#endif
