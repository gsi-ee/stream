#include "base/ProcMgr.h"

#include <stdio.h>
#include <stdlib.h>

#include "base/StreamProc.h"
#include "base/EventProc.h"

base::ProcMgr* base::ProcMgr::fInstance = 0;

base::ProcMgr::ProcMgr() :
   fProc(),
   fMap(),
   fEvProc(),
   fTriggers(10000),    // FIXME: size should be configurable
   fTimeMasterIndex(DummyIndex),
   fRawAnalysisOnly(false)
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


base::H1handle base::ProcMgr::MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle)
{
   // put dummy virtual function here to avoid ACLiC warnings
   return 0;
}

void base::ProcMgr::FillH1(H1handle h1, double x, double weight)
{
   // put dummy virtual function here to avoid ACLiC warnings
}

double base::ProcMgr::GetH1Content(H1handle h1, int nbin)
{
   // put dummy virtual function here to avoid ACLiC warnings
   return 0;
}

void base::ProcMgr::ClearH1(base::H1handle h1)
{
   // put dummy virtual function here to avoid ACLiC warnings
}

base::H2handle base::ProcMgr::MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options)
{
   // put dummy virtual function here to avoid ACLiC warnings
   return 0;
}

void base::ProcMgr::FillH2(H1handle h2, double x, double y, double weight)
{
   // put dummy virtual function here to avoid ACLiC warnings
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




void base::ProcMgr::UserPreLoop()
{
   for (unsigned n=0;n<fProc.size();n++)
      if (fProc[n]) fProc[n]->UserPreLoop();
}

void base::ProcMgr::UserPostLoop()
{
   for (unsigned n=0;n<fProc.size();n++)
      if (fProc[n]) fProc[n]->UserPostLoop();
}


base::ProcMgr* base::ProcMgr::AddProc(StreamProc* proc)
{
   if (fInstance==0) return 0;
   fInstance->fProc.push_back(proc);
   return fInstance;
}

base::ProcMgr*  base::ProcMgr::AddProc(EventProc* proc)
{
   if (fInstance==0) return 0;
   fInstance->fEvProc.push_back(proc);
   return fInstance;
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

void base::ProcMgr::ScanNewData()
{
   for (unsigned n=0;n<fProc.size();n++)
      fProc[n]->ScanNewBuffers();
}

bool base::ProcMgr::SkipAllData()
{
   for (unsigned n=0;n<fProc.size();n++)
      fProc[n]->SkipAllData();
   return true;
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

   // first collect triggers from the processors
   for (unsigned n=0;n<fProc.size();n++)
      fProc[n]->CollectTriggers(fTriggers);

//   printf("CollectNewTriggers\n");

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

   // printf("--------------- ScanDataForNewTriggers numproc %u-------------- \n", (unsigned) fProc.size());

   for (unsigned n=0;n<fProc.size();n++) {
      // printf("Scan in %u %p %s\n", n, fProc[n], fProc[n]->GetName());
      fProc[n]->ScanDataForNewTriggers();
      // printf("Did scan in %u %p %s\n", n, fProc[n], fProc[n]->GetName());
   }

   return true;
}


bool base::ProcMgr::ProduceNextEvent(base::Event* &evt)
{
   // at this moment each processor should finish with buffers scanning
   // for special cases (like MBS or EPICS) processor itself should declare that
   // triggers in between are correctly filled

   if (IsRawAnalysis()) return false;

   unsigned numready = fTriggers.size();

   for (unsigned n=0;n<fProc.size();n++) {
      if (fProc[n]->IsRawScanOnly()) continue;
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

void base::ProcMgr::ProcessEvent(base::Event* evt)
{
   if (evt==0) return;

   for (unsigned n=0;n<fEvProc.size();n++)
      fEvProc[n]->Process(evt);
}

