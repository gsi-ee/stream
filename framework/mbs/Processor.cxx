#include "mbs/Processor.h"

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

      printf("MBS SYNC %u mod %u  time %u.%u\n", sync_num, sync_num % 4096, mbssec, mbsmsec);

      double mbs_time = mbssec + mbsmsec*0.001;

      base::SyncMarker marker;
      marker.uniqueid = sync_num;
      marker.localid = 0;
      marker.local_stamp = sync_num; // use SYNC number itself
      marker.localtm = sync_num;

      AddSyncMarker(marker);

      fLastSync1 = sync_num;

      buf().local_tm = fLastSync1; // use SYNC number as local time
   }

   if (buf().kind == base::proc_CERN_Oct12) {
      // here not interested at all about content
      // just assign time stamp

      buf().local_tm = fLastSync1;
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

      base::GlobalTime_t globaltm = LocalToGlobalTime(fLastSync2, &help_index);

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
