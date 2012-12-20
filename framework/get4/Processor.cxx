#include "get4/Processor.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "base/ProcMgr.h"

#include "get4/SubEvent.h"

get4::Get4Rec::Get4Rec() :
   used(false),
   fChannels(0)
{
   for(unsigned n=0;n<NumChannels;n++) {
      fRisCoarseTm[n] = 0;
      fFalCoarseTm[n] = 0;
      fRisFineTm[n] = 0;
      fFalFineTm[n] = 0;
   }
}



get4::Processor::Processor(unsigned rocid, unsigned get4mask) :
   base::SysCoreProc("ROC", rocid),
   fIter(),
   fIter2()
{
   mgr()->RegisterProc(this, base::proc_RocEvent, rocid);

//   printf("Start histo creation\n");

   fMsgsKind = MakeH1("MsgKind", "kind of messages", 8, 0, 8, "xbin:NOP,-,EPOCH,SYNC,AUX,EPOCH2,GET4,SYS;kind");
   fSysTypes = MakeH1("SysTypes", "Distribution of system messages", 16, 0, 16, "systype");

   CreateBasicHistograms();

//   printf("Histo creation done\n");

   for (unsigned get4=0; (get4<16) && (get4mask!=0); get4++) {
      GET4.push_back(Get4Rec());
      GET4[get4].used = (get4mask & 1) == 1;
      get4mask = get4mask >> 1;
      if (!GET4[get4].used) continue;

      SetSubPrefix("GET4_", get4);

      GET4[get4].fChannels = MakeH1("Channels", "GET4 channels", 8, 0, 4., "ch");

      for(unsigned n=0;n<NumChannels;n++) {
         SetSubPrefix("GET4_", get4, "Ch", n);

         GET4[get4].fRisCoarseTm[n] = MakeH1("RisingCoarse", "rising edge coarse time", 4096, 0, 4096., "bin");

         GET4[get4].fRisFineTm[n] = MakeH1("RisingFine", "rising edge fine time", 128, 0, 128, "bin");

         GET4[get4].fFalCoarseTm[n] = MakeH1("FallingCoarse", "falling edge coarse time", 4096, 0, 4096., "bin");

         GET4[get4].fFalFineTm[n] = MakeH1("FallingFine", "falling fine time", 128, 0, 128, "bin");
      }
   }

   SetSubPrefix("");

//   printf("Histo creation done\n");

   SetNoSyncSource(); // use not SYNC at all
   SetNoTriggerSignal();

   // require at least 100 ns interval between two trigger candidates
   SetTriggerMargin(100);

   fRefId = 99; // impossible combination

   printf("Create GET4 processor for board %u\n", GetBoardId());

}

get4::Processor::~Processor()
{
   // printf("get4::Processor::~Processor\n");
}

void get4::Processor::setRefChannel(unsigned ref_get4, unsigned ref_ch, unsigned ref_edge)
{
   // keep all ids in one word
   fRefId = ref_get4 * 100 + ref_ch*10 + ref_edge;
}


void get4::Processor::AssignBufferTo(get4::Iterator& iter, const base::Buffer& buf)
{
   if (buf.null()) return;

   iter.setFormat(buf().format);
   iter.setRocNumber(buf().boardid);

   unsigned msglen = get4::Message::RawSize(buf().format);

   // we exclude last message which is duplication of SYNC
   iter.assign(buf().buf, buf().datalen - msglen);
}



bool get4::Processor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

   AssignBufferTo(fIter, buf);

   get4::Message& msg = fIter.msg();

   unsigned cnt(0), msgcnt(0);

   bool first = true;

//   printf("======== MESSAGES for board %u ==========\n", fRocId);


//   static int doprint = 1;
//   static double mymarker = 4311410447414.;

   while (fIter.next()) {

      cnt++;

      // ignore epoch message at the end of the buffer
      if (fIter.islast() && msg.isEpochMsg()) continue;

      unsigned rocid = msg.getRocNumber();

      if (rocid != GetBoardId()) {
         printf("Message from wrong ROCID %u, expected %u\n", rocid, GetBoardId());
         continue;
      }

      msgcnt++;

      FillH1(fMsgsKind, msg.getMessageType());


//      fIter.printMessage(roc::msg_print_Human);

      // keep time of first non-epoch message as start point of the buffer
      if (first && !msg.isEpochMsg()) {
         first = false;
         buf().local_tm = fIter.getMsgFullTimeD();
      }

      // this time used for histograming and print selection
      double msgtm = (fIter.getMsgFullTime() % 1000000000000LLU)*1e-9;

      FillH1(fALLt, msgtm);

      if (CheckPrint(msgtm, 1e-6))
         fIter.printMessage(base::msg_print_Human);

      switch (msg.getMessageType()) {

         case base::MSG_NOP:
            break;

         case base::MSG_HIT: {
            printf("FAILURE - no any nXYTER hits in GET4 processor!!!\n");
            exit(5);
            break;
         }

         case base::MSG_EPOCH: {
            break;
         }

         case base::MSG_GET4: {
            // must be ignored

            unsigned get4 = msg.getGet4Number();
            if (!get4_in_use(get4)) break;

            unsigned ch = msg.getGet4ChNum();
            unsigned edge = msg.getGet4Edge();
            unsigned ts = msg.getGet4Ts();
            uint64_t fulltm = fIter.getMsgFullTime();

            // fill rising and falling edges together
            FillH1(GET4[get4].fChannels, ch + edge*0.5);

            if (fRefId == get4 * 100 + ch*10 + edge) {
               base::LocalTriggerMarker marker;
               marker.localid = (get4+1) * 100 + ch*10 + edge;
               marker.localtm = fIter.getMsgFullTimeD();

               // printf("Select TRIGGER: %10.9f\n", marker.localtm*1e-9);

               AddTriggerMarker(marker);

            }

            if (edge==0) {
               FillH1(GET4[get4].fRisCoarseTm[ch], ts / 128);
               FillH1(GET4[get4].fRisFineTm[ch], ts % 128);
            } else {
               FillH1(GET4[get4].fFalCoarseTm[ch], ts / 128);
               FillH1(GET4[get4].fFalFineTm[ch], ts % 128);
            }


            break;
         }

         case base::MSG_EPOCH2: {
            break;
         }

         case base::MSG_SYNC: {
            unsigned sync_ch = msg.getSyncChNum();
            // unsigned sync_id = msg.getSyncData();

            FillH1(fSYNCt[sync_ch], msgtm);

            // for the moment use DABC format where SYNC is always second message
            if (sync_ch == fSyncSource) {

               base::SyncMarker marker;
               marker.uniqueid = msg.getSyncData();
               marker.localid = msg.getSyncChNum();
               marker.local_stamp = fIter.getMsgFullTime();
               marker.localtm = fIter.getMsgFullTimeD(); // nx gave us time in ns

               // printf("ROC%u Find sync %u tm %6.3f\n", rocid, marker.uniqueid, marker.localtm*1e-9);
               // if (marker.uniqueid > 11798000) exit(11);

               // marker.globaltm = 0.; // will be filled by the StreamProc
               // marker.bufid = 0; // will be filled by the StreamProc

               AddSyncMarker(marker);
            }

            if (fTriggerSignal == (10 + sync_ch)) {

               base::LocalTriggerMarker marker;
               marker.localid = 10 + sync_ch;
               marker.localtm = fIter.getMsgFullTimeD();

               AddTriggerMarker(marker);
            }

            break;
         }

         case base::MSG_AUX: {
            unsigned auxid = msg.getAuxChNum();

            FillH1(fAUXt[auxid], msgtm);

            if (fTriggerSignal == auxid) {

               base::LocalTriggerMarker marker;
               marker.localid = auxid;
               marker.localtm = fIter.getMsgFullTimeD();

               AddTriggerMarker(marker);
            }

            break;
         }

         case base::MSG_SYS: {
            FillH1(fSysTypes, msg.getSysMesType());
            break;
         }
      } // switch

   }

   FillMsgPerBrdHist(msgcnt);

   return true;
}

bool get4::Processor::SecondBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

//   printf("Start second buffer scan left %u  right %u  size %u\n", lefttrig, righttrig, fGlobalTrig.size());
//   for (unsigned n=0;n<fGlobalTrig.size();n++)
//      printf("TRIG %u %12.9f flush:%u\n", n, fGlobalTrig[n].globaltm*1e-9, fGlobalTrig[n].isflush);

   AssignBufferTo(fIter2, buf);

   get4::Message& msg = fIter2.msg();

   unsigned cnt(0);

   unsigned help_index = 0; // special index to simplify search of respective syncs

   while (fIter2.next()) {

      cnt++;

      // ignore epoch message at the end of the buffer
      if (fIter2.islast() && msg.isEpochMsg()) continue;

      unsigned rocid = msg.getRocNumber();

      if (rocid != GetBoardId()) continue;

//      printf("Scan message %u type %u\n", cnt, msg.getMessageType());

      switch (msg.getMessageType()) {

         case base::MSG_NOP:
            break;

         case base::MSG_SYNC:
         case base::MSG_AUX:
         case base::MSG_HIT:
            break;


         case base::MSG_GET4: {
            base::GlobalTime_t localtm = fIter2.getMsgFullTimeD();

            base::GlobalTime_t globaltm = LocalToGlobalTime(localtm, &help_index);

            unsigned indx = TestHitTime(globaltm, get4_in_use(msg.getGet4Number()));

            if (indx < fGlobalTrig.size())
               AddMessage(indx, (get4::SubEvent*) fGlobalTrig[indx].subev, get4::MessageExt(msg, globaltm));

            break;
         }

         case base::MSG_EPOCH:
            break;

         case base::MSG_EPOCH2:
            break;

         case base::MSG_SYS:
            break;
      } // switch

   }

   return true;
}
