#include "get4/MbsProcessor.h"

#include <stdlib.h>
#include <stdio.h>
#include <cmath>

#include "base/ProcMgr.h"


get4::MbsProcessor::MbsProcessor(unsigned get4mask, bool is32bit, unsigned totmult) :
   base::StreamProc("Get4Mbs", 0, false),
   fIs32mode(is32bit),
   fTotMult(totmult),
   fWriteCalibr(),
   fUseCalibr(false),
   fAutoCalibr(0)
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

      if (fIs32mode)
         GET4[get4].fChannels = MakeH1("Channels", "GET4 channels", 4, 0, 4, "channel");
      else
         GET4[get4].fChannels = MakeH1("Channels", "GET4 channels", 8, 0, 8, "xbin:0ris,0fal,1ris,1fal,2ris,2fal,3ris,3fal;kind");

      GET4[get4].fErrors = MakeH1("Errors", "GET4 errors", 24, 0, 24, "errid");

      for(unsigned n=0;n<NumGet4Channels;n++) {
         SetSubPrefix("G", get4, "Ch", n);

         // GET4[get4].fRisCoarseTm[n] = MakeH1("RisingCoarse", "rising edge coarse time", 4096, 0, 4096., "bin");

         GET4[get4].CH[n].fRisFineTm = MakeH1("RisingFine", "rising edge fine time", FineCounterBins, 0, FineCounterBins, "bin");
         GET4[get4].CH[n].fRisCal = MakeH1("RisingCal", "rising edge calibration", FineCounterBins, 0, FineCounterBins, "bin;ps");

         // GET4[get4].CH[n].fFalCoarseTm = MakeH1("FallingCoarse", "falling edge coarse time", 4096, 0, 4096., "bin");

         GET4[get4].CH[n].fFalFineTm = MakeH1("FallingFine", "falling fine time", FineCounterBins, 0, FineCounterBins, "bin");
         GET4[get4].CH[n].fFalCal = MakeH1("FallingCal", "falling edge calibration", FineCounterBins, 0, FineCounterBins, "bin;ps");

         GET4[get4].CH[n].fTotTm = MakeH1("ToT", "time-over-threshold", 1000, 0, 1000*BinWidthPs*fTotMult, "ps");
      }
   }

   SetSubPrefix("");

   printf("Create MBS-GET4 processor\n");
}

get4::MbsProcessor::~MbsProcessor()
{
   // printf("get4::MbsProcessor::~Processor\n");
   UserPostLoop();
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

   rec.fHist = MakeH1(hname, htitle, nbins, min, max, "ps");

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
      GET4[n].clearTimes();

   for (unsigned cnt=0; cnt < arrlen; cnt++) {

      uint32_t msg = arr[cnt];

      switch (msg >> 16) {
         case 0xabcd:
            get4id = 0xffff;
            continue;
         case 0xaaaa:
            get4id = 0; epoch = 0;
            continue;
         case 0xbbbb:
            get4id = 1; epoch = 0;
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


      unsigned msgid = fIs32mode ? ((msg >> 30) & 0x3) | 0x10 : (msg >> 22) & 0x3;

      switch (msgid) {
         case 0x0: { // epoch 24 bit
            epoch = (msg >> 1) & 0x1fffff;  // 21 bit
            //printf("epoch: %u\n", epoch);
            break;
         }
         case 0x10: { // epoch 32 bit
            epoch = (msg >> 1) & 0xffffff;  // 24 bit
            //printf("epoch: %u\n", epoch);
            break;
         }
         case 0x11: {  // slow control 32 bit mode

            //unsigned chid  = (msg >> 27) & 0x3;
            //unsigned edge  = (msg >> 26) & 0x1; // 0 - rising, 1 - falling
            //unsigned typ   = (msg >> 24) & 0x3;
            //unsigned data  = msg  & 0xffffff;

            break;
         }
         case 0x02:   // error 24 bit
         case 0x12: { // error 32 bit
            FillH1(fErrPerGet4, get4id);
            unsigned errid = msg & 0x7f;
            if (errid>19) errid = 23;
            FillH1(rec.fErrors, errid);
            break;
         }
         case 0x03: { // hit  24 bit
            unsigned chid = (msg >> 20) & 0x3;
            unsigned edge = (msg >> 19) & 0x1; // 0 - rising, 1 - falling
            unsigned fine = msg & 0x7f;
            unsigned stamp = msg & 0x7ffff;

            bool rising = (edge==0);

            Get4MbsChRec& chrec = rec.CH[chid];

            //printf("ch: %u %s stamp %5u\n", chid, (rising ? "R" : "F"), stamp);

            FillH1(rec.fChannels, chid*2+edge);
            FillH1(rising ? chrec.fRisFineTm : chrec.fFalFineTm, fine);

            if (rising) chrec.rising_stat[fine]++;
                   else chrec.falling_stat[fine]++;

            // ignore hits without epoch
            if (epoch==0) continue;

            uint64_t fullstamp = (((uint64_t) epoch) << 19) | stamp;

            double fulltm = 1.*BinWidthPs*fullstamp;

            if (fUseCalibr) {
               fulltm = 1.*BinWidthPs*(fullstamp & 0xffffffffffff80LLU);
               if (rising) fulltm += chrec.rising_calibr[fine];
                      else fulltm += chrec.falling_calibr[fine];
            }

            if (rising) {
               chrec.lastr = fulltm;
               if (chrec.firstr==0) chrec.firstr = fulltm;
            } else {
               chrec.lastf = fulltm;
               if (chrec.firstf==0) chrec.firstf = fulltm;

               if (chrec.lastr!=0)
                  FillH1(chrec.fTotTm, chrec.lastf - chrec.lastr);

               // printf("ch%u f:%8lu r:%8lu diff:%8f\n", chid, rec.lastf[chid], rec.lastr[chid], 0. + rec.lastf[chid] - rec.lastr[chid]);
            }

            break;
         }
         case 0x13: { // hit  32 bit
            unsigned chid = (msg >> 27) & 0x3;
            // unsigned dll = (msg >> 29) & 0x1; // is dll locked
            unsigned fine = (msg >> 8) & 0x7f;
            unsigned stamp = (msg >> 8) & 0x7ffff;
            unsigned tot = msg & 0xff;

            //printf("ch: %u %s stamp %5u\n", chid, (rising ? "R" : "F"), stamp);

            FillH1(rec.fChannels, chid);

            Get4MbsChRec& chrec = rec.CH[chid];

            FillH1(chrec.fRisFineTm, fine);
            FillH1(chrec.fFalFineTm, (fine + tot*fTotMult) % FineCounterBins);

            chrec.rising_stat[fine]++;
            chrec.falling_stat[(fine + tot*fTotMult) % FineCounterBins]++;

            // ignore hits without epoch
            if (epoch==0) continue;

            uint64_t fulltm = (((uint64_t) epoch) << 19) | stamp;

            chrec.lastr = 1.*BinWidthPs*fulltm;

            //static int mmm = 0;
            //if (++mmm<1000) printf("fine %3u  add %7.1f\n", fine, chrec.rising_calibr[fine]);

            if (fUseCalibr)
               chrec.lastr = 1.*BinWidthPs*(fulltm & 0xffffffffffff80LLU) + chrec.rising_calibr[fine];

            chrec.lastf = chrec.lastr + 1.*BinWidthPs*tot*fTotMult;

            if (chrec.firstr==0) chrec.firstr = chrec.lastr;
            if (chrec.firstf==0) chrec.firstf = chrec.lastf;

            FillH1(chrec.fTotTm, chrec.lastf - chrec.lastr);

            break;
         }

         default:
            printf("get wrong get4 message type 0x%02x - abort\n", msgid);
            return false;
      }
   }

   for (unsigned n=0;n<fRef.size();n++) {
      Get4MbsRef& rec = fRef[n];

      // printf("tm2 %u tm1 %u diff %f\n", GET4[rec.g2].gettm(rec.ch2, rec.r2), GET4[rec.g1].gettm(rec.ch1, rec.r1), 0.+ GET4[rec.g2].gettm(rec.ch2, rec.r2) - GET4[rec.g1].gettm(rec.ch1, rec.r1));

      FillH1(rec.fHist, GET4[rec.g2].CH[rec.ch2].gettm(rec.r2) - GET4[rec.g1].CH[rec.ch1].gettm(rec.r1));
   }

   if (fAutoCalibr>1000) ProduceCalibration(fAutoCalibr);

   return true;
}

bool get4::MbsProcessor::CalibrateChannel(unsigned get4id, unsigned nch, long* statistic, double* calibr, long stat_limit)
{
   double integral[FineCounterBins];
   double sum(0.);
   for (int n=0;n<FineCounterBins;n++) {
      sum += statistic[n];
      integral[n] = sum;
   }

   if (sum<stat_limit) {
      if ((sum>0) && (fAutoCalibr<=0))
         printf("get4:%2u ch:%u too few counts %5.0f for calibration of fine counter\n", get4id, nch, sum);
      return false;
   }


   for (unsigned n=0;n<FineCounterBins;n++) {

      calibr[n] = (integral[n]-statistic[n]/2.) / sum * BinWidthPs * FineCounterBins;

      // printf("  bin:%3u val:%7.2f\n", n, calibr[n]);
   }

   printf("get4:%2u ch:%u Cnts: %7.0f produce calibration\n", get4id, nch, sum);
   // automatically clear statistic
   for (unsigned n=0;n<FineCounterBins;n++) statistic[n] = 0;
   return true;
 }


void get4::MbsProcessor::ProduceCalibration(long stat_limit)
{
   // printf("%s produce channels calibrations\n", GetName());

   for (unsigned get4id=0;get4id<GET4.size();get4id++)
      for (unsigned ch=0;ch<NumGet4Channels;ch++) {

         if (!GET4[get4id].used) continue;
         Get4MbsChRec& rec = GET4[get4id].CH[ch];

         if (CalibrateChannel(get4id, ch, rec.rising_stat, rec.rising_calibr, stat_limit))
            CopyCalibration(rec.rising_calibr, rec.fRisCal);

         if (CalibrateChannel(get4id, ch, rec.falling_stat, rec.falling_calibr, stat_limit))
            CopyCalibration(rec.falling_calibr, rec.fFalCal);
      }

}

void get4::MbsProcessor::CopyCalibration(double* calibr, base::H1handle hcalibr)
{
   ClearH1(hcalibr);

   for (unsigned n=0;n<FineCounterBins;n++) {
      FillH1(hcalibr, n, calibr[n]);
   }
}


void get4::MbsProcessor::StoreCalibration(const std::string& fname)
{
   if (fname.empty()) return;

   FILE* f = fopen(fname.c_str(),"w");
   if (f==0) {
      printf("%s Cannot open file %s for writing calibration\n", GetName(), fname.c_str());
      return;
   }

   uint64_t num = GET4.size();

   fwrite(&num, sizeof(num), 1, f);

   for (unsigned get4id=0;get4id<GET4.size();get4id++)
      for (unsigned ch=0;ch<NumGet4Channels;ch++) {

         Get4MbsChRec& rec = GET4[get4id].CH[ch];

         fwrite(rec.rising_calibr, sizeof(rec.rising_calibr), 1, f);
         fwrite(rec.falling_calibr, sizeof(rec.falling_calibr), 1, f);

      }

   fclose(f);

   printf("%s storing calibration in %s\n", GetName(), fname.c_str());
}

bool get4::MbsProcessor::LoadCalibration(const std::string& fname)
{
   if (fname.empty()) return false;

   FILE* f = fopen(fname.c_str(),"r");
   if (f==0) {
      printf("Cannot open file %s for reading calibration\n", fname.c_str());
      return false;
   }

   uint64_t num(0);

   fread(&num, sizeof(num), 1, f);

   if (num != GET4.size()) {
      printf("%s mismatch of GET4 number in calibration file %u and in processor %u\n", GetName(), (unsigned) num, (unsigned) GET4.size());
   } else {
      for (unsigned get4id=0;get4id<GET4.size();get4id++)
         for (unsigned ch=0;ch<NumGet4Channels;ch++) {

            Get4MbsChRec& rec = GET4[get4id].CH[ch];
            fread(rec.rising_calibr, sizeof(rec.rising_calibr), 1, f);
            fread(rec.falling_calibr, sizeof(rec.falling_calibr), 1, f);

            CopyCalibration(rec.rising_calibr, rec.fRisCal);
            CopyCalibration(rec.falling_calibr, rec.fFalCal);
         }
   }

   fclose(f);

   printf("%s reading calibration from %s done\n", GetName(), fname.c_str());

   fUseCalibr = true;

   return true;
}


void get4::MbsProcessor::SetAutoCalibr(long stat_limit)
{
   fAutoCalibr = stat_limit;
   fUseCalibr = true;

   // initialize with linear calibration, immediately allow to use calibration

   for (unsigned get4id=0;get4id<GET4.size();get4id++)
      for (unsigned ch=0;ch<NumGet4Channels;ch++) {
         Get4MbsChRec& rec = GET4[get4id].CH[ch];
         for (unsigned bin=0;bin<FineCounterBins;bin++) {
            rec.rising_calibr[bin] = 1.*BinWidthPs * bin;
            rec.falling_calibr[bin] = 1.*BinWidthPs * bin;
         }
         CopyCalibration(rec.rising_calibr, rec.fRisCal);
         CopyCalibration(rec.falling_calibr, rec.fFalCal);
      }
}


void get4::MbsProcessor::UserPreLoop()
{
   // LoadCalibration("test.cal");
}


void get4::MbsProcessor::UserPostLoop()
{
   static int first = 1;

   if (first!=1) return;
   first=0;

   printf("************************* hadaq::TdcProcessor postloop *******************\n");

   if (fWriteCalibr.length()>0) {
      ProduceCalibration(100000);
      StoreCalibration(fWriteCalibr);
   }

   //for (unsigned n=0;n<fRef.size();n++) {
   //   TH1* hist = (TH1*) fRef[n].fHist;
   //   if (hist==0) continue;
   //   printf("%s RMS = %5.1f\n", hist->GetName(), hist->GetRMS());
   //}
}
