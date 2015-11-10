#ifndef HADAQ_HLDPROCESSOR_H
#define HADAQ_HLDPROCESSOR_H

#include "base/StreamProc.h"

#include "hadaq/definess.h"

#include "hadaq/TrbProcessor.h"

namespace hadaq {


   /** This is generic processor for HLD data
    * Its only task - redistribute data over TRB processors  */

   typedef std::map<unsigned,TrbProcessor*> TrbProcMap;

   typedef void* AfterCreateFunc(void*);

   class HldProcessor : public base::StreamProc {

      friend class TrbProcessor;

      protected:

         TrbProcMap fMap;            ///< map of trb processors

         unsigned  fEventTypeSelect; ///< selection for event type (lower 4 bits in event id)

         bool fPrintRawData;         ///< true when raw data should be printed

         bool fAutoCreate;           ///< when true, TRB/TDC processors will be created automatically
         std::string fAfterFunc; ///< function called after new elements are created

         std::string fCalibrName;    ///< name of calibration for (auto)created components
         long fCalibrPeriod;         ///< how often calibration should be performed

         base::H1handle fEvType;     ///< HADAQ event type
         base::H1handle fEvSize;     ///< HADAQ event size
         base::H1handle fSubevSize;  ///< HADAQ sub-event size

         /** Returns true when processor used to select trigger signal
          * TRB3 not yet able to perform trigger selection */
         virtual bool doTriggerSelection() const { return false; }

         /** Way to register trb processor */
         void AddTrb(TrbProcessor* trb, unsigned id);


      public:

         HldProcessor(bool auto_create = false, const char* after_func = 0);
         virtual ~HldProcessor();

         /** Search for specified TDC in all subprocessors */
         TdcProcessor* FindTDC(unsigned tdcid) const;

         TrbProcessor* FindTRB(unsigned trbid) const;

         unsigned NumberOfTDC() const;
         TdcProcessor* GetTDC(unsigned indx) const;

         unsigned NumberOfTRB() const;
         TrbProcessor* GetTRB(unsigned indx) const;

         /** Configure calibration for all components
          *  \par name  file prefix for calibrations. Could include path. Will be extend for individual TDC
          *  \par period how often automatic calibration will be performed. 0 - never, -1 - at the end of run */
         void ConfigureCalibration(const std::string& name, long period);

         /** Check auto-calibration for all TRBs */
         double CheckAutoCalibration();

         /** Set event type, only used in the analysis */
         void SetEventTypeSelect(unsigned evid) { fEventTypeSelect = evid; }

         /** Set trigger window not only for itself, bit for all subprocessors */
         virtual void SetTriggerWindow(double left, double right);

         /** Enable/disable store for HLD and all TRB processors */
         virtual void SetStoreKind(unsigned kind = 1);

         /** Scan all messages, find reference signals */
         virtual bool FirstBufferScan(const base::Buffer& buf);

         void SetPrintRawData(bool on = true);
         bool IsPrintRawData() const { return fPrintRawData; }

         void SetAutoCreate(bool on = true) { fAutoCreate = on; }

         /** Function to transform HLD event, used for TDC calibrations */
         unsigned TransformEvent(void* src, unsigned len, void* tgt = 0, unsigned tgtlen = 0);

         virtual void UserPreLoop();

   };
}

#endif
