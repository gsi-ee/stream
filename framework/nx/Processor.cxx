#include "nx/Processor.h"

#include <cstdlib>
#include <cstdio>
#include <cmath>

#include <algorithm>

#include "base/ProcMgr.h"

#include "nx/SubEvent.h"

double nx::Processor::fNXDisorderTm = 17e-6;
bool nx::Processor::fLastEpochCorr = false;

nx::Processor::Processor(unsigned rocid, unsigned nxmask, base::OpticSplitter* spl) :
   base::SysCoreProc("ROC%u", rocid),
   fIter1(),
   fIter2()
{
   // only if configured without splitter,
   // processor should subscribe for data

   if (!spl) {
      mgr()->RegisterProc(this, base::proc_RocEvent, rocid);
      mgr()->RegisterProc(this, base::proc_RawData, rocid);
   }

   fMsgsKind = MakeH1("MsgKind", "kind of messages", 8, 0, 8, "xbin:NOP,HIT,EPOCH,SYNC,AUX,-,-,SYS;kind");
   fSysTypes = MakeH1("SysTypes", "Distribution of system messages", 16, 0, 16, "systype");

   CreateBasicHistograms();

//   printf("Histo creation done\n");

   for (unsigned nx=0; (nx<8) && (nxmask!=0); nx++) {
      NX.push_back(NxRec());
      NX[nx].used = (nxmask & 1) == 1;
      nxmask = nxmask >> 1;
      if (!NX[nx].used) continue;

      SetSubPrefix("NX", nx);

      NX[nx].fChannels = MakeH1("Channels", "NX channels", NumChannels, 0, NumChannels, "ch");

      NX[nx].fADCs = MakeH2("ADC", "ADC distribution", NumChannels, 0., NumChannels, 1024, 0., 4096., "ch;adc");
      NX[nx].fHITt = MakeH1("HITt", "Hit time distribution", 10000, 0., 1000., "s");

      // NX[nx].corr_reset();
   }

   SetSubPrefix("");

//   printf("Histo creation done\n");


   SetSyncSource(0); // use for NX as default SYNC0 for synchronisation

   SetNoTriggerSignal();

   SetTriggerMargin(1e-7);

   fNumHits = 0;
   fNumBadHits = 0;
   fNumCorrHits = 0;

   if (IsLastEpochCorr()) {
      fIter1.SetCorrection(true, nxmask);
      fIter2.SetCorrection(true, nxmask);
   }
}

nx::Processor::~Processor()
{
   printf("%s   NumHits %9d  NumCorr %8d (%5.1f%s)  NumBad %8d (%5.3f%s)\n",
         GetName(), fNumHits,
         fNumCorrHits, fNumHits > 0 ? 100.* fNumCorrHits / fNumHits : 0., "%",
         fNumBadHits, fNumHits > 0 ? 100.* fNumBadHits / fNumHits : 0.,"%");

   //   printf("nx::Processor::~Processor\n");
}


bool nx::Processor::LoadTextBaseline(const std::string& fname)
{
   if (fname.empty()) return false;

   FILE * pedFile = fopen( fname.c_str(), "r" );
   if( ! pedFile ) {
      printf("Pedestal file %s not found\n", fname.c_str());
      return false;
   }

   unsigned roc, nx, ch;
   float ped, width;

   std::vector<unsigned> cnt(NX.size(), 0);

   while(fscanf( pedFile, "%u%u%u%f%f", &roc, &nx, &ch, &ped, &width ) == 5) {
      if (roc!= GetID()) continue;
      if ((nx>=NX.size()) || !NX[nx].used || ch>=NumChannels) continue;
      NX[nx].base_line[ch] = ped;
      cnt[nx]++;
      if (cnt[nx] == NumChannels) {
         printf("Fill base line for NX %u from ROC%u\n", nx, GetID());
         NX[nx].isbaseline = true;
      }
   }
   fclose(pedFile);

   return true;
}



void nx::Processor::AssignBufferTo(nx::Iterator& iter, const base::Buffer& buf)
{
   if (buf.null()) return;

   iter.setFormat(buf().format);
   iter.setRocNumber(buf().boardid);

   if (buf().kind == base::proc_RocEvent) {
      unsigned msglen = nx::Message::RawSize(buf().format);
      // we exclude last message which is duplication of SYNC
      iter.assign(buf().buf, buf().datalen - msglen);
   } else
      iter.assign(buf().buf, buf().datalen);
}



bool nx::Processor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

   AssignBufferTo(fIter1, buf);

   nx::Message& msg = fIter1.msg();

   unsigned cnt(0), msgcnt(0);

   bool first = true;

//   printf("======== MESSAGES for board %u ==========\n", GetID());


//   static int doprint = 0;
//   static double mymarker = 4311410447414.;

   while (fIter1.next()) {

      cnt++;

      // ignore epoch message at the end of the buffer
      if (fIter1.islast() && msg.isEpochMsg()) continue;

      unsigned rocid = msg.getRocNumber();

      if (rocid != GetID()) {
         printf("Message from wrong ROCID %u, expected %u\n", rocid, GetID());
         continue;
      }

      msgcnt++;

      FillH1(fMsgsKind, msg.getMessageType());

      double localtm = fIter1.getMsgTime();
      // this time used for histograming and print selection
      double msgtm = localtm - floor(localtm/1000.)*1000.;

      FillH1(fALLt, msgtm);

      int isok = 0;

      switch (msg.getMessageType()) {

         case base::MSG_NOP:
            break;

         case base::MSG_HIT: {

            FillH1(fHITt, msgtm);

            unsigned nxid = msg.getNxNumber();
            unsigned nxch = msg.getNxChNum();
            unsigned nxadc = msg.getNxAdcValue();

            if (!nx_in_use(nxid)) break;

            fNumHits++;

            if (fIter1.IsCorrection()) {
               isok = fIter1.LastCorrectionRes();
               if ((isok==-1) || (isok==-5)) fNumCorrHits++; else
               if (isok<0) fNumBadHits++;
            }

            FillH1(NX[nxid].fChannels, nxch);
            FillH2(NX[nxid].fADCs, nxch, nxadc);
            FillH1(NX[nxid].fHITt, msgtm);
            break;
         }

         case base::MSG_EPOCH2:
         case base::MSG_GET4: {
            // must be ignored
            printf("FAILURE - no any GET4 in nx processor!!!\n");
            exit(5);
            break;
         }

         case base::MSG_EPOCH: {
            break;
         }

         case base::MSG_SYNC: {
            unsigned sync_ch = msg.getSyncChNum();
            // unsigned sync_id = msg.getSyncData();

            FillH1(fSYNCt[sync_ch], msgtm);

            // use configured sync source
            if (sync_ch == fSyncSource) {

               base::SyncMarker marker;
               marker.uniqueid = msg.getSyncData();
               marker.localid = msg.getSyncChNum();
               marker.local_stamp = fIter1.getMsgStamp();
               marker.localtm = localtm; // nx gave us time in ns

               // printf("%s Find sync %u tm %12.9f\n", GetName(), marker.uniqueid, marker.localtm);
               // if (marker.uniqueid > 11798000) exit(11);

               // marker.globaltm = 0.; // will be filled by the StreamProc
               // marker.bufid = 0; // will be filled by the StreamProc

               AddSyncMarker(marker);
            }

            if (fTriggerSignal == (10 + sync_ch)) {

               base::LocalTimeMarker marker;
               marker.localid = 10 + sync_ch;
               marker.localtm = localtm;

               AddTriggerMarker(marker);
            }

            break;
         }

         case base::MSG_AUX: {
            unsigned auxid = msg.getAuxChNum();

            FillH1(fAUXt[auxid], msgtm);

//            printf("Find AUX%u\n", auxid);

            if (fTriggerSignal == auxid) {

               base::LocalTimeMarker marker;
               marker.localid = auxid;
               marker.localtm = localtm;

//               printf("ROC%u Find AUX%u tm %12.9f\n", rocid, auxid, marker.localtm);

               AddTriggerMarker(marker);
            }

            break;
         }

         case base::MSG_SYS: {
            FillH1(fSysTypes, msg.getSysMesType());
            break;
         }
      } // switch


      // keep time of first non-epoch message as start point of the buffer
      if (first && !msg.isEpochMsg()) {
         first = false;
         buf().local_tm = fIter1.getMsgTime(); // nx-based is always in ns

         // printf("Assign buf time %12.9f\n", buf().local_tm);
      }

      if (CheckPrint(msgtm, 0.00001)) {
         switch (isok) {
            case 1:  printf(" ok "); break;
            case -1: printf("err "); break;
            case -2: printf("nxt "); break;
            case -3: printf("oth "); break;
            case -4: printf("dff "); break;
            case -5: printf("msb "); break;
            default: printf("    "); break;
         }
         fIter1.printMessage(base::msg_print_Human);
      }
   }

   FillMsgPerBrdHist(msgcnt);

   return true;
}


bool nx::Processor::SecondBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

//   printf("Start second buffer scan left %u  right %u  size %u\n", lefttrig, righttrig, fGlobalMarks.size());
//   for (unsigned n=0;n<fGlobalMarks.size();n++)
//      printf("TRIG %u %12.9f flush:%u\n", n, fGlobalMarks.item(n).globaltm*1e-9, fGlobalMarks.item(n).isflush);

//   printf("Start second buffer scan\n");

   AssignBufferTo(fIter2, buf);

   nx::Message& msg = fIter2.msg();

   unsigned cnt = 0;

   unsigned help_index = 0; // special index to simplify search of respective syncs

   while (fIter2.next()) {

      cnt++;

      // ignore epoch message at the end of the buffer
      if (fIter2.islast() && msg.isEpochMsg()) continue;

      unsigned rocid = msg.getRocNumber();

      if (rocid != GetID()) continue;

//      printf("Scan message %u type %u\n", cnt, msg.getMessageType());

      switch (msg.getMessageType()) {

         case base::MSG_NOP:
            break;

         case base::MSG_SYNC:
         case base::MSG_AUX:
         case base::MSG_HIT: {

            // here is time in ns
            double localtm = fIter2.getMsgTime();

            float adc = 0.;

            unsigned nx = msg.getNxNumber();

            bool isnxmsg = msg.isHitMsg() && nx_in_use(nx);

            if (isnxmsg && NX[nx].isbaseline)
               adc = NX[nx].base_line[msg.getNxChNum()] - msg.getNxAdcValue();

            base::GlobalTime_t globaltm = LocalToGlobalTime(localtm, &help_index);

//            printf("%s localtm %12.9f globaltm %12.9f \n", GetName(), localtm, globaltm);

            unsigned indx = TestHitTime(globaltm, isnxmsg);

            if (indx < fGlobalMarks.size())
               AddMessage(indx, (nx::SubEvent*) fGlobalMarks.item(indx).subev, nx::MessageExt(msg, globaltm, adc));

            break;
         }

         case base::MSG_EPOCH: {
            break;
         }

         case base::MSG_SYS: {
            break;
         }

         case base::MSG_GET4:
         case base::MSG_EPOCH2: {
            // must be ignored
            printf("FAILURE - no any GET4 in nx processor!!!\n");
            exit(5);
            break;
         }

      } // switch

   }

   return true;
}
