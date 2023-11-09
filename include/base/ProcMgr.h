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

   class Processor;
   class StreamProc;
   class EventProc;
   class EventStore;

   /** \brief Central data and process manager
    *
    * \ingroup stream_core_classes
    *
    * Class base::ProcMgr is central manager of processors and interface
    * to any external frameworks like ROOT or Go4 or ...
    * It is singleton - the only instance for whole system */

   class ProcMgr {

      friend class Processor;

      protected:

         enum { MaxBrdId = 256 };

         enum {
            NoSyncIndex = 0xfffffffe,
            DummyIndex  = 0xffffffff
         };

         /** map of stream processors */
         typedef std::map<unsigned,StreamProc*> StreamProcMap;

         struct HistBinning {
            int nbins{0};
            double left{0.}, right{0};
            HistBinning() = default;
            HistBinning(int _nbins, double _left, double _right) : nbins(_nbins), left(_left), right(_right) {}
            HistBinning& operator=(const HistBinning& src) {
               nbins = src.nbins;
               left = src.left;
               right = src.right;
               return *this;
            }
         };

         std::string              fSecondName;         ///<! name of second.C script
         std::vector<StreamProc*> fProc;               ///<! all stream processors
         StreamProcMap            fMap;                ///<! map for fast access
         std::vector<EventProc*>  fEvProc;             ///<! all event processors
         GlobalMarksQueue         fTriggers;           ///<!< list of current triggers
         unsigned                 fTimeMasterIndex;    ///<! processor index, which time is used for all other subsystems
         AnalysisKind             fAnalysisKind;       ///<! ignore all events, only single scan, not output events
         TTree                   *fTree{nullptr};      ///<! abstract tree pointer, will be used in ROOT implementation
         int                      fDfltHistLevel{0};   ///<! default histogram fill level for any new created processor
         int                      fDfltStoreKind{0};   ///<! default store kind for any new created processor
         base::Event             *fTrigEvent{nullptr}; ///<! current event, filled when performing triggered analysis
         int                      fDebug{0};           ///<! debug level
         bool                     fBlockHistCreation{false}; ///<! if true no new histogram should be created
         std::map<std::string,HistBinning> fCustomBinning; ///<! custom binning

         static ProcMgr* fInstance;                     ///<! instance

         /** range for sync messages */
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

         static void ClearInstancePointer(ProcMgr *mgr = nullptr);

         ProcMgr* AddProcessor(Processor* proc);

         static ProcMgr* AddProc(Processor* proc);

         /** Set name of second macro, which executed after first.C. Normally it is second.C */
         void SetSecondName(const std::string &name = "second.C") { fSecondName = name; }

         /** Clear name of second macro, will not be executed at all */
         void ClearSecondName() { SetSecondName(""); }

         /** Returns number of second macro */
         const std::string &GetSecondName() const { return fSecondName; }

         /** Enter processor for processing data of specified kind */
         bool RegisterProc(StreamProc* proc, unsigned kind, unsigned brdid);

         /** Get number of registered processors */
         unsigned NumProc() const { return fProc.size(); }

         /** Get processor by sequence number */
         StreamProc* GetProc(unsigned n) const { return n < NumProc() ? fProc[n] : nullptr; }

         /** Find processor by name */
         StreamProc* FindProc(const char* name) const;

         void SetHistFilling(int lvl);

         /** Set debug level */
         void SetDebug(int lvl = 0) { fDebug = lvl; }
         /** Returns debug level */
         int GetDebug() const { return fDebug; }

         void SetBlockHistCreation(bool on = true) { fBlockHistCreation = on; }
         bool IsBlockHistCreation() const { return fBlockHistCreation; }

         /** Set store kind for all processors */
         virtual void SetStoreKind(unsigned kind = 1);

         /** When returns true, indicates that simple histogram format is used */
         virtual bool InternalHistFormat() const { return true; }

         /** Add run log */
         virtual void AddRunLog(const char * /* msg */) {}
         /** Add error log */
         virtual void AddErrLog(const char * /* msg */) {}
         /** Returns true if logging is enabled */
         virtual bool DoLog() { return false; }
         virtual void PrintLog(const char *msg);

         /** Set if histograms folders should be created in sorted alphabetical order, default false */
         virtual void SetSortedOrder(bool = true) {}
         /** Returns true if histograms folders created in sorted alphabetical order */
         virtual bool IsSortedOrder() { return false; }

         virtual H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = nullptr);
         virtual bool GetH1NBins(H1handle h1, int &nbins);
         virtual void FillH1(H1handle h1, double x, double weight = 1.);
         virtual double GetH1Content(H1handle h1, int bin);
         virtual void SetH1Content(H1handle h1, int bin, double v = 0.);
         virtual void ClearH1(H1handle h1);
         virtual void CopyH1(H1handle tgt, H1handle src);
         /** Set histogram title */
         virtual void SetH1Title(H1handle, const char*) {}
         /** Tag histogram time */
         virtual void TagH1Time(H1handle) {}

         virtual H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = nullptr);
         virtual bool GetH2NBins(H2handle h2, int &nbins1, int &nbins2);
         virtual void FillH2(H2handle h2, double x, double y, double weight = 1.);
         virtual double GetH2Content(H2handle h2, int bin1, int bin2);
         virtual void SetH2Content(H2handle h2, int bin1, int bin2, double v = 0.);
         virtual void ClearH2(H2handle h2);
         /** Set histogram title */
         virtual void SetH2Title(H2handle, const char*) {}
         /** Tag histogram time */
         virtual void TagH2Time(H2handle) {}

         void SetCustomBinning(const char *name, int nbins, double left, double right)
         {
            fCustomBinning[name] = HistBinning(nbins, left, right);
         }

         /** Clear all histograms */
         virtual void ClearAllHistograms() {}

         virtual C1handle MakeC1(const char* name, double left, double right, base::H1handle h1 = nullptr);
         virtual void ChangeC1(C1handle c1, double left, double right);
         virtual int TestC1(C1handle c1, double value, double *dist = nullptr);
         virtual double GetC1Limit(C1handle c1, bool isleft = true);

         /** create data store, for the moment - ROOT tree */
         virtual bool CreateStore(const char* /* storename */) { return false; }
         /** Close store */
         virtual bool CloseStore() { return false; }
         /** Create branch */
         virtual bool CreateBranch(const char* /* name */, const char* /* class_name */, void** /* obj */) { return false; }
         /** Create branch */
         virtual bool CreateBranch(const char* /* name */, void* /* member */, const char* /* kind */) { return false; }
         /** Store event */
         virtual bool StoreEvent() { return false; }

         /** method to register ROOT objects, object should be derived from TObject class
          * if returns true, object is registered and will be owned by framework */
         virtual bool RegisterObject(TObject* /* tobj */, const char* /* subfolder */ = nullptr) { return false; }


         /** method to call function by name */
         virtual bool CallFunc(const char* /* funcname */, void* /* arg */) { return false; }

         // this is list of generic methods for common data processing

         /** Returns true if raw analysis is configured */
         bool IsRawAnalysis() const { return fAnalysisKind == kind_Raw; }

         /** Enable/disable raw analysis. If on - only data scan is performed, no any output events with second scan can be produced. No second.C will be involved */
         void SetRawAnalysis(bool on = true) { fAnalysisKind = on ? kind_Raw : kind_Stream; }

         /** Returns true if triggered analysis is configured */
         bool IsTriggeredAnalysis() const { return fAnalysisKind == kind_Triggered; }

         /** Enabled/disable triggered analysis is configured. If on - analyzed data will be processed as triggered, no time correlation is performed  */
         void SetTriggeredAnalysis(bool on = true) { fAnalysisKind = on ? kind_Triggered : kind_Stream; }

         /** Returns true if full timed stream analysis is configured */
         bool IsStreamAnalysis() const { return fAnalysisKind == kind_Stream; }

         /** Set sorting flag for all registered processors */
         void SetTimeSorting(bool on);

         /** Specify processor index, which is used as time reference for all others */
         void SetTimeMasterIndex(unsigned indx) { fTimeMasterIndex = indx; }

         void ProvideRawData(const Buffer& buf, bool fast_process = false);

         bool AnalyzeSyncMarkers();

         bool CollectNewTriggers();

         bool ScanDataForNewTriggers();

         bool AnalyzeNewData(base::Event* &evt);

         /** Returns true if trigger even exists */
         bool HasTrigEvent() const { return fTrigEvent != nullptr; }

         bool AddToTrigEvent(const std::string& name, base::SubEvent* sub);

         bool ProduceNextEvent(base::Event* &evt);

         virtual bool ProcessEvent(base::Event* evt);

         void UserPreLoop(Processor* only_proc = nullptr, bool call_when_running = false);

         void UserPostLoop(Processor* only_proc = nullptr);
   };
}

#endif
