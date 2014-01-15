#include "base/StreamProc.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "base/ProcMgr.h"
#include "base/Event.h"

unsigned base::StreamProc::fMarksQueueCapacity = 10000;
unsigned base::StreamProc::fBufsQueueCapacity = 100;

base::StreamProc::StreamProc(const char* name, unsigned brdid, bool basehist) :
   base::Processor(name, brdid),
   fQueue(fBufsQueueCapacity),
   fQueueScanIndex(0),
   fQueueScanIndexTm(0),
   fRawScanOnly(false),
   fSynchronisationKind(sync_Inter),
   fSyncs(fMarksQueueCapacity),
   fSyncScanIndex(0),
   fLocalMarks(fMarksQueueCapacity),
   fTriggerAcceptMaring(0.),
   fLastLocalTriggerTm(0.),
   fGlobalMarks(fMarksQueueCapacity),
   fGlobalTrigScanIndex(0),
   fGlobalTrigRightIndex(0),
   fTimeSorting(false),
   fTriggerTm(0),
   fMultipl(0),
   fTriggerWindow(0)
{
   fMgr = base::ProcMgr::AddProc(this);

   if (basehist) CreateTriggerHist(40, 2500, -1e-6, 4e-6);
}


base::StreamProc::~StreamProc()
{
   fSyncs.clear();
   fQueue.clear();
   fLocalMarks.clear();
   // TODO: cleanup of event data
   fGlobalMarks.clear();
}

void base::StreamProc::CreateTriggerHist(unsigned multipl, unsigned nbins, double left, double right)
{
   SetSubPrefix();

   fTriggerTm = MakeH1("TriggerTm", "Time relative to trigger", nbins, left, right, "s");
   fMultipl = MakeH1("Multipl", "Subevent multiplicity", multipl, 0, multipl, "hits");
   fTriggerWindow = MakeC1("TrWindow", 5e-7, 10e-7, fTriggerTm);
}


base::GlobalTime_t base::StreamProc::LocalToGlobalTime(base::GlobalTime_t localtm, unsigned* indx)
{
   // method uses helper index to locate faster interval where interpolation could be done
   // For the interpolation value of index is following
   //  0                      - last value left to the first sync
   //  [1..numReadySyncs()-1] - last value between indx-1 and indx syncs
   //  numReadySyncs()        - last value right to the last sync
   // For the left/right side sync index indicates sync number used

   // TODO: one could probably use some other methods of time conversion

   if (!IsSynchronisationRequired()) return localtm;

   if (numReadySyncs() == 0) {
      printf("No any sync for time calibration\n");
      exit(7);
      return 0.;
   }

   // use liner approximation only when more than one sync available
   if ((fSynchronisationKind==sync_Inter) && (numReadySyncs()>1)) {

      // we should try to use helper index only if it is inside existing sync range
      bool try_indx = (indx!=0) && (*indx>0) && (*indx < (numReadySyncs() - 1));

      for (unsigned cnt=0; cnt<numReadySyncs(); cnt++) {
         unsigned n = 0;

         if (cnt==0) {
            // use first loop iteration to check if provided index
            // suited for interpolation,
            if (try_indx) n = *indx - 1; else continue;
         } else {
            n = cnt - 1;
            // if reference index was provided, no need to check it again
            if (try_indx && ((*indx - 1) == n)) continue;
         }

         // TODO: make time diff via LocalStampConverter
         double dist1 = localtm - getSync(n).localtm;
         double dist2 = getSync(n+1).localtm - localtm;

         if ((dist1>=0.) && (dist2>0)) {
            //double k1 = dist2 / (dist1 + dist2);
            double diff1 = dist1 / (dist1 + dist2) * (getSync(n+1).globaltm - getSync(n).globaltm);
            //return getSync(*indx).globaltm*k1 + getSync(*indx+1).globaltm*k2;

            if ((dist1>0) && ((diff1/dist1 < 0.9) || (diff1/dist1 > 1.1))) {
               printf("Something wrong with time calc %10.9g %10.9g\n", dist1, diff1);

               printf("%s converting localtm %12.9f dist1 %12.9f dist2 %12.9f\n", GetName(), localtm, dist1, dist2);

               printf("sync1_local  %14.9f  sync2_local  %14.9f  diff %12.9f\n", getSync(n).localtm, getSync(n+1).localtm, getSync(n+1).localtm - getSync(n).localtm);
               printf("sync1_global %14.9f  sync2_global %14.9f  diff %12.9f\n", getSync(n).globaltm, getSync(n+1).globaltm, getSync(n+1).globaltm - getSync(n).globaltm);

               exit(1);
            }

            if (indx) *indx = n + 1;

            //return getSync(*indx).globaltm + dist1;
            return getSync(n).globaltm + diff1;
         }
      }
   }

   // time will be calibrated with the SYNC on the left side
   if (fSynchronisationKind==sync_Left) {
      printf("fSynchronisationKind==sync_Left not yet implemented\n");
      exit(7);
   }

   // time will be calibrated with the SYNC on the right side
   if (fSynchronisationKind==sync_Right) {

      unsigned cnt = 0;
      bool use_selected = false;
      if ((indx!=0) && (*indx < numReadySyncs())) { cnt = *indx; use_selected = true; }

      while (cnt < numReadySyncs()) {

         // TODO: make time diff via LocalStampConverter
         double dist1 = getSync(cnt).localtm - localtm;

         // be sure that marker on right side
         if (dist1>=0) {
            if ((cnt==0) || ((localtm - getSync(cnt-1).localtm) > 0)) {
               if (indx) *indx = cnt;
               return getSync(cnt).globaltm - dist1;
            }
         }

         if (use_selected) { cnt = 0; use_selected = false; } else cnt++;
      }
   }


   // this is just shift relative to boundary SYNC markers
   // only possible when distance in nanoseconds

   double dist1 = localtm - getSync(0).localtm;
   double dist2 = localtm - getSync(numReadySyncs()-1).localtm;

   if (fabs(dist1) < fabs(dist2)) {
      if (indx) *indx = 0;
      return getSync(0).globaltm + dist1;
   }

   if (indx) *indx = numReadySyncs();
   return getSync(numReadySyncs()-1).globaltm + dist2;
}


bool base::StreamProc::AddNextBuffer(const Buffer& buf)
{
   // printf("%4s Add buffer queue size %u\n", GetName(), fQueue.size());

   if (fQueue.full()) printf("%s queue if full size %u\n", GetName(), fQueue.size());

   fQueue.push(buf);

   return true;
}

bool base::StreamProc::ScanNewBuffers()
{
//   printf("Scan buffers last %u size %u\n", fQueueScanIndex, fQueue.size());

   bool isany = false;

   while (fQueueScanIndex < fQueue.size()) {
      base::Buffer& buf = fQueue.item(fQueueScanIndex);
      // if first scan failed, release buffer
      // TODO: probably, one could remove buffer immediately
      if (!FirstBufferScan(buf))
         buf.reset();
      else
         isany = true;
      fQueueScanIndex++;
   }

   // for raw scanning any other steps are not interesting
   if (IsRawScanOnly()) SkipAllData();

   return isany;
}

bool base::StreamProc::ScanNewBuffersTm()
{
   // here we recalculate times for each buffer
   // this only can be done when appropriate syncs are produced

   // for raw processor no any time is interesting
   if (IsRawScanOnly()) return true;

   // printf("%s ScanNewBuffersTm() indx %u size %u\n", GetName(), fQueueScanIndexTm, fQueue.size());

   while (fQueueScanIndexTm < fQueue.size()) {
      base::Buffer& buf = fQueue.item(fQueueScanIndexTm);

      // when empty buffer, just ignore it - it will be skipped sometime
      if (!buf.null()) {

         unsigned sync_index(0);

         GlobalTime_t tm = LocalToGlobalTime(buf().local_tm, &sync_index);

         // printf("%s Scan buffer %u local %12.9f global %12.9f sync_index %u\n", GetName(), fQueueScanIndexTm, buf().local_tm, tm, sync_index);

         // only accept time calculation when interpolation
         if ((fSynchronisationKind==sync_Inter) && !IsSyncIndexWithInterpolation(sync_index)) break;
         // when sync on the left side is last, wait for next sync
         if ((fSynchronisationKind==sync_Left) && (sync_index==numReadySyncs()-1)) break;

         buf().global_tm = tm;
      }
      // printf("Set for buffer %u global tm %8.6f\n", fQueueScanIndexTm, fQueue.item(fQueueScanIndexTm).rec().globaltm);
      fQueueScanIndexTm++;
   }

   return true;
}


base::GlobalTime_t base::StreamProc::ProvidePotentialFlushTime(GlobalTime_t last_marker)
{
   // approach is simple - one propose as flush time begin of nbuf-2
   // make it little bit more complex - use buffers with time stamps

//   printf("ProvidePotentialFlushTime queue size %u scan indx %u \n", fQueue.size(), fQueueScanIndexTm);

   if (fQueueScanIndexTm<3) return 0.;

   for (unsigned n=1; n<fQueueScanIndexTm-2; n++)
      if (fQueue.item(n).rec().global_tm > last_marker) return fQueue.item(n).rec().global_tm;

   return 0.;
}

bool base::StreamProc::VerifyFlushTime(const base::GlobalTime_t& flush_time)
{
   // verify that proposed flush time can be processed already now
   // for that buffer time should be assigned

   // when processor doing raw scan, one can ignore flushing as well
   if (IsRawScanOnly()) return true;

   if ((flush_time==0.) || (fQueueScanIndexTm<2)) return false;

   for (unsigned n=0;n<fQueueScanIndexTm-1;n++)
      if (fQueue.item(n).rec().global_tm > flush_time) return true;

   return true;
}


void base::StreamProc::AddSyncMarker(base::SyncMarker& marker)
{
   if (!IsSynchronisationRequired()) {
      printf("No sync should be supplied !!!!!\n");
      exit(5);
   }

   if ((numSyncs()>0) && (numSyncs() % 1000 == 0)) {
      printf("%s too much syncs %u - something wrong??\n", GetName(), numSyncs());
   }

   marker.globaltm = 0.;
   marker.bufid = fQueueScanIndex;
   fSyncs.push(marker);
}

bool base::StreamProc::AddTriggerMarker(LocalTimeMarker& marker, double tm_range)
{
   // last local trigger is remembered to exclude trigger duplication or
   // too close distances

   if (fLastLocalTriggerTm != 0.) {

      if (tm_range==0.) tm_range = fTriggerAcceptMaring;

      double dist = marker.localtm - fLastLocalTriggerTm;

      // make very simple rule - distance should be bigger than specified range
      if (dist <= tm_range) {
         // printf("Reject trigger %12.9f\n", marker.localtm);
         return false;
      }
   }

   if (fLocalMarks.full()) {
      printf("Local markers queue is full in processor %s\n", GetName());
      exit(7);
   }

   fLocalMarks.push(marker);

//   printf("Add trigger %12.9f\n", marker.localtm);

   // keep time of last trigger
   fLastLocalTriggerTm = marker.localtm;

   return true;
}

unsigned base::StreamProc::findSyncWithId(unsigned syncid) const
{
   for (unsigned n=0; n<fSyncs.size(); n++)
      if (fSyncs.item(n).uniqueid == syncid) return n;

   return fSyncs.size();
}


bool base::StreamProc::eraseSyncAt(unsigned indx)
{
   if (indx < fSyncs.size()) {
      fSyncs.erase_item(indx);
      if (fSyncScanIndex>indx) fSyncScanIndex--;
      return true;
   }
   return false;
}

bool base::StreamProc::eraseFirstSyncs(unsigned num_erase)
{
   if (fSyncScanIndex > num_erase)
      fSyncScanIndex-=num_erase;
   else
      fSyncScanIndex = 0;

   fSyncs.pop_items(num_erase);

   return true;
}



bool base::StreamProc::SkipBuffers(unsigned num_skip)
{
   if (num_skip > fQueue.size()) num_skip = fQueue.size();

   if (num_skip==0) return false;

   fQueue.pop_items(num_skip);

   // erase all syncs wich are reference to skipped buffers except one
   // always keep at least two syncs
   unsigned erase_cnt(0);
   while (((erase_cnt + 1) < fSyncs.size()) && (fSyncs.item(erase_cnt+1).bufid < num_skip)) erase_cnt++;
   eraseFirstSyncs(erase_cnt);

//   printf("%4s skips %2u buffers and %2u syncs, remained %3u buffers and %3u syncs\n", GetName(), num_skip, erase_cnt, fQueue.size(), fSyncs.size());

   for (unsigned n=0;n<fSyncs.size();n++) {
//      printf("   sync %u  was from buffer %u\n", n, fSyncs.item(n).bufid);
      if (fSyncs.item(n).bufid>num_skip)
         fSyncs.item(n).bufid-=num_skip;
      else
         fSyncs.item(n).bufid = 0;
   }

//   printf("Skip %u buffers numsync %u sync0id %u sync0tm %8.3f numbufs %u\n", num_skip, fSyncs.size(), fSyncs[0].uniqueid, fSyncs[0].globaltm*1e-9, fQueue.size());

   // local triggers must be cleanup by other means,
   // TODO: if it would not be possible, one could use front time of buffers to skip triggers
   if (fLocalMarks.size()>1000)
      printf("Too much %u local trigger remaining - why\n?", (unsigned) fLocalMarks.size());

   if (fQueueScanIndex>=num_skip) {
      fQueueScanIndex-=num_skip;
   } else {
      fQueueScanIndex = 0;
      printf("!!! Problem with skipping and fQueueScanIndex !!!\n");
      exit(7);
   }

   if (fQueueScanIndexTm>=num_skip) {
      fQueueScanIndexTm-=num_skip;
   } else {
      fQueueScanIndexTm = 0;
      printf("!!! Problem with skipping and fQueueScanIndexTm !!!\n");
      exit(7);
   }

   return true;
}

void base::StreamProc::SkipAllData()
{
   fQueueScanIndexTm = fQueue.size();

   SkipBuffers(fQueue.size());

   fLocalMarks.clear();
   fGlobalMarks.clear();

   fSyncs.clear();
   fSyncScanIndex = 0;
}



bool base::StreamProc::CollectTriggers(GlobalMarksQueue& trigs)
{
   // TODO: one can make more sophisticated rules like time combination of several AUXs or even channles
   // TODO: another way - multiplicity histograms

   unsigned syncindx(0); // index used to help convert global to local time

//   printf("Collect new triggers numSync %u %u \n", numSyncs(), fLocalMarks.size());

   while (fLocalMarks.size() > 0) {

      GlobalMarker marker;

      marker.globaltm = LocalToGlobalTime(fLocalMarks.front().localtm, &syncindx);

//      printf("Start scan trig local %u %u sync_indx %u %12.9f\n", num_trig, fLocalMarks.size(), syncindx, marker.globaltm*1e-9);

      if (fSynchronisationKind == sync_Inter) {
         // when synchronization used, one should verify where exact our local trigger is

         // this is a case when marker right to the last sync
         if (syncindx == numReadySyncs()) break;

         if (syncindx == 0) {
            printf("Strange - trigger time left to first sync - try to continue\n");
         }
      }

      if (fSynchronisationKind == sync_Left) {
         // when sync marker on left side and it is last marker, wait for better time
         if (syncindx == numReadySyncs()-1) break;
      }


      fLocalMarks.pop();
      trigs.push(marker);
   }

   return true;
}

bool base::StreamProc::DistributeTriggers(const base::GlobalMarksQueue& queue)
{
   // here just duplicate of main trigger queue is created
   // TODO: make each trigger with unique id

   // no need for trigger when doing only raw scan
   if (IsRawScanOnly()) return true;

   while (fGlobalMarks.size() < queue.size()) {
      unsigned indx = fGlobalMarks.size();

      fGlobalMarks.push(queue.item(indx));

      fGlobalMarks.back().SetInterval(GetC1Limit(fTriggerWindow, true), GetC1Limit(fTriggerWindow, false));

//      if (fGlobalMarks.back().normal())
//         printf("%s trigger %12.9f %12.9f\n", GetName(), fGlobalMarks.back().lefttm, fGlobalTrig.back().righttm);
   }

//   printf("%s triggers after append new items\n", GetName());
//   for (unsigned n=0;n<fGlobalMarks.size();n++)
//      printf("TRIG %u %12.9f\n", n, fGlobalMarks.item(n).globaltm*1e-9);

   return true;
}


bool base::StreamProc::AppendSubevent(base::Event* evt)
{
   if (IsRawScanOnly()) return true;

   if (fGlobalMarks.size()==0) {
      printf("global trigger queue empty !!!\n");
      exit(14);
      return false;
   }

   if (fGlobalMarks.front().normal())
      FillH1(fMultipl, fGlobalMarks.front().subev ? fGlobalMarks.front().subev->Multiplicity() : 0);

   if (fGlobalMarks.front().subev!=0) {
      if (evt!=0) {
         if (IsTimeSorting()) fGlobalMarks.front().subev->Sort();
         evt->AddSubEvent(GetName(), fGlobalMarks.front().subev);
      } else {
         printf("Something wrong - subevent could not be assigned normal %d!!!!\n", fGlobalMarks.front().normal());
         delete fGlobalMarks.front().subev;
      }
      fGlobalMarks.front().subev = 0;
   }

   fGlobalMarks.pop();

   if (fGlobalTrigScanIndex==0) {
      printf("Index of ready event is 0 - how to understand???\n");
      exit(12);
   } else {
      fGlobalTrigScanIndex--;
   }

//   printf("%s triggers after remove first item\n", GetName());
//   for (unsigned n=0;n<fGlobalMarks.size();n++)
//      printf("TRIG %u %12.9f\n", n, fGlobalMarks.item(n).globaltm*1e-9);

   return true;
}


unsigned base::StreamProc::TestHitTime(const base::GlobalTime_t& hittime, bool normal_hit, bool can_close_event)
{
   double dist(0.), best_dist(-1e15), best_trigertm(-1e15);

   unsigned res_indx(fGlobalMarks.size()), best_indx(fGlobalMarks.size());

   for (unsigned indx=fGlobalTrigScanIndex; indx<fGlobalTrigRightIndex; indx++) {

       if (indx>=fGlobalMarks.size()) {
          printf("ALARM!!!!\n");
          exit(10);
       }

       GlobalMarker& marker = fGlobalMarks.item(indx);

       int test = marker.TestHitTime(hittime, &dist);

       // remember best distance for normal trigger,
       // message can go inside only for normal trigger
       // but we need to check position relative to trigger to be able perform flushing

       if (marker.normal()) {
          if (fabs(best_dist) > fabs(dist)) {
             best_dist = dist;
             best_trigertm = hittime - marker.globaltm;
             best_indx = indx;
          }

          if (test==0) {
//             printf("Assign hit to evnt %3u  left %3u\n", indx, fGlobalTrigScanIndex);

             res_indx = indx;
             break;
          }
       }

       if (test>0) {
          // current hit message is far away right from trigger,
          // one can declare event ready

//        printf("Find message on the right side from event %u distance %8.6f time %8.6f\n", indx, dist, triggertm);

          if (can_close_event && (dist>MaximumDisorderTm())) {
             if (indx==fGlobalTrigScanIndex) {
//                if (fGlobalTrig[indx].normal())
//                   printf("Declare trigger %12.9f ready\n", marker.globaltm);
                fGlobalTrigScanIndex++;

             } else {
                printf("Check hit time error trig_indx:%u trig_tm:%12.9f left_indx:%u left_tm:%12.9f dist:%12.9f- check \n",
                      indx, marker.globaltm,
                      fGlobalTrigScanIndex, fGlobalMarks.item(fGlobalTrigScanIndex).globaltm, dist);
                exit(17);
             }
          }
       } else {
          // when significantly left to the trigger
          // distance will be negative
       }
    }

//   if (res_indx == fGlobalTrig.size()) {
//      printf("non-assigned hit\n");
//   }

   // account hit time in histogram
   if (normal_hit && (best_indx<fGlobalMarks.size()))
      FillH1(fTriggerTm, best_trigertm);

   //printf("Test message %12.9f again trigger %12.9f test = %d dist = %9.0f\n", globaltm*1e-9, fGlobalTrig[indx].globaltm*1e-9, test, dist);

   return normal_hit ? res_indx : fGlobalMarks.size();
}

bool base::StreamProc::ScanDataForNewTriggers()
{
   // first of all, one should find right boundary, where data can be scanned
   // while when buffer scanned for the second time, it will be automatically rejected

   // there is nice rule - last trigger and last buffer always remains
   // time of last trigger is used to check which buffers can be scanned
   // time of last buffer is used to check which triggers we could check

   // printf("Proc:%s  Try scan data numtrig %u scan tm %u raw %u\n", GetName(), fGlobalMarks.size(), fQueueScanIndexTm, IsRawScanOnly());

   // never do any seconds scan in such situation
   if (IsRawScanOnly()) return true;

   // never scan last buffer
   if (fQueueScanIndexTm < 2) return true;

   // never scan when no triggers are exists
   if (fGlobalMarks.size() == 0) return true;

   // define triggers which we could scan
   fGlobalTrigRightIndex = fGlobalTrigScanIndex;

   if (fGlobalMarks.size() > 1) {

      if (fQueueScanIndexTm > fQueue.size()) {
         printf("Something wrong with index fQueueScanIndexTm %u\n", fQueueScanIndexTm);
         exit(12);
      }

      // define buffer index, which will be used for time boundary calculations
      // normally it should be last buffer with time assigned
      unsigned buffer_index_tm = fQueueScanIndexTm-1;

//      printf("Start search from index %u\n", buffer_index_tm);

      while (fQueue.item(buffer_index_tm).null()) {
         buffer_index_tm--;
         // last buffer should remain in queue anyway
         if (buffer_index_tm==0) return true;
      }
      // this is maximum time for the trigger which has chance to get all data from buffer with index fQueue.size()-2
      double trigger_time_limit = fQueue.item(buffer_index_tm).rec().global_tm - GetC1Limit(fTriggerWindow, false) - MaximumDisorderTm();

//      printf("Trigger time limit is %12.9f\n", trigger_time_limit*1e-9);

      while (fGlobalTrigRightIndex < fGlobalMarks.size()-1) {
         if (fGlobalMarks.item(fGlobalTrigRightIndex).globaltm > trigger_time_limit) break;
         fGlobalTrigRightIndex++;
      }
   }

   if (fGlobalTrigRightIndex==0) {
      // printf("No triggers are select for scanning\n");
      return true;
   }

   // at the same time, we must define upper_buf_limit to exclude case
   // that trigger time will be generated after we scan and drop buffer

   unsigned upper_buf_limit = 0;

   double buffer_timeboundary = fGlobalMarks.item(fGlobalTrigRightIndex-1).globaltm + GetC1Limit(fTriggerWindow, true) - MaximumDisorderTm();

   while (upper_buf_limit < fQueueScanIndexTm - 1) {
      // only when next buffer start tm less than left boundary of last trigger,
      // one can include previous buffer into the scan
      base::Buffer& buf = fQueue.item(upper_buf_limit+1);

      if (!buf.null())
         if (buf.rec().global_tm > buffer_timeboundary) break;
//      printf("Buffer %12.9f will be scanned, boundary %12.9f\n", fQueue.item(upper_buf_limit).rec().globaltm*1e-9, buffer_timeboundary*1e-9);
      upper_buf_limit++;
   }

   // if ((upper_buf_limit==0) && (fQueue.size()>5)) {
   //   upper_buf_limit = fQueue.size() - 5;
      // FIXME: this only work with synchronized inputs,
      // in the wild DAQ one should implement flush trigger!
   //   printf("FORCE to scan %u buffers - no chance to expect new triggers\n", upper_buf_limit);
   // }

   // nothing to do
   if (upper_buf_limit==0) {
//      printf("Nothing to do when try to scan for new trigger number %u buflimit %u queue %u \n", fGlobalTrig.size(), upper_buf_limit, fQueue.size());

      return true;
   }

   // till now it is very generic code how events limits and buffer limits are defined
   // therefore it is moved here

   // now one should scan selected buffers second time for data selection

//   printf("Proc:%p doing scan of %u buffers\n", this, upper_buf_limit);

   for (unsigned nbuf=0; nbuf<upper_buf_limit; nbuf++) {
//      printf("Second scan of buffer %u  size %u\n", nbuf, fQueue.size());

      if (nbuf>=fQueue.size()) {
         printf("Something went wrong\n");
         exit(11);
      }

      Buffer& buf = fQueue.item(nbuf);

      if (!buf.null())
         SecondBufferScan(buf);
   }

   // at the end all these buffer can be skipped from the queue
   // at the same time all sync will be skipped as well

   SkipBuffers(upper_buf_limit);

//   printf("After skip buffers queue size %u\n", fQueue.size());

   return true;
}
