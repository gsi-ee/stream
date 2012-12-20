#include "mbs/Processor.h"

#include <stdio.h>

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
   // SetSynchronisationRequired(false);

   // only raw scan, data can be immediately removed
   // SetRawScanOnly(true);

   fConv1.SetTimeSystem(7, 1.);
   fConv2.SetTimeSystem(7, 1.);

}

mbs::Processor::~Processor()
{
}

unsigned produce_fake_stamp(unsigned syncid)
{
    return (syncid - (2466426 - 100)) % 128;
   //return syncid;
}
   

bool mbs::Processor::FirstBufferScan(const base::Buffer& buf)
{
//   printf("Scan MBS buffer %u \n", buf().kind);

   if (buf().kind == base::proc_Triglog) {

      unsigned indx(0);

      uint32_t sync_num = buf.getuint32(indx++);

      uint32_t pattern = buf.getuint32(indx++);

      uint32_t mbssec = buf.getuint32(indx++);
      uint32_t mbsmsec = buf.getuint32(indx++);

      // rest is not interesting

      // double mbs_time = mbssec + mbsmsec*0.001;

      unsigned stamp = produce_fake_stamp(sync_num);
      fConv1.MoveRef(stamp);

      double localtm = fConv1.ToSeconds(stamp);

      printf("MBS SYNC %u stamp %u tm %12.1f  \n", sync_num, stamp, localtm);
      
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

      unsigned stamp = produce_fake_stamp(fLastSync1);

      buf().local_tm = fConv1.ToSeconds(stamp);
   }


   return true;
}

bool mbs::Processor::SecondBufferScan(const base::Buffer& buf)
{
   if (buf().kind == base::proc_Triglog) {
      fLastSync2 = buf.getuint32(0);
      unsigned stamp = produce_fake_stamp(fLastSync2);
      fConv2.MoveRef(stamp);

   }

   if (buf().kind == base::proc_CERN_Oct12) {
      // here we just need to decide which events has to have MBS part

      unsigned help_index(0);
      
      unsigned stamp = produce_fake_stamp(fLastSync2);

      double localtm = fConv2.ToSeconds(stamp);

      base::GlobalTime_t globaltm = LocalToGlobalTime(localtm, &help_index);

      unsigned indx = TestHitTime(globaltm, true);

      if (indx < fGlobalTrig.size()) {
         mbs::SubEvent* ev = (mbs::SubEvent*) fGlobalTrig[indx].subev;

         if (ev==0) {
            ev = new mbs::SubEvent;
            fGlobalTrig[indx].subev = ev;
         }

         // fill raw data here
         printf("Fill MBS data for trigger %u\n", indx);
      }

   }


   return true;
}
