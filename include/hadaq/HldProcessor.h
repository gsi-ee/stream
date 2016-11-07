#ifndef HADAQ_HLDPROCESSOR_H
#define HADAQ_HLDPROCESSOR_H

#include "base/StreamProc.h"

#include "base/EventProc.h"

#include "hadaq/definess.h"

#include "hadaq/TrbProcessor.h"

namespace hadaq {


   /** This is generic processor for HLD data
    * Its only task - redistribute data over TRB processors  */

   typedef std::map<unsigned,TrbProcessor*> TrbProcMap;

   struct HldMessage {
      uint8_t trig_type;

      HldMessage() : trig_type(0) {}
      HldMessage(const HldMessage& src) : trig_type(src.trig_type) {}
   };

   class HldSubEvent : public base::SubEvent {
      public:
         HldMessage fMsg;

         HldSubEvent() : base::SubEvent(), fMsg() {}
         HldSubEvent(const HldMessage& msg) : base::SubEvent(), fMsg(msg) {}
         virtual ~HldSubEvent() {}

         /** Method returns event multiplicity - that ever it means */
         virtual unsigned Multiplicity() const { return 1; }
   };

   // processor used to filter-out all HLD events with specific trigger
   // only these events will be processed and stored

   class HldFilter : public base::EventProc {
      protected:
         unsigned fOnlyTrig;
      public:

         HldFilter(unsigned trig = 0x1) : base::EventProc(), fOnlyTrig(trig) {}
         virtual ~HldFilter() {}

         virtual bool Process(base::Event* ev)
         {
            hadaq::HldSubEvent* sub =
                  dynamic_cast<hadaq::HldSubEvent*> (ev->GetSubEvent("HLD"));

            if (sub==0) return false;

            return sub->fMsg.trig_type == fOnlyTrig;
         }
   };

   class HldProcessor : public base::StreamProc {

      friend class TrbProcessor;

      protected:

         TrbProcMap fMap;            ///< map of trb processors

         unsigned  fEventTypeSelect; ///< selection for event type (lower 4 bits in event id)

         bool fPrintRawData;         ///< true when raw data should be printed

         bool fAutoCreate;           ///< when true, TRB/TDC processors will be created automatically
         std::string fAfterFunc; ///< function called after new elements are created

         std::string fCalibrName;      ///< name of calibration for (auto)created components
         long fCalibrPeriod;           ///< how often calibration should be performed
         unsigned fCalibrTriggerMask;  ///< mask with enabled event ID, default all

         base::H1handle fEvType;     ///< HADAQ event type
         base::H1handle fEvSize;     ///< HADAQ event size
         base::H1handle fSubevSize;  ///< HADAQ sub-event size

         HldMessage     fMsg;        ///< used for TTree store
         HldMessage    *pMsg;        ///< used for TTree store

         hadaqs::RawEvent  fLastEvHdr;    //! copy of last event header (without data)

         /** Returns true when processor used to select trigger signal
          * TRB3 not yet able to perform trigger selection */
         virtual bool doTriggerSelection() const { return false; }

         /** Way to register trb processor */
         void AddTrb(TrbProcessor* trb, unsigned id);

         virtual void CreateBranch(TTree*);

         virtual void Store(base::Event* ev);

      public:

         HldProcessor(bool auto_create = false, const char* after_func = "");
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
          *  \par period how often automatic calibration will be performed. 0 - never, -1 - at the end of run
          *  \par trig specifies trigger type used for calibration (0xFFFF means all kind of triggers) */
         void ConfigureCalibration(const std::string& name, long period, unsigned trig = 0xFFFF);

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

         hadaqs::RawEvent& GetLastEventHdr() { return fLastEvHdr; }
   };



}

#endif
