#ifndef BASE_STREAMPROC_H
#define BASE_STREAMPROC_H

#include <string>

#include "base/Processor.h"

namespace base {

   class Event;

   /** \brief Abstract processor of data streams
    *
    * \ingroup stream_core_classes
    *
    * Class base::StreamProc is abstract processor of data streams
    * from TRB3 or GET4 or nXYTER or any other kind of data.
    * Main motivation for the class is unify way how data-streams can be processed
    * and how all kind of time calculations could be done.  */

   class StreamProc : public Processor {
      friend class ProcMgr;

      public:
         enum SyncKind {
            sync_None,         ///< no synchronization
            sync_Inter,        ///< use time interpolation between two markers
            sync_Left,         ///< use sync marker on left side
            sync_Right         ///< use sync marker on right side
         };


      protected:

         typedef RecordsQueue<base::Buffer, false> BuffersQueue;

         typedef RecordsQueue<base::SyncMarker, false> SyncMarksQueue;

         BuffersQueue fQueue;                     ///<! buffers queue

         unsigned        fQueueScanIndex;         ///< index of next buffer which should be scanned
         unsigned        fQueueScanIndexTm;       ///< index of buffer to scan and set correct times of the buffer head

         AnalysisKind    fAnalysisKind;           ///< defines that processor is doing

         SyncKind        fSynchronisationKind;    ///< kind of synchronization
         SyncMarksQueue  fSyncs;                  ///< list of sync markers
         unsigned        fSyncScanIndex;          ///< sync scan index, indicate number of syncs which can really be used for synchronization
         bool            fSyncFlag;               ///< boolean, used in sync adjustment procedure

         LocalMarkersQueue  fLocalMarks;          ///< queue with local markers
         double          fTriggerAcceptMaring;    ///< time margin (in local time) to accept new trigger
         GlobalTime_t    fLastLocalTriggerTm;     ///< time of last local trigger

         GlobalMarksQueue fGlobalMarks;           ///< list of global triggers in work

         unsigned fGlobalTrigScanIndex;           ///< index with first trigger which is not yet ready
         unsigned fGlobalTrigRightIndex;          ///< temporary value, used during second buffers scan

         bool fTimeSorting;                       ///< defines if time sorting should be used for the messages

         base::H1handle fTriggerTm;  ///<! histogram with time relative to the trigger
         base::H1handle fMultipl;    ///<! histogram of event multiplicity

         base::C1handle fTriggerWindow;   ///<  window used for data selection

         static unsigned fMarksQueueCapacity;   ///< maximum number of items in the marksers queue
         static unsigned fBufsQueueCapacity;   ///< maximum number of items in the queue

         /** Make constructor protected - no way to create base class instance */
         StreamProc(const char* name = "", unsigned brdid = DummyBrdId, bool basehist = true);

         /** Method indicate if any kind of time-synchronization technique
          * should be applied for the processor. Following values can be applied:
          *   0 - no sync, local time will be used (with any necessary conversion)
          *   1 - interpolation, for every hit sync on left and right side should be used
          *   2 - sync message on left side will be used for calibration
          *   3 - sync message on left side will be used for calibration */
         void SetSynchronisationKind(SyncKind kind = sync_Inter) { fSynchronisationKind = kind; }

         void AddSyncMarker(SyncMarker& marker);

         /** Add new local trigger.
           * Method first proves that new trigger marker stays in time order
           *  and have minimal distance to previous trigger */
         bool AddTriggerMarker(LocalTimeMarker& marker, double tm_range = 0.);

         /** Method converts local time (in ns representation) to global time
          * TODO: One could introduce more precise method, which works with stamps*/
         GlobalTime_t LocalToGlobalTime(GlobalTime_t localtm, unsigned* sync_index = 0);

         /** Method return true when sync_index is means interpolation of time */
         bool IsSyncIndexWithInterpolation(unsigned indx) const
         { return (indx>0) && (indx<numReadySyncs()); }

         /** Returns true when processor used to select trigger signal */
         virtual bool doTriggerSelection() const { return false; }

         /** Method should return time, which could be flushed from the processor */
         virtual GlobalTime_t ProvidePotentialFlushTime(GlobalTime_t last_marker);

         /** Method must ensure that processor scanned such time and can really skip this data */
         bool VerifyFlushTime(const base::GlobalTime_t& flush_time);

         /** Time constant, defines how far disorder of messages can go */
         virtual double MaximumDisorderTm() const { return 0.; }

         /** Method decides to which trigger window belong hit
          *  normal_hit - indicates that time is belong to data, which than can be assigned to output
          *  can_close_event - when true, hit time can be used to decide that event is ready */
         unsigned TestHitTime(const base::GlobalTime_t& hittime, bool normal_hit, bool can_close_event = true);

         // TODO: this is another place for future improvement
         // one can preallocate number of subevents with place ready for some messages
         // than one can use these events instead of creating them on the fly

         template<class EventClass, class MessageClass>
         void AddMessage(unsigned indx, EventClass* ev, const MessageClass& msg)
         {
            if (ev==0) {
               ev = new EventClass;
               fGlobalMarks.item(indx).subev = ev;
            }
            ev->AddMsg(msg);
         }

         /** Removes sync at specified position */
         bool eraseSyncAt(unsigned indx);

         /** Remove specified number of syncs */
         bool eraseFirstSyncs(unsigned sync_num);

      public:

         virtual ~StreamProc();

         /** Enable/disable time sorting of data in output event */
         void SetTimeSorting(bool on) { fTimeSorting = on; }
         /** Is time sorting enabled */
         bool IsTimeSorting() const { return fTimeSorting; }

         /** Set minimal distance between two triggers */
         void SetTriggerMargin(double margin = 0.) { fTriggerAcceptMaring = margin; }

         void CreateTriggerHist(unsigned multipl = 40, unsigned nbins = 2500, double left = -1e-6, double right = 4e-6);

         /** Set window relative to some reference signal, which will be used as
          * region-of-interest interval to select messages from the stream */
         virtual void SetTriggerWindow(double left, double right)
         { ChangeC1(fTriggerWindow, left, right); }

         /** Method set raw-scan only mode for processor
          * Processor will not be used for any data selection */
         void SetRawScanOnly() { fAnalysisKind = kind_RawOnly; }
         /** Is only raw scan will be performed */
         bool IsRawScanOnly() const { return fAnalysisKind == kind_RawOnly; }

         /** Is raw analysis only */
         bool IsRawAnalysis() const { return fAnalysisKind <= kind_Raw; }
         /** Is triggered events analysis */
         bool IsTriggeredAnalysis() const { return fAnalysisKind == kind_Triggered; }
         /** Is full stream analysis */
         bool IsStreamAnalysis() const { return fAnalysisKind == kind_Stream; }

         /** Method indicate if any kind of time-synchronization technique
          * should be applied for the processor.
          * If true, sync messages must be produced by processor and will be used.
          * If false, local time stamps could be immediately used (with any necessary conversion) */
         bool IsSynchronisationRequired() const { return fSynchronisationKind != sync_None; }

         /** Returns minimal number of syncs required for time synchronisation */
         unsigned minNumSyncRequired() const {
            switch (fSynchronisationKind) {
               case sync_None:  return 0;
               case sync_Inter: return 2;
               case sync_Left:  return 1;
               case sync_Right: return 1;
            }
            return 0;
         }

         /** Provide next port of data to the processor */
         virtual bool AddNextBuffer(const Buffer& buf);

         /** \brief Scanning all new buffers in the queue
          *  \returns true when any new data was scanned */
         virtual bool ScanNewBuffers();

         /** With new calibration set (where possible) time of buffers */
         virtual bool ScanNewBuffersTm();

         /** Method to remove all buffers, all triggers and so on */
         virtual void SkipAllData();

         /** Force processor to skip buffers from input */
         virtual bool SkipBuffers(unsigned cnt);

         /** Returns total number of sync markers */
         unsigned numSyncs() const { return fSyncs.size(); }
         /** Returns number of read sync markers */
         unsigned numReadySyncs() const { return fSyncScanIndex; }
         /** Returns sync marker */
         SyncMarker& getSync(unsigned n) { return fSyncs.item(n); }
         unsigned findSyncWithId(unsigned syncid) const;

         /** Method to deliver detected triggers from processor to central manager */
         virtual bool CollectTriggers(GlobalMarksQueue& queue);

         /** This is method to get back identified triggers from central manager */
         virtual bool DistributeTriggers(const GlobalMarksQueue& queue);

         /** Here each processor should scan data again for new triggers
          * Method made virtual while some subprocessors will do it in connection with others */
         virtual bool ScanDataForNewTriggers();

         /** Returns number of already build events */
         unsigned NumReadySubevents() const { return fGlobalTrigScanIndex; }

         /** Append data for first trigger to the main event */
         virtual bool AppendSubevent(base::Event* evt);

         /** Central method to scan new data in the queue
          * This should include:
          *   - data indexing;
          *   - raw histogram filling;
          *   - search for special time markers;
          *   - multiplicity histogramming (if necessary) */
         virtual bool FirstBufferScan(const base::Buffer&) { return false; }

         /** Second generic scan of buffer
          * Here selection of data for region-of-interest should be performed */
         virtual bool SecondBufferScan(const base::Buffer&) { return false; }

         /** Generic method to store processor data,
          * In case of ROOT one should copy event data in temporary structures,
          * which are mapped to the branch */
         virtual void Store(Event*) {}

         /** Generic method to store processor data,
          * In case of ROOT one should copy event data in temporary structures,
          * which are mapped to the branch */
         virtual void ResetStore() {}

         /** Set markers queue capacity */
         static void SetMarksQueueCapacity(unsigned sz) { fMarksQueueCapacity = sz; }
         /** Set buffers queue capacity */
         static void SetBufsQueueCapacity(unsigned sz) { fBufsQueueCapacity = sz; }

   };

}

#endif
