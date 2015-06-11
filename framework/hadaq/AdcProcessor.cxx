#include "hadaq/AdcProcessor.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <bitset>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbProcessor.h"
#include <cmath>

using namespace std;

vector<double> hadaq::AdcProcessor::storage; 

hadaq::AdcProcessor::AdcProcessor(TrbProcessor* trb, unsigned subid, unsigned numchannels, double samplingPeriod) :
   SubProcessor(trb, "ADC_%04x", subid),
   fSamplingPeriod(samplingPeriod),
   fStoreVect(),
   pStoreVect(0)
{
   fChannels = 0;

   if (HistFillLevel() > 1) {
      fKinds = MakeH1("Kinds", "Messages kinds", 16, 0, 16, "kinds");
      fChannels = MakeH1("Channels", "Messages per channels", numchannels, 0, numchannels, "ch");
   }

   for (unsigned ch=0;ch<numchannels;ch++) {
      ChannelRec rec; 
      SetSubPrefix("Ch", ch);
      rec.fValues = MakeH1("Values","Distribution of values (unsigned)", 1<<10, 0, 1<<10, "value");
      rec.fWaveform = MakeH2("Waveform", "Integrated Waveform", 512, 0, 512, 1<<11, -(1<<10), 1<<10, "sample;value");
      rec.fRawDiffTiming = MakeH1("RawDiffTiming","Timing difference",10000,-500,500,"t / ns");      
      rec.fSamplesSinceTrigger = MakeH1("SamplesSinceTrigger","Samples since trigger", 512, 0, 512, "#samples");
      rec.fIntegral = MakeH1("Integral","Summed integral",10000,0,10000,"integral");
      rec.fCFDSamples = MakeH2("CFDSamples","Samples of the zero crossing",2,0,2,1000,-500,500,"crossing;value");
      rec.fCFDDiffTiming = MakeH1("CFDDiffTiming","Timing difference",10000,-500,500,"t / ns");
      rec.fEdgeSamples = MakeH2("EdgeSamples","Samples of the leading edge",4,0,4,1000,-500,500,"egde;value");
      rec.fEdgeDiffTiming = MakeH1("EdgeDiffTiming","Timing difference",10000,-500,500,"t / ns");
      rec.fSamples1 =  MakeH2("Samples1","Samples of zeroX and leading edge",6,0,6,1000,-500,500,"sample;value");
      rec.fSamples2 =  MakeH2("Samples2","Samples of zeroX and leading edge",6,0,6,1000,-500,500,"sample;value");      
      SetSubPrefix();
      fCh.push_back(rec);
   }
}


hadaq::AdcProcessor::~AdcProcessor()
{
}

void hadaq::AdcProcessor::SetDiffChannel(unsigned ch, int diffch)
{
   fCh[ch].diffCh = diffch;
}

double calc_variance(const vector<double>& v) {
   double sum = 0;
   double sum2 = 0;
   for(size_t i=0;i<v.size();i++) {
      sum += v[i];
      sum2 += v[i]*v[i];
   }
   sum  /= v.size();
   sum2 /= v.size();
   const double var = sum2 - sum*sum;
   return var;
}

double calc_a_chi2(const vector<double>& xx, const vector<double>& yy, 
                 const vector<double>& sx, const vector<double>& sy, 
                 double& a,
                 const double b_angle
                 ) {
   const size_t n = xx.size();
   const double b = tan(b_angle);
   // calculate a and weights ww
   vector<double> ww(n);
   double suma=0, sumw=0;
   for(size_t i=0;i<n;i++) {
      double ww_ = b*sx[i]*b*sx[i] + sy[i]*sy[i];
      ww_ = 1/ww_;
      ww[i] = ww_;
      sumw += ww_;
      suma += ww[i]*(yy[i]-b*xx[i]);
   }
   a = suma/sumw;
   // calc chi2 based on a and b
   double chi2 = 0;
   for(size_t i=0;i<n;i++) {
      const double num = yy[i]-a-b*xx[i];
      chi2 += num*num*ww[i];
   }
   return chi2;
}



double find_chi2_minimum(
      const vector<double>& xx, const vector<double>& yy, 
      const vector<double>& sx, const vector<double>& sy, 
      double& a,
      double b_angle_a, double b_angle_b, double& b_angle
      )
{
   const double eps = 1e-8;
   const double t = 1e-4;
   
   double c;
   double d;
   double e;
   double fu;
   double fv;
   double fw;
   double fx;
   double m;
   double p;
   double q;
   double r;
   double sa;
   double sb;
   double t2;
   double tol;
   double u;
   double v;
   double w;

   c = 0.5 * ( 3.0 - sqrt ( 5.0 ) );
   
   sa = b_angle_a;
   sb = b_angle_b;
   b_angle = sa + c * ( b_angle_b - b_angle_a );
   w = b_angle;
   v = w;
   e = 0.0;
   fx = calc_a_chi2(xx,yy,sx,sy,a, b_angle);
   fw = fx;
   fv = fw;
   
   for ( ; ; )
   { 
      m = 0.5 * ( sa + sb ) ;
      tol = eps * fabs ( b_angle ) + t;
      t2 = 2.0 * tol;

      if ( fabs ( b_angle - m ) <= t2 - 0.5 * ( sb - sa ) )
      {
         break;
      }

      r = 0.0;
      q = r;
      p = q;
      
      if ( tol < fabs ( e ) )
      {
         r = ( b_angle - w ) * ( fx - fv );
         q = ( b_angle - v ) * ( fx - fw );
         p = ( b_angle - v ) * q - ( b_angle - w ) * r;
         q = 2.0 * ( q - r );
         if ( 0.0 < q )
         {
            p = - p;
         }
         q = fabs ( q );
         r = e;
         e = d;
      }
      
      if ( fabs ( p ) < fabs ( 0.5 * q * r ) && 
           q * ( sa - b_angle ) < p && 
           p < q * ( sb - b_angle ) )
      {

         d = p / q;
         u = b_angle + d;

         if ( ( u - sa ) < t2 || ( sb - u ) < t2 )
         {
            if ( b_angle < m )
            {
               d = tol;
            }
            else
            {
               d = - tol;
            }
         }
      }

      else
      {
         if ( b_angle < m )
         {
            e = sb - b_angle;
         }
         else
         {
            e = sa - b_angle;
         }
         d = c * e;
      }

      if ( tol <= fabs ( d ) )
      {
         u = b_angle + d;
      }
      else if ( 0.0 < d )
      {
         u = b_angle + tol;
      }
      else
      {
         u = b_angle - tol;
      }
      
      fu = calc_a_chi2(xx,yy,sx,sy,a, u);

      if ( fu <= fx )
      {
         if ( u < b_angle )
         {
            sb = b_angle;
         }
         else
         {
            sa = b_angle;
         }
         v = w;
         fv = fw;
         w = b_angle;
         fw = fx;
         b_angle = u;
         fx = fu;
      }
      else
      {
         if ( u < b_angle )
         {
            sa = u;
         }
         else
         {
            sb = u;
         }
         
         if ( fu <= fw || w == b_angle )
         {
            v = w;
            fv = fw;
            w = u;
            fw = fu;
         }
         else if ( fu <= fv || v == b_angle || v == w )
         {
            v = u;
            fv = fu;
         }
      }
   }
   return fx;
}

void fitxy(const vector<double>& x_, const vector<double>& y_, 
           const vector<double>& sigy, 
           double& a, double& b) {
   double S   = 0;
   double Sx  = 0;
   double Sy  = 0;
   double Sxx = 0;
   double Sxy = 0;
   for(size_t i=0;i<y_.size();i++) {
      const double s = sigy[i];
      const double s2 = s*s;
      const double x  = x_[i];
      const double y  = y_[i];
      S   += 1/s2;
      Sx  += x/s2;
      Sy  += y/s2;
      Sxx += x*x/s2;
      Sxy += x*y/s2;
   }
   const double D = S*Sxx-Sx*Sx;
   a = (Sxx*Sy - Sx*Sxy)/D;
   b = (S*Sxy-Sx*Sy)/D;
}

void fitxye(const vector<double>& x_,   const vector<double>& y_, 
            const vector<double>& sigx, const vector<double>& sigy,
            double& a, double& b) {
   double varx = calc_variance(x_);
   double vary = calc_variance(y_);
   double scale = sqrt(varx/vary);
   const size_t n = x_.size();
   vector<double> xx(n), yy(n), sx(n), sy(n), ww(n);
   for(size_t i=0;i<n;i++) {
      xx[i] = x_[i];
      yy[i] = y_[i]*scale;
      sx[i] = sigx[i];
      sy[i] = sigy[i]*scale;
      ww[i] = sqrt(sx[i]*sx[i]+sy[i]*sy[i]);      
   }
   
   // conventional fit for starting point of b
   fitxy(xx,yy,sy,a,b);
   
   double b_angle = atan(b);
   
   // find minimum
   find_chi2_minimum(xx,yy,sx,sy,a, 0, b_angle, b_angle);
   
   // unscale
   a /= scale;
   b = tan(b_angle)/scale;
}

double getfraction(const vector<short>& edges) {
   const size_t n = edges.size();
   vector<double> x_(n), y_(n), sx(n), sy(n);
   for(size_t i=0; i<y_.size(); i++) {
      x_[i] = i;
      y_[i] = edges[i];
      sx[i] = 0.5;
      sy[i] = 0.1+0.01*edges[i];
   }
   double a, b;
   //fitxy(x_, y_, sy, a, b);
   fitxye(x_,y_,sx,sy,a,b);
   return -a/b;
}

bool hadaq::AdcProcessor::FirstBufferScan(const base::Buffer& buf)
{
   unsigned len = buf.datalen()/4;
   uint32_t* arr = (uint32_t*) buf.ptr();

   // printf("First scan len %u\n", len);
   // BeforeFill(); // optional
   // use iterator only if context is important

   // start decoding the samples
   uint32_t nSample = 0; // number of msg from the same ADC channel, aka Sample
   uint32_t lastCh = 0;  // remember last used channel for dumping verbose data
   int lastTriggerEpoch = -1; // remember last trigger epoch
   vector<int> triggerEpochs;
   
   vector< vector<short> > raw_samples;
   raw_samples.resize(fCh.size());
   
   for (unsigned n=0;n<len;n++) {
      AdcMessage msg(arr[n]);

      uint32_t kind = msg.getKind();
      FillH1(fKinds, kind);

      // kind==0xd is CFD data word header of CFD firmware
      if(kind == 0xd) {
         uint32_t ch = msg.getCh(); // has same structure as verbose data word
         if(ch>=fCh.size())
            continue;
         // there should be now 5 values after this header word
         const unsigned expected_len = 5;
         if(n+expected_len>len)
            continue;
         // we should have seen some trigger epoch word
         if(lastTriggerEpoch<0)
            continue;

         ChannelRec& r = fCh[ch]; // helpful shortcut
        
         // extract the epoch counter
         // upper 8bits in this word,
         // lower 16bits in following word
         const unsigned epochCounter =
               (((arr[n] >> 8) & 0xff) << 16)
               + ((arr[n+1] >> 16) & 0xffff);
         const int samplesSinceTrigger = epochCounter - lastTriggerEpoch;
         FillH1(r.fSamplesSinceTrigger, samplesSinceTrigger);

         // integral 
         const short integral = arr[n+1] & 0xffff;
         FillH1(r.fIntegral, integral);
         
         // CFD timing
         const short valBeforeZeroX = (arr[n+2] >> 16) & 0xffff;
         const short valAfterZeroX = arr[n+2] & 0xffff;
         const double fraction_CFD = (double)valBeforeZeroX/(valBeforeZeroX-valAfterZeroX);
         const double fineTiming_CFD = (samplesSinceTrigger + fraction_CFD)*fSamplingPeriod;
         r.timing_CFD = fineTiming_CFD;
         FillH2(r.fCFDSamples, 0, valBeforeZeroX);
         FillH2(r.fCFDSamples, 1, valAfterZeroX);
         r.samples.clear();
         r.samples.push_back(valBeforeZeroX);
         r.samples.push_back(valAfterZeroX);
         
         // four samples of the "edge"
         vector<short> edges;
         edges.push_back(arr[n+4] & 0xffff);
         edges.push_back((arr[n+4] >> 16) & 0xffff);
         edges.push_back(arr[n+3] & 0xffff);
         edges.push_back((arr[n+3] >> 16) & 0xffff); 
         r.samples.insert(r.samples.end(), edges.begin(), edges.end());
        
         // "fit" the four points to straight line f(x) = a + bx

         const double fineTiming_Edge = (samplesSinceTrigger + getfraction(edges))*fSamplingPeriod;
                 
         for(size_t i=0;i<edges.size();i++) 
            FillH2(r.fEdgeSamples, i, edges[i]);
         r.timing_Edge = fineTiming_Edge;
         
         
         // don't forget to move forward
         n += expected_len;
      }
      // kind==0xe is trigger epoch word of CFD firmware
      else if(kind == 0xe) {
         // the lower 24bit are the epoch counter as gray code
         std::bitset<24> trigger_epoch_gray(arr[n]);
         std::bitset<24> trigger_epoch_bin(0);
         trigger_epoch_bin[23] = trigger_epoch_gray[23];
         for(int i=22 ; i>=0 ; i--) {
            // B(i) = B(i+1) xor G(i), see Wikipedia :)
            trigger_epoch_bin[i] = trigger_epoch_bin[i+1] ^ trigger_epoch_gray[i];
         }
         lastTriggerEpoch = trigger_epoch_bin.to_ulong();
         triggerEpochs.push_back(lastTriggerEpoch);
      }
      // kind==0 is verbose data word for both firmware versions
      else if(kind == 0) {
         uint32_t ch = msg.getCh();
         short int value = msg.getValue();

         if(ch>=fCh.size())
            continue;

         FillH1(fChannels, ch);
         FillH1(fCh[ch].fValues, value);

         // check if msg still belongs to same ADC channel
         // catch first msg as special case
         if(n==0 || ch != lastCh) {
            lastCh = ch;
            nSample = 0;
         }
         else {
            nSample++;
         }

         FillH2(fCh[ch].fWaveform, nSample, value);
         
         raw_samples[ch].push_back(value);
      }
      // other kinds like PSA data or compressed ADC words unsupported for now
      // they are just ignored
   }

   // if (!fCrossProcess) AfterFill(); // optional

   // analyze raw samples for first two channels
   for(size_t ch=0;ch<raw_samples.size();ch++) {
      if(ch>1)
         continue;
      // calc baseline
      const vector<short>& samples = raw_samples[ch];
      double baseline = 0;
      size_t n=0;
      const size_t cut = 100;
      for(size_t i=cut;i<samples.size();i++) {
         baseline += samples[i];
         n++;
      }
      baseline /= n;
      
      std::vector<double> filtered(cut,baseline);
      for(size_t i=4;i<cut-4;i++) {
         filtered[i] = 
               -0.086 * samples[i-2] 
               +0.343 * samples[i-1]
               +0.486 * samples[i+0]
               +0.343 * samples[i+1]
               -0.086 * samples[i+2];
         
//         filtered[i] = 
//               +0.035 * samples[i-4]
//               -0.128 * samples[i-3]
//               +0.070 * samples[i-2] 
//               +0.315 * samples[i-1]
//               +0.417 * samples[i+0]
//               +0.315 * samples[i+1]
//               +0.070 * samples[i+2]
//               -0.128 * samples[i+3]
//               +0.035 * samples[i+4]
//               ;
         
//         filtered[i] = samples[i];
      }
      
      
      //cout << baseline << endl;
      const size_t delay = 1;
      const int thresh = 32;
      double cfd_prev;
      for(size_t i=0;i<cut;i++) {
         double sample = -(filtered[i]-baseline);
         if(sample<thresh)
            continue;
         double sample_delay = -(filtered[i+delay]-baseline);
         double cfd = 4*sample-3*sample_delay;
         if(i>0) {
            if(cfd_prev<0 && cfd>=0) {
               
               const double fraction_Raw = (double)cfd_prev/(cfd_prev-cfd);
               fCh[ch].timing_Raw = (triggerEpochs[ch]+i+fraction_Raw)*fSamplingPeriod;
               //cout << fCh[ch].timing_Raw << endl;
            }
         }         
         cfd_prev = cfd;
      }
   }
   
   
   // do some reference timing business
   for(size_t ch=0;ch<fCh.size();ch++) {
      const ChannelRec& c = fCh[ch];
      if(c.diffCh<0)
         continue;
      const double diff_Raw = c.timing_Raw - fCh[c.diffCh].timing_Raw;      
      const double diff_CFD = c.timing_CFD - fCh[c.diffCh].timing_CFD;
      const double diff_Edge = c.timing_Edge - fCh[c.diffCh].timing_Edge;
      
      FillH1(c.fRawDiffTiming, diff_Raw);
      FillH1(c.fCFDDiffTiming, diff_CFD);
      FillH1(c.fEdgeDiffTiming, diff_Edge); 
      
      
      base::H2handle h = diff_CFD<211 ? c.fSamples1 : c.fSamples2;
      for(size_t i=0;i<c.samples.size();i++) 
         FillH2(h, i, c.samples[i]);
      
      
      
      // only for ch==0
//      if(ch!=0)
//         continue;
//      if(storage.size()<1000) {
//         storage.push_back(diff_Edge);
//      }
//      else {
//         double var = calc_variance(storage);
//         //cout << "RMS: " << sqrt(var) << endl;
//         storage.clear();
//      }
   }

   return true;

}

bool hadaq::AdcProcessor::SecondBufferScan(const base::Buffer& buf)
{
   unsigned len = buf.datalen()/4;
   uint32_t* arr = (uint32_t*) buf.ptr();

   // printf("Second scan len %u\n", len);

   // use iterator only if context is important
   for (unsigned n=0;n<len;n++) {
      AdcMessage msg(arr[n]);

      // ignore all other kinds
      if (msg.getKind()!=1) continue;

      // we test hits, but do not allow to close events
      // this is normal procedure
      // unsigned indx = TestHitTime(0., true, false);
      unsigned indx = 0; // index 0 is event index in triggered-based analysis

      if (indx < fGlobalMarks.size()) {
         AddMessage(indx, (hadaq::AdcSubEvent*) fGlobalMarks.item(indx).subev, msg);
      }
   }

   return true;
}


void hadaq::AdcProcessor::CreateBranch(TTree* t)
{
   pStoreVect = &fStoreVect;
   mgr()->CreateBranch(t, GetName(), "std::vector<hadaq::AdcMessage>", (void**) &pStoreVect);
}

void hadaq::AdcProcessor::Store(base::Event* ev)
{
   fStoreVect.clear();

   hadaq::AdcSubEvent* sub =
         dynamic_cast<hadaq::AdcSubEvent*> (ev->GetSubEvent(GetName()));

   // when subevent exists, use directly pointer on messages vector
   if (sub!=0)
      pStoreVect = sub->vect_ptr();
   else
      pStoreVect = &fStoreVect;
}
