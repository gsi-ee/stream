#include <stdio.h>
#include <math.h>
#include "TH1.h"
#include "TFile.h"
#include "TCanvas.h"

#define NumTDC 4

#define NumChannels 64

const unsigned TDCids[NumTDC] = { 0x940, 0x941, 0x942, 0x943 };

const char* filenames[] = { "res/40C-1.root",
                            "res/50C-1.root",
                            "res/42C-1.root",
                            "res/44C-1.root",
                            "res/46C-1.root",
                            "res/48C-1.root",
                             0 };

bool BadChannel(unsigned ch) { return (ch==30) || (ch==37); }


TH1* GetHist(TFile* f, const char* fullname) {
   TObject* obj = f->Get(fullname);

   TH1* h1 = dynamic_cast<TH1*> (obj);
   if (h1!=0) h1->SetDirectory(0);

   return h1;
}

double CalcK(int nch, TH1* h1, TH1* h2, double& weight, bool debug = false)
{
   double sum0 = 0, sum1 = 0, sum2 = 0, dsum1 = 0, dsum2 = 0;

   for (int n=100;n<=600;n++) {
      Double_t v1 = h1->GetBinContent(n);
      Double_t v2 = h2->GetBinContent(n);
      if ((v1 < 1) || (v2 < 1)) continue;

      if ((v1 >= 4998.5) || (v2 > 4998.5)) break;

      double k = v2/v1 - 1.;
      double d = v2 - v1;

      sum0 += 1.;
      sum1 += k;
      sum2 += k*k;
      dsum1 += d;
      dsum2 += d*d;
   }

   double mean = (sum0>0) ? sum1 / sum0 : 0.;
   double dmean = (sum0>0) ? dsum1 / sum0 : 0.;

   double sigma = (sum0>0) ? sqrt(sum2 / sum0 - mean*mean) : 0.;
   double dsigma = (sum0>0) ? sqrt(dsum2 / sum0 - dmean*dmean) : 0.;

   if (debug) {
      double relsigma = 99.9;
      if (fabs(mean) > 0) relsigma = sigma/fabs(mean)*100.;
      printf(" Ch:%2d mean:%8.5f Sigma = %6.5f %7.1f%s sum0:%3.0f  shft:%6.2f  sigm:%5.3f\n", nch, mean, sigma, relsigma, "%", sum0, dmean, dsigma);
   }

   // if (sigma > 0.5*fabs(mean)) { weight = 0; return 0; }

   weight = 1/sigma;

   return mean;
}

void MakeDivide(TH1* h1, TH1* h2) {
   h1->Divide(h2);
   for (int n=1;n<=600;n++) {
      if (h1->GetBinContent(n) == 0) h1->SetBinContent(n,1);
   }

   // h1->Draw();
}

bool GetMeanRms(TH1* h1, double &mean, double &rms, double cut = 0.01) {
   mean = 0.;
   rms = 0.;

   if (h1==0) return false;

   int nbins = h1->GetNbinsX();

   int left = 1; int right = nbins;
   double sum0 = 0;
   for (int n=left; n<=right;n++) sum0 += h1->GetBinContent(n);


   double suml = 0, sumr = 0;
   while ((suml < sum0*cut) && (left < right)) {
      suml += h1->GetBinContent(left);
      left++;
   }

   if (left > 5) left -= 5;

   while ((sumr < sum0*cut) && (left < right)) {
      sumr += h1->GetBinContent(right);
      right--;
   }

   if (right < nbins - 5) right += 5;

   h1->GetXaxis()->SetRange(left, right);

   mean = h1->GetMean();
   rms = h1->GetRMS();
   return true;
}

float GetTDCTemp(TFile* f, unsigned tdcid)
{
   TString name;
   name.Form("Histograms/TDC_%04X/TDC_%04X_Temperature", tdcid, tdcid);

   TH1* h1 = GetHist(f, name.Data());

   if (h1==0) return 40;

   float t = h1->GetMean();

   delete h1;

   if (t<100) return t;

   printf("Fail to get correct temperature for %s %s get:%8.2f\n", f->GetName(), name.Data(), t);

   if (tdcid==0x941) return 44.93;
   if (tdcid==0x943) return 46.08;
   return 40.;
}


void ShowTDCStat(TFile* f1, TFile* f2, unsigned tdcid, int numch, float dmean = 0.01, float drms = 0.01) {

   TString name;

   printf("TDC 0x%04X %s %s t1:%5.2f t2:%5.2f\n", tdcid, f1->GetName(), f2->GetName(), GetTDCTemp(f1, tdcid), GetTDCTemp(f2, tdcid));

   for (int nch=2;nch<=numch;++nch) {
      name.Form("Histograms/TDC_%04X/Ch%d/TDC_%04X_Ch%d_RisingRef", tdcid, nch, tdcid, nch);

      TH1* h1 = GetHist(f1, name.Data());
      TH1* h2 = GetHist(f2, name.Data());

      double mean1, rms1, mean2, rms2;

      GetMeanRms(h1, mean1, rms1, 0.001);

      GetMeanRms(h2, mean2, rms2, 0.001);

      delete h1;
      delete h2;

      printf(" Ch:%2d pos:%8.3f rms:%5.3f dpos:%6.3f%s drms:%6.3f%s\n", nch, mean1, rms1,
                            mean2-mean1, (fabs(mean2-mean1) > dmean ? "*" : " "),
                            rms2-rms1, (fabs(rms2-rms1) > drms ? "*" : " "));
   }
}


void EstimateMeanShift(float* table = 0) {

   double sum0[NumChannels+1], sum1[NumChannels+1], sum2[NumChannels+1];
   for (int n=0;n<=NumChannels;n++) {
      sum0[n] = 0;
      sum1[n] = 0;
      sum2[n] = 0;
   }
   TString name;


   TFile* f1 = TFile::Open(filenames[0]);

   int filecnt = 1;

   while (filenames[filecnt] != 0) {

      TFile* f2 = TFile::Open(filenames[filecnt++]);

      for (int tdc=0;tdc<NumTDC;tdc++) {
         unsigned tdcid = TDCids[tdc];

         double t1 = GetTDCTemp(f1, tdcid);
         double t2 = GetTDCTemp(f2, tdcid);

         // printf("TDC %04X T1 = %5.2f T2 = %5.2f\n", tdcid, t1, t2);

         if (fabs(t2-t1) < 1) {
            // printf("Too small tmp difference !!!! \n");
            continue;
         }

         for (int nch=2;nch<=NumChannels;++nch) {
            if (BadChannel(nch)) continue;

            name.Form("Histograms/TDC_%04X/Ch%d/TDC_%04X_Ch%d_RisingRef", tdcid, nch, tdcid, nch);

            TH1* h1 = GetHist(f1, name.Data());
            TH1* h2 = GetHist(f2, name.Data());

            double mean1, rms1, mean2, rms2;

            GetMeanRms(h1, mean1, rms1, 0.001);

            GetMeanRms(h2, mean2, rms2, 0.001);

            delete h1;
            delete h2;

            double shift = (mean2-mean1) / (t2-t1);

            sum0[nch] += 1.;
            sum1[nch] += shift;
            sum2[nch] += shift*shift;
         }
      }
      delete f2;
   }
   delete f1;

   // first channel assumed to have 0 shift (while shift measure relative to this channel)
   sum0[1] = 1;
   sum1[1] = 0.;
   sum2[1] = 1e-6;


   double min = 1000, max = -1000;

   for (int n=0;n<=NumChannels;n++) {

      double mean = 0, rms = 0;

      if (sum0[n] > 0) {
         mean = sum1[n] / sum0[n];
         rms = sqrt(sum2[n] / sum0[n] - mean * mean);
      }

      sum1[n] = mean;
      sum2[n] = rms;

      if (!BadChannel(n) && (sum0[n] > 0)) {
         if (mean < min) min = mean;
         if (mean > max) max = mean;
      }
   }

   printf("SHIFT MIN = %6.2f ps/C MAX = %6.2f ps/C\n", min*1000, max*1000);

   // correction calculated so that after change maximal correction = 5*minimal
   double corr = max + (max-min)*0.25;

   printf("CORR MIN = %6.2f ps/C MAX = %6.2f ps/C ratio = %f  \n", (corr-min)*1000, (corr-max)*1000, (corr-min) / (corr-max));

   for (int n=0;n<=NumChannels;n++) {
      double chcorr = corr-sum1[n];
      // if correction cannot be estimated, use mean value
      if (sum0[n] == 0) chcorr = corr - (max + min) / 2;

      printf("Ch:%2d  Mean:%6.2f ps/C CORR:%6.2f ps/C  Dev:%6.2f ps/C %s\n", n, sum1[n]*1000, chcorr*1000, sum2[n]*1000, (sum2[n] > 0.0025 ? "bad?" : ""));
      if (table) table[n] = chcorr;
   }
}

double EstimateGlobalCalibrCoef() {

   TString name;
   double maink = 0., nummain = 0., main2 = 0.;

   TFile* f1 = TFile::Open(filenames[0]);

   if (f1==0) {
      printf("Cannot get files for calibr coef\n");
      exit(1);
   }

   int filecnt = 1;

   while (filenames[filecnt] != 0) {

      TFile* f2 = TFile::Open(filenames[filecnt++]);
      if (f2==0) {
         printf("Cannot get files for calibr coef\n");
         exit(1);
      }

      for (int tdc=0;tdc<NumTDC;tdc++) {

         unsigned tdcid = TDCids[tdc];

         double sum0(0.), sum1(0.), sum2(0);

         name.Form("Histograms/TDC_%04X/TDC_%04X_Temperature", tdcid, tdcid);
         TH1* h1 = GetHist(f1, name.Data());
         TH1* h2 = GetHist(f2, name.Data());

         double t1 = h1->GetMean();
         double t2 = h2->GetMean();
         printf("TDC %04X %s T1 = %5.2f %s T2 = %5.2f\n", tdcid, f1->GetName(), t1, f2->GetName(), t2);

         for (int nch=1;nch<=NumChannels;++nch) {
            name.Form("Histograms/TDC_%04X/Ch%d/TDC_%04X_Ch%d_RisingCalibr", tdcid, nch, tdcid, nch);

            h1 = GetHist(f1, name.Data());
            h2 = GetHist(f2, name.Data());

            double w = 0;
            double k = CalcK(nch, h1, h2, w, false);

            delete h1;
            delete h2;

            if (w<=0) continue;

            sum0 += w;
            sum1 += k*w;
            sum2 += k*k*w;
         }

         double mean = (sum0>0) ? sum1 / sum0 : 0;

         double sigma = (sum0>0) ? sqrt(sum2/sum0 - mean*mean) : 0;

         printf(" mean:%6.5f Sigma = %6.5f %6.1f%s\n", mean, sigma, fabs(mean) > 0 ? sigma/fabs(mean)*100 : 99.9, "%");

         if (fabs(t2-t1) > 1) {
            double fk = mean / (t2-t1);
            printf("  final K:%8.6f\n", fk);
            maink += fk;
            main2 += fk*fk;
            nummain += 1.;
         }
      }

      delete f2;
   }
   delete f1;

   if (nummain > 2) {
      double globalk = maink/nummain;
      double sigma = sqrt(main2/nummain - globalk*globalk);

      printf("GLOBAL K = %8.6f +- %8.6f\n", globalk, sigma);
      return globalk;
   }

   return 0.;
}

#ifdef __CINT__

void WriteCalibrations(const char* src, const char* tgt, float k, float* tshift, float* sensor_shift)
{
   for (int tdc=0;tdc<NumTDC;tdc++) {

      hadaq::TdcProcessor* proc = new hadaq::TdcProcessor(0, TDCids[0], NumChannels+1, 2);

      proc->LoadCalibration(src);

      proc->SetCalibrTempCoef(k);

      printf("Calibr temp %5.2f\n", proc->GetCalibrTemp());

      for(unsigned ch=0;ch<=NumChannels;ch++)
         proc->SetChannelTempShift(ch, tshift[ch]);

      proc->StoreCalibration(tgt, TDCids[tdc]);

      delete proc;
   }
}

#else

void WriteCalibrations(const char* src, const char* tgt, float k, float* tshift, float* sensor_shift)
{
   printf("NO WAY TO OVERWRITE CALIBRATIONS in COMPILED MODE !!!!\n");
}

#endif



void show_res() {
   const char* fname1 = "res/0-25-1.root";
   //const char* fname2 = "res/0-66-1.root";
   //const char* fname2 = "res/gl066.root";
   const char* fname2 = "res/globA-1-33.root";

   TFile* f1 = TFile::Open(fname1);
   TFile* f2 = TFile::Open(fname2);

   for (int tdc=0;tdc<NumTDC;tdc++) {

      unsigned tdcid = TDCids[tdc];

      ShowTDCStat(f1, f2, TDCids[tdc], NumChannels, 0.01, 0.01);
   }

   return;

   f2 = TFile::Open("res/com-0-66.root");

   for (int tdc=0;tdc<NumTDC;tdc++) {

      unsigned tdcid = TDCids[tdc];

      ShowTDCStat(f1, f2, TDCids[tdc], NumChannels, 0.01, 0.01);
      break;
   }

   f2 = TFile::Open("res/com-0-25.root");

   for (int tdc=0;tdc<NumTDC;tdc++) {

      unsigned tdcid = TDCids[tdc];

      ShowTDCStat(f1, f2, TDCids[tdc], NumChannels, 0.01, 0.01);
      break;
   }

}

void show_indiv(TFile* f) {

   //for (int tdc=1;tdc<NumTDC;tdc++) {
   //   EstimateIndividualCoef(f, TDCids[0], f, TDCids[tdc]);
   // }
}


// here we compare two TDCs from different files
// Calculates coefficient translated into temp difference
// Idea to introduce 'virtual' temp shift to adjust difference between such measurements
double EstimateTempShift(TFile* f1, unsigned tdc1, TFile* f2, unsigned tdc2, float gk)
{
   TString name;

   double sum0(0.), sum1(0.), sum2(0);

   double t1 = GetTDCTemp(f1, tdc1);
   double t2 = GetTDCTemp(f2, tdc2);

   for (int nch=1;nch<=NumChannels;++nch) {

      if (BadChannel(nch)) continue;

      name.Form("Histograms/TDC_%04X/Ch%d/TDC_%04X_Ch%d_RisingCalibr", tdc1, nch, tdc1, nch);
      TH1* h1 = GetHist(f1, name.Data());

      name.Form("Histograms/TDC_%04X/Ch%d/TDC_%04X_Ch%d_RisingCalibr", tdc2, nch, tdc2, nch);
      TH1* h2 = GetHist(f2, name.Data());

      double w = 0;
      double k = CalcK(nch, h1, h2, w, false);
      if (w<=0) continue;

      sum0 += w;
      sum1 += k*w;
      sum2 += k*k*w;
   }

   double mean = (sum0>0) ? sum1 / sum0 : 0;
   double sigma = (sum0>0) ? sqrt(sum2/sum0 - mean*mean) : 0;

   double t2bis = t1 + mean/gk;

   printf("f1:%s TDC:%04x t1:%5.2f f2:%s TDC:%04X  t2:%5.2f  k:%8.5f rms:%7.5f Tbis:%5.2f dT:%6.2f \n",
            f1->GetName(), tdc1, t1, f2->GetName(), tdc2, t2, mean, sigma, t2bis, t2bis-t2);

   return t2bis-t2;
}


void EstimateSensorsShifts(float globk, float* sensors_shifts)
{
   float sum0[NumTDC];
   float sum1[NumTDC];
   float sum2[NumTDC];
   for (int n=0;n<NumTDC;n++) {
      sensors_shifts[n] = 0;
      sum0[n] = 0;
      sum1[n] = 0;
      sum2[n] = 0;
   }

   int cnt = 0;
   while (filenames[cnt] != 0) {
      TFile* f = TFile::Open(filenames[cnt++]);
      for (int tdc=1;tdc<NumTDC;tdc++) {

         float dt = EstimateTempShift(f, TDCids[0], f, TDCids[tdc], globk);

         if (dt != 0.) {
            sum0[tdc] += 1;
            sum1[tdc] += dt;
            sum2[tdc] += dt*dt;
         }
      }
      delete f;
   }

   for (int n=1;n<NumTDC;n++) {
      if (sum0[n]>2) {
         double mean = sum1[n] / sum0[n];
         double sigma = sqrt(sum2[n] / sum0[n] - mean*mean);

         printf("TDC 0x%04X  Sensor dT = %5.2f +- %5.2f\n", TDCids[n], mean, sigma);

         sensors_shifts[n] = mean;
      }
   }
}

void temp() {

   // show_all_indiv(); return;

   // show_res(); return;

   // const char* fname1 = "res/40C-1.root";
   // const char* fname2 = "res/50C-1.root";
   // const char* fname3 = "res/0-66-D.root";

   float gk = EstimateGlobalCalibrCoef();

   float sensor_shifts[NumTDC];
   EstimateSensorsShifts(gk, sensor_shifts);

   //show_indiv(f1);
   //show_indiv(f2);

   // ShowTDCStat(f1, f2, 0x940, 64);

   float mean_shift[NumChannels+1];

   EstimateMeanShift(mean_shift);

   WriteCalibrations("cal/40C-1_", "cal/global_", gk, mean_shift, sensor_shifts);
}
