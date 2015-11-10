#ifndef BASE_PROGMGR_H
#define BASE_PROGMGR_H

#include <vector>
#include <map>

#include "base/defines.h"
#include "base/Buffer.h"
#include "base/Markers.h"
#include "base/Event.h"

class TTree;
class TObject;

namespace base {

   /** Class base::ProcMgr is central manager of processors and interface
    * to any external frameworks like ROOT or Go4 or ...
    * It is singleton - the only instance for whole system  */

   class Processor;
   class StreamProc;
   class EventProc;
   class EventStore;

   class ProcMgr {

      friend class Processor;

      protected:

         enum { MaxBrdId = 256 };

         enum {
            NoSyncIndex = 0xfffffffe,
            DummyIndex  = 0xffffffff
         };

         typedef std::map<unsigned,StreamProc*> StreamProcMap;

         std::vector<StreamProc*> fProc;            //! all stream processors
         StreamProcMap            fMap;             //! map for fast access
         std::vector<EventProc*>  fEvProc;          //! all event processors
         GlobalMarksQueue         fTriggers;        //!< list of current triggers
         unsigned                 fTimeMasterIndex; //!< processor index, which time is used for all other subsystems
         AnalysisKind             fAnalysisKind;    //!< ignore all events, only single scan, not output events
         TTree                   *fTree;            //!< abstract tree pointer, will be used in ROOT implementation
         int                      fDfltHistLevel;   //!< default histogram fill level for any new created processor
         base::Event             *fTrigEvent;       //!< current event, filled when performing triggered analysis

         static ProcMgr* fInstance;                 //!

         virtual unsigned SyncIdRange() const { return 0x1000000; }

         /** Method calculated difference id2-id1,
           * used for sync markers identification
           * Sync ID overflow is taken into account */
         int SyncIdDiff(unsigned id1, unsigned id2) const;

         void DeleteAllProcessors();

      public:
         ProcMgr();
         virtual ~ProcMgr();

         static ProcMgr* instance();

         static void ClearInstancePointer();

         ProcMgr* AddProcessor(Processor* proc);

         static ProcMgr* AddProc(Processor* proc);

         /** Enter processor for processing data of specified kind */
         bool RegisterProc(StreamProc* proc, unsigned kind, unsigned brdid);

         unsigned NumProc() const { return fProc.size(); }
         StreamProc* GetProc(unsigned n) const { return n<NumProc() ? fProc[n] : 0; }
         StreamProc* FindProc(const char* name) const;

         void SetHistFilling(int lvl);

         /** When returns true, indicates that simple histogram format is used */
         virtual bool InternalHistFormat() const { return true; }

         virtual H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = 0);
         virtual void FillH1(H1handle h1, double x, double weight = 1.);
         virtual double GetH1Content(H1handle h1, int nbin);
         virtual void ClearH1(base::H1handle h1);

         virtual H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = 0);
         virtual void FillH2(H1handle h2, double x, double y, double weight = 1.);
         virtual void ClearH2(base::H2handle h2);

         virtual C1handle MakeC1(const char* name, double left, double right, base::H1handle h1 = 0);
         virtual void ChangeC1(C1handle c1, double left, double right);
         /** Condition check 0 - inside, -1 left , +1 - right
          * If variable dist specified, will contain distance to left (-1) or right (+1) boundary   */
         virtual int TestC1(C1handle c1, double value, double* dist = 0);
         virtual double GetC1Limit(C1handle c1, bool isleft = true);

         // create data store, for the moment - ROOT tree
         virtual bool CreateStore(const char* storename) { return false; }
         virtual bool CloseStore() { return false; }
         virtual bool CreateBranch(TTree* t, const char* name, const char* class_name, void** obj) { return false; }
         virtual bool CreateBranch(TTree* t, const char* name, void* member, const char* kind) { return false; }

         // method to register ROOT objects, object should be derived from TObject class
         // if returns true, object is registered and will be owned by framework
         virtual bool RegisterObject(TObject* tobj, const char* subfolder = 0) { return false; }


         // method to call function by name
         virtual bool CallFunc(const char* funcname, void* arg) { return false; }

         // this is list of generic methods for common data processing

         bool IsRawAnalysis() const { return fAnalysisKind == kind_RawOnly; }
         void SetRawAnalysis(bool on = true) { fAnalysisKind = on ? kind_RawOnly : kind_Stream; }

         bool IsTriggeredAnalysis() const { return fAnalysisKind == kind_Triggered; }
         void SetTriggeredAnalysis(bool on = true) { fAnalysisKind = on ? kind_Triggered : kind_Stream; }

         bool IsStreamAnalysis() const { return fAnalysisKind == kind_Stream; }

         /** Set sorting flag for all registered processors */
         void SetTimeSorting(bool on);

         /** Specify processor index, which is used as time reference for all others */
         void SetTimeMasterIndex(unsigned indx) { fTimeMasterIndex = indx; }

         /** Method to provide raw data on base of data kind to the processor */
         void ProvideRawData(const Buffer& buf);


         /** Check current sync markers */
         bool AnalyzeSyncMarkers();

         /** Method to collect triggers */
         bool CollectNewTriggers();

         /** Method to produce data for new triggers */
         bool ScanDataForNewTriggers();

         /** Analyze new data, if triggered analysis configured - immediately produce new event */
         bool AnalyzeNewData(base::Event* &evt);

         bool AddToTrigEvent(const std::string& name, base::SubEvent* sub);

         /** Very central method - select if possible data for next event
          * Only can be done that each processor is agree to deliver data within
          * trigger interval. It may not be a case when messages from future buffers may be required */
         bool ProduceNextEvent(base::Event* &evt);

         /** Process event - consequently calls all event processors */
         virtual bool ProcessEvent(base::Event* evt);

         void UserPreLoop(Processor* only_proc = 0);

         void UserPostLoop(Processor* only_proc = 0);
   };
}

#endif
