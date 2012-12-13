#include "nx/Processor.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <algorithm>

#include "base/ProcMgr.h"

#include "nx/SubEvent.h"

double nx::Processor::fNXDisorderTm = 17000.;
bool nx::Processor::fLastEpochCorr = false;

/*
void nx::NxRec::corr_reset()
{
   lasthittm = 0;
   reversecnt = 0;

   for (unsigned n=0;n<128;n++) lasthit[n] = 0;
   lastch = 0;
   lastepoch = 0;

   lastfulltm = 0;
}


void nx::NxRec::corr_nextepoch(unsigned epoch)
{
   reversecnt = 0;

   bool nextepoch = (epoch == lastepoch+1);

   for (unsigned n=0;n<128;n++)
      if ((lasthit[n]>=16384) && nextepoch)
         lasthit[n]-=16384;
      else
         lasthit[n]=0;

   lastepoch = epoch;
}

int nx::NxRec::corr_nexthit_old(nx::Message& msg, bool docorr)
{
   // return 0 - no last epoch, 1 - last epoch is ok, -1 - last epoch err, -2 - next epoch error

   if (msg.getNxLastEpoch()) {
      if ((reversecnt>0) && (reversecnt>lasthittm)) return 1;

      if (reversecnt==0) {
         if (msg.getNxTs()>16340) return 1;
         if ((lasthittm>16000) && (msg.getNxTs()>16000)) return 1;
      }

      if (docorr) msg.setNxLastEpoch(0);
      return -1;
   } else {

      if ((msg.getNxTs() < lasthittm) && (reversecnt>0)) {
         if ((lasthittm>2*reversecnt) && (msg.getNxTs() < lasthittm - 2*reversecnt)) return -2;
      }

      reversecnt+=32;

      lasthittm = msg.getNxTs();
      // msg.getNxChNum();
   }

   return 0;
}


int nx::NxRec::corr_nexthit_next(nx::Message& msg, bool docorr)
{
   // check time stamp three times
   // first as it is, is ok - done,
   // second - if lastepoch is set and we try without it or lastepoch is not set but we suppose next epoch

   // return 0 - no last epoch,
   //        1 - last epoch is ok,
   //       -1 - last epoch err,
   //       -2 - next epoch error
   //       -3 - any other error

   bool normalhit = false;
   bool correctedhit = false;

   unsigned hitts(0);

   for (int numtry = 0; numtry<2; numtry++) {

      hitts = msg.getNxTs();

      if (numtry==0) {
         if (!msg.getNxLastEpoch()) hitts += 16384;
      } else {
         if (msg.getNxLastEpoch()) hitts += 16384;
                              else hitts += 16384*2;
      }

      unsigned id = (msg.getNxChNum() - 1) % 128;
      unsigned maxdiff = 0;
      unsigned loop_time = 0;
      while (id != msg.getNxChNum()) {
         if (lasthit[id]!=0) loop_time+=32;
         if (hitts < lasthit[id]) {
            unsigned diff = lasthit[id] - hitts;
            if (diff > maxdiff) maxdiff = diff;
         }
         id = (id - 1) % 128;
      }

      if ((numtry==0) && (maxdiff <= loop_time)) {
         // if last-epoch set, but full loop time less than distance to epoch, than error
         if (msg.getNxLastEpoch() && (loop_time + 32 < (16384 - hitts))) continue;

         normalhit = true;
         break;
      }

      if ((maxdiff <= loop_time) && (numtry==1)) {
         correctedhit = true;
         break;
      }
   }

   int newres = 0;

   if (normalhit || correctedhit) {

      lastch = (lastch + 1) % 128;

      while (lastch != msg.getNxChNum()) {
         lasthit[lastch] = 0;
         lastch = (lastch + 1) % 128;
      }

      lasthit[lastch] = hitts;
   } else {
      if (msg.getNxLastEpoch()) newres = -1; else newres = -3;
   }

   if (normalhit && msg.getNxLastEpoch()) newres = 1;
   if (correctedhit && msg.getNxLastEpoch()) newres = -1;
   if (correctedhit && !msg.getNxLastEpoch()) newres = -2;

   int oldres = corr_nexthit_old(msg, docorr);

   return oldres;

//   if (oldres != newres) newres = -4;
//   return newres;
}

*/

int nx::NxRec::corr_nexthit(nx::Message& msg, uint64_t fulltm, bool docorr, bool firstscan)
{
   // try to make as simple as possible
   // if last-epoch specified, one should check FIFO fill status

   // return 1 - last epoch is ok,
   //        0 - no last epoch,
   //       -1 - last epoch err,
   //       -2 - next epoch error
   //       -3 - any other error
   //       -5 - last epoch err, MSB err


   uint64_t& lastfulltm = firstscan ? lastfulltm1 : lastfulltm2;

   int res = 0;

   if (msg.getNxLastEpoch()) {

      // do not believe that message can have last-epoch marker
      if (msg.getNxTs()<15800) res = -1; else
      // fifo fill status should be equal to 1 - most probable situation
      if (((msg.getNxLtsMsb()+7) & 0x7) != ((msg.getNxTs()>>11)&0x7)) res = -5; else
      // if message too far in past relative to previous
      if (fulltm + 800 < lastfulltm) res = -1; else
      // we decide that lastepoch bit is set correctly
      res = 1;

   } else {

      // imperical 800 ns value
      if (fulltm + 800 < lastfulltm) {
         if (msg.getNxTs() < 30) res = -2;
                            else res = -3;
      }
   }

   lastfulltm = fulltm;
   if ((res==-1) || (res==-5)) {
      lastfulltm += 16*1024;
      if (docorr) msg.setNxLastEpoch(0);
   }

   return res;
}



nx::Processor::Processor(unsigned rocid, unsigned nxmask) :
   base::SysCoreProc("ROC", rocid),
   fIter(),
   fIter2()
{
   mgr()->RegisterProc(this, base::proc_RocEvent, rocid);

//   printf("Start histo creation\n");

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

      NX[nx].fChannels = MakeH1("Channels", "NX channels", 128, 0, 128, "ch");

      NX[nx].fADCs = MakeH2("ADC", "ADC distribution", 128, 0., 128., 1024, 0., 4096., "ch;adc");
      NX[nx].fHITt = MakeH1("HITt", "Hit time distribution", 10000, 0., 1000., "s");

      // NX[nx].corr_reset();
   }

   SetSubPrefix("");

//   printf("Histo creation done\n");


   SetSyncSource(0); // use for NX as default SYNC0 for synchronisation

   SetNoTriggerSignal();

   fNumHits = 0;
   fNumBadHits = 0;
   fNumCorrHits = 0;
}

nx::Processor::~Processor()
{
   printf("%s   NumHits %9d  NumCorr %8d (%5.1f%s)  NumBad %8d (%5.3f%s)\n",
         GetProcName().c_str(), fNumHits,
         fNumCorrHits, fNumHits > 0 ? 100.* fNumCorrHits / fNumHits : 0., "%",
         fNumBadHits, fNumHits > 0 ? 100.* fNumBadHits / fNumHits : 0.,"%");

   //   printf("nx::Processor::~Processor\n");
}

void nx::Processor::AssignBufferTo(nx::Iterator& iter, const base::Buffer& buf)
{
   if (buf.null()) return;

   iter.setFormat(buf().format);
   iter.setRocNumber(buf().boardid);

   unsigned msglen = nx::Message::RawSize(buf().format);

   // we exclude last message which is duplication of SYNC
   iter.assign(buf().buf, buf().datalen - msglen);
}



bool nx::Processor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

   AssignBufferTo(fIter, buf);

   nx::Message& msg = fIter.msg();

   unsigned cnt(0), msgcnt(0);

   bool first = true;

//   printf("======== MESSAGES for board %u ==========\n", GetBoardId());


//   static int doprint = 0;
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

      uint64_t fulltm = fIter.getMsgFullTime();

      // this time used for histograming and print selection
      double msgtm = (fulltm % 1000000000000LLU)*1e-9;

      FillH1(fALLt, msgtm);

      int isok = 0;

      switch (msg.getMessageType()) {

         case base::MSG_NOP:
            break;

         case base::MSG_HIT: {
            unsigned nxid = msg.getNxNumber();
            unsigned nxch = msg.getNxChNum();
            unsigned nxadc = msg.getNxAdcValue();

            if (!nx_in_use(nxid)) break;

            fNumHits++;

            if (IsLastEpochCorr()) {
               isok = NX[nxid].corr_nexthit(msg, fulltm, true, true);
               if ((isok==-1) || (isok==-5)) fNumCorrHits++; else
               if (isok<0) fNumBadHits++;
            }

            FillH1(NX[nxid].fChannels, nxch);
            FillH2(NX[nxid].fADCs, nxch, nxadc);
            FillH1(NX[nxid].fHITt, msgtm);

            /*

            bool data_hit(true), ped_hit(false);

            if( data_hit ) {
               fNxTm[nxid][nxch] = fulltm;

               fHITt[nxid]->Fill((msgtm % 10000) * 0.1);

               fChs[nxid]->Fill(nxch);
               fADCs[nxid]->Fill(nxch, nxadc);
               if (fParam->baselineCalibr) {
                  nxadc_corr = fPedestals->GetPedestal(rocid, nxid, nxch) - nxadc;
                  ROC[rocid].fADCs_wo_baseline[nxid]->Fill(nxch, nxadc_corr);
               } else {
                  nxadc_corr = 4095 - nxadc;
               }
            }

            if( ped_hit ) {
                ROC[rocid].fBaseline[nxid]->Fill(nxch, nxadc);
            }
*/
            break;
         }

         case base::MSG_GET4: {
            // must be ignored
            printf("FAILURE - no any GET4 in nx processor!!!\n");
            exit(5);
            break;
         }

         case base::MSG_EPOCH: {
//            if (IsLastEpochCorr())
//               for (unsigned nx=0;nx<NX.size();nx++)
//                  NX[nx].corr_nextepoch(msg.getEpochNumber());

            break;
         }

         case base::MSG_EPOCH2: {
            printf("FAILURE - no any GET4 EPOCHs in nx processor!!!\n");
            exit(5);
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
               marker.local_stamp = fulltm;
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
               marker.localtm = fulltm;

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
               marker.localtm = fulltm;

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
         buf().local_tm = fIter.getMsgFullTimeD(); // nx-based is always in ns
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
         fIter.printMessage(base::msg_print_Human);
      }
   }

   FillMsgPerBrdHist(msgcnt);

   return true;
}

unsigned nx::Processor::GetTriggerMultipl(unsigned indx)
{
   nx::SubEvent* ev = (nx::SubEvent*) fGlobalTrig[indx].subev;

   return ev ? ev->fExtMessages.size() : 0;
}


bool nx::Processor::SecondBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

//   printf("Start second buffer scan left %u  right %u  size %u\n", lefttrig, righttrig, fGlobalTrig.size());
//   for (unsigned n=0;n<fGlobalTrig.size();n++)
//      printf("TRIG %u %12.9f flush:%u\n", n, fGlobalTrig[n].globaltm*1e-9, fGlobalTrig[n].isflush);

   AssignBufferTo(fIter2, buf);

   nx::Message& msg = fIter2.msg();

   unsigned cnt = 0;

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
         case base::MSG_HIT: {

            // here is time in ns anyway, use it as it is
            uint64_t stamp = fIter2.getMsgFullTime();

            bool isnxmsg = msg.isHitMsg() && nx_in_use(msg.getNxNumber());

            if (isnxmsg && IsLastEpochCorr())
               NX[msg.getNxNumber()].corr_nexthit(msg, stamp, true, false);

            base::GlobalTime_t globaltm = LocalToGlobalTime(stamp, &help_index);

            unsigned indx = TestHitTime(globaltm, isnxmsg);

            if ((indx < fGlobalTrig.size()) && !fGlobalTrig[indx].isflush) {
               nx::SubEvent* ev = (nx::SubEvent*) fGlobalTrig[indx].subev;

               if (ev==0) {
                  ev = new nx::SubEvent;
                  fGlobalTrig[indx].subev = ev;
//                     printf("Create new event %u\n", cnt);
               }

               ev->fExtMessages.push_back(nx::MessageExtended(msg, globaltm));
            }
            break;
         }

         case base::MSG_GET4: {
            // must be ignored
            printf("FAILURE - no any GET4 in nx processor!!!\n");
            exit(5);
            break;
         }

         case base::MSG_EPOCH: {
            break;
         }

         case base::MSG_EPOCH2: {
            printf("FAILURE - no any GET4 EPOCHs in nx processor!!!\n");
            exit(5);
            break;
         }

         case base::MSG_SYS: {
            break;
         }
      } // switch

   }

   return true;
}

void nx::Processor::SortDataInSubEvent(base::SubEvent* subev)
{
   nx::SubEvent* nxsub = (nx::SubEvent*) subev;

   std::sort(nxsub->fExtMessages.begin(), nxsub->fExtMessages.end());
}

