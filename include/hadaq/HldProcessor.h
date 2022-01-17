#ifndef HADAQ_HLDPROCESSOR_H
#define HADAQ_HLDPROCESSOR_H

#include "base/EventProc.h"

#include "hadaq/definess.h"

#include "hadaq/TrbProcessor.h"

namespace hadaq {


   /** map of trb processors */
   typedef std::map<unsigned,TrbProcessor*> TrbProcMap;

   /** \brief HLD message
     *
     * \ingroup stream_hadaq_classes
     *
     * Data extracted by \ref hadaq::HldProcessor and stored in \ref hadaq::HldSubEvent */

   struct HldMessage {
      uint8_t trig_type; ///< trigger type
      uint32_t seq_nr;   ///< event sequence number
      uint32_t run_nr;   ///< run number

      /** constructor */
      HldMessage() : trig_type(0), seq_nr(0), run_nr(0) {}

      /** copy constructor */
      HldMessage(const HldMessage& src) : trig_type(src.trig_type), seq_nr(src.seq_nr), run_nr(src.run_nr) {}
   };

   /** \brief HLD subevent
     *
     * \ingroup stream_hadaq_classes
     *
     * Stored in the TTree by the \ref hadaq::HldProcessor */

   class HldSubEvent : public base::SubEvent {
      public:
         HldMessage fMsg;   ///< message

         /** constructor */
         HldSubEvent() : base::SubEvent(), fMsg() {}
         /** copy constructor */
         HldSubEvent(const HldMessage& msg) : base::SubEvent(), fMsg(msg) {}
         /** destructor */
         virtual ~HldSubEvent() {}

         /** Method returns event multiplicity - that ever it means */
         virtual unsigned Multiplicity() const { return 1; }
   };

   /** \brief HLD filter
     *
     * \ingroup stream_hadaq_classes
     *
     * processor used to filter-out all HLD events with specific trigger
     * only these events will be processed and stored */

   class HldFilter : public base::EventProc {
      protected:
         unsigned fOnlyTrig;   ///< configured trigger to filter
      public:

         /** constructor */
         HldFilter(unsigned trig = 0x1) : base::EventProc(), fOnlyTrig(trig) {}

         /** destructor */
         virtual ~HldFilter() {}

         /** process event */
         virtual bool Process(base::Event* ev)
         {
            hadaq::HldSubEvent* sub =
                  dynamic_cast<hadaq::HldSubEvent*> (ev->GetSubEvent("HLD"));

            if (sub==0) return false;

            return sub->fMsg.trig_type == fOnlyTrig;
         }
   };

   /** \brief HLD processor
     *
     * \ingroup stream_hadaq_classes
     *
     * Top-level processor of HLD events
     * This is generic processor for HLD data
     * Its only task - redistribute data over TRB processors  */

   class HldProcessor : public base::StreamProc {

      friend class TrbProcessor;

      protected:

         TrbProcMap fMap;            ///< map of trb processors

         unsigned  fEventTypeSelect; ///< selection for event type (lower 4 bits in event id)
         bool      fFilterStatusEvents; ///< filter out status events

         bool fPrintRawData;         ///< true when raw data should be printed

         bool fAutoCreate;           ///< when true, TRB/TDC processors will be created automatically
         std::string fAfterFunc; ///< function called after new elements are created

         std::string fCalibrName;      ///< name of calibration for (auto)created components
         long fCalibrPeriod;           ///< how often calibration should be performed
         unsigned fCalibrTriggerMask;  ///< mask with enabled event ID, default all

         base::H1handle fEvType;       ///< HADAQ event type
         base::H1handle fEvSize;       ///< HADAQ event size
         base::H1handle fSubevSize;    ///< HADAQ sub-event size
         base::H1handle fHitsPerTDC;   ///< HADAQ hits per TDC
         base::H1handle fErrPerTDC;    ///< HADAQ errors per TDC
         base::H2handle fHitsPerTDCChannel; ///< HADAQ hits per TDC channel
         base::H2handle fErrPerTDCChannel;  ///< HADAQ errors per TDC channel
         base::H2handle fCorrPerTDCChannel; ///< HADAQ corrections per TDC channel
         base::H2handle fQaFinePerTDCChannel;  ///< HADAQ QA fine time per TDC channel
         base::H2handle fQaToTPerTDCChannel;  ///< HADAQ QA ToT per TDC channel
         base::H2handle fQaEdgesPerTDCChannel;  ///< HADAQ QA edges per TDC channel
         base::H2handle fQaErrorsPerTDCChannel;  ///< HADAQ QA errors per TDC channel
         base::H1handle fQaSummary;  ///< HADAQ QA summary histogram
         // JAM2021: some new histograms for hades tdc calibration check
         base::H2handle fToTPerTDCChannel;  ///< HADAQ ToT per TDC channel, real values
         base::H2handle fShiftPerTDCChannel;  ///< HADAQ calibrated shift per TDC channel, real values
         base::H1handle fExpectedToTPerTDC;  ///< HADAQ expected ToT per TDC sed for calibration
         base::H2handle fDevPerTDCChannel;  ///< HADAQ ToT deviation per TDC channel
         base::H2handle fPrevDiffPerTDCChannel;  ///< HADAQ rising edge time difference to previous channel
         HldMessage     fMsg;        ///< used for TTree store
         HldMessage    *pMsg;        ///< used for TTree store

         hadaqs::RawEvent  fLastEvHdr;    ///<! copy of last event header (without data)

         long fLastHadesTm;               ///<! last hades time

         /** Returns true when processor used to select trigger signal
          * TRB3 not yet able to perform trigger selection */
         virtual bool doTriggerSelection() const { return false; }

         /** Way to register trb processor */
         void AddTrb(TrbProcessor* trb, unsigned id);

         virtual void CreateBranch(TTree*);

         virtual void Store(base::Event* ev);
         virtual void ResetStore();

         void CreatePerTDCHisto();

         void DoHadesHistSummary();

         void SetCrossProcess(bool on);

      public:

         HldProcessor(bool auto_create = false, const char* after_func = "");
         virtual ~HldProcessor();

         /** Returns current event id */
         uint32_t GetEventId() const { return fMsg.seq_nr; }
         /** Returns current run id */
         uint32_t GetRunId() const { return fMsg.run_nr; }

         unsigned NumberOfTDC() const;
         TdcProcessor* GetTDC(unsigned indx) const;
         TdcProcessor* FindTDC(unsigned tdcid) const;

         unsigned NumberOfTRB() const;
         TrbProcessor* GetTRB(unsigned indx) const;
         TrbProcessor* FindTRB(unsigned trbid) const;

         void ConfigureCalibration(const std::string& fileprefix, long period, unsigned trig = 0xFFFF);

         /** Set event type, only used in the analysis */
         void SetEventTypeSelect(unsigned evid) { fEventTypeSelect = evid; }
         unsigned GetEventTypeSelect() const { return fEventTypeSelect; }

         void SetFilterStatusEvents(bool on = true) { fFilterStatusEvents = on; }
         bool GetFilterStatusEvents() const { return fFilterStatusEvents; }

         virtual void SetTriggerWindow(double left, double right);

         virtual void SetStoreKind(unsigned kind = 1);

         /** Scan all messages, find reference signals */
         virtual bool FirstBufferScan(const base::Buffer& buf);

         void SetPrintRawData(bool on = true);
         /** Returns true if print raw data configured */
         bool IsPrintRawData() const { return fPrintRawData; }

         /** Enable auto-create mode */
         void SetAutoCreate(bool on = true) { fAutoCreate = on; }

         unsigned TransformEvent(void* src, unsigned len, void* tgt = 0, unsigned tgtlen = 0);

         virtual void UserPreLoop();

         /** Return reference on last event header structure */
         hadaqs::RawEvent& GetLastEventHdr() { return fLastEvHdr; }
   };

}

#endif
