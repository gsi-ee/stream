#include "get4/MbsProcessor.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "base/ProcMgr.h"


get4::MbsProcessor::MbsProcessor(unsigned get4mask) :
   base::StreamProc("Get4Mbs", 0, false)
{
   SetRawScanOnly();

   mgr()->RegisterProc(this, 1, 0);

//   printf("Start histo creation\n");

   fMsgPerGet4 = MakeH1("MsgPerGet4", "Message counts per Get4", 16, 0, 16, "get4");
   fErrPerGet4 = MakeH1("ErrPerGet4", "Errors per Get4", 16, 0, 16, "get4");

//   fSlMsgPerGet4  = MakeH2("SlPerGet4",  "Distribution of Slow control messages between Get4", 16, 0, 16, 4, 0, 4, "get4; SL type");
//   fScalerPerGet4 = MakeH2("ScalerGet4", "Distribution of SL Scaler values between Get4", 16, 0, 16, 4096, 0, 8192, "get4");
//   fDeadPerGet4   = MakeH2("DeadTmGet4", "Distribution of Dead Time values between Get4", 16, 0, 16, 2048, 0, 4096, "get4");
//   fSpiPerGet4    = MakeH2("SpiOutGet4", "Distribution of SPI Out between Get4", 16, 0, 16, 820, 0, 8200, "get4");
//   fSeuPerGet4    = MakeH2("SeuScaGet4", "Distribution of SEU scaler values between Get4", 16, 0, 16, 410, 0, 410, "get4; SEU");
//   fDllPerGet4    = MakeH2("DllPerGet4", "DLL flag values for all hits, Get4 as unit, ch as sub-unit", 4*16, 0, 16, 2, 0, 2, "get4.ch; DLL Flag");

//   printf("Histo creation done\n");

   for (unsigned get4=0; (get4<16) && (get4mask!=0); get4++) {
      GET4.push_back(Get4MbsRec());
      GET4[get4].used = (get4mask & 1) == 1;
      get4mask = get4mask >> 1;
      if (!GET4[get4].used) continue;

      SetSubPrefix("G", get4);

      GET4[get4].fChannels = MakeH1("Channels", "GET4 channels", 8, 0, 8, "xbin:0ris,0fal,1ris,1fal,2ris,2fal,3ris,3fal;kind");
      GET4[get4].fErrors = MakeH1("Errors", "GET4 errors", 24, 0, 24, "errid");

      for(unsigned n=0;n<NumGet4Channels;n++) {
         SetSubPrefix("G", get4, "Ch", n);

         // GET4[get4].fRisCoarseTm[n] = MakeH1("RisingCoarse", "rising edge coarse time", 4096, 0, 4096., "bin");

         GET4[get4].fRisFineTm[n] = MakeH1("RisingFine", "rising edge fine time", 128, 0, 128, "bin");

         // GET4[get4].fFalCoarseTm[n] = MakeH1("FallingCoarse", "falling edge coarse time", 4096, 0, 4096., "bin");

         GET4[get4].fFalFineTm[n] = MakeH1("FallingFine", "falling fine time", 128, 0, 128, "bin");

         GET4[get4].fTotTm[n] = MakeH1("ToT", "time-over-threshold", 1024, 0, 1024, "bin");
      }
   }

   SetSubPrefix("");

   printf("Create MBS-GET4 processor\n");
}

get4::MbsProcessor::~MbsProcessor()
{
   // printf("get4::MbsProcessor::~Processor\n");
}

bool get4::MbsProcessor::AddRef(unsigned g2, unsigned ch2, bool r2,
                                unsigned g1, unsigned ch1, bool r1,
                                int nbins, double min, double max)
{
   if (!get4_in_use(g1) || !get4_in_use(g2)) return false;
   if ((ch1>=NumGet4Channels) || (ch2>=NumGet4Channels)) return false;

   char hname[100], htitle[100];

   fRef.push_back(Get4MbsRef());

   Get4MbsRef& rec = fRef[fRef.size()-1];

   rec.g1 = g1;
   rec.g2 = g2;
   rec.ch1 = ch1;
   rec.ch2 = ch2;
   rec.r1 = r1;
   rec.r2 = r2;

   sprintf(hname,"G%u_Ch%u_%s_minus_G%u_Ch%u_%s", g2, ch2, (r2 ? "R" : "F"), g1, ch1, (r1 ? "R" : "F"));

   sprintf(htitle,"G%u Ch%u %s - G%u Ch%u %s", g2, ch2, (r2 ? "R" : "F"), g1, ch1, (r1 ? "R" : "F"));

   rec.fHist = MakeH1(hname, htitle, nbins, min, max);

   return true;
}


bool get4::MbsProcessor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

   uint32_t* arr = (uint32_t*) buf.ptr();

   unsigned arrlen = buf.datalen() / 4;
   unsigned get4id = 0xffff;
   unsigned epoch(0);

   // printf("analyze buffer %u\n", arrlen);

   for (unsigned n=0;n<GET4.size();n++)
      GET4[n].clear();

   for (unsigned cnt=0; cnt < arrlen; cnt++) {

      uint32_t msg = arr[cnt];

      switch (msg >> 16) {
         case 0xabcd:
            get4id = 0xffff;
            continue;
         case 0xaaaa:
            get4id = 0;
            continue;
         case 0xbbbb:
            get4id = 1;
            continue;
         case 0xcccc:
            get4id = 0xffff;
            continue;
      }

      if (get4id >= GET4.size()) {
         printf("Get message without get4 id specified\n");
         return false;
      }

      FillH1(fMsgPerGet4, get4id);

      Get4MbsRec& rec = GET4[get4id];
      if (!rec.used) continue;

      // printf("analyze message 0x%08x\n", msg);

      switch (msg >> 22) {
         case 0x0: { // epoch
            epoch = (msg >> 1) & 0xffffff;
            //printf("epoch: %u\n", epoch);
            break;
         }
         case 0x2: { // error
            FillH1(fErrPerGet4, get4id);
            unsigned errid = msg & 0x7f;
            if (errid>19) errid = 23;
            FillH1(rec.fErrors, errid);
            break;
         }
         case 0x3: { // hit
            unsigned chid = (msg >> 20) & 0x3;
            unsigned edge = (msg >> 19) & 0x1; // 0 - rising, 1 - falling
            unsigned fine = msg & 0x7f;
            unsigned stamp = msg & 0x7ffff;

            bool rising = (edge==0);

            //printf("ch: %u %s stamp %5u\n", chid, (rising ? "R" : "F"), stamp);

            FillH1(rec.fChannels, chid*2+edge);
            FillH1(rising ? rec.fRisFineTm[chid] : rec.fFalFineTm[chid], fine);

            uint64_t fulltm = (((uint64_t) epoch) << 19) | stamp;

            if (rising) {
               rec.lastr[chid] = fulltm;
               if (rec.firstr[chid]==0) rec.firstr[chid] = fulltm;
            } else {
               rec.lastf[chid] = fulltm;
               if (rec.firstf[chid]==0) rec.firstf[chid] = fulltm;

               if (rec.lastr[chid]!=0)
                  FillH1(rec.fTotTm[chid], rec.lastf[chid] - rec.lastr[chid]);

               // printf("ch%u f:%8lu r:%8lu diff:%8f\n", chid, rec.lastf[chid], rec.lastr[chid], 0. + rec.lastf[chid] - rec.lastr[chid]);
            }

            break;
         }
         default:
            printf("get wrong get4 message type %u - abort\n", (unsigned) msg >> 22);
            return false;
      }

   }

   for (unsigned n=0;n<fRef.size();n++) {
      Get4MbsRef& rec = fRef[n];

      // printf("tm2 %u tm1 %u diff %f\n", GET4[rec.g2].gettm(rec.ch2, rec.r2), GET4[rec.g1].gettm(rec.ch1, rec.r1), 0.+ GET4[rec.g2].gettm(rec.ch2, rec.r2) - GET4[rec.g1].gettm(rec.ch1, rec.r1));

      FillH1(rec.fHist, 0. + GET4[rec.g2].gettm(rec.ch2, rec.r2) - GET4[rec.g1].gettm(rec.ch1, rec.r1));
   }

   return true;
}

