#ifndef BASE_STREAMPROC_H
#define BASE_STREAMPROC_H

#include <vector>

#include <string>

#include "base/Buffer.h"
#include "base/SubEvent.h"

namespace base {

   /** Class base::StreamProc is abstract processor of data streams
    * from nXYTER or GET4s or any other kind of data.
    * Main motivation for the class is unify way how data-streams can be processed
    * and how all kind of time calculations could be done.  */

   class ProcMgr;
   class Event;

   typedef void* H1handle;
   typedef void* H2handle;
   typedef void* C1handle;


   struct SyncMarker {
      unsigned      uniqueid;    //!< unique identifier for marker like 24-bit number of SYNC message
      unsigned      localid;     //!< some local id, custom number for each subsystem
      LocalStamp_t  local_stamp; //!< 64-bit integer for local stamp representation
      GlobalTime_t  localtm;     //!< local time (ns),
      GlobalTime_t  globaltm;    //!< global time (ns), used for all selections
      unsigned      bufid;       //!< use it for keep reference from which buffer it is

      SyncMarker() : uniqueid(0), localid(0), local_stamp(0), localtm(0.), globaltm(0.), bufid(0) {}
   };


   struct LocalTriggerMarker {
      GlobalTime_t  localtm;     //!< local time in ns,
      unsigned      localid;     //!< arbitrary id, like aux number or sync number

      LocalTriggerMarker() : localtm(0.), localid(0)  {}
   };


   struct GlobalTriggerMarker {
      GlobalTime_t globaltm;     //!< global time - reference time of marker
      GlobalTime_t lefttm;       //!< left range for hit selection
      GlobalTime_t righttm;      //!< right range for hit selection

      SubEvent*     subev;        //!< structure with data, selected for the trigger, ownership
      bool          isflush;      //!< indicate that trigger is just for flushing, no real data is important

      GlobalTriggerMarker(GlobalTime_t tm = 0.) :
         globaltm(tm), lefttm(0.), righttm(0.), subev(0), isflush(false) {}

      GlobalTriggerMarker(const GlobalTriggerMarker& src) :
         globaltm(src.globaltm), lefttm(src.lefttm), righttm(src.righttm), subev(src.subev), isflush(src.isflush) {}

      ~GlobalTriggerMarker() { /** should we here destroy subevent??? */ }

      bool null() const { return globaltm<=0.; }
      void reset() { globaltm = 0.; isflush = false; }

      /** return true when interval defines normal event */
      bool normal() const { return !isflush; }

      void SetInterval(double left, double right);

      int TestHitTime(const GlobalTime_t& hittime, double* dist = 0);
   };


   typedef std::vector<base::GlobalTriggerMarker> GlobalTriggerMarksQueue;


   class StreamProc {
      friend class ProcMgr;

      protected:

         typedef std::vector<base::Buffer> BuffersQueue;

         typedef std::vector<base::SyncMarker> SyncMarksQueue;

         typedef std::vector<base::LocalTriggerMarker> LocalTriggerMarksQueue;

         std::string fName;                       //! processor name, used for event naming

         ProcMgr*   fMgr;                         //!

         BuffersQueue fQueue;                     //!

         unsigned     fQueueScanIndex;            //!< index of next buffer which should be scanned
         unsigned     fQueueScanIndexTm;          //!< index of buffer to scan and set correct times of the buffer head

         bool         fRawScanOnly;               //!< indicates if only raw scan will be done, processor will not be used for any data selection

         bool            fIsSynchronisationRequired; //!< true if sync is required
         SyncMarksQueue  fSyncs;                  //!< list of sync markers
         unsigned        fSyncScanIndex;          //!< sync scan index, indicate number of syncs which can really be used for synchronization
         bool            fSyncFlag;               //!< boolean, used in sync adjustment procedure

         LocalTriggerMarksQueue fLocalTrig;       //!< list of local triggers
         double          fTriggerAcceptMaring;    //!< time margin (in local time) to accept new trigger
         GlobalTime_t    fLastLocalTriggerTm;     //!< time of last local trigger

         GlobalTriggerMarksQueue fGlobalTrig;     //!< list of global triggers in work

         unsigned fGlobalTrigScanIndex;           //!< index with first trigger which is not yet ready
         unsigned fGlobalTrigRightIndex;          //!< temporary value, used during second buffers scan

         bool fTimeSorting;                       //!< defines if time sorting should be used for the messages

         std::string   fPrefix;                   //!< prefix, used for histogram names
         std::string   fSubPrefixD;               //!< sub-prefix for histogram directory
         std::string   fSubPrefixN;               //!< sub-prefix for histogram names

         base::H1handle fTriggerTm;  //! histogram with time relative to the trigger
         base::H1handle fMultipl;    //! histogram of event multiplicity

         base::C1handle fTriggerWindow;   //!<  window used for data selection

         /** Make constructor protected - no way to create base class instance */
         StreamProc(const char* name = "", int indx = -1);

         /** Set subprefix for histograms and conditions */
         void SetSubPrefix(const char* subname = "", int indx = -1, const char* subname2 = "", int indx2 = -1);

         H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = 0);
         void FillH1(H1handle h1, double x, double weight = 1.);

         H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = 0);
         void FillH2(H1handle h2, double x, double y, double weight = 1.);

         C1handle MakeC1(const char* name, double left, double right, H1handle h1 = 0);
         void ChangeC1(C1handle c1, double left, double right);
         int TestC1(C1handle c1, double value, double* dist = 0);
         double GetC1Limit(C1handle c1, bool isleft = true);


         /** Method set raw-scan only mode for processor
          * Processor will not be used for any data selection */
         void SetRawScanOnly(bool on = true) { fRawScanOnly = on; }

         /** Method indicate if any kind of time-synchronization technique
          * should be applied for the processor.
          * If true, sync messages must be produced by processor and will be used.
          * If false, local time stamps could be immediately used (with any necessary conversion) */
         void SetSynchronisationRequired(bool on = true) { fIsSynchronisationRequired = on; }

         void AddSyncMarker(SyncMarker& marker);

         /** Add new local trigger.
           * Method first proves that new trigger marker stays in time order
           *  and have minimal distance to previous trigger */
         bool AddTriggerMarker(LocalTriggerMarker& marker, double tm_range = 0.);

         /** TODO: one should account time wrap for local stamps */
         double local_time_dist(GlobalTime_t tm1, GlobalTime_t tm2) { return tm2-tm1; }

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

         template<class EventClass, class MessageClass>
         void AddMessage(unsigned indx, EventClass* ev, const MessageClass& msg)
         {
            if (ev==0) {
               ev = new EventClass;
               fGlobalTrig[indx].subev = ev;
            }
            ev->AddMsg(msg);
         }

         /** Removes sync at specified position */
         bool eraseSyncAt(unsigned indx);

         /** Remove specified number of syncs */
         bool eraseFirstSyncs(unsigned sync_num);

      public:

         virtual ~StreamProc();

         //  void AssignMgr(ProcMgr* m) { fMgr = m; }
         ProcMgr* mgr() const { return fMgr; }

         const std::string GetProcName() const { return fName; }

         /** Enable/disable time sorting of data in output event */
         void SetTimeSorting(bool on) { fTimeSorting = on; }
         bool IsTimeSorting() const { return fTimeSorting; }

         /** Set minimal distance between two triggers */
         void SetTriggerMargin(double margin = 0.) { fTriggerAcceptMaring = margin; }

         /** Set window relative to some reference signal, which will be used as
          * region-of-interest interval to select messages from the stream */
         virtual void SetTriggerWindow(double left, double right)
         { ChangeC1(fTriggerWindow, left, right); }


         bool IsRawScanOnly() const { return fRawScanOnly; }

         /** Method indicate if any kind of time-synchronization technique
          * should be applied for the processor.
          * If true, sync messages must be produced by processor and will be used.
          * If false, local time stamps could be immediately used (with any necessary conversion) */
         bool IsSynchronisationRequired() const { return fIsSynchronisationRequired; }

         /** Provide next port of data to the processor */
         virtual bool AddNextBuffer(const Buffer& buf);

         /** Scanning all new buffers in the queue */
         virtual bool ScanNewBuffers();

         /** With new calibration set (where possible) time of buffers */
         virtual bool ScanNewBuffersTm();

         /** Method to remove all buffers, all triggers and so on */
         virtual void SkipAllData();

         /** Force processor to skip buffers from input */
         virtual bool SkipBuffers(unsigned cnt);

         /** Returns total number of sync markers */
         unsigned numSyncs() const { return fSyncs.size(); }
         unsigned numReadySyncs() const { return fSyncScanIndex; }
         SyncMarker& getSync(unsigned n) { return fSyncs[n]; }

         /** Method to deliver detected triggers from processor to central manager */
         virtual bool CollectTriggers(GlobalTriggerMarksQueue& queue);

         /** This is method to get back identified triggers from central manager */
         virtual bool DistributeTriggers(const GlobalTriggerMarksQueue& queue);

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
         virtual bool FirstBufferScan(const base::Buffer& buf) { return false; }

         /** Second generic scan of buffer
          * Here selection of data for region-of-interest should be performed */
         virtual bool SecondBufferScan(const base::Buffer& buf) { return false; }
   };

}

#endif
