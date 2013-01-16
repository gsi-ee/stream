#include "base/StreamProc.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "base/ProcMgr.h"
#include "base/Event.h"

void base::GlobalTriggerMarker::SetInterval(double left, double right)
{
   if (left>right) {
      printf("left > right in time interval - failure\n");
      exit(7);
   }

   lefttm = globaltm + left;
   righttm = globaltm + right;
}

int base::GlobalTriggerMarker::TestHitTime(const GlobalTime_t& hittime, double* dist)
{
   // be aware that condition like [left, right) is tested
   // therefore if left==right, hit will never be assigned to such condition

   if (dist) *dist = 0.;
   if (hittime < lefttm) {
      if (dist) *dist = hittime - lefttm;
      return -1;
   } else
   if (hittime >= righttm) {
      if (dist) *dist = hittime - righttm;
      return 1;
   }
   return 0;
}


// ====================================================================

base::StreamProc::StreamProc(const char* name, unsigned brdid) :
   fName(name),
   fBoardId(0),
   fMgr(0),
   fQueue(),
   fQueueScanIndex(0),
   fQueueScanIndexTm(0),
   fRawScanOnly(false),
   fIsSynchronisationRequired(true),
   fSyncs(),
   fSyncScanIndex(0),
   fLocalTrig(),
   fTriggerAcceptMaring(0.),
   fLastLocalTriggerTm(0.),
   fGlobalTrig(),
   fGlobalTrigScanIndex(0),
   fTimeSorting(false),
   fPrefix(),
   fSubPrefixD(),
   fSubPrefixN()
{
   if (brdid < 0xffffffff) {
      char sbuf[100];
      snprintf(sbuf, sizeof(sbuf), "%u", brdid);
      fName.append(sbuf);
      fBoardId = brdid;
   }

   fPrefix = fName;

   fMgr = base::ProcMgr::AddProc(this);

   fTriggerTm = MakeH1("TriggerTm", "Time relative to trigger", 2500, -1e-6, 4e-6, "s");
   fMultipl = MakeH1("Multipl", "Subevent multiplicity", 40, 0., 40., "hits");
   fTriggerWindow = MakeC1("TrWindow", 5e-7, 10e-7, fTriggerTm);
}


base::StreamProc::~StreamProc()
{
//   printf("Start bufs clear\n");
   fSyncs.clear();
   fQueue.clear();
   fLocalTrig.clear();
   // TODO: cleanup of event data
   fGlobalTrig.clear();
//   printf("Done bufs clear\n");
}

void base::StreamProc::SetSubPrefix(const char* name, int indx, const char* subname2, int indx2)
{
   if ((name==0) || (*name==0)) {
      fSubPrefixD.clear();
      fSubPrefixN.clear();
      return;
   }

   fSubPrefixN = name;
   fSubPrefixD = name;
   if (indx>=0) {
      char sbuf[100];
      snprintf(sbuf,sizeof(sbuf), "%d", indx);
      fSubPrefixD.append(sbuf);
      fSubPrefixN.append(sbuf);
   }
   fSubPrefixD.append("/");
   fSubPrefixN.append("_");

   if ((subname2!=0) && (*subname2!=0)) {
      fSubPrefixD.append(subname2);
      fSubPrefixN.append(subname2);
      if (indx2>=0) {
         char sbuf[100];
         snprintf(sbuf,sizeof(sbuf), "%d", indx2);
         fSubPrefixD.append(sbuf);
         fSubPrefixN.append(sbuf);
      }
      fSubPrefixD.append("/");
      fSubPrefixN.append("_");
   }


}


base::H1handle base::StreamProc::MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle)
{
   std::string hname = fPrefix + "/";
   if (!fSubPrefixD.empty()) hname += fSubPrefixD;
   hname += fPrefix + "_";
   if (!fSubPrefixN.empty()) hname += fSubPrefixN;
   hname.append(name);

   std::string htitle = fName;
   htitle.append(" ");
   if (!fSubPrefixN.empty()) htitle += fSubPrefixN + " ";
   htitle.append(title);

   return mgr()->MakeH1(hname.c_str(), htitle.c_str(), nbins, left, right, xtitle);
}


void base::StreamProc::FillH1(H1handle h1, double x, double weight)
{
   mgr()->FillH1(h1, x, weight);
}

base::H2handle base::StreamProc::MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options)
{
   std::string hname = fPrefix + "/";
   if (!fSubPrefixD.empty()) hname += fSubPrefixD;
   hname += fPrefix + "_";
   if (!fSubPrefixN.empty()) hname += fSubPrefixN;
   hname.append(name);

   std::string htitle = fName;
   htitle.append(" ");
   if (!fSubPrefixN.empty()) htitle += fSubPrefixN + " ";
   htitle.append(title);

   return mgr()->MakeH2(hname.c_str(), htitle.c_str(), nbins1, left1, right1, nbins2, left2, right2, options);
}

void base::StreamProc::FillH2(H1handle h2, double x, double y, double weight)
{
   mgr()->FillH2(h2, x, y, weight);
}


base::C1handle base::StreamProc::MakeC1(const char* name, double left, double right, H1handle h1)
{
   std::string cname = fPrefix + "/";
   if (!fSubPrefixD.empty()) cname += fSubPrefixD;
   cname += fPrefix + "_";
   if (!fSubPrefixN.empty()) cname += fSubPrefixN;
   cname.append(name);

   return mgr()->MakeC1(cname.c_str(), left, right, h1);
}


void base::StreamProc::ChangeC1(C1handle c1, double left, double right)
{
   mgr()->ChangeC1(c1, left, right);
}


int base::StreamProc::TestC1(C1handle c1, double value, double* dist)
{
   return mgr()->TestC1(c1, value, dist);
}


double base::StreamProc::GetC1Limit(C1handle c1, bool isleft)
{
   return mgr()->GetC1Limit(c1, isleft);
}

base::GlobalTime_t base::StreamProc::LocalToGlobalTime(base::GlobalTime_t localtm, unsigned* indx)
{
   // method uses helper index to locate faster interval where interpolation could be done
   // value of index is following
   //  0                      - last value left to the first sync
   //  [1..numReadySyncs()-1] - last value between indx-1 and indx syncs
   //  numReadySyncs()        - last value right to the last sync

   // TODO: one could probably use some other methods of time conversion

   if (!IsSynchronisationRequired()) return localtm;

   if (numReadySyncs() == 0) {
      printf("No any sync for time calibration\n");
      exit(7);
      return 0.;
   }

   // use liner approximation only when more than one sync available
   if (numReadySyncs()>1) {

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

         double dist1 = localtm - getSync(n).localtm;
         double dist2 = getSync(n+1).localtm - localtm;

         if ((dist1>=0.) && (dist2>0)) {
            //double k1 = dist2 / (dist1 + dist2);
            double diff1 = dist1 / (dist1 + dist2) * (getSync(n+1).globaltm - getSync(n).globaltm);
            //return getSync(*indx).globaltm*k1 + getSync(*indx+1).globaltm*k2;

            if ((dist1>0) && ((diff1/dist1 < 0.9) || (diff1/dist1 > 1.1))) {
               printf("Something wrong with time calc %8.6f %8.6f\n", dist1, diff1);
               exit(1);
            }

            if (indx) *indx = n + 1;

            //return getSync(*indx).globaltm + dist1;
            return getSync(n).globaltm + diff1;
         }
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
   fQueue.push_back(buf);

   return true;
}

bool base::StreamProc::ScanNewBuffers()
{
//   printf("Scan buffers last %u size %u\n", fQueueScanIndex, fQueue.size());

   while (fQueueScanIndex < fQueue.size()) {
      base::Buffer& buf = fQueue[fQueueScanIndex];
      // if first scan failed, release buffer
      // TODO: probably, one could remove buffer immediately
      if (!FirstBufferScan(buf)) buf.reset();
      fQueueScanIndex++;
   }

   // for raw scanning any other steps are not interesting
   if (IsRawScanOnly()) SkipAllData();

   return true;
}

bool base::StreamProc::ScanNewBuffersTm()
{
   // here we recalculate times for each buffer
   // this only can be done when appropriate syncs are produced

   // for raw processor no any time is interesting
   if (IsRawScanOnly()) return true;

   while (fQueueScanIndexTm < fQueue.size()) {
      base::Buffer& buf = fQueue[fQueueScanIndexTm];

      // when empty buffer, just ignore it - it will be skipped sometime
      if (!buf.null()) {

         unsigned sync_index(0);

         GlobalTime_t tm = LocalToGlobalTime(buf().local_tm, &sync_index);

         // only accept time calculation when interpolation
         if (IsSynchronisationRequired() && !IsSyncIndexWithInterpolation(sync_index)) break;

         buf().global_tm = tm;
      }
      // printf("Set for buffer %u global tm %8.6f\n", fQueueScanIndexTm, fQueue[fQueueScanIndexTm]().globaltm);
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
      if (fQueue[n].rec().global_tm > last_marker) return fQueue[n].rec().global_tm;

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
      if (fQueue[n].rec().global_tm > flush_time) return true;

   return true;
}


void base::StreamProc::AddSyncMarker(base::SyncMarker& marker)
{
   if (!IsSynchronisationRequired()) {
      printf("No sync should be supplied !!!!!\n");
      exit(5);
   }

   if ((numSyncs()>0) && (numSyncs() % 1000 == 0)) {
      printf("Too much syncs %u - something wrong??\n", numSyncs());
   }

   marker.globaltm = 0.;
   marker.bufid = fQueueScanIndex;
   fSyncs.push_back(marker);
}

bool base::StreamProc::AddTriggerMarker(LocalTriggerMarker& marker, double tm_range)
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

   fLocalTrig.push_back(marker);

//   printf("Add trigger %12.9f\n", marker.localtm);

   // keep time of last trigger
   fLastLocalTriggerTm = marker.localtm;

   return true;
}

unsigned base::StreamProc::findSyncWithId(unsigned syncid) const
{
   for (unsigned n=0; n<fSyncs.size(); n++)
      if (fSyncs[n].uniqueid == syncid) return n;

   return fSyncs.size();
}


bool base::StreamProc::eraseSyncAt(unsigned indx)
{
   if (indx < fSyncs.size()) {
      fSyncs.erase(fSyncs.begin() + indx);
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

   if (num_erase >= fSyncs.size()) {
      fSyncs.clear();
      return true;
   }

   fSyncs.erase(fSyncs.begin(), fSyncs.begin() + num_erase);

   return true;
}



bool base::StreamProc::SkipBuffers(unsigned num_skip)
{
   if (num_skip > fQueue.size()) num_skip = fQueue.size();

   if (num_skip==0) return false;

   fQueue.erase(fQueue.begin(), fQueue.begin() + num_skip);

   // erase all syncs with reference to first buffer
   unsigned erase_cnt(0);
   while ((erase_cnt + 1 < fSyncs.size()) && (fSyncs[erase_cnt].bufid + 1  < num_skip)) erase_cnt++;
   eraseFirstSyncs(erase_cnt);

   for (unsigned n=0;n<fSyncs.size();n++)
      if (fSyncs[n].bufid>num_skip)
         fSyncs[n].bufid-=num_skip;
      else
         fSyncs[n].bufid = 0;

//   printf("Skip %u buffers numsync %u sync0id %u sync0tm %8.3f numbufs %u\n", num_skip, fSyncs.size(), fSyncs[0].uniqueid, fSyncs[0].globaltm*1e-9, fQueue.size());

   // local triggers must be cleanup by other means,
   // TODO: if it would not be possible, one could use front time of buffers to skip triggers
   if (fLocalTrig.size()>1000)
      printf("Too much %u local trigger remaining - why\n?", fLocalTrig.size());

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

   fLocalTrig.clear();
   fGlobalTrig.clear();

   fSyncs.clear();
   fSyncScanIndex = 0;
}



bool base::StreamProc::CollectTriggers(GlobalTriggerMarksQueue& trigs)
{
   // TODO: one can make more sophisticated rules like time combination of several AUXs or even channles
   // TODO: another way - multiplicity histograms

   unsigned num_trig(0), syncindx(0); // index used to help convert global to local time

//   printf("Collect new triggers numSync %u %u \n", numSyncs(), fLocalTrig.size());

   while (num_trig<fLocalTrig.size()) {

      GlobalTriggerMarker marker;

      marker.globaltm = LocalToGlobalTime(fLocalTrig[num_trig].localtm, &syncindx);

/*      if (fabs(3026028138200. - fLocalTrig[num_trig].localtm) < 10.) {
         printf("Converting localtm: %12.9f res: %12.9f \n", fLocalTrig[num_trig].localtm*1e-9, marker.globaltm*1e-9);
         for (unsigned n=0; n<numReadySyncs();n++)
            printf("SYNC%u  localtm:%12.9f global:%12.9f\n", n, getSync(n).localtm*1e-9, getSync(n).globaltm*1e-9);
      }
*/

//      printf("Start scan trig local %u %u sync_indx %u %12.9f\n", num_trig, fLocalTrig.size(), syncindx, marker.globaltm*1e-9);

      if (IsSynchronisationRequired()) {
         // when synchronization used, one should verify where exact our local trigger is

         // this is a case when marker right to the last sync
         if (syncindx == numReadySyncs()) break;

         if (syncindx == 0) {
            printf("Strange - trigger time left to first sync - try to continue\n");
         }
      }

//      printf("Proc:%s TRIGG: %12.9f localtm:%12.9f syncindx %u  numsyncs %u\n",
//            GetName(), marker.globaltm*1e-9, fLocalTrig[num_trig].localtm*1e-9,
//            syncindx, numReadySyncs());

      num_trig++;
      trigs.push_back(marker);
   }

   if (num_trig == fLocalTrig.size())
      fLocalTrig.clear();
   else
      fLocalTrig.erase(fLocalTrig.begin(), fLocalTrig.begin()+num_trig);

   return true;
}

bool base::StreamProc::DistributeTriggers(const base::GlobalTriggerMarksQueue& queue)
{
   // here just duplicate of main trigger queue is created
   // TODO: make each trigger with unique id

   // no need for trigger when doing only raw scan
   if (IsRawScanOnly()) return true;

   while (fGlobalTrig.size() < queue.size()) {
      unsigned indx = fGlobalTrig.size();

      fGlobalTrig.push_back(base::GlobalTriggerMarker(queue[indx]));

      fGlobalTrig.back().SetInterval(GetC1Limit(fTriggerWindow, true), GetC1Limit(fTriggerWindow, false));

//      if (fGlobalTrig.back().normal())
//         printf("%s trigger %12.9f %12.9f\n", GetName(), fGlobalTrig.back().lefttm, fGlobalTrig.back().righttm);
   }

//   printf("%s triggers after append new items\n", GetName());
//   for (unsigned n=0;n<fGlobalTrig.size();n++)
//      printf("TRIG %u %12.9f\n", n, fGlobalTrig[n].globaltm*1e-9);

   return true;
}


bool base::StreamProc::AppendSubevent(base::Event* evt)
{
   if (IsRawScanOnly()) return true;

   if (fGlobalTrig.size()==0) {
      printf("global trigger queue empty !!!\n");
      exit(14);
      return false;
   }

   if (fGlobalTrig[0].normal())
      FillH1(fMultipl, fGlobalTrig[0].subev ? fGlobalTrig[0].subev->Multiplicity() : 0);

   if (fGlobalTrig[0].subev!=0) {
      if (evt!=0) {
         if (IsTimeSorting()) fGlobalTrig[0].subev->Sort();
         evt->AddSubEvent(GetName(), fGlobalTrig[0].subev);
      } else {
         printf("Something wrong - subevent could not be assigned normal %d!!!!\n", fGlobalTrig[0].normal());
         delete fGlobalTrig[0].subev;
      }
      fGlobalTrig[0].subev = 0;
   }

   fGlobalTrig.erase(fGlobalTrig.begin());
   if (fGlobalTrigScanIndex==0) {
      printf("Index of ready event is 0 - how to understand???\n");
      exit(12);
   } else {
      fGlobalTrigScanIndex--;
   }

//   printf("%s triggers after remove first item\n", GetName());
//   for (unsigned n=0;n<fGlobalTrig.size();n++)
//      printf("TRIG %u %12.9f\n", n, fGlobalTrig[n].globaltm*1e-9);


   return true;
}


unsigned base::StreamProc::TestHitTime(const base::GlobalTime_t& hittime, bool normal_hit, bool can_close_event)
{
   double dist(0.), best_dist(-1e15), best_trigertm(-1e15);

   unsigned res_indx(fGlobalTrig.size()), best_indx(fGlobalTrig.size());

   for (unsigned indx=fGlobalTrigScanIndex; indx<fGlobalTrigRightIndex; indx++) {

       if (indx>=fGlobalTrig.size()) {
          printf("ALARM!!!!\n");
          exit(10);
       }

       int test = fGlobalTrig[indx].TestHitTime(hittime, &dist);

       // remember best distance for normal trigger,
       // message can go inside only for normal trigger
       // but we need to check position relative to trigger to be able perform flushing

       if (fGlobalTrig[indx].normal()) {
          if (fabs(best_dist) > fabs(dist)) {
             best_dist = dist;
             best_trigertm = hittime - fGlobalTrig[indx].globaltm;
             best_indx = indx;
          }

          if (test==0) {
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
//                   printf("Declare trigger %12.9f ready\n", fGlobalTrig[indx].globaltm);
                fGlobalTrigScanIndex++;

             } else {
                printf("Check hit time error trig_indx:%u trig_tm:%12.9f left_indx:%u left_tm:%12.9f dist:%12.9f- check \n",
                      indx, fGlobalTrig[indx].globaltm,
                      fGlobalTrigScanIndex, fGlobalTrig[fGlobalTrigScanIndex].globaltm, dist);
                exit(17);
             }
          }
       } else {
          // when significantly left to the trigger
          // distance will be negative
       }
    }

   // account hit time in histogram
   if (normal_hit && (best_indx<fGlobalTrig.size()))
      FillH1(fTriggerTm, best_trigertm);

   //printf("Test message %12.9f again trigger %12.9f test = %d dist = %9.0f\n", globaltm*1e-9, fGlobalTrig[indx].globaltm*1e-9, test, dist);

   return normal_hit ? res_indx : fGlobalTrig.size();
}

bool base::StreamProc::ScanDataForNewTriggers()
{
   // first of all, one should find right boundary, where data can be scanned
   // while when buffer scanned for the second time, it will be automatically rejected

   // there is nice rule - last trigger and last buffer always remains
   // time of last trigger is used to check which buffers can be scanned
   // time of last buffer is used to check which triggers we could check

//   printf("Proc:%p  Try scan data numtrig %u scan tm %u raw %u\n", GetName(), fGlobalTrig.size(), fQueueScanIndexTm, IsRawScanOnly());

   // never do any seconds scan in such situation
   if (IsRawScanOnly()) return true;

   // never scan last buffer
   if (fQueueScanIndexTm < 2) return true;

   // never scan when no triggers are exists
   if (fGlobalTrig.size() == 0) return true;

   // define triggers which we could scan
   fGlobalTrigRightIndex = fGlobalTrigScanIndex;

   if (fGlobalTrig.size() > 1) {

      if (fQueueScanIndexTm > fQueue.size()) {
         printf("Something wrong with index fQueueScanIndexTm %u\n", fQueueScanIndexTm);
         exit(12);
      }

      // define buffer index, which will be used for time boundary calculations
      // normally it should be last buffer with time assigned
      unsigned buffer_index_tm = fQueueScanIndexTm-1;

//      printf("Start search from index %u\n", buffer_index_tm);

      while (fQueue[buffer_index_tm].null()) {
         buffer_index_tm--;
         // last buffer should remain in queue anyway
         if (buffer_index_tm==0) return true;
      }
      // this is maximum time for the trigger which has chance to get all data from buffer with index fQueue.size()-2
      double trigger_time_limit = fQueue[buffer_index_tm].rec().global_tm - GetC1Limit(fTriggerWindow, false) - MaximumDisorderTm();

//      printf("Trigger time limit is %12.9f\n", trigger_time_limit*1e-9);

      while (fGlobalTrigRightIndex < fGlobalTrig.size()-1) {
         if (fGlobalTrig[fGlobalTrigRightIndex].globaltm > trigger_time_limit) break;
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

   double buffer_timeboundary = fGlobalTrig[fGlobalTrigRightIndex-1].globaltm + GetC1Limit(fTriggerWindow, true) - MaximumDisorderTm();

   while (upper_buf_limit < fQueueScanIndexTm - 1) {
      // only when next buffer start tm less than left boundary of last trigger,
      // one can include previous buffer into the scan
      base::Buffer& buf = fQueue[upper_buf_limit+1];

      if (!buf.null())
         if (buf.rec().global_tm > buffer_timeboundary) break;
//      printf("Buffer %12.9f will be scanned, boundary %12.9f\n", fQueue[upper_buf_limit]().globaltm*1e-9, buffer_timeboundary*1e-9);
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

      if (!fQueue[nbuf].null())
         SecondBufferScan(fQueue[nbuf]);
   }

   // at the end all these buffer can be skipped from the queue
   // at the same time all sync will be skipped as well

   SkipBuffers(upper_buf_limit);

//   printf("After skip buffers queue size %u\n", fQueue.size());

   return true;
}
