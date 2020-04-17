#include "mbs/Processor.h"

#include <cstdio>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "mbs/SubEvent.h"


mbs::Processor::Processor() :
   base::StreamProc("MBS")
{

   mgr()->RegisterProc(this, base::proc_Triglog, 0);

   mgr()->RegisterProc(this, base::proc_CERN_Oct12, 1);

   fLastSync1 = 0;
   fLastSync2 = 0;

   // this is raw-scan processor, therefore no synchronization is required for it
   SetSynchronisationKind(sync_None);

   // only raw scan, data can be immediately removed
   // SetRawScanOnly();

   // LocalStampConverter used just to emulate time scale.
   //  Every sync is just new second and LocalStampConverter prevents wrap
   //   of such artificial time scale
   // Some time scale is required to let framework decide which buffer can be processed

   fConv1.SetTimeSystem(24, 1.);
}

mbs::Processor::~Processor()
{
}

bool mbs::Processor::FirstBufferScan(const base::Buffer& buf)
{
//   printf("Scan MBS buffer %u \n", buf().kind);

   if (buf().kind == base::proc_Triglog) {

      unsigned indx(0);

      uint32_t sync_num = buf.getuint32(indx++);

//      uint32_t pattern = buf.getuint32(indx++);
//      uint32_t mbssec = buf.getuint32(indx++);
//      uint32_t mbsmsec = buf.getuint32(indx++);

      // rest is not interesting

      // double mbs_time = mbssec + mbsmsec*0.001;

      fConv1.MoveRef(sync_num);
      double localtm = fConv1.ToSeconds(sync_num);

//      printf("MBS SYNC %u tm %12.1f  \n", sync_num, localtm);
      
      base::SyncMarker marker;
      marker.uniqueid = sync_num;
      marker.localid = 0;
      marker.local_stamp = localtm; // use SYNC number itself
      marker.localtm = localtm;

      AddSyncMarker(marker);

      fLastSync1 = sync_num;

      buf().local_tm = localtm; // use SYNC number as local time
   }

   if (buf().kind == base::proc_CERN_Oct12) {
      // here not interested at all about content
      // just assign time stamp

      buf().local_tm = fConv1.ToSeconds(fLastSync1);
   }


   return true;
}

bool mbs::Processor::SecondBufferScan(const base::Buffer& buf)
{
   if (buf().kind == base::proc_Triglog) {
      fLastSync2 = buf.getuint32(0);
      // fConv2.MoveRef(fLastSync2);
   }

   if (buf().kind == base::proc_CERN_Oct12) {
      // here we just need to decide which events has to have MBS part

//      unsigned help_index(0);

//      double localtm = fConv2.ToSeconds(fLastSync2);

      unsigned myid = findSyncWithId(fLastSync2);
      if (myid==numSyncs()) {
         printf("Something happened - we could not locate SYNC message %u for MBS\n", fLastSync2);
         return false;
      }

      base::GlobalTime_t globaltm = getSync(myid).globaltm;

//      base::GlobalTime_t globaltm = LocalToGlobalTime(localtm, &help_index);

//      printf("CHECK sync %u  tm1 %12.9f tm2 %12.9f  numsyncs %u\n", fLastSync2, synctm, globaltm, numSyncs());
//      for (unsigned cnt=0;cnt<numSyncs();cnt++)
//         printf("    SYNC%03u  id:%u  tm:%12.9f\n", cnt, getSync(cnt).uniqueid, getSync(cnt).globaltm);

      unsigned indx = TestHitTime(globaltm, true);

      if (indx < fGlobalMarks.size()) {
         mbs::SubEvent* ev = (mbs::SubEvent*) fGlobalMarks.item(indx).subev;

         if (ev==0) {
            ev = new mbs::SubEvent;
            fGlobalMarks.item(indx).subev = ev;
         }

         // fill raw data here
//         printf("Fill MBS data for trigger %u  sync %u global %12.9f \n", indx, fLastSync2, globaltm);
      }

   }

   return true;
}
