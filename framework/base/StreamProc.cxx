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

base::StreamProc::StreamProc(const char* name, int indx) :
   fName(name),
   fMgr(0),
   fQueue(),
   fQueueScanIndex(0),
   fQueueScanIndexTm(0),
   fIsSynchronisationRequired(true),
   fSyncs(),
   fSyncScanIndex(0),
   fGlobalTrig(),
   fGlobalTrigScanIndex(0),
   fTimeSorting(false),
   fPrefix(),
   fSubPrefixD(),
   fSubPrefixN()
{
   if (indx>=0) {
      char sbuf[100];
      snprintf(sbuf, sizeof(sbuf),"%d",indx);
      fName.append(sbuf);
   }

   fPrefix = fName;

   fMgr = base::ProcMgr::AddProc(this);

   fTriggerTm = MakeH1("TriggerTm", "Time relative to trigger", 2500, -1000., 4000., "ns");
   fMultipl = MakeH1("Multipl", "Subevent multiplicity", 40, 0., 40., "hits");
   triggerWindow = MakeC1("TrWindow", 500, 1000, fTriggerTm);
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

bool base::StreamProc::eraseSyncAt(unsigned indx)
{
   if (indx < fSyncs.size()) {
      fSyncs.erase(fSyncs.begin() + indx);
      if (fSyncScanIndex>indx) fSyncScanIndex--;
      return true;
   }
   return false;
}

base::GlobalTime_t base::StreamProc::LocalToGlobalTime(base::GlobalTime_t localtm, unsigned* indx)
{
   if (!IsSynchronisationRequired()) return localtm;

   if (numSyncs() == 0) {
      printf("No any sync for time calibration\n");
      exit(7);
      return 0.;
   }

   if ((numSyncs()>0) && (numSyncs() % 1000 == 0)) {
      printf("Too much syncs %u - something wrong??\n", numSyncs());
   }

   // use liner approximation only when more than one sync available
   if (numSyncs()>1) {

      if ((indx!=0) &&  (*indx < numSyncs()-1) ) {
         double dist1 = local_time_dist(getSync(*indx).localtm, localtm);
         double dist2 = local_time_dist(localtm, getSync(*indx+1).localtm);

         if ((dist1>=0.) && (dist2>0)) {
            //double k1 = dist2 / (dist1 + dist2);
            double diff1 = dist1 / (dist1 + dist2) * (getSync(*indx+1).globaltm - getSync(*indx).globaltm);
            //return getSync(*indx).globaltm*k1 + getSync(*indx+1).globaltm*k2;

            if ((dist1>0) && ((diff1/dist1 < 0.9) || (diff1/dist1 > 1.1))) {
               printf("Simothing wrong with time calc %8.6f %8.6f\n", dist1, diff1);
               exit(1);
            }

            //return getSync(*indx).globaltm + dist1;
            return getSync(*indx).globaltm + diff1;
         }
      }

      for (unsigned n=0; n<numSyncs()-1; n++) {
         double dist1 = local_time_dist(getSync(n).localtm, localtm);
         double dist2 = local_time_dist(localtm, getSync(n+1).localtm);

         if ((dist1>=0.) && (dist2>0)) {
            //double k1 = dist2 / (dist1 + dist2);
            double diff1 = dist1 / (dist1 + dist2) * (getSync(n+1).globaltm - getSync(n).globaltm);
            //return getSync(*indx).globaltm*k1 + getSync(*indx+1).globaltm*k2;

            if ((dist1>0) && ((diff1/dist1 < 0.9) || (diff1/dist1 > 1.1))) {
               printf("Something wrong with time calc %8.6f %8.6f\n", dist1, diff1);
               exit(1);
            }

            //return getSync(*indx).globaltm + dist1;
            return getSync(n).globaltm + diff1;

         }
      }
   }

   // this is just shift relative to boundary SYNC markers
   // only possible when distance in nanoseconds

   double dist1 = local_time_dist(getSync(0).localtm, localtm);
   double dist2 = local_time_dist(getSync(numSyncs()-1).localtm, localtm);

   if (fabs(dist1) < fabs(dist2))
      return getSync(0).globaltm + dist1;

   return getSync(numSyncs()-1).globaltm + dist2;
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
      FirstBufferScan(fQueue[fQueueScanIndex]);
      fQueueScanIndex++;
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


bool base::StreamProc::ScanNewBuffersTm()
{
   // here we recalculate times for each buffer
   // this only can be done when appropriate syncs are produced

   unsigned scan_limit = fQueue.size();
   if (fSyncScanIndex < fSyncs.size())
      scan_limit = fSyncs[fSyncScanIndex].bufid;

   while (fQueueScanIndexTm < scan_limit) {
      fQueue[fQueueScanIndexTm]().global_tm = LocalToGlobalTime(fQueue[fQueueScanIndexTm]().local_tm);
      // printf("Set for buffer %u global tm %8.6f\n", fQueueScanIndexTm, fQueue[fQueueScanIndexTm]().globaltm);
      fQueueScanIndexTm++;
   }

   return true;
}


base::GlobalTime_t base::StreamProc::ProvidePotentialFlushTime(GlobalTime_t last_marker)
{
   // approach is simple - one propose as flush time begin of nbuf-2
   // make it little bit more complex - use buffers with time stamps

   if (fQueueScanIndexTm<3) return 0.;

   for (unsigned n=1; n<fQueueScanIndexTm-2; n++)
      if (fQueue[n].rec().global_tm > last_marker) return fQueue[n].rec().global_tm;

   return 0.;
}

bool base::StreamProc::VerifyFlushTime(const base::GlobalTime_t& flush_time)
{
   // verify that proposed flush time can be processed already now
   // for that buffer time should be assigned

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

   marker.globaltm = 0.;
   marker.bufid = fQueueScanIndex;
   fSyncs.push_back(marker);
}


void base::StreamProc::AddTriggerMarker(LocalTriggerMarker& marker)
{
   marker.bufid = fQueueScanIndex;
   fLocalTrig.push_back(marker);
}


bool base::StreamProc::SkipBuffers(unsigned num_skip)
{
   if (num_skip > fQueue.size()) num_skip = fQueue.size();

   if (num_skip==0) return false;

   fQueue.erase(fQueue.begin(), fQueue.begin() + num_skip);

   // erase all syncs with reference to first buffer
   while ((fSyncs.size()>1) && (fSyncs[0].bufid<num_skip)) {
      fSyncs.erase(fSyncs.begin());
      if (fSyncScanIndex>0) fSyncScanIndex--;
   }

   for (unsigned n=0;n<fSyncs.size();n++)
      if (fSyncs[n].bufid>num_skip)
         fSyncs[n].bufid-=num_skip;
      else
         fSyncs[n].bufid = 0;

//   printf("Skip %u buffers numsync %u sync0id %u sync0tm %8.3f numbufs %u\n", num_skip, fSyncs.size(), fSyncs[0].uniqueid, fSyncs[0].globaltm*1e-9, fQueue.size());


   while ((fLocalTrig.size()>0) && (fLocalTrig[0].bufid<num_skip))
      fLocalTrig.erase(fLocalTrig.begin());

   for (unsigned n=0;n<fLocalTrig.size();n++)
      fLocalTrig[n].bufid-=num_skip;

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

bool base::StreamProc::CollectTriggers(GlobalTriggerMarksQueue& trigs)
{
   // TODO: one can make more sophisticated rules like time combination of several AUXs or even channles
   // TODO: another way - multiplicity histograms

   unsigned num_trig(0);

   for (unsigned n=0;n<fLocalTrig.size();n++) {

      // do not provide triggers, which cannot be correctly assigned in time
      // TODO: it is simplified way, one could use time itself
      if (fLocalTrig[n].bufid >= fQueueScanIndexTm) break;

      num_trig++;

      GlobalTriggerMarker marker;

      marker.globaltm = LocalToGlobalTime(fLocalTrig[n].localtm);

//      printf("TRIGG: %12.9f\n", marker.globaltm*1e-9);

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

   while (fGlobalTrig.size() < queue.size()) {
      unsigned indx = fGlobalTrig.size();

      fGlobalTrig.push_back(base::GlobalTriggerMarker(queue[indx]));

      fGlobalTrig.back().SetInterval(GetC1Limit(triggerWindow, true), GetC1Limit(triggerWindow, false));

   }

//   printf("%s triggers after append new items\n", GetProcName().c_str());
//   for (unsigned n=0;n<fGlobalTrig.size();n++)
//      printf("TRIG %u %12.9f\n", n, fGlobalTrig[n].globaltm*1e-9);

   return true;
}


bool base::StreamProc::AppendSubevent(base::Event* evt)
{
   if (fGlobalTrig.size()==0) {
      printf("global trigger queue empty !!!\n");
      exit(14);
      return false;
   }

   if (fGlobalTrig[0].normal())
      FillH1(fMultipl, GetTriggerMultipl(0));

   if (fGlobalTrig[0].subev!=0) {
      if (evt!=0) {
         if (IsTimeSorting()) SortDataInSubEvent(fGlobalTrig[0].subev);
         evt->AddSubEvent(GetProcName(), fGlobalTrig[0].subev);
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

//   printf("%s triggers after remove first item\n", GetProcName().c_str());
//   for (unsigned n=0;n<fGlobalTrig.size();n++)
//      printf("TRIG %u %12.9f\n", n, fGlobalTrig[n].globaltm*1e-9);


   return true;
}


unsigned base::StreamProc::TestHitTime(const base::GlobalTime_t& hittime, bool normal_hit)
{
   double dist(0.), best_dist(-1e15), best_trigertm(-1e15);

   unsigned res_trigindx = fGlobalTrig.size();

   for (unsigned indx=fGlobalTrigScanIndex; indx<fGlobalTrigRightIndex; indx++) {

       if (indx>=fGlobalTrig.size()) {
          printf("ALARM!!!!\n");
          exit(10);
       }

       int test = fGlobalTrig[indx].TestHitTime(hittime, &dist);

       // remember best distance for
       if (fGlobalTrig[indx].normal() && (fabs(best_dist) > fabs(dist))) {
          best_dist = dist;
          best_trigertm = hittime - fGlobalTrig[indx].globaltm;
       }

       if (test==0) {
          res_trigindx = indx;
          break;
       }

       if (test>0) {
          // current hit message is far away right from trigger,
          // one can declare event ready

//        printf("Find message on the right side from event %u distance %8.6f time %8.6f\n", indx, dist, triggertm);

          if (dist>MaximumDisorderTm()) {
             if (indx==fGlobalTrigScanIndex) {
                // printf("Declare event %u ready\n", lefttrig);
                fGlobalTrigScanIndex++;

             } else {
                printf("Something completely wrong indx:%u %12.9f left:%u %12.9f - check \n",
                      indx, fGlobalTrig[indx].globaltm*1e-9,
                      fGlobalTrigScanIndex, fGlobalTrig[fGlobalTrigScanIndex].globaltm*1e-9);
                exit(17);
             }
          }
       } else {
          // when significantly left to the trigger
          // distance will be negative
       }
    }

   if (normal_hit && (best_trigertm>-1e15)) {
      FillH1(fTriggerTm, best_trigertm);
      //printf("Test message %12.9f again trigger %12.9f test = %d dist = %9.0f\n", globaltm*1e-9, fGlobalTrig[indx].globaltm*1e-9, test, dist);
   }

   return normal_hit ? res_trigindx : fGlobalTrig.size();
}

bool base::StreamProc::ScanDataForNewTriggers()
{
   // first of all, one should find right boundary, where data can be scanned
   // while when buffer scanned for the second time, it will be automatically rejected

   // there is nice rule - last trigger and last buffer always remains
   // time of last trigger is used to check which buffers can be scanned
   // time of last buffer is used to check which triggers we could check


   // never scan last buffer
   if (fQueueScanIndexTm < 2) return true;

   // never scan when no triggers are exists
   if (fGlobalTrig.size() == 0) return true;

   // define triggers which we could scan
   fGlobalTrigRightIndex = fGlobalTrigScanIndex;

   if (fGlobalTrig.size() > 1) {

      // this is maximum time for the trigger which has chance to get all data from buffer with index fQueue.size()-2
      double trigger_time_limit = fQueue[fQueueScanIndexTm-1].rec().global_tm - GetC1Limit(triggerWindow, false) - MaximumDisorderTm();

      while (fGlobalTrigRightIndex < fGlobalTrig.size()-1) {
         if (fGlobalTrig[fGlobalTrigRightIndex].globaltm > trigger_time_limit) break;
         fGlobalTrigRightIndex++;
      }
   }

   if (fGlobalTrigRightIndex==0) {
      printf("No triggers are select for scanning\n");
      return true;
   }

   // at the same time, we must define upper_buf_limit to exclude case
   // that trigger time will be generated after we scan and drop buffer

   unsigned upper_buf_limit = 0;

   double buffer_timeboundary = fGlobalTrig[fGlobalTrigRightIndex-1].globaltm + GetC1Limit(triggerWindow, true) - MaximumDisorderTm();

   while (upper_buf_limit < fQueueScanIndexTm - 1) {
      // only when next buffer start tm less than left boundary of last trigger,
      // one can include previous buffer into the scan
      if (fQueue[upper_buf_limit+1].rec().global_tm > buffer_timeboundary) break;
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

   // till now it is very generic code, probably we will move it to the generic part of the code

   // from here it is just scan of the NX buffers

//   printf("Scan now %u buffers for [%u %u) triggers \n", upper_buf_limit, fGlobalTrigScanIndex, upper_trig_limit);

   for (unsigned nbuf=0; nbuf<upper_buf_limit; nbuf++) {
//      printf("Second scan of buffer %u  size %u\n", nbuf, fQueue.size());

      if (nbuf>=fQueue.size()) {
         printf("Something went wrong\n");
         exit(11);
      }

      SecondBufferScan(fQueue[nbuf]);
   }

//   printf("Before skip %u buffers queue size %u\n", upper_buf_limit, fQueue.size());

   SkipBuffers(upper_buf_limit);

//   printf("After skip buffers queue size %u\n", fQueue.size());

   return true;
}
