#include "base/ProcMgr.h"

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "base/StreamProc.h"
#include "base/EventProc.h"

base::ProcMgr* base::ProcMgr::fInstance = 0;

base::ProcMgr::ProcMgr() :
   fProc(),
   fMap(),
   fEvProc(),
   fTriggers(10000),                // FIXME: size should be configurable
   fTimeMasterIndex(DummyIndex),
   fAnalysisKind(kind_Stream),
   fTree(0),
   fDfltHistLevel(0),
   fDfltStoreKind(0),
   fTrigEvent(0)
{
   if (fInstance==0) fInstance = this;
}

base::ProcMgr::~ProcMgr()
{
   DeleteAllProcessors();
   // printf("Delete processors done\n");

   if (fInstance == this) fInstance = 0;
}

base::ProcMgr* base::ProcMgr::instance()
{
   return fInstance;
}

void base::ProcMgr::ClearInstancePointer()
{
   fInstance = 0;
}

base::StreamProc* base::ProcMgr::FindProc(const char* name) const
{
   for (unsigned n=0;n<fProc.size();n++) {
      // printf("Delete processor %u %p\n", n, fProc[n]);
      if (strcmp(name, fProc[n]->GetName())==0) return fProc[n];
   }
   return 0;
}

void base::ProcMgr::DeleteAllProcessors()
{
   for (unsigned n=0;n<fProc.size();n++) {
      // printf("Delete processor %u %p\n", n, fProc[n]);
      delete fProc[n];
      fProc[n] = 0;
   }

   fProc.clear();
}

void base::ProcMgr::SetHistFilling(int lvl)
{
   fDfltHistLevel = lvl;
   for (unsigned n=0;n<fProc.size();n++)
      fProc[n]->SetHistFilling(lvl);
   for (unsigned n=0;n<fEvProc.size();n++)
      fEvProc[n]->SetHistFilling(lvl);
}

void base::ProcMgr::SetStoreKind(unsigned kind)
{
   fDfltStoreKind = kind;
   for (unsigned n=0;n<fProc.size();n++)
      fProc[n]->SetStoreKind(kind);
   for (unsigned n=0;n<fEvProc.size();n++)
      fEvProc[n]->SetStoreKind(kind);
}

base::H1handle base::ProcMgr::MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle)
{
   if (!InternalHistFormat()) return 0;

   double* arr = new double[nbins+5];
   arr[0] = nbins;
   arr[1] = left;
   arr[2] = right;
   for (int n=0;n<nbins+2;n++) arr[n+3] = 0.;
   return arr;
}

void base::ProcMgr::FillH1(H1handle h1, double x, double weight)
{
   // put code here, but it should be already performed in processor
   if (!InternalHistFormat() || !h1) return;

   double* arr = (double*) h1;
   int nbin = (int) arr[0];
   int bin = (int) (nbin * (x - arr[1]) / (arr[2] - arr[1]));
   if (bin<0) arr[3]+=weight; else
   if (bin>=nbin) arr[4+nbin]+=weight; else arr[4+bin]+=weight;
}

double base::ProcMgr::GetH1Content(H1handle h1, int bin)
{
   // put code here, but it should be called already performed in processor
   if (!InternalHistFormat() || !h1) return 0.;

   double* arr = (double*) h1;
   int nbin = (int) arr[0];
   if (bin<0) return arr[3];
   if (bin>=nbin) return arr[4+nbin];
   return arr[4+bin];
}

void base::ProcMgr::ClearH1(base::H1handle h1)
{
   // put code here, but it should be called already performed in processor

   if (!InternalHistFormat() || !h1) return;

   double* arr = (double*) h1;
   for (int n=0;n<arr[0]+2;n++) arr[n+3] = 0.;
}


base::H2handle base::ProcMgr::MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options)
{
   if (!InternalHistFormat()) return 0;

   double* bins = new double[(nbins1+2)*(nbins2+2)+6];
   bins[0] = nbins1;
   bins[1] = left1;
   bins[2] = right1;
   bins[3] = nbins2;
   bins[4] = left2;
   bins[5] = right2;
   for (int n=0;n<nbins1*nbins2;n++) bins[n+6] = 0.;

   return (base::H2handle) bins;
}

void base::ProcMgr::FillH2(H1handle h2, double x, double y, double weight)
{
   if (!h2 || !InternalHistFormat()) return;
   double* arr = (double*) h2;

   int nbin1 = (int) arr[0];
   int nbin2 = (int) arr[3];

   int bin1 = (int) (nbin1 * (x - arr[1]) / (arr[2] - arr[1]));
   int bin2 = (int) (nbin2 * (y - arr[4]) / (arr[5] - arr[4]));

   if (bin1<0) bin1 = -1; else if (bin1>nbin1) bin1 = nbin1;
   if (bin2<0) bin2 = -1; else if (bin2>nbin2) bin2 = nbin2;

   arr[6 + (bin1+1) + (bin2+1)*(nbin1+2)]+=weight;
}

void base::ProcMgr::ClearH2(base::H2handle h2)
{
   if (!h2 || !InternalHistFormat()) return;
   double* arr = (double*) h2;

   int nbin1 = (int) arr[0];
   int nbin2 = (int) arr[3];
   for (int n=0;n<(nbin1+2)*(nbin2+2);n++) arr[6+n] = 0.;
}

base::C1handle base::ProcMgr::MakeC1(const char* name, double left, double right, base::H1handle h1)
{
   // put dummy virtual function here to avoid ACLiC warnings
   return 0;
}

void base::ProcMgr::ChangeC1(C1handle c1, double left, double right)
{
   // put dummy virtual function here to avoid ACLiC warnings
}

int base::ProcMgr::TestC1(C1handle c1, double value, double* dist)
{
   // put dummy virtual function here to avoid ACLiC warnings
   return 0;
}

double base::ProcMgr::GetC1Limit(C1handle c1, bool isleft)
{
   // put dummy virtual function here to avoid ACLiC warnings
   return 0;
}

void base::ProcMgr::UserPreLoop(Processor* only_proc, bool call_when_running)
{
   for (unsigned n=0;n<fProc.size();n++) {
      if (fProc[n]==0) continue;
      if ((only_proc!=0) && (fProc[n]!=only_proc)) continue;

      if (fProc[n]->fAnalysisKind != kind_RawOnly)
         fProc[n]->fAnalysisKind = fAnalysisKind;

      if (fTree && fProc[n]->IsStoreEnabled())
         fProc[n]->CreateBranch(fTree);

      if (!IsStreamAnalysis())
         fProc[n]->SetSynchronisationKind(base::StreamProc::sync_None);

      if (!call_when_running)
         fProc[n]->UserPreLoop();
   }

   for (unsigned n=0;n<fEvProc.size();n++) {
      if (fEvProc[n]==0) continue;
      if ((only_proc!=0) && (fEvProc[n]!=only_proc)) continue;

      if (fTree && fEvProc[n]->IsStoreEnabled())
         fEvProc[n]->CreateBranch(fTree);

      if (!call_when_running)
         fEvProc[n]->UserPreLoop();
   }
}

void base::ProcMgr::UserPostLoop(Processor* only_proc)
{
   for (unsigned n=0;n<fProc.size();n++) {
      if ((only_proc!=0) && (fProc[n]!=only_proc)) continue;
      if (fProc[n]) fProc[n]->UserPostLoop();
   }

   for (unsigned n=0;n<fEvProc.size();n++) {
      if ((only_proc!=0) && (fEvProc[n]!=only_proc)) continue;
      if (fEvProc[n]) fEvProc[n]->UserPostLoop();
   }

   // close store file already here
   if (only_proc==0) CloseStore();
}

base::ProcMgr* base::ProcMgr::AddProcessor(Processor* proc)
{
   StreamProc* sproc = dynamic_cast<StreamProc*> (proc);
   EventProc* eproc = dynamic_cast<EventProc*> (proc);
   if (proc && (proc->mgr()!=this)) proc->SetManager(this);
   if (sproc) fProc.push_back(sproc);
   if (eproc) fEvProc.push_back(eproc);
   return this;
}

base::ProcMgr* base::ProcMgr::AddProc(Processor* proc)
{
   return fInstance==0 ? 0 : fInstance->AddProcessor(proc);
}

bool base::ProcMgr::RegisterProc(StreamProc* proc, unsigned kind, unsigned brdid)
{
   if (proc==0) return false;

   bool find = false;
   for (unsigned n=0;n<fProc.size();n++)
      if (fProc[n] == proc) find = true;

   if (!find) return false;

   if (brdid>=MaxBrdId) {
      printf("Board id %u is too high - failure\n", brdid);
      exit(6);
   }

   unsigned index = kind * MaxBrdId + brdid;

   fMap[index] = proc;

   return true;
}


void base::ProcMgr::SetTimeSorting(bool on)
{
   for (unsigned n=0;n<fProc.size();n++)
      fProc[n]->SetTimeSorting(on);
}


void base::ProcMgr::ProvideRawData(const Buffer& buf)
{
   if (buf.null()) return;

   if (buf().boardid >= MaxBrdId) {
      printf("Board id %u is too high - failure\n", buf().boardid);
      exit(6);
   }

   unsigned index = buf().kind * MaxBrdId + buf().boardid;

   StreamProcMap::iterator it = fMap.find(index);

   if (it == fMap.end()) return;

   // printf("Provide new data kind %d  board %u\n", buf().kind, buf().boardid);

   it->second->AddNextBuffer(buf);
}

int base::ProcMgr::SyncIdDiff(unsigned id1, unsigned id2) const
{
   if (id1==id2) return 0;

   int res = 0;
   int range = SyncIdRange();

   if (id1 < id2) {
      res = id2 - id1;
      if (res > range/2) res -= range;
   } else {
      res = id1 - id2;
      if (res > range/2) res = range - res;
                    else res = -res;
   }

   return res;
}



bool base::ProcMgr::AnalyzeSyncMarkers()
{
   // TODO: configure which processor is time master
   // TODO: work with unsynchronized SYNC messages - not always the same id in the front
   // TODO: process not only last sync message

   // in raw analysis we should not call this function
   if (IsRawAnalysis()) return false;

   // in triggered analysis synchronization already done by trigger
   if (IsTriggeredAnalysis()) return true;

   bool isenough = true;

   if (fProc.size()==0) return false;

   // in the beginning decide who is time master
   if (fTimeMasterIndex == DummyIndex) {
      unsigned first = NoSyncIndex;

      for (unsigned n=0;n<fProc.size();n++) {
         if (!fProc[n]->IsSynchronisationRequired()) continue;

         if (first == NoSyncIndex) first = n;

         if (fProc[n]->doTriggerSelection()) {
            fTimeMasterIndex = n;
            break;
         }
      }

      if (fTimeMasterIndex == DummyIndex) fTimeMasterIndex = first;

//      printf("************ SELECT MASTER INDEX %u ******* \n", fTimeMasterIndex);
   }


   StreamProc* master = 0;

   // if no synchronization at all, return
   if (fTimeMasterIndex == NoSyncIndex) {
      // printf("Ignore sync scanning completely\n");
      goto skip_sync_scanning;
   }

   // we require at least 2 syncs on each stream
   for (unsigned n=0;n<fProc.size();n++) {

      if (fProc[n]->numSyncs() < fProc[n]->minNumSyncRequired()) {
          printf("No enough %u syncs on processor %s!!!\n", fProc[n]->numSyncs(), fProc[n]->GetName());
          // exit(5);
          isenough = false;
      }

   }

   if (!isenough) return false;


   master = fProc[fTimeMasterIndex];

   // if all markers in the master are validated, nothing to do
   while (master->fSyncScanIndex < master->numSyncs()) {

      SyncMarker& master_marker = master->getSync(master->fSyncScanIndex);

      bool is_curr_sync_ok = true;

      for (unsigned n=0;n<fProc.size();n++) {
         // do not analyze sync-master itself
         if (n==fTimeMasterIndex) continue;

         StreamProc* slave = fProc[n];

         slave->fSyncFlag = false;

         if (!slave->IsSynchronisationRequired()) continue;

         bool is_slave_ok = false;

         while (slave->fSyncScanIndex < slave->numSyncs()) {

            SyncMarker& slave_marker = slave->getSync(slave->fSyncScanIndex);

            int diff = SyncIdDiff(master_marker.uniqueid, slave_marker.uniqueid);

            // find same sync as master - very nice
            if (diff==0) {
               slave_marker.globaltm = master_marker.localtm;
               is_slave_ok = true;
               slave->fSyncFlag = true; // indicate that this slave has same sync
               break;
            }

            // master sync is bigger, slave sync must be ignored
            if (diff<0) {
               // we even remove it while no any reasonable stamp can be assigned to it
               printf("Erase SYNC %u in processor %s\n", slave_marker.uniqueid, slave->GetName());
               slave->eraseSyncAt(slave->fSyncScanIndex);
               continue;
            }

            // slave sync id is bigger, stop analyzing, but could do calibration
            if (diff>0) {
               // printf("Find hole in SYNC sequences in processor %u\n", n);
               is_slave_ok = true;
               break;
            }
         }

         if (!is_slave_ok) is_curr_sync_ok = false;
      }

      // if we find that on all other processors master sync is accepted,
      // we can declare master sync ready and shift all correspondent indexes
      if (!is_curr_sync_ok) break;

      master_marker.globaltm = master_marker.localtm;
      master->fSyncFlag = true;

      // shift scan index on the processors with the same id as master have
      for (unsigned n=0;n<fProc.size();n++)
         if (fProc[n]->fSyncFlag)
            fProc[n]->fSyncScanIndex++;
   }


skip_sync_scanning:

   // we require at least two valid syncs on each stream for interpolation
   for (unsigned n=0;n<fProc.size();n++) {

      // one can simply ignore optional streams here

      // every processor need at least two valid syncs for time calibrations
      if (fProc[n]->numReadySyncs() < fProc[n]->minNumSyncRequired()) return false;

      // let also assign global times for the buffers here
      fProc[n]->ScanNewBuffersTm();
   }


   return true;
}

bool base::ProcMgr::CollectNewTriggers()
{
   // central place where triggers should be produced
   // in addition, we should perform flushing of data
   // therefore if triggers are not produced for long time,
   // one should create special "flush" trigger which force
   // flushing of the data

   if (IsRawAnalysis()) return false;

   // first collect triggers from the processors
   for (unsigned n=0;n<fProc.size();n++)
      fProc[n]->CollectTriggers(fTriggers);

//   printf("CollectNewTriggers\n");

   if (IsTriggeredAnalysis()) {

      // be sure to have at least one trigger in the list
      if (fTriggers.size()==0)
         fTriggers.push(GlobalMarker(0.));
   } else
   // create flush event when master has already two buffers and
   // time is reached by all sub-systems
   if (fProc.size() > 0) {

      unsigned use_indx = fTimeMasterIndex < fProc.size() ? fTimeMasterIndex : 0;

      // if we request flush time, it should be bigger than last trigger marker
      GlobalTime_t flush_time =
            fProc[use_indx]->ProvidePotentialFlushTime(fTriggers.size() > 0 ? fTriggers.back().globaltm : 0.);

//      printf("Try flush time %12.9f\n", flush_time);

      // now verify that each processor is accept such flushtime
      // important that every component obtained and analyzed this region,
      // otherwise it could happen that normal new trigger appears after such flush trigger
      if (flush_time != 0.)
         for (unsigned n=0;n<fProc.size();n++)
            if (!fProc[n]->VerifyFlushTime(flush_time)) { flush_time = 0.; break; }

      //printf("after verify %12.9f\n", flush_time*1e-9);

//      flush_time = 0.;

      if (flush_time != 0.) {
//         printf("FLUSH: %12.9f\n", flush_time);
         fTriggers.push(GlobalMarker(flush_time));
         fTriggers.back().isflush = true;
      }
   }

//   printf("Now we have %u triggers\n", fTriggers.size());

   // and redistribute back global triggers list
   for (unsigned n=0;n<fProc.size();n++)
      fProc[n]->DistributeTriggers(fTriggers);

   return true;
}


bool base::ProcMgr::ScanDataForNewTriggers()
{
   // here we want that each processor scan its data again for new triggers
   // which we already distribute to each processor. In fact, this could run in
   // individual thread of each processor

   for (unsigned n=0;n<fProc.size();n++)
      fProc[n]->ScanDataForNewTriggers();

   return true;
}

bool base::ProcMgr::AnalyzeNewData(base::Event* &evt)
{
   // scan raw data
   // if triggered analysis configured, fill event at the same time

   if (IsTriggeredAnalysis()) {
      if (evt==0)
         evt = new base::Event;
      else
         evt->DestroyEvents();
      evt->SetTriggerTime(0.);
      fTrigEvent = evt;
   }

   // scan new data in the processors
   for (unsigned n=0;n<fProc.size();n++)
      fProc[n]->ScanNewBuffers();

   if (IsRawAnalysis()) return false;

   if (IsTriggeredAnalysis()) {
      fTrigEvent = 0;
      return true;
   }

   // analyze new sync markers
   if (AnalyzeSyncMarkers()) {
      // get and redistribute new triggers
      CollectNewTriggers();
      // scan for new triggers
      ScanDataForNewTriggers();
   }

   return ProduceNextEvent(evt);
}

bool base::ProcMgr::AddToTrigEvent(const std::string& name, base::SubEvent* sub)
{
   // method used to add data, extracted with first scan, to the special triggered event
   // if subevent not accepted, it will be deleted

   if (fTrigEvent==0) { delete sub; return false; }

   fTrigEvent->AddSubEvent(name, sub);

   return true;
}



bool base::ProcMgr::ProduceNextEvent(base::Event* &evt)
{
   // at this moment each processor should finish with buffers scanning
   // for special cases (like MBS or EPICS) processor itself should declare that
   // triggers in between are correctly filled

   if (!IsStreamAnalysis()) return false;

   unsigned numready = fTriggers.size();

   for (unsigned n=0;n<fProc.size();n++) {
      if (fProc[n]->IsRawAnalysis()) continue;
      unsigned local = fProc[n]->NumReadySubevents();
      if (local<numready) numready = local;
   }

   // printf("Try to produce data for %u triggers numready %u\n", fTriggers.size(), numready);

   while (numready > 0) {

      //   printf("Total event %u ready %u  next trigger %6.3f\n", fTriggers.size(), numready,
      //         fTriggers.size() > 0 ? fTriggers.front().globaltm*1e-9 : 0.);

      if (fTriggers.front().isflush) {
         // printf("Remove flush event %12.9f\n", fTriggers.front().globaltm*1e-9);
         for (unsigned n=0;n<fProc.size();n++)
            fProc[n]->AppendSubevent(0);
         fTriggers.pop();
         numready--;
         continue;
      }

      if (evt==0)
         evt = new base::Event;
      else
         evt->DestroyEvents();

      evt->SetTriggerTime(fTriggers.front().globaltm);

      // here all subevents from first event are collected
      // at the same time event should be removed from all local/global lists
      for (unsigned n=0;n<fProc.size();n++)
         fProc[n]->AppendSubevent(evt);
      fTriggers.pop();

      return true;
   }

   return false;
}

bool base::ProcMgr::ProcessEvent(base::Event* evt)
{
   // printf("base::ProcMgr::ProcessEvent %p\n", evt);

   if (evt==0) return false;

   // call event processors one after another until event is discarded
   for (unsigned n=0;n<fEvProc.size();n++)
      if (!fEvProc[n]->Process(evt)) return false;

   for (unsigned n=0;n<fProc.size();n++)
      if (fProc[n]->IsStoreEnabled())
         fProc[n]->Store(evt);

   StoreEvent();

   return true;
}

