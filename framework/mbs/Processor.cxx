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
}

mbs::Processor::~Processor()
{
}

unsigned calc_local_tm(unsigned syncid) 
{
   // return (syncid - (2466426 - 100)) % 128;
   return syncid;
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

      double mbs_time = mbssec + mbsmsec*0.001;

      unsigned localtm = calc_local_tm(sync_num);

      printf("MBS SYNC %u mod %u  time %u.%u\n", sync_num, localtm, mbssec, mbsmsec);

      
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

      buf().local_tm = calc_local_tm(fLastSync1);
   }


   return true;
}

bool mbs::Processor::SecondBufferScan(const base::Buffer& buf)
{
   if (buf().kind == base::proc_Triglog) {
      fLastSync2 = buf.getuint32(0);
   }

   if (buf().kind == base::proc_CERN_Oct12) {
      // here we just need to decide which events has to have MBS part

      unsigned help_index(0);
      
      double localtm = calc_local_tm(fLastSync2);

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
