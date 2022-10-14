#include "hadaq/TdcProcessor.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <stdarg.h>
#include <unistd.h>
#include <ctime>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbProcessor.h"
#include "hadaq/HldProcessor.h"
#include "hadaq/TdcSubEvent.h"
#include <iostream>



#define RAWPRINT( args ...) if(IsPrintRawData()) printf( args )


#define ADDERROR(code, args ...) if(((1 << code) & gErrorMask) || mgr()->DoLog()) AddError( code, args )


unsigned hadaq::TdcProcessor::gNumFineBins = FineCounterBins;
unsigned hadaq::TdcProcessor::gTotRange = 100;
unsigned hadaq::TdcProcessor::gHist2dReduce = 10;  ///<! reduce factor for points in 2D histogram

unsigned hadaq::TdcProcessor::gErrorMask = 0xffffffffU;
bool hadaq::TdcProcessor::gAllHistos = false;
double hadaq::TdcProcessor::gTrigDWindowLow = 0;
double hadaq::TdcProcessor::gTrigDWindowHigh = 0;
bool hadaq::TdcProcessor::gUseDTrigForRef = false;
bool hadaq::TdcProcessor::gUseAsDTrig = false;
int hadaq::TdcProcessor::gHadesMonitorInterval = -111;
bool hadaq::TdcProcessor::gHadesReducedMonitor = false;
int hadaq::TdcProcessor::gTotStatLimit = 100;
double hadaq::TdcProcessor::gTotRMSLimit = 0.15;
int hadaq::TdcProcessor::gDefaultLinearNumPoints = 2;
bool hadaq::TdcProcessor::gIgnoreCalibrMsgs = false;
bool hadaq::TdcProcessor::gStoreCalibrTables = false;

unsigned BUBBLE_SIZE = 19;

//////////////////////////////////////////////////////////////////////////////////////////////
/// obsolete, noop

void hadaq::TdcProcessor::SetMaxBoardId(unsigned)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Set default values for TDC creation
/// \param numfinebins - maximal value of fine counter bins, used for all histograms and calibrations, default 600
/// \param totrange - time in ns for ToT histograms, default 100
/// \param hist2dreduced - reducing factor on some 2D histograms, default 10 - means one bin instead 10 ns of ToT

void hadaq::TdcProcessor::SetDefaults(unsigned numfinebins, unsigned totrange, unsigned hist2dreduced)
{
   gNumFineBins = numfinebins;
   gTotRange = totrange;
   gHist2dReduce = hist2dreduced;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Set errors mask which are printout, set 0 to disable errors printout
/// See \ref hadaq::TdcProcessor::EErrors to list of detected errors

void hadaq::TdcProcessor::SetErrorMask(unsigned mask)
{
   gErrorMask = mask;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Automatically create histograms for all channels - even they not appear in data

void hadaq::TdcProcessor::SetAllHistos(bool on)
{
   gAllHistos = on;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Configure to ignore all kind of calibration data stored in HLD file
/// Let analysis HLD stored by HADES DAQ as it was not calibrated at all

void hadaq::TdcProcessor::SetIgnoreCalibrMsgs(bool on)
{
   gIgnoreCalibrMsgs = on;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Configure window (in nanoseconds), where time stamps from 0xD trigger will be accepted for calibration

void hadaq::TdcProcessor::SetTriggerDWindow(double low, double high)
{
   gTrigDWindowLow = low;
   gTrigDWindowHigh = high;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Configure ToT calibration parameters
/// \param minstat - minimal number of counts to make ToT calibration, default 100
/// \param rms - maximal allowed RMS for ToT histogram in ns, default 0.15

void hadaq::TdcProcessor::SetToTCalibr(int minstat, double rms)
{
   gTotStatLimit = minstat;
   gTotRMSLimit = rms;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Set default number of point in linear approximation

void hadaq::TdcProcessor::SetDefaultLinearNumPoints(int cnt)
{
   gDefaultLinearNumPoints = (cnt < 2) ? 2 : ((cnt > 100) ? 100 : cnt);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Enable usage of 0xD trigger in ref histogram filling

void hadaq::TdcProcessor::SetUseDTrigForRef(bool on)
{
   gUseDTrigForRef = on;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Use all data as 0xD trigger

void hadaq::TdcProcessor::SetUseAsDTrig(bool on)
{
   gUseAsDTrig = on;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Configure interval in seconds for HADES monitoring histograms filling

void hadaq::TdcProcessor::SetHadesMonitorInterval(int tm)
{
   gHadesMonitorInterval = tm;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Return interval in seconds for HADES monitoring histograms filling

int hadaq::TdcProcessor::GetHadesMonitorInterval()
{
   return gHadesMonitorInterval;
}


//////////////////////////////////////////////////////////////////////////////////////////////
/// JAM22: with this flag may suppress some histograms for hades tdc calib monitor

void hadaq::TdcProcessor::SetHadesReducedMonitoring(bool on)
{
   gHadesReducedMonitor = on;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Return interval in seconds for HADES monitoring histograms filling

bool hadaq::TdcProcessor::IsHadesReducedMonitoring()
{
   return gHadesReducedMonitor;
}




//////////////////////////////////////////////////////////////////////////////////////////////
/// enable storage of calibration tables for V4 TDC

void hadaq::TdcProcessor::SetStoreCalibrTables(bool on)
{
   gStoreCalibrTables = on;
}

///////////////////////////////////////////////////////////////////////////
/// constructor
/// \param trb - instance of \ref hadaq::TrbProcessor
/// \param tdcid - TDC id
/// \param numchannels - number of channels
/// \param edge_mask - edges mask, see \ref hadaq::TdcProcessor::EEdgesMasks
/// \param ver4 - is TDC V4 should be expected

hadaq::TdcProcessor::TdcProcessor(TrbProcessor* trb, unsigned tdcid, unsigned numchannels, unsigned edge_mask, bool ver4) :
   SubProcessor(trb, "TDC_%04X", tdcid),
   fVersion4(ver4),
   fIter1(),
   fIter2(),
   fNumChannels(numchannels),
   fNumFineBins(gNumFineBins),
   fCh(),
   fCalibrTemp(30.),
   fCalibrTempCoef(0.004432),
   fCalibrUseTemp(false),
   fCalibrTriggerMask(0xFFFF),
   fCalibrAmount(0),
   fCalibrProgress(0),
   fCalibrStatus("NoCalibr"),
   fCalibrQuality(0.),
   fTempCorrection(0.),
   fCurrentTemp(-1.),
   fDesignId(0),
   fCalibrTempSum0(0),
   fCalibrTempSum1(0),
   fCalibrTempSum2(0),
   fDummyVect(),
   pStoreVect(nullptr),
   fDummyFloat(),
   pStoreFloat(nullptr),
   fDummyDouble(),
   pStoreDouble(nullptr),
   fEdgeMask(edge_mask),
   fCalibrCounts(0),
   fAutoCalibr(false),
   fAutoCalibrOnce(false),
   fAllCalibrMode(-1),
   fAllTotMode(-1),
   fWriteCalibr(),
   fWriteEveryTime(false),
   fUseLinear(false),
   fEveryEpoch(false),
   fUseLastHit(false),
   fUseNativeTrigger(false),
   fCompensateEpochReset(false),
   fCompensateEpochCounter(0),
   fCh0Enabled(false),
   fLastTdcHeader(),
   fLastTdcTrailer(),
   fSkipTdcMessages(0),
   f400Mhz(false),
   fCustomMhz(200.),
   fPairedChannels(false)
{
   fIsTDC = true;

   if (fNumFineBins==0) fNumFineBins = FineCounterBins;

   if (trb) {
      // when normal trigger is used as sync points, than use trigger time on right side to calculate global time
      if (trb->IsUseTriggerAsSync()) SetSynchronisationKind(sync_Right);
      if ((trb->NumSubProc()==1) && trb->IsUseTriggerAsSync()) fUseNativeTrigger = true;
      fCompensateEpochReset = trb->fCompensateEpochReset;
   }

   fChannels = nullptr;
   fHits = nullptr;
   fErrors = nullptr;
   fUndHits = nullptr;
   fCorrHits = nullptr;
   fMsgsKind = nullptr;
   fAllFine = nullptr;
   fAllCoarse = nullptr;
   fRisingCalibr = nullptr;
   fFallingCalibr = nullptr;
   fTotShifts = nullptr;
   fTempDistr = nullptr;
   fhRaisingFineCalibr = nullptr;
   fhTotVsChannel = nullptr;
   fhTotMoreCounter = nullptr;
   fhTotMinusCounter = nullptr;
   fHitsRate = nullptr;
   fRateCnt = 0;
   fLastRateTm = -1;

   fHldId = 0;                   ///<! sequence number of processor in HLD
   fHitsPerHld = nullptr;
   fErrPerHld = nullptr;
   fChHitsPerHld = nullptr;
   fChErrPerHld = nullptr; ///<! errors per TDC channel - from HLD
   fChCorrPerHld = nullptr;  ///<! corrections per TDC channel - from HLD
   fQaFinePerHld = nullptr;  ///<! QA fine counter per TDC channel - from HLD
   fQaToTPerHld = nullptr;  ///<! QA ToT per TDC channel - from HLD
   fQaEdgesPerHld = nullptr;  ///<! QA Edges per TDC channel - from HLD
   fQaErrorsPerHld = nullptr;  ///<! QA Errors per TDC channel - from HLD

    // JAM2021: additional histos for HADES TDC calib monitor
   fToTPerTDCChannel= nullptr;  ///< HADAQ ToT per TDC channel, real values
   fShiftPerTDCChannel= nullptr;  ///< HADAQ calibrated shift per TDC channel, real values
   fExpectedToTPerTDC= nullptr;  ///< HADAQ expected ToT per TDC  used for calibration
   fDevPerTDCChannel= nullptr;
   fTPreviousPerTDCChannel= nullptr;
   fhSigmaTotVsChannel = nullptr;
   fhRisingPrevDiffVsChannel = nullptr;

   fToTdflt = true;
   fToTvalue = ToTvalue;
   fToThmin = ToThmin;
   fToThmax = ToThmax;
   fTotUpperLimit = 20;
   fTotStatLimit = gTotStatLimit;
   fTotRMSLimit = gTotRMSLimit;

   fLinearNumPoints = gDefaultLinearNumPoints;

   if (HistFillLevel() > 1) {
      fChannels = MakeH1("Channels", "Messages per TDC channels", numchannels, 0, numchannels, "ch");
      if (DoFallingEdge() && DoRisingEdge())
         fHits = MakeH1("Edges", "Edges counts TDC channels (rising/falling)", numchannels*2, 0, numchannels, "ch");
      fErrors = MakeH1("Errors", "Errors in TDC channels", numchannels, 0, numchannels, "ch");
      fUndHits = MakeH1("UndetectedHits", "Undetected hits in TDC channels", numchannels, 0, numchannels, "ch");
      fCorrHits = MakeH1("CorrectedHits", "Corrected hits in TDC channels", numchannels, 0, numchannels, "ch");

      if (!fVersion4)
         fMsgsKind = MakeH1("MsgKind", "kind of messages", 8, 0, 8, "xbin:Trailer,Header,Debug,Epoch,Hit,-,-,Calibr;kind");
      else
         fMsgsKind = MakeH1("MsgKind", "kind of messages", 8, 0, 8, "xbin:HDR,EPOC,TMDR,TMDT,-,-,-,-;kind");

      fAllFine = MakeH2("FineTm", "fine counter value", numchannels, 0, numchannels, (fNumFineBins==1000 ? 100 : fNumFineBins), 0, fNumFineBins, "ch;fine");

      if (!hadaq::TdcProcessor::IsHadesReducedMonitoring()) {
         fhRaisingFineCalibr =
            MakeH2("RaisingFineTmCalibr", "raising calibrated fine counter value", numchannels, 0, numchannels,
                   (fNumFineBins == 1000 ? 100 : fNumFineBins), 0, fNumFineBins, "ch;calibrated fine");
      }

      fAllCoarse = MakeH2("CoarseTm", "coarse counter value", numchannels, 0, numchannels, 2048, 0, 2048, "ch;coarse");

      fhTotVsChannel = MakeH2("TotVsChannel", "ToT", numchannels, 0, numchannels, gTotRange*100/(gHist2dReduce > 0 ? gHist2dReduce : 1), 0., gTotRange, "ch;ToT [ns]");

      if (!hadaq::TdcProcessor::IsHadesReducedMonitoring()) {
         fhTotMoreCounter =
            MakeH1("TotMoreCounter", "ToT > 20 ns counter in TDC channels", numchannels, 0, numchannels, "ch");
         fhTotMinusCounter =
            MakeH1("TotMinusCounter", "ToT < 0 ns counter in TDC channels", numchannels, 0, numchannels, "ch");
      }
      if (DoRisingEdge())
         fRisingCalibr  = MakeH2("RisingCalibr",  "rising edge calibration", numchannels, 0, numchannels, fNumFineBins, 0, fNumFineBins, "ch;fine");

      if (DoFallingEdge()) {
         fFallingCalibr = MakeH2("FallingCalibr", "falling edge calibration", numchannels, 0, numchannels, fNumFineBins, 0, fNumFineBins, "ch;fine");
         fTotShifts = MakeH1("TotShifts", "Calibrated time shift for falling edge", numchannels, 0, numchannels, "kind:F;ch;ns");
      }


    fhSigmaTotVsChannel = MakeH2("ToTSigmaVsChannel", "SigmaToT", numchannels, 0, numchannels, 500, 0, +5, "ch; Sigma ToT [ns]");

    fhRisingPrevDiffVsChannel= MakeH2("RisingChanneslDiff", "Rising dt to reference channel 0 ", numchannels, 0, numchannels, 2000, -10, 10, "ch; Delta t [ns]");
   }

   for (unsigned ch=0;ch<numchannels;ch++)
      fCh.emplace_back();

   for (unsigned ch=0;ch<numchannels;ch++)
      fCh[ch].CreateCalibr(fNumFineBins, fVersion4 ? hadaq::TdcMessage::CoarseUnit280() : hadaq::TdcMessage::CoarseUnit());

   // always create histograms for channel 0
   CreateChannelHistograms(0);
   if (gAllHistos)
      for (unsigned ch=1;ch<numchannels;ch++)
         CreateChannelHistograms(ch);

   fWriteCalibr.clear();
   fWriteEveryTime = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// destructor

hadaq::TdcProcessor::~TdcProcessor()
{
   for (unsigned ch=0;ch<NumChannels();ch++) {
      fCh[ch].ReleaseCalibr();
      fCh[ch].ReleaseToTHist();
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Configure 400 MHz mode

void hadaq::TdcProcessor::Set400Mhz(bool on)
{
   f400Mhz = on;
   fCustomMhz = on ? 400. : 200.;

   for (unsigned ch=0;ch<fNumChannels;ch++)
      fCh[ch].FillCalibr(fNumFineBins, 200. / fCustomMhz * hadaq::TdcMessage::CoarseUnit());
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Set custom frequency

void hadaq::TdcProcessor::SetCustomMhz(float freq)
{
   f400Mhz = true;
   fCustomMhz = freq;

   for (unsigned ch=0;ch<fNumChannels;ch++)
      fCh[ch].FillCalibr(fNumFineBins, 200. / fCustomMhz * hadaq::TdcMessage::CoarseUnit());
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Check if error should be printed

bool hadaq::TdcProcessor::CheckPrintError()
{
   return fTrb ? fTrb->CheckPrintError() : true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// add new error

void hadaq::TdcProcessor::AddError(unsigned code, const char *fmt, ...)
{
   va_list args;
   int length = 256;
   char *buffer = nullptr;

   std::string sbuf(GetName());
   while (true) {
      if (buffer) delete [] buffer;
      buffer = new char [length];
      va_start(args, fmt);
      auto result = vsnprintf(buffer, length, fmt, args);
      va_end(args);

      if (result < 0) length *= 2; else                 // this is error handling in glibc 2.0
      if (result >= length) length = result + 1; else   // this is error handling in glibc 2.1
      break;
   }
   sbuf.append(" ");
   sbuf.append(buffer);
   delete [] buffer;

   if (CheckPrintError())
      printf("%s\n", sbuf.c_str());

   if (fTrb) fTrb->EventError(sbuf.c_str());
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// create all histograms for the channel

bool hadaq::TdcProcessor::CreateChannelHistograms(unsigned ch)
{
   if ((HistFillLevel() < 3) || (ch>=NumChannels())) return false;

   SetSubPrefix2("Ch", ch);

   if (DoRisingEdge() && !fCh[ch].fRisingFine) {
      fCh[ch].fRisingFine = MakeH1("RisingFine", "Rising fine counter", fNumFineBins, 0, fNumFineBins, "fine");
      fCh[ch].fRisingMult = MakeH1("RisingMult", "Rising event multiplicity", 128, 0, 128, "nhits");
      fCh[ch].fRisingCalibr = MakeH1("RisingCalibr", "Rising calibration function", fNumFineBins, 0, fNumFineBins, "fine;kind:F");
      // copy calibration only when histogram created
      CopyCalibration(fCh[ch].rising_calibr, fCh[ch].fRisingCalibr, ch, fRisingCalibr);
   }

   if (DoFallingEdge() && !fCh[ch].fFallingFine && ((ch > 0) || fVersion4)) {
      fCh[ch].fFallingFine = MakeH1("FallingFine", "Falling fine counter", fNumFineBins, 0, fNumFineBins, "fine");
      fCh[ch].fFallingCalibr = MakeH1("FallingCalibr", "Falling calibration function", fNumFineBins, 0, fNumFineBins, "fine;kind:F");
      fCh[ch].fFallingMult = MakeH1("FallingMult", "Falling event multiplicity", 128, 0, 128, "nhits");
      fCh[ch].fTot = MakeH1("Tot", "Time over threshold", gTotRange*100, 0, gTotRange, "ns");
      // fCh[ch].fTot0D = MakeH1("Tot0D", "Time over threshold with 0xD trigger", TotBins, fToThmin, fToThmax, "ns");
      // copy calibration only when histogram created
      CopyCalibration(fCh[ch].falling_calibr, fCh[ch].fFallingCalibr, ch, fFallingCalibr);
   }

   SetSubPrefix2();

   return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////
/// Disable calibration for specified channels

void hadaq::TdcProcessor::DisableCalibrationFor(unsigned firstch, unsigned lastch)
{
   if (lastch<=firstch) lastch = firstch+1;
   if (lastch>=NumChannels()) lastch = NumChannels();
   for (unsigned n=firstch;n<lastch;n++)
      fCh[n].docalibr = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// set real ToT value for 0xD trigger and min/max for histogram accumulation
/// default is 30ns, and 50ns - 80ns range

void hadaq::TdcProcessor::SetToTRange(double tot, double hmin, double hmax)
{
   fToTdflt = false;
   fToTvalue = tot;
   fToThmin = hmin;
   fToThmax = hmax;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Configure 0xD trigger ToT based on hwtype

void hadaq::TdcProcessor::ConfigureToTByHwType(unsigned hwtype)
{
   // ID table according to TRB3 manual, page 15
   bool recognized = true;
   switch(hwtype) {
      case 0x90: SetToTRange(30., 50., 80.); break; // TRB3 central FPGA
      case 0x91: SetToTRange(30., 50., 80.); break; // TRB3 peripheral FPGA
      case 0x95: SetToTRange(30., 50., 80.); break; // TRB3sc
      case 0x96: SetToTRange(20., 15., 60.); break; // DiRich
      case 0xA5: SetToTRange(20., 15., 60.); break; // TRB5sc
      case 0xA7: SetToTRange(20., 15., 60.); break; // ???
      default: fToTdflt = false; recognized = false; // keep as is but mark as not default value
   }

   if (recognized) {
      char msg[1000];
      snprintf(msg, sizeof(msg), "%s assign ToT config len:%4.1f hmin:%4.1f hmax:%4.1f\n", GetName(), fToTvalue, fToThmin, fToThmax);
      mgr()->PrintLog(msg);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// execute posloop function - check if calibration should be performed

void hadaq::TdcProcessor::UserPostLoop()
{
   if (!fWriteCalibr.empty() && !fWriteEveryTime) {
      if (fCalibrCounts==0) ProduceCalibration(true, fUseLinear);
      StoreCalibration(fWriteCalibr);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Create basic histograms for specified channels.
/// If array not specified, histograms for all channels are created.
/// In array last element must be 0 or out of channel range. Call should be like:
/// int channels[] = {33, 34, 35, 36, 0};
/// tdc->CreateHistograms( channels );

void hadaq::TdcProcessor::CreateHistograms(int *arr)
{
   if (!arr) {
      for (unsigned ch=0;ch<NumChannels();ch++)
         CreateChannelHistograms(ch);
   } else
   while ((*arr>0) && (*arr < (int) NumChannels())) {
      CreateChannelHistograms(*arr++);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Set reference signal for the TDC channel ch
/// \param ch      channel for which reference histogram will be created
/// \param refch   specifies number of reference channel
/// \param reftdc  specifies tdc id, used for ref channel. default (0xffff) same TDC will be used
/// \param npoints number of points in ref histogram
/// \param left    left limit of histogram
/// \param right   right limit of histogram
/// \param twodim  if extra two dimensional histograms should be created
/// If redtdc contains 0x70000 (like 0x7c010), than direct difference without channel 0 will be calculated
/// To be able use other TDCs, one should enable TTrbProcessor::SetCrossProcess(true);
/// If left-right range are specified, ref histograms are created.
/// If twodim==true, 2-D histogram which will accumulate correlation between
/// time difference to ref channel and:
/// - fine_counter (shift 0 ns)
/// - fine_counter of ref channel (shift -1 ns)
/// - coarse_counter/4 (shift -2 ns)

void hadaq::TdcProcessor::SetRefChannel(unsigned ch, unsigned refch, unsigned reftdc,
                                        int npoints, double left, double right, bool twodim)
{
   if ((ch>=NumChannels()) || (HistFillLevel()<4)) return;

   if (((reftdc == 0xffff) || (reftdc>0xfffff)) && ((reftdc & 0xf0000) != 0x70000)) reftdc = GetID();

   // ignore invalid settings
   if ((ch==refch) && ((reftdc & 0xffff) == GetID())) return;

   if ((refch == 0) && (ch != 0) && ((reftdc & 0xffff) != GetID())) {
      printf("%s cannot set reference to zero channel from other TDC\n", GetName());
      return;
   }

   fCh[ch].refch = refch;
   fCh[ch].reftdc = reftdc & 0xffff;
   fCh[ch].refabs = (reftdc & 0xf0000) == 0x70000;

   // if other TDC configured as ref channel, enable cross processing
   if (fTrb) fTrb->SetCrossProcessAll();

   CreateChannelHistograms(ch);
   if (fCh[ch].reftdc == GetID()) CreateChannelHistograms(refch);

   char sbuf[1024], saxis[1024], refname[512];
   if (fCh[ch].reftdc == GetID()) {
      snprintf(refname, sizeof(refname), "Ch%u", fCh[ch].refch);
   } else {
      snprintf(refname, sizeof(refname), "TDC 0x%04x Ch%u", fCh[ch].reftdc, fCh[ch].refch);
   }

   if ((left < right) && (npoints>1)) {
      SetSubPrefix2("Ch", ch);
      if (DoRisingEdge()) {

         if (!fCh[ch].fRisingRef) {
            snprintf(sbuf, sizeof(sbuf), "difference to %s", refname);
            snprintf(saxis, sizeof(saxis), "Ch%u - %s, ns", ch, refname);
            fCh[ch].fRisingRef = MakeH1("RisingRef", sbuf, npoints, left, right, saxis);
         }

         if (twodim && !fCh[ch].fRisingRef2D) {
            snprintf(sbuf, sizeof(sbuf), "corr diff %s and fine counter", refname);
            snprintf(saxis, sizeof(saxis), "Ch%u - %s, ns;fine counter", ch, refname);
            fCh[ch].fRisingRef2D = MakeH2("RisingRef2D", sbuf, 500, left, right, 100, 0, 500, saxis);
         }
      }

      SetSubPrefix2();
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Set reference signal for time extracted from v4 TMDS message
/// \param ch   configured channel
/// \param refch   reference channel
/// \param npoints number of points in ref histogram
/// \param left    left limit of histogram
/// \param right   right limit of histogram

void hadaq::TdcProcessor::SetRefTmds(unsigned ch, unsigned refch, int npoints, double left, double right)
{
   if ((ch>=NumChannels()) || (refch>=NumChannels()) || (HistFillLevel()<4)) return;

   // ignore invalid settings
   if ((refch==0) || (ch == 0) || (ch == refch)) {
      printf("%s cannot set reference between TMDS channel %u %u\n", GetName(), ch, refch);
      return;
   }

   fCh[ch].refch_tmds = refch;

   CreateChannelHistograms(ch);
   CreateChannelHistograms(refch);

   char sbuf[1024], saxis[1024], refname[512];
   snprintf(refname, sizeof(refname), "Ch%u", refch);

   if ((left < right) && (npoints > 1) && DoRisingEdge()) {
      SetSubPrefix2("Ch", ch);

      if (!fCh[ch].fRisingTmdsRef) {
         snprintf(sbuf, sizeof(sbuf), "TMDS difference to %s", refname);
         snprintf(saxis, sizeof(saxis), "Ch%u - %s, ns", ch, refname);
         fCh[ch].fRisingTmdsRef = MakeH1("RisingTmdsRef", sbuf, npoints, left, right, saxis);
      }

      SetSubPrefix2();
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Configure double-reference histogram
/// Required that for both channels references are specified via SetRefChannel() command.
/// If ch2 > 1000, than channel from other TDC can be used. tdcid = (ch2 - 1000) / 1000

bool hadaq::TdcProcessor::SetDoubleRefChannel(unsigned ch1, unsigned ch2,
                                              int npx, double xmin, double xmax,
                                              int npy, double ymin, double ymax)
{
   if (HistFillLevel()<4) return false;

   if ((ch1>=NumChannels()) || (fCh[ch1].refch>=NumChannels()))  return false;

   unsigned reftdc = 0xffff;
   if (ch2 > 0xffff) { reftdc = ch2 >> 16; ch2 = ch2 & 0xFFFF; }
   if (reftdc >= 0xffff) reftdc = GetID();

   unsigned ch = ch1, refch = ch2;

   if (reftdc == GetID()) {
      if ((ch2>=NumChannels()) || (fCh[ch2].refch>=NumChannels()))  return false;
      if (ch1<ch2) { ch = ch2; refch = ch1; }
   } else {
      //if (reftdc > GetID())
      //   printf("WARNING - double ref channel from TDC with higher ID %u > %u\n", reftdc, GetID());
      if (fTrb) fTrb->SetCrossProcessAll();
   }

   fCh[ch].doublerefch = refch;
   fCh[ch].doublereftdc = reftdc;

   char sbuf[1024];
   char saxis[1024];

   if (DoRisingEdge()) {

      if (!fCh[ch].fRisingRefRef && (npy == 0)) {
         if (reftdc == GetID()) {
            snprintf(sbuf, sizeof(sbuf), "double reference with Ch%u", refch);
            snprintf(saxis, sizeof(saxis), "(ch%u-ch%u) - (refch%u) ns", ch, fCh[ch].refch, refch);
         } else {
            snprintf(sbuf, sizeof(sbuf), "double reference with TDC 0x%04x Ch%u", reftdc, refch);
            snprintf(saxis, sizeof(saxis), "(ch%u-ch%u)  - (tdc 0x%04x refch%u) ns", ch, fCh[ch].refch, reftdc, refch);
         }

         SetSubPrefix2("Ch", ch);
         fCh[ch].fRisingRefRef = MakeH1("RisingRefRef", sbuf, npx, xmin, xmax, saxis);
         SetSubPrefix2();
      }


      if (!fCh[ch].fRisingDoubleRef && (npy>0)) {
         if (reftdc == GetID()) {
            snprintf(sbuf, sizeof(sbuf), "double correlation to Ch%u", refch);
            snprintf(saxis, sizeof(saxis), "ch%u-ch%u ns;ch%u-ch%u ns", ch, fCh[ch].refch, refch, fCh[refch].refch);
         } else {
            snprintf(sbuf, sizeof(sbuf), "double correlation to TDC 0x%04x Ch%u", reftdc, refch);
            snprintf(saxis, sizeof(saxis), "ch%u-ch%u ns;tdc 0x%04x refch%u ns", ch, fCh[ch].refch, reftdc, refch);
         }

         SetSubPrefix2("Ch", ch);
         fCh[ch].fRisingDoubleRef = MakeH2("RisingDoubleRef", sbuf, npx, xmin, xmax, npy, ymin, ymax, saxis);
         SetSubPrefix2();
      }
   }

   return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Create rate histogram to count hits per second (excluding channel 0)

void hadaq::TdcProcessor::CreateRateHisto(int np, double xmin, double xmax)
{
   SetSubPrefix2();
   fHitsRate = MakeH1("HitsRate", "Hits rate", np, xmin, xmax, "hits/sec");
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Enable print of TDC data when time difference to ref channel belong to specified interval
/// Work ONLY when reference channel 0 is used.
/// One could set maximum number of events to print
/// In any case one should first set reference channel

bool hadaq::TdcProcessor::EnableRefCondPrint(unsigned ch, double left, double right, int numprint)
{
   if (ch>=NumChannels()) return false;
   if (fCh[ch].refch >= NumChannels()) {
      fprintf(stderr,"Reference channel not specified, conditional print cannot work\n");
      return false;
   }
   if (fCh[ch].reftdc != GetID()) {
      fprintf(stderr,"Only when reference channel on same TDC specified, conditional print can be enabled\n");
      return false;
   }
   if (fCh[ch].refch > ch) {
      fprintf(stderr,"Reference channel %u bigger than channel id %u, conditional print may not work\n", fCh[ch].refch, ch);
   }

   SetSubPrefix2("Ch", ch);

   fCh[ch].fRisingRefCond = MakeC1("RisingRefPrint", left, right, fCh[ch].fRisingRef);
   fCh[ch].rising_cond_prnt = numprint > 0 ? numprint : 100000000;

   SetSubPrefix2();

   return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Reset different values before scan subevent

void hadaq::TdcProcessor::BeforeFill()
{
   for (unsigned ch = 0; ch < NumChannels(); ch++) {
      ChannelRec &rec = fCh[ch];
      rec.rising_hit_tm = 0;
      rec.rising_last_tm = 0;
      rec.rising_ref_tm = 0.;
      rec.rising_new_value = false;
      rec.rising_tmds = 0.;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Fill histograms after scan subevent

void hadaq::TdcProcessor::AfterFill(SubProcMap* subprocmap)
{

   // complete logic only when hist level is specified
   if (HistFillLevel()>=4)
   for (unsigned ch=0;ch<NumChannels();ch++) {

      ChannelRec &rec = fCh[ch];

      DefFillH1(rec.fRisingMult, rec.rising_cnt, 1.); rec.rising_cnt = 0;
      DefFillH1(rec.fFallingMult, rec.falling_cnt, 1.); rec.falling_cnt = 0;

      if (rec.fRisingTmdsRef && (rec.refch_tmds < NumChannels())) {
         double tm1 = rec.rising_tmds;
         double tm0 = fCh[rec.refch_tmds].rising_tmds;
         if ((tm1!=0) && (tm0!=0))
            DefFillH1(rec.fRisingTmdsRef, (tm1-tm0) * 1e9, 1.);
      }

      unsigned ref = rec.refch;
      if (ref > 0xffff) continue; // no any settings for ref channel, can ignore

      unsigned reftdc = rec.reftdc;
      if (reftdc >= 0xffff) reftdc = GetID();

      TdcProcessor* refproc = nullptr;
      if (reftdc == GetID())
         refproc = this;
      else if (fTrb)
         refproc = fTrb->FindTDC(reftdc);

      if (!refproc && subprocmap) {
         auto iter = subprocmap->find(reftdc);
         if ((iter != subprocmap->end()) && iter->second->IsTDC())
            refproc = (TdcProcessor*) iter->second;
      }

      // RAWPRINT("TDC%u Ch:%u Try to use as ref TDC%u %u proc:%p\n", GetID(), ch, reftdc, ref, refproc);

      // printf("TDC %s %d %d same %d %p %p  %f %f \n", GetName(), ch, ref, (this==refproc), this, refproc, rec.rising_hit_tm, refproc->fCh[ref].rising_hit_tm);

      if (refproc && ((ref > 0) || (refproc != this)) && (ref < refproc->NumChannels()) && ((ref != ch) || (refproc != this))) {
         if (DoRisingEdge() && (rec.rising_hit_tm != 0) && (refproc->fCh[ref].rising_hit_tm != 0)) {

            double tm = rec.rising_hit_tm; // relative time to ch0 on same TDC
            double tm_ref = refproc->fCh[ref].rising_hit_tm; // relative time to ch0 on referenced TDC

            if ((refproc != this) && (ch > 0) && (ref > 0) && rec.refabs) {
               tm += fCh[0].rising_hit_tm; // produce again absolute time for channel
               tm_ref += refproc->fCh[0].rising_hit_tm; // produce again absolute time for reference channel
            }

            rec.rising_ref_tm = tm - tm_ref;

            double diff = rec.rising_ref_tm*1e9;

            // when refch is 0 on same board, histogram already filled
            if ((ref != 0) || (refproc != this))
               DefFillH1(rec.fRisingRef, diff, 1.);

            DefFillH2(rec.fRisingRef2D, diff, rec.rising_fine, 1.);
            DefFillH2(rec.fRisingRef2D, (diff-1.), refproc->fCh[ref].rising_fine, 1.);
            DefFillH2(rec.fRisingRef2D, (diff-2.), rec.rising_coarse/4, 1.);
            RAWPRINT("Difference rising %04x:%02u\t %04x:%02u\t %12.3f\t %12.3f\t %7.3f  coarse %03x - %03x = %4d  fine %03x %03x \n",
                  GetID(), ch, reftdc, ref,
                  tm*1e9,  tm_ref*1e9, diff,
                  rec.rising_coarse, refproc->fCh[ref].rising_coarse, (int) (rec.rising_coarse - refproc->fCh[ref].rising_coarse),
                  rec.rising_fine, refproc->fCh[ref].rising_fine);

            // make double reference only for local channels
            // if ((rec.doublerefch < NumChannels()) &&
            //    (rec.fRisingDoubleRef != 0) &&
            //    (fCh[rec.doublerefch].rising_ref_tm != 0)) {
            //   DefFillH1(rec.fRisingDoubleRef, diff, fCh[rec.doublerefch].rising_ref_tm*1e9);
            // }
         }
      }

      // fill double-reference histogram, using data from any reference TDC
      if ((rec.doublerefch < NumChannels()) && (rec.fRisingDoubleRef || rec.fRisingRefRef)) {

         ref = rec.doublerefch;
         reftdc = rec.doublereftdc;
         if (reftdc>=0xffff) reftdc = GetID();
         refproc = nullptr;
         if (reftdc == GetID()) refproc = this; else
         if (fTrb) refproc = fTrb->FindTDC(reftdc);

         if (!refproc && subprocmap) {
            auto iter = subprocmap->find(reftdc);
            if ((iter != subprocmap->end()) && iter->second->IsTDC())
               refproc = (TdcProcessor*) iter->second;
         }

         if (refproc && (ref<refproc->NumChannels()) && ((ref != ch) || (refproc != this))) {
            if ((rec.rising_ref_tm != 0) && (refproc->fCh[ref].rising_ref_tm != 0)) {
               DefFillH1(rec.fRisingRefRef, (rec.rising_ref_tm - refproc->fCh[ref].rising_ref_tm)*1e9, 1.);
               DefFillH2(rec.fRisingDoubleRef, rec.rising_ref_tm*1e9, refproc->fCh[ref].rising_ref_tm*1e9, 1.);
            }
         }
      }
   }

   fCalibrProgress = TestCanCalibrate(false);
   if ((fCalibrProgress>=1.) && fAutoCalibr) PerformAutoCalibrate();
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Return number of accumulated statistics for the channel

long hadaq::TdcProcessor::CheckChannelStat(unsigned ch)
{
   ChannelRec &rec = fCh[ch];
   if (!rec.docalibr) return 0;

   if (fEdgeMask == edge_CommonStatistic)
      return rec.all_rising_stat + rec.all_falling_stat;

   long stat = 0;

   if (DoRisingEdge() && (rec.all_rising_stat>0)) stat = rec.all_rising_stat;

   if (DoFallingEdge() && (rec.all_falling_stat>0) && (fEdgeMask == edge_BothIndepend))
      if ((stat == 0) || (rec.all_falling_stat < stat)) stat = rec.all_falling_stat;

   return stat;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Check if automatic calibration can be performed - enough statistic is accumulated

double hadaq::TdcProcessor::TestCanCalibrate(bool fillhist, std::string *status)
{
   if (fCalibrCounts<=0) return 0.;

   bool isany = false;
   long min = 1000000000L, max = 0;
   unsigned minch = 0;

   for (unsigned ch = 0; ch < NumChannels(); ch++) {
      long stat = CheckChannelStat(ch);
      if (stat > 0) {
         isany = true;
         if (stat < min) {
            min = stat;
            minch = ch;
         } else if (stat > max) {
            max = stat;
         }

      }
      if (fillhist && fCalHitsPerBrd) SetH2Content(*fCalHitsPerBrd, fSeqeunceId, ch, stat);
   }

   double min_progress = isany ? 1.*min/fCalibrCounts : 0.,
          max_progress = isany ? 1.*max/fCalibrCounts : 0.;

   if (status) {
      if ((max_progress < 0.5) || (min_progress > 0.5*max_progress))
         *status = "Accumulating";
      else
         *status = std::string(GetName()) + std::string("_AccDiff_ch") + std::to_string(minch);
   }

   return min_progress;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Perform automatic calibration of channels

bool hadaq::TdcProcessor::PerformAutoCalibrate()
{
   ProduceCalibration(true, fUseLinear || ((fCalibrCounts > 0) && (fCalibrCounts % 10000 == 77)));
   if (!fWriteCalibr.empty() && fWriteEveryTime)
      StoreCalibration(fWriteCalibr);
   if (fAutoCalibrOnce && (fCalibrCounts>0)) {
      fAutoCalibrOnce = false;
      fAutoCalibr = false;
   }

   return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Start mode, when all data will be used for calibrations

void hadaq::TdcProcessor::BeginCalibration(long cnt)
{
   fCalibrCounts = cnt;
   fAutoCalibrOnce = false;
   fAutoCalibr = false;
   fAllCalibrMode = 1;
   fAllTotMode = -1; // first detect 10 0xD triggers
   fAllDTrigCnt = 0;

   fCalibrStatus = "Accumulating";
   fCalibrQuality = 0.7;
   fCalibrProgress = 0.;
   fCalibrAmount = 0;

   // ensure that calibration statistic is empty
   for (unsigned ch=0;ch<NumChannels();ch++)
      ClearChannelStat(ch);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Complete calibration mode, create calibration and calibration files

void hadaq::TdcProcessor::CompleteCalibration(bool dummy, const std::string &filename, const std::string &subname)
{
   if (fAllCalibrMode<=0) return;
   fAllCalibrMode = 0;
   fCalibrCounts = 0;
   fAllTotMode = -1;
   fAllDTrigCnt = 0;

   ProduceCalibration(true, fUseLinear, dummy);

   if (!filename.empty()) StoreCalibration(filename);
   if (!subname.empty()) StoreCalibration(subname);
   // if (!fWriteCalibr.empty()) StoreCalibration(fWriteCalibr);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Find min and max values of fine counter in calibration

void hadaq::TdcProcessor::FindFMinMax(const std::vector<float> &func, int nbin, int &fmin, int &fmax)
{
   double coarse_unit = fVersion4 ? hadaq::TdcMessage::CoarseUnit280() : hadaq::TdcMessage::CoarseUnit();

   if (func.size() != 5) {
      fmin = 0;
      fmax = nbin-1;
      float min = coarse_unit*1e-6;
      float max = coarse_unit*0.999999;
      while ((fmin<nbin) && (func[fmin]<min)) fmin++;
      while ((fmax>fmin) && (func[fmax]>max)) fmax--;
   } else {
      fmin = (int) func[1];
      fmax = (int) func[3];
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Extract calibration value

float hadaq::TdcProcessor::ExtractCalibr(const std::vector<float> &func, unsigned bin)
{
   if (!fCalibrUseTemp || (fCalibrTemp <= 0) || (fCurrentTemp <= 0) || (fCalibrTempCoef<=0))
      return ExtractCalibrDirect(func, bin);

   // TODO: if using temperature, also extrapolate linear approximation
   float temp = fCurrentTemp + fTempCorrection;

   float val = func[bin] * (1+fCalibrTempCoef*(temp-fCalibrTemp));

   double coarse_unit = fVersion4 ? hadaq::TdcMessage::CoarseUnit280() : hadaq::TdcMessage::CoarseUnit();

   if ((temp < fCalibrTemp) && (func[bin] >= coarse_unit*0.9999)) {
      // special case - lower temperature and bin which was not observed during calibration
      // just take linear extrapolation, using points bin-30 and bin-80

      float val0 = func[bin-30] + (func[bin-30] - func[bin-80]) / 50 * 30;

      val = val0 * (1+fCalibrTempCoef*(temp-fCalibrTemp));
   }

   return val < coarse_unit ? val : coarse_unit;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Method transform TDC data, if output specified, use it otherwise change original data

unsigned hadaq::TdcProcessor::TransformTdcData(hadaqs::RawSubevent* sub, uint32_t *rawdata, unsigned indx, unsigned datalen, hadaqs::RawSubevent* tgt, unsigned tgtindx)
{
   // do nothing in case of empty TDC sub-sub-event
   if (datalen == 0) return 0;

   hadaq::TdcMessage msg, calibr;

   unsigned tgtindx0(tgtindx), cnt(0),
         hitcnt(0), hit1cnt(0), epochcnt(0), errcnt(0), calibr_indx(0), calibr_num(0),
         nrising(0), nfalling(0),
         nmatches(0), // try to check sequence of hits, if fails
         trig_type = sub->GetTrigTypeTrb3();

   bool use_in_calibr = ((1 << trig_type) & fCalibrTriggerMask) != 0;
   bool is_0d_trig = (trig_type == 0xD);

   if (fAllCalibrMode == 0) {
      use_in_calibr = false;
   } else if (fAllCalibrMode > 0) {
      use_in_calibr = true;
   }

   // do not check progress value too often - this requires extra computations
   bool check_calibr_progress = false;
   if (use_in_calibr) {
      double limit = fCalibrCounts*0.07;
      if (limit < 50) limit = 50; else if (limit>1000) limit = 1000;
      if (++fCalibrAmount > limit) {
         fCalibrAmount = 0;
         check_calibr_progress = true;
      }
   }

   uint32_t epoch = 0, chid, fine, kind, coarse(0), new_fine,
            idata, *tgtraw = tgt ? (uint32_t *) tgt->RawData() : nullptr;
   bool isrising, hard_failure, fast_loop = HistFillLevel() < 2, changed_msg;
   double corr, ch0tm = 0;

   // if (fAllTotMode==1) printf("%s dtrig %d do_tot %d dofalling %d\n", GetName(), is_0d_trig, do_tot, DoFallingEdge());

   while (datalen-- > 0) {

      idata = rawdata[indx++];
      msg.assign(HADAQ_SWAP4(idata));

      cnt++;

      // HERE ONLY FOR DEBUG PURPOSES - check performance
      //if (tgt) {
      //   tgt->SetData(tgtindx++, msg.getData());
      //   continue;
      //}

      kind = msg.getKind();

      // do not fill for every hit, try to use counters
      // DefFastFillH1(fMsgsKind, kind >> 29, 1);

      if (kind == hadaq::tdckind_Hit) hitcnt++; else
      if (kind == hadaq::tdckind_Hit1) hit1cnt++; else {
         if (kind == hadaq::tdckind_Epoch) {
            epoch = msg.getEpochValue();
            epochcnt++;
         } else if (kind == hadaq::tdckind_Calibr) {
            DefFastFillH1(fMsgsKind, kind >> 29, 1);
            calibr.assign(msg.getData()); // copy message into
            calibr_indx = tgtindx;
            calibr_num = 0;
         } else if (kind == hadaq::tdckind_Header) {
            DefFastFillH1(fMsgsKind, kind >> 29, 1);
            if (use_in_calibr && fToTdflt && DoFallingEdge())
               ConfigureToTByHwType(msg.getHeaderHwType());
         } else {
            DefFastFillH1(fMsgsKind, kind >> 29, 1);
         }

         if (tgtraw) tgtraw[tgtindx++] = idata; // tgt->SetData(tgtindx++, msg.getData());
         continue;
      }

      // from here HitMsg and Hit1Msg handled similarly

      chid = msg.getHitChannel();
      fine = msg.getHitTmFine();
      isrising = msg.isHitRisingEdge();
      changed_msg = false;

      // if (fAllTotMode==1) printf("%s ch %u rising %d fine %x\n", GetName(), chid, isrising, fine);

      if (fPairedChannels && (chid > 0) && (chid % 2 == 0) && isrising) {
         changed_msg = true;
         chid--; // previous channel
         isrising = false; // even channels are falling edges for odd channels
      }

      hard_failure = false;
      if (chid >= NumChannels()) {
         hard_failure = true;
         chid = 0; // use dummy channel
         errcnt++;
      }

      ChannelRec &rec = fCh[chid];

      if (fine >= fNumFineBins) {
         hard_failure = true;
         errcnt++;
      }

      // ignore temperature compensation
      corr = hard_failure ? 0. : ExtractCalibrDirect(isrising ? rec.rising_calibr : rec.falling_calibr, fine);

      // corr = hard_failure ? 0. : (isrising ? rec.rising_calibr[fine] : rec.falling_calibr[fine]);

      if (!tgt) {
         coarse = msg.getHitTmCoarse();
         if (isrising) {
            // simple approach for rising edge - replace data in the source buffer
            // value from 0 to 1000 is 5 ps unit, should be SUB from coarse time value
            uint32_t new_fine = (uint32_t) (corr/5e-12);
            if (new_fine >= 1000) new_fine = 1000;
            if (hard_failure) new_fine = 0x3ff;
            msg.setAsHit2(new_fine);
         } else {
            // simple approach for falling edge - replace data in the source buffer
            // value from 0 to 500 is 10 ps unit, should be SUB from coarse time value

            unsigned corr_coarse = 0;
            if (rec.tot_shift > 0.) {
               // if tot_shift calibrated (in ns), included it into correction
               // in such case which should add correction into coarse counter
               corr += rec.tot_shift*1e-9;
               corr_coarse = (unsigned) (corr/5e-9);
               corr -= corr_coarse*5e-9;
            } else if (rec.tot_shift < 0.) {
               // case of HADES TOF TDC, shift coarse to right
               corr += rec.tot_shift*1e-9;
               while ((corr < 0.) && (coarse < 0x7ff)) { corr += 5e-9; coarse++; }
               if (corr < 0.)
                  hard_failure = true;
            }

            uint32_t new_fine = (uint32_t) (corr/10e-12);
            if (new_fine >= 500) new_fine = 500;

            if (corr_coarse > coarse)
               new_fine |= 0x200; // indicate that corrected time belongs to the previous epoch

            if (hard_failure) new_fine = 0x3ff;

            msg.setAsHit2(new_fine);
            msg.setHitTmCoarse(coarse - corr_coarse);
         }
         if (changed_msg) {
            msg.setHitChannel(chid);
            msg.setHitEdge(isrising ? 1 : 0);
         }
         sub->SetData(indx-1, msg.getData());
      } else {
         // copy data to the target, introduce extra messages with calibrated

         if (isrising) {
            // rising edge correction used for 5 ns range, approx 0.305ps binning
            new_fine = (uint32_t) (corr/5e-9*0x3ffe);
         } else {
            // account TOT shift
            corr += rec.tot_shift*1e-9;

            if (changed_msg && (corr < 0.)) {
               uint32_t coarse = msg.getHitTmCoarse();

               // move coarse time and compensate correction
               while ((coarse < 0x7ff) && (corr < 0.)) { coarse++; corr += 5e-9; }
               msg.setHitTmCoarse(coarse);
               if (corr < 0.) {
                  // printf("%s Fail to compensate channel %u\n", GetName(), chid);
                  corr = 0;
                  hard_failure = true;
               }
            }

            new_fine = (uint32_t) (corr/5e-8*0x3ffe);
         }

         if ((new_fine > 0x3ffe) || hard_failure) new_fine = 0x3fff;

         if (calibr_indx == 0) {
            calibr_indx = tgtindx++;
            calibr_num = 0;
            calibr.assign(tdckind_Calibr);
         }

         // set data of calibr message, do not use fast access
         calibr.setCalibrFine(calibr_num++, new_fine);

         // tgt->SetData(calibr_indx, calibr.getData());
         if (calibr_num == 2) {
            tgtraw[calibr_indx] = HADAQ_SWAP4(calibr.getData());
            calibr_indx = 0;
            calibr_num = 0;
         }

         // copy original hit message
         // tgt->SetData(tgtindx++, msg.getData());
         if (changed_msg) {
            msg.setHitChannel(chid);
            msg.setHitEdge(isrising ? 1 : 0);
            tgtraw[tgtindx++] = HADAQ_SWAP4(msg.getData());
         } else {
            tgtraw[tgtindx++] = idata;
         }
      }

      if (hard_failure) continue;

      if (use_in_calibr && (kind == hadaq::tdckind_Hit)) {
         coarse = msg.getHitTmCoarse();
         double tm = ((epoch << 11) | coarse) * 5e-9 - corr;
         bool usehit = true;

         if ((chid > 0) && ch0tm && (gTrigDWindowLow < gTrigDWindowHigh-1.)) {
            double localtm = (tm - ch0tm)*1e9;
            usehit = (gTrigDWindowLow <= localtm) && (localtm <= gTrigDWindowHigh);
         }

         if (isrising) {
            if (chid == 0) ch0tm = tm;
            if (usehit) {
               if (chid > 0) {
                  if (nmatches+1 == chid*2) nmatches++; else nmatches = 0;
                  nrising++;
               } else {
                  nmatches = 1;
               }
               rec.rising_stat[fine]++;
               rec.all_rising_stat++;
               rec.rising_last_tm = tm;
               rec.rising_new_value = true;
            }
         } else {
            if (usehit) {
               nfalling++;
               if (nmatches == chid*2) nmatches++; else nmatches = 0;
               rec.falling_stat[fine]++;
               rec.all_falling_stat++;
               if (rec.rising_new_value) {
                  double tot = (tm - rec.rising_last_tm)*1e9;

                  // DefFillH1(rec.fTot, tot, 1.);
                  // add shift again to be on safe side when calculate new shift at the end
                  rec.last_tot = tot + rec.tot_shift;
                  rec.rising_new_value = false;
               }
            }
         }

         // trigger check of calibration only when enough statistic in that channel
         // done only once for specified channel
         if (!check_calibr_progress && rec.docalibr && !rec.check_calibr && (fCalibrCounts > 0)) {
            long stat = CheckChannelStat(chid);

            // if ToT mode enabled, make first check at half of the statistic to make preliminary calibrations
            if (stat >= fCalibrCounts * ((fAllTotMode==0) ? 0.5 : 1.)) {
               rec.check_calibr = true;
               check_calibr_progress = true;
            }
         }
      }

      if (fast_loop) continue;

      FastFillH1(fChannels, chid);
      FastFillH1(fHits, (chid*2 + (isrising ? 0 : 1)));
      DefFastFillH2(fAllFine, chid, fine);
      DefFastFillH2(fAllCoarse, chid, coarse);
      if ((HistFillLevel() > 2) && (!rec.fRisingFine || !rec.fFallingFine))
         CreateChannelHistograms(chid);

      if (isrising) {
         FastFillH1(rec.fRisingFine, fine);
      } else {
         FastFillH1(rec.fFallingFine, fine);
      }
   }

   // if last calibration message not yet copied into output
   if ((calibr_num == 1) && tgtraw && calibr_indx) {
      tgtraw[calibr_indx] = HADAQ_SWAP4(calibr.getData());
      calibr_indx = 0;
   }

   if (hitcnt) DefFastFillH1(fMsgsKind, hadaq::tdckind_Hit >> 29, hitcnt);
   if (hit1cnt) DefFastFillH1(fMsgsKind, hadaq::tdckind_Hit1 >> 29, hit1cnt);
   if (epochcnt) DefFastFillH1(fMsgsKind, hadaq::tdckind_Epoch >> 29, epochcnt);

   if (cnt && fMsgPerBrd) FastFillH1(*fMsgPerBrd, fSeqeunceId, cnt);
   // fill number of "good" hits
   if (hitcnt && fHitsPerBrd) FastFillH1(*fHitsPerBrd, fSeqeunceId, hitcnt);
   if (errcnt && fErrPerBrd) FastFillH1(*fErrPerBrd, fSeqeunceId, errcnt);

   if ((hitcnt>0) && use_in_calibr && (fCurrentTemp>0)) {
      fCalibrTempSum0 += 1.;
      fCalibrTempSum1 += fCurrentTemp;
      fCalibrTempSum2 += fCurrentTemp*fCurrentTemp;
   }

   if (fAllCalibrMode > 0) {
      // detect 0xD trigger ourself
      if (!is_0d_trig && (nrising == nfalling) && (nrising+1 == NumChannels())) is_0d_trig = true;
      if (!is_0d_trig && (nmatches > 16)) is_0d_trig = true;

      double limit = GetPairedChannels() ? 0.4 : 0.8;

      // if in calibration mode more than 90% of falling and rising edges - mark as d trigger, HADES workaround
      if (!is_0d_trig && ((trig_type == 0) || (trig_type == 1)) && (nrising > limit*NumChannels()) && (nfalling > limit*NumChannels())) is_0d_trig = true;

      if (is_0d_trig) fAllDTrigCnt++;
      if ((fAllDTrigCnt>10) && (fAllTotMode<0)) fAllTotMode = 0;
   }

   // do ToT calculations only when preliminary calibration was produced
   bool do_tot = use_in_calibr && is_0d_trig && DoFallingEdge() && (fAllTotMode==1);

   if (do_tot)
      for (unsigned ch=1;ch<NumChannels();ch++) {
         ChannelRec& rec = fCh[ch];

         if (rec.hascalibr) {
            if ((rec.last_tot >= fToThmin) && (rec.last_tot < fToThmax)) {
               int bin = (int) ((rec.last_tot - fToThmin) / (fToThmax - fToThmin) * TotBins);
               if (rec.tot0d_hist.empty()) rec.CreateToTHist();
               rec.tot0d_hist[bin]++;
               rec.tot0d_cnt++;
            } else {
               rec.tot0d_misscnt++;
            }
         }

         rec.last_tot = 0.;
         rec.rising_new_value = false;
      }

   if (check_calibr_progress) {
      fCalibrProgress = TestCanCalibrate(true, &fCalibrStatus);
      fCalibrQuality = (fCalibrProgress > 2) ? 0.9 : 0.7 + fCalibrProgress*0.1;

      if ((fAllTotMode == 0) && (fCalibrProgress >= 0.5)) {
         ProduceCalibration(false, fUseLinear, false, true);
         fAllTotMode = 1; // now can start accumulate ToT values
      }

      if ((fCalibrProgress>=1.) && fAutoCalibr) PerformAutoCalibrate();
   }

   return tgt ? (tgtindx - tgtindx0) : cnt;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Emulate transformation

void hadaq::TdcProcessor::EmulateTransform(int dummycnt)
{
   if (fAllCalibrMode>0) {
      fCalibrProgress = dummycnt*1e-3;
      fCalibrQuality = (fCalibrProgress > 2) ? 0.9 : 0.7 + fCalibrProgress*0.1;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// print buble - obsolete

void PrintBubble(unsigned* bubble) {
   // print in original order
   // for (unsigned d=BUBBLE_SIZE;d>0;d--) printf("%04x",bubble[d-1]);

   // print in reverse order
   for (unsigned d=0;d<BUBBLE_SIZE;d++) {
      unsigned origin = bubble[d], swap = 0;
      for (unsigned dd = 0;dd<16;++dd) {
         swap = (swap << 1) | (origin & 1);
         origin = origin >> 1;
      }
      printf("%04x",swap);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// print buble binary - obsolete

void PrintBubbleBinary(unsigned* bubble, int p1 = -1, int p2 = -1) {
   if (p1<0) p1 = 0;
   if (p2<=p1) p2 = BUBBLE_SIZE*16;

   int pos = 0;
   char sbuf[1000];
   char* ptr  = sbuf;

   for (unsigned d=0;d<BUBBLE_SIZE;d++) {
      unsigned origin = bubble[d];
      for (unsigned dd = 0;dd<16;++dd) {
         if ((pos>=p1) && (pos<=p2))
            *ptr++ = (origin & 0x1) ? '1' : '0';
         origin = origin >> 1;
         pos++;
      }
   }

   *ptr++ = 0;
   printf("%s", sbuf);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// check buble - obsolete

unsigned BubbleCheck(unsigned* bubble, int &p1, int &p2) {
   p1 = 0; p2 = 0;

   unsigned pos = 0, last = 1, nflip = 0;

   int b1 = 0, b2 = 0;

   unsigned fliparr[BUBBLE_SIZE*16];

   for (unsigned n=0;n<BUBBLE_SIZE; n++) {
      unsigned data = bubble[n] & 0xFFFF;
      if (n < BUBBLE_SIZE-1) data = data | ((bubble[n+1] & 0xFFFF) << 16); // use word to recognize bubble

      // this is error - first bit always 1
      if ((n==0) && ((data & 1) == 0)) { return -1; }

      for (unsigned b=0;b<16;b++) {
         if ((data & 1) != last) {
            if (last==1) {
               if (p1==0) p1 = pos; // take first change from 1 to 0
            } else {
               p2 = pos; // use last change from 0 to 1
            }
            nflip++;
         }

         fliparr[pos] = nflip; // remember flip counts to analyze them later

         // check for simple bubble at the beginning 1101000 or 0x0B in swapped order
         // set position on last 1 ? Expecting following sequence
         //  1110000 - here pos=4
         //     ^
         //  1110100 - here pos=5
         //      ^
         //  1111100 - here pos=6
         //       ^
         if ((data & 0xFF) == 0x0B) b1 = pos+3;

         // check for simple bubble at the end 00001011 or 0xD0 in swapped order
         // set position of 0 in bubble, expecting such sequence
         //  0001111 - here pos=4
         //     ^
         //  0001011 - here pos=5
         //      ^
         //  0000011 - here pos=6
         //       ^
         if ((data & 0xFF) == 0xD0) b2 = pos+5;

         // simple bubble at very end 00000101 or 0xA0 in swapped order
         // here not enough space for two bits
         if (((pos == BUBBLE_SIZE*16 - 8)) && (b2 == 0) && ((data & 0xFF) == 0xA0))
            b2 = pos + 6;

         last = (data & 1);
         data = data >> 1;
         pos++;
      }
   }

   if (nflip == 2) return 0; // both are ok

   if ((nflip == 4) && (b1>0) && (b2==0)) { p1 = b1; return 0x10; } // bubble in the begin

   if ((nflip == 4) && (b1==0) && (b2>0)) { p2 = b2; return 0x01; } // bubble at the end

   if ((nflip == 6) && (b1>0) && (b2>0)) { p1 = b1; p2 = b2; return 0x11; } // bubble on both side

   // up to here was simple errors, now we should do more complex analysis

   if (p1 < p2 - 8) {
      // take flip count at the middle and check how many transitions was in between
      int mid = (p2+p1)/2;
      // hard error in the beginning
      if (fliparr[mid] + 1 == fliparr[p2]) return 0x20;
      // hard error in begin, bubble at the end
      if ((fliparr[mid] + 3 == fliparr[p2]) && (b2>0)) { p2 = b2; return 0x21; }

      // hard error at the end
      if (fliparr[p1] == fliparr[mid]) return 0x02;
      // hard error at the end, bubble at the begin
      if ((fliparr[p1] + 2 == fliparr[mid]) && (b1>0)) { p1 = b1; return 0x12; }
   }

   return 0x22; // mark both as errors, should analyze better
}


unsigned* rom_encoder_rising = nullptr;
unsigned rom_encoder_rising_cnt = 0;
unsigned* rom_encoder_falling = nullptr;
unsigned rom_encoder_falling_cnt = 0;

//////////////////////////////////////////////////////////////////////////////////////////////
/// this is how it used in the FPGA, table provided by Cahit
/// obsolete

unsigned BubbleCheckCahit(unsigned* bubble, int &p1, int &p2, bool debug = false, int maskid = 0, int shift = 0, bool = false) {

   if (!rom_encoder_rising) {

      rom_encoder_rising = new unsigned[400];
      rom_encoder_falling = new unsigned[400];

      const char* fname = "rom_encoder_bin.mem";

      FILE* f = fopen(fname, "r");
      if (!f) return -1;

      char sbuf[1000], ddd[100], *res = nullptr;
      unsigned  *tgt = nullptr, *tgtcnt = nullptr;

      unsigned mask, code;
      int num, dcnt = 1;

      while ((res = fgets(sbuf, sizeof(sbuf)-1, f))) {
         if (strlen(res) < 5) continue;
         if (strstr(res,"Rising")) { tgt = rom_encoder_rising; tgtcnt = &rom_encoder_rising_cnt; continue; }
         if (strstr(res,"Falling")) { tgt = rom_encoder_falling; tgtcnt = &rom_encoder_falling_cnt; continue; }
         const char* comment = strstr(res,"#");
         if (comment && ((comment-res)<5)) continue;

         num = sscanf(res,"%x %s : %u", &mask, ddd, &code);

         if ((num!=3) || !tgt) continue;

         //unsigned swap = 0;
         //for (unsigned k=0;k<9;++k)
         //   if (mask & (1<<k)) swap = swap | (1<<(8-k));
         // printf("read: 0x%03x 0x%03x %u   %s  ", mask, swap, code, res);


         if (maskid == dcnt) {
            printf("SHIFT MASK %03x : %u + %d\n", mask, code, shift);
            if ((int)code + shift >= 0) code += shift;
         }

         tgt[(*tgtcnt)++] = mask;
         tgt[(*tgtcnt)++] = code;
         dcnt++;

         if (*tgtcnt >= 400) { printf("TOO MANY ENTRIES!!!\n"); exit(43); }
      }

      printf("READ %u rising %u falling %d dcnt entries from %s\n", rom_encoder_rising_cnt, rom_encoder_falling_cnt, dcnt, fname);

      fclose(f);
   }

   if ((rom_encoder_rising_cnt == 0) || (rom_encoder_falling_cnt == 0)) return 0x22;

   int pos = 0;
   unsigned data = 0xFFFF0000;
   //unsigned selmask = 0;
   //bool seen_selected = false;

   //static int mcnt = 0;

   /*
   if (maskid>0) {
      if (maskid <= (int) rom_encoder_rising_cnt/2) selmask = rom_encoder_rising[(maskid-1)*2]; else
      if (maskid <= (int) (rom_encoder_rising_cnt + rom_encoder_falling_cnt)/2) selmask = rom_encoder_falling[(maskid-1)*2 - rom_encoder_rising_cnt];
      //if (mcnt++ < 5) printf("SELMASK 0x%x\n", selmask);
   }
   */

   p1 = 0; p2 = 0;

   for (unsigned n=0;n<BUBBLE_SIZE+2; n++) {

      if (n<BUBBLE_SIZE) {
         unsigned origin = bubble[n], swap = 0;
         for (unsigned dd = 0;dd<16;++dd) {
            swap = (swap << 1) | (origin & 1);
            origin = origin >> 1;
         }
         data = data | swap;
      } else {
         data = data | 0xFFFF;
      }

      for (int half=0;half<2;half++) {

         unsigned search = (data & 0x3FF0000) >> 16;

         if ((search != 0) && (search != 0x3FF)) {

//            if (search == selmask) seen_selected = true;

            if (debug) printf("search %04x pos %03d\n", search, pos);

            for (unsigned k=0;k<rom_encoder_rising_cnt;k+=2)
               if (rom_encoder_rising[k] == search) {
                  if (p2==0) p2 = pos - rom_encoder_rising[k+1] - 1;
                  // printf("rising  pos:%3u data %08x found 0x%03x shift: %u p2: %d\n", pos, data, search, rom_encoder_rising[k+1], p2);
               }

            for (unsigned k=0;k<rom_encoder_falling_cnt;k+=2)
               if (rom_encoder_falling[k] == search) {
                  if (p1==0) p1 = pos - rom_encoder_falling[k+1] - 1;
                  // printf("falling pos:%3u data %08x found 0x%03x shift: %u p1: %d\n", pos, data, search, rom_encoder_falling[k+1], p1);
               }
         }

         pos += 8;
         data = data << 8;
      }
   }

   // if (selmask && !seen_selected && useselmask) return 0x22; // ignore all messages if we do not see selected mask

   return ((p1>0) ? 0x00 : 0x20) | ((p2>0) ? 0x00 : 0x02);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Scan all messages, find reference signals
/// Major data analysis method

bool hadaq::TdcProcessor::DoBufferScan(const base::Buffer& buf, bool first_scan)
{
   if (buf.null()) {
      if (first_scan) printf("Something wrong - empty buffer should not appear in the first scan/n");
      return false;
   }

   uint32_t syncid = 0xffffffff;
   // copy first 4 bytes - it is syncid
   if (buf().format==0)
      memcpy(&syncid, buf.ptr(), 4);


   int buf_kind = buf().kind;

   // if data could be used for calibration
   int use_for_calibr = first_scan && (((1 << buf_kind) & fCalibrTriggerMask) != 0) ? 1 : 0;

   // use in ref calculations only physical trigger, exclude 0xD or 0xE
   bool use_for_ref = buf_kind < (gUseDTrigForRef ? 0xE : 0xD);

   // disable taking last hit for trigger DD
   if ((use_for_calibr > 0) && ((buf_kind == 0xD) || gUseAsDTrig)) {
      if (IsTriggeredAnalysis() &&  (gTrigDWindowLow < gTrigDWindowHigh)) use_for_calibr = 3; // accept time stamps only for inside window
      // use_for_calibr = 2; // always use only last hit
   }

   // if data could be used for TOT calibration
   bool do_tot = (use_for_calibr > 0) && ((buf_kind == 0xD) || gUseAsDTrig) && DoFallingEdge();

   // use temperature compensation only when temperature available
   bool do_temp_comp = fCalibrUseTemp && (fCurrentTemp > 0) && (fCalibrTemp > 0) && (fabs(fCurrentTemp - fCalibrTemp) < 30.);

   unsigned cnt = 0, hitcnt = 0;

   bool iserr = false, isfirstepoch = false, rawprint = false, missinghit = false, dostore = false;

   if (first_scan && IsTriggeredAnalysis() && IsStoreEnabled() && mgr()->HasTrigEvent()) {
      dostore = true;
      switch (GetStoreKind()) {
         case 1: {
            hadaq::TdcSubEvent* subevnt = new hadaq::TdcSubEvent(buf.datalen()/6);
            mgr()->AddToTrigEvent(GetName(), subevnt);
            pStoreVect = subevnt->vect_ptr();
            break;
         }
         case 2: {
            hadaq::TdcSubEventFloat* subevnt = new hadaq::TdcSubEventFloat(buf.datalen()/6);
            mgr()->AddToTrigEvent(GetName(), subevnt);
            pStoreFloat = subevnt->vect_ptr();
            break;
         }
         case 3: {
            hadaq::TdcSubEventDouble* subevnt = new hadaq::TdcSubEventDouble(buf.datalen()/6);
            mgr()->AddToTrigEvent(GetName(), subevnt);
            pStoreDouble = subevnt->vect_ptr();
            break;
         }

         default: break; // not supported
      }
   }

   //static int ddd = 0;
   //if (ddd++ % 10000 == 0) printf("%s dostore %d istriggered %d hasevt %d kind %d\n", GetName(), dostore, IsTriggeredAnalysis(), mgr()->HasTrigEvent(), GetStoreKind());
   uint32_t first_epoch(0);

   unsigned epoch_shift = 0;
   if (fCompensateEpochReset) {
      if (first_scan) buf().user_tag = fCompensateEpochCounter;
      epoch_shift = buf().user_tag;
   }

   TdcIterator& iter = first_scan ? fIter1 : fIter2;

   if (buf().format==0)
      iter.assign((uint32_t*) buf.ptr(4), buf.datalen()/4-1, false);
   else
      iter.assign((uint32_t*) buf.ptr(0), buf.datalen()/4, buf().format==2);

   unsigned help_index = 0;

   double localtm = 0., minimtm = 0., ch0time = 0.;

   hadaq::TdcMessage& msg = iter.msg();
   hadaq::TdcMessage calibr;
   unsigned ncalibr = 20, temp = 0, lowid = 0, highid = 0; // clear indicate that no calibration data present

   if (fSkipTdcMessages > 0) {
      unsigned skipcnt = fSkipTdcMessages;
      while (skipcnt-- > 0)
         if (!iter.next()) break;
   }

   while (iter.next()) {

      // if (!first_scan) msg.print();

      cnt++;

      if (first_scan)
         FastFillH1(fMsgsKind, msg.getKind() >> 29);

      if (cnt==1) {
         if (!msg.isHeaderMsg()) {
            iserr = true;
            ADDERROR(errNoHeader, "Missing header message");
         } else {

            if (first_scan) fLastTdcHeader = msg;

            if (fToTdflt && first_scan)
               ConfigureToTByHwType(msg.getHeaderHwType());
            //unsigned errbits = msg.getHeaderErr();
            //if (errbits && first_scan)
            //   if (CheckPrintError()) printf("%5s found error bits: 0x%x\n", GetName(), errbits);
         }

         continue;
      }

      if (msg.isEpochMsg()) {

         uint32_t ep = iter.getCurEpoch();

         if (fCompensateEpochReset) {
            ep += epoch_shift;
            iter.setCurEpoch(ep);
         }

         // second message always should be epoch of channel 0
         if (cnt==2) {
            isfirstepoch = true;
            first_epoch = ep;
         }
         continue;
      }

      if (msg.isCalibrMsg()) {
         if ((use_for_calibr == 0) && !gIgnoreCalibrMsgs) {
            // take into account calibration messages only when data not used for calibration
            ncalibr = 0;
            calibr = msg;
         }
         continue;
      }

      if (msg.isHitMsg()) {
         unsigned chid = msg.getHitChannel();
         unsigned fine = msg.getHitTmFine();
         unsigned coarse = msg.getHitTmCoarse();
         bool isrising = msg.isHitRisingEdge();
         unsigned bad_fine = 0x3FF;

         if (fPairedChannels) {
            if ((chid > 0) && (chid % 2 == 0)) {
               chid--; // previous channel
               isrising = false; // always falling edge
            }
         }

         if (f400Mhz) {
            unsigned coarse25 = (coarse << 1) | ((fine & 0x200) ? 1 : 0);
            fine = fine & 0x1FF;
            bad_fine = 0x1ff;
            unsigned epoch = iter.isCurEpoch() ? iter.getCurEpoch() : 0;

            localtm = ((epoch << 12) | coarse25) * 1000. / fCustomMhz * 1e-9;
         } else {
            localtm = iter.getMsgTimeCoarse();
         }

         if (chid >= NumChannels()) {
            ADDERROR(errChId, "Channel number %u bigger than configured %u", chid, NumChannels());
            iserr = true;
            continue;
         }

         if (!iter.isCurEpoch()) {
            // one expects epoch before each hit message, if not data are corrupted and we can ignore it
            ADDERROR(errEpoch, "Missing epoch for hit from channel %u", chid);
            iserr = true;
            if (fChErrPerHld) DefFillH2(*fChErrPerHld, fHldId, chid, 1);
            continue;
         }

         if (IsEveryEpoch())
            iter.clearCurEpoch();

         if (fine == bad_fine) {
            if (first_scan) {
               // special case - missing hit, just ignore such value
               ADDERROR(err3ff, "Missing hit in channel %u fine counter is %x", chid, fine);

               missinghit = true;
               FastFillH1(fChannels, chid);
               FastFillH1(fUndHits, chid);

               if (fChErrPerHld) DefFillH2(*fChErrPerHld, fHldId, chid, 1);
            }
            continue;
         }

         ChannelRec& rec = fCh[chid];

         double corr = 0.;
         bool raw_hit = false;

         if (msg.getKind() == tdckind_Hit2) {
            if (isrising) {
               corr = fine * 5e-12;
            } else {
               corr = (fine & 0x1FF) * 10e-12;
               if (fine & 0x200) corr += 0x800 * 5e-9; // complete epoch should be subtracted
            }
         } else {

            if (fine >= fNumFineBins) {
               FastFillH1(fErrors, chid);
               if (fChErrPerHld) DefFillH2(*fChErrPerHld, fHldId, chid, 1);
               ADDERROR(errFine, "Fine counter %u out of allowed range 0..%u in channel %u", fine, fNumFineBins, chid);
               iserr = true;
               continue;
            }

            raw_hit = msg.isHit0Msg();

            if (ncalibr < 2) {
               // use correction from special message

               uint32_t calibr_fine = calibr.getCalibrFine(ncalibr++);
               if (calibr_fine == 0x3fff) {
                  if (first_scan) {
                     FastFillH1(fErrors, chid);
                     if (fChErrPerHld) DefFillH2(*fChErrPerHld, fHldId, chid, 1);
                     ADDERROR(errFine, "Invalid value in calibrated message in channel %u", chid);
                  }
                  continue;
               }

               corr = calibr_fine*5e-9/0x3ffe;
               if (isrising && fhRaisingFineCalibr) DefFillH2(fhRaisingFineCalibr, chid, calibr_fine, 1.);
               if (!isrising) corr *= 10.; // range for falling edge is 50 ns.
            } else {

               // main calibration for fine counter
               corr = ExtractCalibr(isrising ? rec.rising_calibr : rec.falling_calibr, fine);

               // apply TOT shift for falling edge (should it be also temp dependent)?
               if (!isrising) corr += rec.tot_shift*1e-9;

               // negative while value should be add to the stamp
               if (do_temp_comp) corr -= (fCurrentTemp + fTempCorrection - fCalibrTemp) * rec.time_shift_per_grad * 1e-9;
            }
         }

         // apply correction
         localtm -= corr;

         if ((chid==0) && (ch0time==0)) ch0time = localtm;

         if (IsTriggeredAnalysis()) {
            if (ch0time==0)
               ADDERROR(errCh0, "channel 0 time not found when first HIT in channel %u appears", chid);

            localtm -= ch0time;
         }

         // printf("%s first %d ch %3u tm %12.9f\n", GetName(), first_scan, chid, localtm);

         // fill histograms only for normal channels
         if (first_scan) {

            // if (chid>0) printf("%s HIT ch %u tm %12.9f diff %12.9f\n", GetName(), chid, localtm, localtm - ch0time);

            if (chid==0) {
               if (fLastRateTm < 0) fLastRateTm = ch0time;
               if (ch0time - fLastRateTm > 1) {
                  FillH1(fHitsRate, fRateCnt / (ch0time - fLastRateTm));
                  fLastRateTm = ch0time;
                  fRateCnt = 0;
               }
            } else {
               fRateCnt++;
            }

            // ensure that histograms are created
            if ((HistFillLevel() > 2) && !rec.fRisingFine)
               CreateChannelHistograms(chid);

            bool use_fine_for_stat = true;
            if (!f400Mhz && !msg.isHit0Msg())
               use_fine_for_stat = false;
            else if (use_for_calibr == 3)
               use_fine_for_stat = (gTrigDWindowLow <= localtm*1e9) && (localtm*1e9 <= gTrigDWindowHigh);

            FastFillH1(fChannels, chid);
            DefFillH1(fHits, (chid + (isrising ? 0.25 : 0.75)), 1.);
            if (raw_hit) DefFillH2(fAllFine, chid, fine, 1.);
            DefFillH2(fAllCoarse, chid, coarse, 1.);
            if (fChHitsPerHld) DefFillH2(*fChHitsPerHld, fHldId, chid, 1);

            if (msg.isHit1Msg()) {
               DefFillH1(fCorrHits, chid, 1);
               if (fChCorrPerHld) DefFillH2(*fChCorrPerHld, fHldId, chid, 1);
            }

            if (isrising) {
               // processing of rising edge
               if (raw_hit && use_fine_for_stat) {
                  switch (use_for_calibr) {
                     case 1:
                     case 3:
                        rec.rising_stat[fine]++;
                        rec.all_rising_stat++;
                        if (fCalHitsPerBrd) DefFillH2(*fCalHitsPerBrd, fSeqeunceId, chid, 1.); // accumulate only rising edges
                        break;
                     case 2:
                        rec.last_rising_fine = fine;
                        break;
                  }
               }

               if (raw_hit) FastFillH1(rec.fRisingFine, fine);

               rec.rising_cnt++;

               bool print_cond = false;

               //JAM 11-2021: prevdiff histogram against rising edges of previous channel:
//                if(chid>0 && fCh[chid].rising_last_tm){
//                     double prevdiff=(fCh[chid-1].rising_last_tm - localtm) * 1e9;
               // JAM 7-12-21: better plot dt against ref channel?
                if(chid>0 && ch0time){
                     double refdiff=(localtm - ch0time) * 1e9;
                     if(refdiff > -1000 && refdiff < 1000) {
                        DefFillH2(fhRisingPrevDiffVsChannel, chid, refdiff, 1.);
                        if(fTPreviousPerTDCChannel)
                            SetH2Content(*fTPreviousPerTDCChannel, fHldId, chid,  refdiff);  // show only most recent value
                     }

               }


               rec.rising_last_tm = localtm;
               rec.rising_new_value = true;

               if (use_for_ref && ((rec.rising_hit_tm == 0.) || fUseLastHit)) {
                  rec.rising_hit_tm = (chid > 0) ? localtm : ch0time;
                  rec.rising_coarse = coarse;
                  rec.rising_fine = fine;

                  if ((rec.rising_cond_prnt > 0) && (rec.reftdc == GetID()) &&
                      (rec.refch < NumChannels()) && (fCh[rec.refch].rising_hit_tm != 0.)) {
                     double diff = (localtm - fCh[rec.refch].rising_hit_tm) * 1e9;
                     if (TestC1(rec.fRisingRefCond, diff) == 0) {
                        rec.rising_cond_prnt--;
                        print_cond = true;
                     }
                  }
               }

               if (print_cond) rawprint = true;

               // special case - when ref channel defined as 0, fill all hits
               if ((chid != 0) && (rec.refch == 0) && (rec.reftdc == GetID()) && use_for_ref) {
                  rec.rising_ref_tm = localtm;

                  DefFillH1(rec.fRisingRef, (localtm*1e9), 1.);

                  if (IsPrintRawData() || print_cond)
                  printf("Difference rising %04x:%02u\t %04x:%02u\t %12.3f\t %12.3f\t %7.3f  coarse %03x - %03x = %4d  fine %03x %03x \n",
                          GetID(), chid, rec.reftdc, rec.refch,
                          localtm*1e9,  fCh[0].rising_hit_tm*1e9, localtm*1e9,
                          coarse, fCh[0].rising_coarse, (int) (coarse - fCh[0].rising_coarse),
                          fine, fCh[0].rising_fine);
               }

            } else {
               // processing of falling edge
               if (raw_hit && use_fine_for_stat) {
                  switch (use_for_calibr) {
                     case 1:
                     case 3:
                        rec.falling_stat[fine]++;
                        rec.all_falling_stat++;
                        break;
                     case 2:
                        rec.last_falling_fine = fine;
                        break;
                  }
               }

               if (raw_hit) FastFillH1(rec.fFallingFine, fine);

               rec.falling_cnt++;

               if (rec.rising_new_value && (rec.rising_last_tm!=0)) {
                  double tot = (localtm - rec.rising_last_tm)*1e9;
                  // TODO chid
                  DefFillH2(fhTotVsChannel, chid, tot, 1.);

                  if (fhTotMinusCounter && (tot < 0. )) {
                      DefFillH1(fhTotMinusCounter, chid, 1.);
                  }
                  if (fhTotMoreCounter && (tot > fTotUpperLimit)) {
                      DefFillH1(fhTotMoreCounter, chid, 1.);
                  }
                  DefFillH1(rec.fTot, tot, 1.);
                  rec.rising_new_value = false;
                  // JAM 11-2021: add ToT sigma histogram here:
                  double totvar=  pow((tot-fToTvalue),2.0);
                  double totsigma = sqrt(totvar);

                  DefFillH2(fhSigmaTotVsChannel, chid, totsigma, 1.);

                   // here put something new for global histograms JAM2021:
                   if (fToTPerTDCChannel) {

                            // we first always show most recent value, no averaging here;
                            if((tot>0) && (tot < 1000)) // JAM 7-12-21 suppress noise fakes
                                SetH2Content(*fToTPerTDCChannel, fHldId, chid, tot);

                            // TODO: later get previous statistics for this channel and weight new entry correctly for averaging: (performance?!)
//                             int nBins1, nBins2;
//                             GetH2NBins(fhTotVsChannel, nBins1, nBins2);
//                             double nEntries = 0.;
//                             for (int i = 0; i < nBins2; i++){
//                                 nEntries += GetH2Content(fhTotVsChannel, chid, i);
//                                 }
//                            double oldtot= GetH2Content(*fToTPerTDCChannel, fHldId, chid);
//                            double averagetot=tot;
//                            if (nEntries) averagetot= (oldtot * (nEntries-1) + tot) / nEntries;
//                            SetH2Content(*fToTPerTDCChannel, fHldId, chid, averagetot);
                        }
                     if(fDevPerTDCChannel && (tot>0) && (tot < 1000)) // JAM 7-12-21 suppress noise fakes
                     {
                            rec.tot_dev+=totvar; // JAM misuse  this data field to get overall sigma of file
                            rec.tot0d_cnt++; // JAM misuse calibration counter here to evaluate sigma
                            double currentsigma= sqrt(rec.tot_dev/rec.tot0d_cnt);
                            if(currentsigma<10)
                                SetH2Content(*fDevPerTDCChannel, fHldId, chid,  currentsigma);
                    }


                  // use only raw hit
                  if (raw_hit && do_tot) rec.last_tot = tot + rec.tot_shift;
               }
            }

            if (!iserr) {
               hitcnt++;
               if ((minimtm == 0.) || (localtm < minimtm)) minimtm = localtm;

               if (dostore)
                  switch(GetStoreKind()) {
                     case 1:
                        pStoreVect->emplace_back(msg, (chid>0) ? localtm : ch0time);
                        break;
                     case 2:
                        if (chid>0)
                           pStoreFloat->emplace_back(chid, isrising, localtm*1e9);
                        break;
                     case 3:
                        pStoreDouble->emplace_back(chid, isrising, ch0time + localtm);
                        break;
                     default: break;
                  }
            }
         } else

         // for second scan we check if hit can be assigned to the events
         if (((chid > 0) || fCh0Enabled) && !iserr) {

            base::GlobalTime_t globaltm = LocalToGlobalTime(localtm, &help_index);

            // printf("TDC%u Test TDC message local:%11.9f global:%11.9f\n", GetID(), localtm, globaltm);

            // we test hits, but do not allow to close events
            unsigned indx = TestHitTime(globaltm, true, false);

            if (indx < fGlobalMarks.size()) {
               switch(GetStoreKind()) {
                  case 1:
                     AddMessage(indx, (hadaq::TdcSubEvent*) fGlobalMarks.item(indx).subev, hadaq::TdcMessageExt(msg, chid>0 ? globaltm : ch0time));
                     break;
                  case 2:
                     if (chid>0)
                        AddMessage(indx, (hadaq::TdcSubEventFloat*) fGlobalMarks.item(indx).subev, hadaq::MessageFloat(chid, isrising, (globaltm - ch0time)*1e9));
                     break;
                  case 3:
                     AddMessage(indx, (hadaq::TdcSubEventDouble*) fGlobalMarks.item(indx).subev, hadaq::MessageDouble(chid, isrising, globaltm));
                     break;
               }
            }
         }

         continue;
      }

      // process other messages kinds

      switch (msg.getKind()) {
        case tdckind_Trailer:
           if (first_scan) fLastTdcTrailer = msg;
           break;
        case tdckind_Header: {
           // never come here - handle in very begin
           if (first_scan) fLastTdcHeader = msg;
           /*unsigned errbits = msg.getHeaderErr();
           if (errbits && first_scan) {
              if (CheckPrintError())
                 printf("%5s found error bits: 0x%x\n", GetName(), errbits);
           }
           */

           break;
        }
        case tdckind_Debug: {
           switch (msg.getDebugKind()) {
              case 0x0E: temp = msg.getDebugValue();  break;
              case 0x10: lowid = msg.getDebugValue(); break;
              case 0x11: highid = msg.getDebugValue(); break;
           }

           break;
        }
        case tdckind_Epoch:
           break;
        default:
           ADDERROR(errUncknHdr, "Unknown message header 0x%x", msg.getKind());
           break;
      }
   }

   if (isfirstepoch && !iserr) {
      // if we want to compensate epoch reset, use epoch of trigger channel+1
      // +1 to exclude small probability of hits after trigger with epoch+1 value
      if (fCompensateEpochReset && first_scan)
         fCompensateEpochCounter = first_epoch+1;

      iter.setRefEpoch(first_epoch);
   }

   if (first_scan) {

      // special case for 0xD trigger - use only last hit messages for accumulating statistic
      if (use_for_calibr == 2)
         for (unsigned ch=0;ch<NumChannels();ch++) {
            ChannelRec& rec = fCh[ch];
            if (rec.last_rising_fine > 0) {
               rec.rising_stat[rec.last_rising_fine]++;
               rec.all_rising_stat++;
               rec.last_rising_fine = 0;
               if (fCalHitsPerBrd) DefFillH2(*fCalHitsPerBrd, fSeqeunceId, ch, 1.); // accumulate only rising edges
            }
            if (rec.last_falling_fine > 0) {
               rec.falling_stat[rec.last_falling_fine]++;
               rec.all_falling_stat++;
               rec.last_falling_fine = 0;
            }
         }

      // when doing TOT calibration, use only last TOT value - before one could find other signals
      if (do_tot)
         for (unsigned ch = 1; ch < NumChannels(); ch++) {
            // printf("%s Channel %d last_tot %5.3f has_calibr %d min %5.2f max %5.2f \n", GetName(), ch, fCh[ch].last_tot, fCh[ch].hascalibr, fToThmin, fToThmax);

            if (fCh[ch].hascalibr) {
               if ((fCh[ch].last_tot >= fToThmin) && (fCh[ch].last_tot < fToThmax)) {
                  if (fCh[ch].tot0d_hist.empty()) fCh[ch].CreateToTHist();
                  int bin = (int) ((fCh[ch].last_tot - fToThmin) / (fToThmax - fToThmin) * TotBins);
                  fCh[ch].tot0d_hist[bin]++;
                  fCh[ch].tot0d_cnt++;
               } else {
                  fCh[ch].tot0d_misscnt++;
               }
            }
            fCh[ch].last_tot = 0.;
         }

      if (lowid || highid) {
         unsigned id = (highid << 16) | lowid;

         if ((fDesignId != 0) && (fDesignId != id))
            ADDERROR(errDesignId, "mismatch in design id before:0x%x now:0x%x", fDesignId, id);

         fDesignId = id;
      }

      if ((temp!=0) && (temp < 2400)) {
         fCurrentTemp = temp/16.;

         if ((HistFillLevel() > 1) && !fTempDistr)  {
            printf("%s FirstTemp:%5.2f CalibrTemp:%5.2f UseTemp:%d\n", GetName(), fCurrentTemp, fCalibrTemp, fCalibrUseTemp);

            int mid = round(fCurrentTemp);
            SetSubPrefix2();
            fTempDistr = MakeH1("Temperature", "Temperature distribution", 600, mid-30, mid+30, "C");
         }
         FillH1(fTempDistr, fCurrentTemp);
      }

      if ((hitcnt>0) && (use_for_calibr>0) && (fCurrentTemp>0)) {
         fCalibrTempSum0 += 1.;
         fCalibrTempSum1 += fCurrentTemp;
         fCalibrTempSum2 += fCurrentTemp*fCurrentTemp;
      }

      // if we use trigger as time marker
      if (fUseNativeTrigger && (ch0time!=0)) {
         base::LocalTimeMarker marker;
         marker.localid = 1;
         marker.localtm = ch0time;

         // printf("%s Create TRIGGER %11.9f\n", GetName(), ch0time);

         AddTriggerMarker(marker);
      }

      if ((syncid != 0xffffffff) && (ch0time!=0)) {

         // printf("%s Create SYNC %u tm %12.9f\n", GetName(), syncid, ch0time);
         base::SyncMarker marker;
         marker.uniqueid = syncid;
         marker.localid = 0;
         marker.local_stamp = ch0time;
         marker.localtm = ch0time;
         AddSyncMarker(marker);
      }

      buf().local_tm = minimtm;

//      printf("Proc:%p first scan iserr:%d  minm: %12.9f\n", this, iserr, minimtm*1e-9);

      if (fMsgPerBrd) DefFillH1(*fMsgPerBrd, fSeqeunceId, cnt);

      // fill number of "good" hits
      if (fHitsPerBrd) DefFillH1(*fHitsPerBrd, fSeqeunceId, hitcnt);

      // number of hits per TDC in HLD
      if (fHitsPerHld) DefFillH1(*fHitsPerHld, fHldId, hitcnt);

      if (iserr || missinghit) {
         if (fErrPerBrd) DefFillH1(*fErrPerBrd, fSeqeunceId, 1.);
         if (fErrPerHld) DefFillH1(*fErrPerHld, fHldId, 1.);
      }
   } else {

      // use first channel only for flushing
      if (ch0time!=0)
         TestHitTime(LocalToGlobalTime(ch0time, &help_index), false, true);
   }

   // if no cross-processing required, can fill extra histograms directly
   if (first_scan && !IsCrossProcess()) AfterFill();

   if (rawprint && first_scan) {
      printf("%s RAW data event 0x%x\n", GetName(), (unsigned) (GetHLD() ? GetHLD()->GetEventId() : 0));
      TdcIterator iter;
      if (buf().format==0)
         iter.assign((uint32_t*) buf.ptr(4), buf.datalen()/4-1, false);
      else
         iter.assign((uint32_t*) buf.ptr(0), buf.datalen()/4, buf().format==2);
      while (iter.next()) iter.printmsg();
   }

   return !iserr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Scan all messages, find reference signals
/// Major data analysis method

bool hadaq::TdcProcessor::DoBuffer4Scan(const base::Buffer& buf, bool first_scan)
{
   if (buf.null()) {
      if (first_scan) printf("Something wrong - empty buffer should not appear in the first scan/n");
      return false;
   }

   uint32_t syncid(0xffffffff);
   // copy first 4 bytes - it is syncid
   if (buf().format==0)
      memcpy(&syncid, buf.ptr(), 4);

   int buf_kind = buf().kind;

   // if data could be used for calibration
   int use_for_calibr = first_scan && (((1 << buf_kind) & fCalibrTriggerMask) != 0) ? 1 : 0;

   // use in ref calculations only physical trigger, exclude 0xD or 0xE
   bool use_for_ref = buf_kind < (gUseDTrigForRef ? 0xE : 0xD);

   // disable taking last hit for trigger DD
   if ((use_for_calibr > 0) && ((buf_kind == 0xD) || gUseAsDTrig)) {
      if (IsTriggeredAnalysis() &&  (gTrigDWindowLow < gTrigDWindowHigh)) use_for_calibr = 3; // accept time stamps only for inside window
      // use_for_calibr = 2; // always use only last hit
   }

   // if data could be used for TOT calibration
   bool do_tot = (use_for_calibr > 0) && ((buf_kind == 0xD) || gUseAsDTrig) && DoFallingEdge();

   // use temperature compensation only when temperature available
   bool do_temp_comp = fCalibrUseTemp && (fCurrentTemp > 0) && (fCalibrTemp > 0) && (fabs(fCurrentTemp - fCalibrTemp) < 30.);

   unsigned cnt(0), hitcnt(0);

   bool iserr(false), isfirstepoch(false), rawprint(false), missinghit(false), dostore(false);

   if (first_scan && IsTriggeredAnalysis() && IsStoreEnabled() && mgr()->HasTrigEvent()) {
      dostore = true;
      switch (GetStoreKind()) {
         case 1: {
            hadaq::TdcSubEvent* subevnt = new hadaq::TdcSubEvent(buf.datalen()/6);
            mgr()->AddToTrigEvent(GetName(), subevnt);
            pStoreVect = subevnt->vect_ptr();
            break;
         }
         case 2: {
            hadaq::TdcSubEventFloat* subevnt = new hadaq::TdcSubEventFloat(buf.datalen()/6);
            mgr()->AddToTrigEvent(GetName(), subevnt);
            pStoreFloat = subevnt->vect_ptr();
            break;
         }
         case 3: {
            hadaq::TdcSubEventDouble* subevnt = new hadaq::TdcSubEventDouble(buf.datalen()/6);
            mgr()->AddToTrigEvent(GetName(), subevnt);
            pStoreDouble = subevnt->vect_ptr();
            break;
         }

         default: break; // not supported
      }
   }

   //static int ddd = 0;
   //if (ddd++ % 10000 == 0) printf("%s dostore %d istriggered %d hasevt %d kind %d\n", GetName(), dostore, IsTriggeredAnalysis(), mgr()->HasTrigEvent(), GetStoreKind());

   uint32_t first_epoch(0);

   unsigned epoch_shift = 0;
   if (fCompensateEpochReset) {
      if (first_scan) buf().user_tag = fCompensateEpochCounter;
      epoch_shift = buf().user_tag;
   }

   TdcIterator& iter = first_scan ? fIter1 : fIter2;

   if (buf().format==0)
      iter.assign((uint32_t*) buf.ptr(4), buf.datalen()/4-1, false);
   else
      iter.assign((uint32_t*) buf.ptr(0), buf.datalen()/4, buf().format==2);

   unsigned help_index(0);

   double localtm(0.), minimtm(0), ch0time(0);

   hadaq::TdcMessage& msg = iter.msg();
   hadaq::TdcMessage calibr;
   unsigned ncalibr(20), lowid(0), highid(0); // clear indicate that no calibration data present
   bool nextTMDTfailure = false;

   if (fSkipTdcMessages > 0) {
      unsigned skipcnt = fSkipTdcMessages;
      while (skipcnt-- > 0)
         if (!iter.next4()) break;
   }

   while (iter.next4()) {

      // if (!first_scan) msg.print();

      cnt++;

      if (cnt==1) {
         if (!msg.isHDR()) {
            iserr = true;
            ADDERROR(errNoHeader, "Missing header message");
         } else {

            if (first_scan) fLastTdcHeader = msg;

            // TODO: ToT configuring by header, now should be stored in footer

            // if (fToTdflt && first_scan)
            //   ConfigureToTByHwType(msg.getHeaderHwType());
         }

         continue;
      }

      if (msg.isEPOC()) {

         if (first_scan && fMsgsKind)
            FastFillH1(fMsgsKind, 1);

         uint32_t ep = iter.getCurEpoch();
         nextTMDTfailure = msg.getEPOCError();

         if (fCompensateEpochReset) {
            ep += epoch_shift;
            iter.setCurEpoch(ep);
         }

         // second message always should be epoch of channel 0
         if (cnt == 2) {
            isfirstepoch = true;
            first_epoch = ep;
         }
         continue;
      }

      if (msg.isCalibrMsg()) {
         if ((use_for_calibr == 0) && !gIgnoreCalibrMsgs) {
            // take into account calibration messages only when data not used for calibration
            ncalibr = 0;
            calibr = msg;
         }
         continue;
      }

      if (msg.isTMDS()) {
         unsigned chid = msg.getTMDSChannel() + 1;
         unsigned coarse = msg.getTMDSCoarse();
         unsigned pattern = msg.getTMDSPattern();

         if (chid >= NumChannels()) {
            ADDERROR(errChId, "Channel number %u bigger than configured %u", chid, NumChannels());
            iserr = true;
            continue;
         }

         ChannelRec& rec = fCh[chid];

         localtm = (((uint64_t) iter.getCurEpoch()) << 12 | coarse) * hadaq::TdcMessage::CoarseUnit280(); // 280 MHz

         unsigned mask = 0x100, cnt = 8;
         while (((pattern & mask) == 0) && (cnt > 0)) {
            mask = mask >> 1;
            cnt--;
         }
         localtm -= hadaq::TdcMessage::CoarseUnit280()/8*cnt;

         if (IsTriggeredAnalysis()) {
            if (ch0time==0)
               ADDERROR(errCh0, "channel 0 time not found when first HIT in channel %u appears", chid);
            localtm -= ch0time;
         }

         if (rec.rising_tmds == 0)
            rec.rising_tmds = localtm;

         continue;
      }

      if (msg.isTMDR() || msg.isTMDT()) {

         unsigned chid = 0, fine = 0, coarse = 0;
         bool isrising = false, isfalling = false;

         if (msg.isTMDR()) {
            if (first_scan && fMsgsKind)
               FastFillH1(fMsgsKind, 2);

            chid = 0;
            isrising = (msg.getTMDRMode() == 0) || (msg.getTMDRMode() == 2);
            isfalling = (msg.getTMDRMode() == 1) || (msg.getTMDRMode() == 3);
            coarse = msg.getTMDRCoarse();
            fine = msg.getTMDRFine();
         } else {
            if (first_scan && fMsgsKind)
               FastFillH1(fMsgsKind, 3);

            chid = msg.getTMDTChannel() + 1;
            isrising = (msg.getTMDTMode() == 0) || (msg.getTMDTMode() == 2);
            isfalling = (msg.getTMDTMode() == 1) || (msg.getTMDTMode() == 3);
            coarse = msg.getTMDTCoarse();
            fine = msg.getTMDTFine();

            if (nextTMDTfailure) {
               FastFillH1(fErrors, chid);
               nextTMDTfailure = false;
               continue;
            }
         }

         if (!isrising && !isfalling) continue;

         // localtm = iter.getMsgTimeCoarse();

         localtm = (((uint64_t) iter.getCurEpoch()) << 12 | coarse) * hadaq::TdcMessage::CoarseUnit280(); // 280 MHz

         if (chid >= NumChannels()) {
            ADDERROR(errChId, "Channel number %u bigger than configured %u", chid, NumChannels());
            iserr = true;
            continue;
         }

         ChannelRec& rec = fCh[chid];

         if ((fine == 0x1ff) || (fine == 0x1fe) || (fine == 0x1fd)) {
            if (first_scan) {
               // special case - missing hit, just ignore such value
               ADDERROR(err3ff, "Missing hit in channel %u fine counter is %x", chid, fine);

               missinghit = true;
               FastFillH1(fChannels, chid);
               FastFillH1(fUndHits, chid);

               if (fChErrPerHld) DefFillH2(*fChErrPerHld, fHldId, chid, 1);
            }
            continue;
         }

         if (fine >= fNumFineBins) {
            FastFillH1(fErrors, chid);
            if (fChErrPerHld) DefFillH2(*fChErrPerHld, fHldId, chid, 1);
            ADDERROR(errFine, "Fine counter %u out of allowed range 0..%u in channel %u", fine, fNumFineBins, chid);
            iserr = true;
            continue;
         }

         double corr = 0.;

         if (ncalibr < 2) {
            // use correction from special message
            uint32_t calibr_fine = calibr.getCalibrFine(ncalibr++);
            if (calibr_fine == 0x3fff) {
               FastFillH1(fErrors, chid);
               if (fChErrPerHld) DefFillH2(*fChErrPerHld, fHldId, chid, 1);
               ADDERROR(errFine, "Invalid value in calibratem message for channel %u", chid);
               iserr = true;
               continue;
            }

            corr = calibr_fine*5e-9/0x3ffe;
            if (isrising && fhRaisingFineCalibr) DefFillH2(fhRaisingFineCalibr, chid, calibr_fine, 1.);
            if (!isrising) corr *= 10.; // range for falling edge is 50 ns.
         } else {

            // main calibration for fine counter
            corr = ExtractCalibr(isrising ? rec.rising_calibr : rec.falling_calibr, fine);

            // apply TOT shift for falling edge (should it be also temp dependent)?
            if (!isrising) corr += rec.tot_shift*1e-9;

            // negative while value should be add to the stamp
            if (do_temp_comp) corr -= (fCurrentTemp + fTempCorrection - fCalibrTemp) * rec.time_shift_per_grad * 1e-9;
         }

         // apply correction
         localtm -= corr;

         if ((chid==0) && (ch0time==0) && isrising) ch0time = localtm;

         if (IsTriggeredAnalysis()) {
            if (ch0time==0)
               ADDERROR(errCh0, "channel 0 time not found when first HIT in channel %u appears", chid);

            localtm -= ch0time;
         }

         // printf("%s first %d ch %3u tm %12.9f\n", GetName(), first_scan, chid, localtm);

         // fill histograms only for normal channels
         if (first_scan) {

            // if (chid>0) printf("%s HIT ch %u tm %12.9f diff %12.9f\n", GetName(), chid, localtm, localtm - ch0time);

            if (chid==0) {
               if (fLastRateTm < 0) fLastRateTm = ch0time;
               if (ch0time - fLastRateTm > 1) {
                  FillH1(fHitsRate, fRateCnt / (ch0time - fLastRateTm));
                  fLastRateTm = ch0time;
                  fRateCnt = 0;
               }
            } else {
               fRateCnt++;
            }

            // ensure that histograms are created
            if ((HistFillLevel() > 2) && !rec.fRisingFine)
               CreateChannelHistograms(chid);

            bool raw_hit = true;
            bool use_fine_for_stat = true;
            if (use_for_calibr == 3)
               use_fine_for_stat = (gTrigDWindowLow <= localtm*1e9) && (localtm*1e9 <= gTrigDWindowHigh);

            FastFillH1(fChannels, chid);
            DefFillH1(fHits, (chid + (isrising ? 0.25 : 0.75)), 1.);
            if (raw_hit) DefFillH2(fAllFine, chid, fine, 1.);
            DefFillH2(fAllCoarse, chid, coarse, 1.);
            if (fChHitsPerHld) DefFillH2(*fChHitsPerHld, fHldId, chid, 1);


            if (isrising) {
               // processing of rising edge
               if (raw_hit && use_fine_for_stat) {
                  switch (use_for_calibr) {
                     case 1:
                     case 3:
                        rec.rising_stat[fine]++;
                        rec.all_rising_stat++;
                        if (fCalHitsPerBrd) DefFillH2(*fCalHitsPerBrd, fSeqeunceId, chid, 1.); // accumulate only rising edges
                        break;
                     case 2:
                        rec.last_rising_fine = fine;
                        break;
                  }
               }

               if (raw_hit) FastFillH1(rec.fRisingFine, fine);

               rec.rising_cnt++;

               bool print_cond = false;

               rec.rising_last_tm = localtm;
               rec.rising_new_value = true;

               if (use_for_ref && (fUseLastHit || (rec.rising_hit_tm == 0.))) {
                  rec.rising_hit_tm = (chid > 0) ? localtm : ch0time;
                  rec.rising_coarse = coarse;
                  rec.rising_fine = fine;

                  if ((rec.rising_cond_prnt > 0) && (rec.reftdc == GetID()) &&
                      (rec.refch < NumChannels()) && (fCh[rec.refch].rising_hit_tm!=0)) {
                     double diff = (localtm - fCh[rec.refch].rising_hit_tm) * 1e9;
                     if (TestC1(rec.fRisingRefCond, diff) == 0) {
                        rec.rising_cond_prnt--;
                        print_cond = true;
                     }
                  }
               }

               if (print_cond) rawprint = true;

               // special case - when ref channel defined as 0, fill all hits
               if ((chid!=0) && (rec.refch==0) && (rec.reftdc == GetID()) && use_for_ref) {
                  rec.rising_ref_tm = localtm;

                  DefFillH1(rec.fRisingRef, (localtm*1e9), 1.);

                  if (IsPrintRawData() || print_cond)
                  printf("Difference rising %04x:%02u\t %04x:%02u\t %12.3f\t %12.3f\t %7.3f  coarse %03x - %03x = %4d  fine %03x %03x \n",
                          GetID(), chid, rec.reftdc, rec.refch,
                          localtm*1e9,  fCh[0].rising_hit_tm*1e9, localtm*1e9,
                          coarse, fCh[0].rising_coarse, (int) (coarse - fCh[0].rising_coarse),
                          fine, fCh[0].rising_fine);
               }

            } else {
               // processing of falling edge
               if (raw_hit && use_fine_for_stat) {
                  switch (use_for_calibr) {
                     case 1:
                     case 3:
                        rec.falling_stat[fine]++;
                        rec.all_falling_stat++;
                        break;
                     case 2:
                        rec.last_falling_fine = fine;
                        break;
                  }
               }

               if (raw_hit) FastFillH1(rec.fFallingFine, fine);

               rec.falling_cnt++;

               if (rec.rising_new_value && (rec.rising_last_tm != 0)) {
                  double tot = (localtm - rec.rising_last_tm)*1e9;
                  // TODO chid
                  DefFillH2(fhTotVsChannel, chid, tot, 1.);
                  if (fhTotMinusCounter && (tot < 0. )) {
                     DefFillH1(fhTotMinusCounter, chid, 1.);
                  }
                  if (fhTotMinusCounter && (tot > fTotUpperLimit)) {
                     DefFillH1(fhTotMoreCounter, chid, 1.);
                  }
                  DefFillH1(rec.fTot, tot, 1.);
                  rec.rising_new_value = false;





                  // use only raw hit
                  if (raw_hit && do_tot) rec.last_tot = tot + rec.tot_shift;
               }
            }

            if (!iserr) {
               hitcnt++;
               if ((minimtm==0) || (localtm < minimtm)) minimtm = localtm;

               if (dostore)
                  switch(GetStoreKind()) {
                     case 1:
                        pStoreVect->emplace_back(msg, (chid>0) ? localtm : ch0time);
                        break;
                     case 2:
                        if (chid>0)
                           pStoreFloat->emplace_back(chid, isrising, localtm*1e9);
                        break;
                     case 3:
                        pStoreDouble->emplace_back(chid, isrising, ch0time + localtm);
                        break;
                     default: break;
                  }
            }
         } else

         // for second scan we check if hit can be assigned to the events
         if (((chid>0) || fCh0Enabled) && !iserr) {

            base::GlobalTime_t globaltm = LocalToGlobalTime(localtm, &help_index);

            // printf("TDC%u Test TDC message local:%11.9f global:%11.9f\n", GetID(), localtm, globaltm);

            // we test hits, but do not allow to close events
            unsigned indx = TestHitTime(globaltm, true, false);

            if (indx < fGlobalMarks.size()) {
               switch(GetStoreKind()) {
                  case 1:
                     AddMessage(indx, (hadaq::TdcSubEvent*) fGlobalMarks.item(indx).subev, hadaq::TdcMessageExt(msg, chid>0 ? globaltm : ch0time));
                     break;
                  case 2:
                     if (chid>0)
                        AddMessage(indx, (hadaq::TdcSubEventFloat*) fGlobalMarks.item(indx).subev, hadaq::MessageFloat(chid, isrising, (globaltm - ch0time)*1e9));
                     break;
                  case 3:
                     AddMessage(indx, (hadaq::TdcSubEventDouble*) fGlobalMarks.item(indx).subev, hadaq::MessageDouble(chid, isrising, globaltm));
                     break;
               }
            }
         }

         continue;
      }

      // process other messages kinds

   }

   if (isfirstepoch && !iserr) {
      // if we want to compensate epoch reset, use epoch of trigger channel+1
      // +1 to exclude small probability of hits after trigger with epoch+1 value
      if (fCompensateEpochReset && first_scan)
         fCompensateEpochCounter = first_epoch+1;

      iter.setRefEpoch(first_epoch);
   }

   if (first_scan) {

      // special case for 0xD trigger - use only last hit messages for accumulating statistic
      if (use_for_calibr == 2)
         for (unsigned ch=0;ch<NumChannels();ch++) {
            ChannelRec& rec = fCh[ch];
            if (rec.last_rising_fine > 0) {
               rec.rising_stat[rec.last_rising_fine]++;
               rec.all_rising_stat++;
               rec.last_rising_fine = 0;
               if (fCalHitsPerBrd) DefFillH2(*fCalHitsPerBrd, fSeqeunceId, ch, 1.); // accumulate only rising edges
            }
            if (rec.last_falling_fine > 0) {
               rec.falling_stat[rec.last_falling_fine]++;
               rec.all_falling_stat++;
               rec.last_falling_fine = 0;
            }
         }

      // when doing TOT calibration, use only last TOT value - before one could find other signals
      if (do_tot)
         for (unsigned ch=1;ch<NumChannels();ch++) {
            // printf("%s Channel %d last_tot %5.3f has_calibr %d min %5.2f max %5.2f \n", GetName(), ch, fCh[ch].last_tot, fCh[ch].hascalibr, fToThmin, fToThmax);

            if (fCh[ch].hascalibr && (fCh[ch].last_tot >= fToThmin) && (fCh[ch].last_tot < fToThmax)) {
               if (fCh[ch].tot0d_hist.empty()) fCh[ch].CreateToTHist();
               int bin = (int) ((fCh[ch].last_tot - fToThmin) / (fToThmax - fToThmin) * TotBins);
               fCh[ch].tot0d_hist[bin]++;
               fCh[ch].tot0d_cnt++;
            }
            fCh[ch].last_tot = 0.;
         }

      if (lowid || highid) {
         unsigned id = (highid << 16) | lowid;

         if ((fDesignId != 0) && (fDesignId != id))
            ADDERROR(errDesignId, "mismatch in design id before:0x%x now:0x%x", fDesignId, id);

         fDesignId = id;
      }

      /*
      if ((temp != 0) && (temp < 2400)) {
         fCurrentTemp = temp/16.;

         if ((HistFillLevel() > 1) && (fTempDistr == 0))  {
            printf("%s FirstTemp:%5.2f CalibrTemp:%5.2f UseTemp:%d\n", GetName(), fCurrentTemp, fCalibrTemp, fCalibrUseTemp);

            int mid = round(fCurrentTemp);
            SetSubPrefix2();
            fTempDistr = MakeH1("Temperature", "Temperature distribution", 600, mid-30, mid+30, "C");
         }
         FillH1(fTempDistr, fCurrentTemp);
      }

      if ((hitcnt>0) && (use_for_calibr>0) && (fCurrentTemp>0)) {
         fCalibrTempSum0 += 1.;
         fCalibrTempSum1 += fCurrentTemp;
         fCalibrTempSum2 += fCurrentTemp*fCurrentTemp;
      }

      */

      // if we use trigger as time marker
      if (fUseNativeTrigger && (ch0time!=0)) {
         base::LocalTimeMarker marker;
         marker.localid = 1;
         marker.localtm = ch0time;
         AddTriggerMarker(marker);
      }

      if ((syncid != 0xffffffff) && (ch0time!=0)) {
         base::SyncMarker marker;
         marker.uniqueid = syncid;
         marker.localid = 0;
         marker.local_stamp = ch0time;
         marker.localtm = ch0time;
         AddSyncMarker(marker);
      }

      buf().local_tm = minimtm;

//      printf("Proc:%p first scan iserr:%d  minm: %12.9f\n", this, iserr, minimtm*1e-9);

      if (fMsgPerBrd) DefFillH1(*fMsgPerBrd, fSeqeunceId, cnt);

      // fill number of "good" hits
      if (fHitsPerBrd) DefFillH1(*fHitsPerBrd, fSeqeunceId, hitcnt);

      // number of hits per TDC in HLD
      if (fHitsPerHld) DefFillH1(*fHitsPerHld, fHldId, hitcnt);

      if (iserr || missinghit) {
         if (fErrPerBrd) DefFillH1(*fErrPerBrd, fSeqeunceId, 1.);
         if (fErrPerHld) DefFillH1(*fErrPerHld, fHldId, 1.);
      }
   } else {

      // use first channel only for flushing
      if (ch0time!=0)
         TestHitTime(LocalToGlobalTime(ch0time, &help_index), false, true);
   }

   // if no cross-processing required, can fill extra histograms directly
   if (first_scan && !IsCrossProcess()) AfterFill();

   if (rawprint && first_scan) {
      printf("%s RAW data event 0x%x\n", GetName(), (unsigned) (GetHLD() ? GetHLD()->GetEventId() : 0));
      TdcIterator iter;
      if (buf().format==0)
         iter.assign((uint32_t*) buf.ptr(4), buf.datalen()/4-1, false);
      else
         iter.assign((uint32_t*) buf.ptr(0), buf.datalen()/4, buf().format==2);

      iter.printall4();
   }

   return !iserr;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Special hades histograms creation

void hadaq::TdcProcessor::DoHadesHistAnalysis()
{
    int nofChannels = NumChannels();
    for (int iCh = 0; iCh < nofChannels; iCh++) {
        double testFine = DoTestFineTimeH2(iCh, fAllFine);
        double testTot = DoTestToT(iCh);
        double testEdges = DoTestEdges(iCh);
        double testErrors = DoTestErrors(iCh);

        if (fQaFinePerHld) SetH2Content(*fQaFinePerHld, fHldId, iCh, testFine);
        if (fQaToTPerHld) SetH2Content(*fQaToTPerHld, fHldId, iCh, testTot);
        if (fQaEdgesPerHld) SetH2Content(*fQaEdgesPerHld, fHldId, iCh, testEdges);
        if (fQaErrorsPerHld) SetH2Content(*fQaErrorsPerHld, fHldId, iCh, testErrors);
        // JAM 2021:
        if(fExpectedToTPerTDC)
            {
               SetH1Content(*fExpectedToTPerTDC, fHldId, fToTvalue);
            }

        ChannelRec& rec = fCh[iCh];
        if(fShiftPerTDCChannel)
           {
              SetH2Content(*fShiftPerTDCChannel, fHldId, iCh,  rec.tot_shift);
           }

            //tot_dev - this is not recovered from calibration files! we might not fill it here
//         if(fDevPerTDCChannel)
//              {
//               SetH2Content(*fDevPerTDCChannel, fHldId, iCh,  rec.tot_dev); // JAM DEBUG rec.tot_dev
//              }

    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Test ToT

double hadaq::TdcProcessor::DoTestToT(int iCh)
{
   double dresult =0;
   if (!hadaq::TdcProcessor::IsHadesReducedMonitoring() && fhTotVsChannel && fhTotMoreCounter && fhTotMinusCounter) {
        int nBins1, nBins2;
        GetH2NBins(fhTotVsChannel, nBins1, nBins2);

        int nBins = GetH1NBins(fhTotMoreCounter);
        if (iCh < 0 || iCh >= nBins) return 0.;
        double moreContent = GetH1Content(fhTotMoreCounter, iCh);
        double minusContent = GetH1Content(fhTotMinusCounter, iCh);

        double nEntries = 0.;
        for (int i = 0; i < nBins2; i++){
            nEntries += GetH2Content(fhTotVsChannel, iCh, i);
        }

        if (nEntries == 0) return 0.;
        double ratio = (moreContent + minusContent) / nEntries;

        int result = (int) (100. * (1. - ratio));
        if (result <= 0) result = 0;
        if (result >= 99) result = 99;

        double resultEntries = (nEntries >= 100) ? 0.99 : (nEntries / 100.);
        dresult = 1.*result + resultEntries;
     }

    return dresult;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Test errors in channel

double hadaq::TdcProcessor::DoTestErrors(int iCh)
{
    int nBins = GetH1NBins(fErrors);
    if (iCh < 0 || iCh >= nBins) return 0.;

    double nEntries = GetH1Content(fChannels, iCh);

    double nErrors = GetH1Content(fErrors, iCh) + GetH1Content(fUndHits, iCh) + GetH1Content(fCorrHits, iCh);

    if (nEntries <= 0) return 0.;

    if (nErrors == 0) return 100.;

    double ratio = nErrors / nEntries;
    // if (ratio > 0.01) return 1.;

    // expand range by factor 100 that error rate bigger than 0.01 is already RED color
    int result = 100 - (int) 100/0.01 * ratio;
    if (result <= 0) result = 0;
    if (result >= 99) result = 99;

    double resultEntries = (nEntries >= 100) ? 0.99 : (nEntries / 100.);
    double dresult = 1.*result + resultEntries;

    return dresult;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Test edges count

double hadaq::TdcProcessor::DoTestEdges(int iCh)
{
    if (iCh == 0) return 0.;

    int nBins = GetH1NBins(fHits);
    if (iCh < 0 || iCh * 2  + 1 >= nBins) return 0.;
    double raising = GetH1Content(fHits, 2 * iCh);
    double falling = GetH1Content(fHits, 2 * iCh + 1);

    double nEntries = raising + falling;

    if (nEntries < 10) return 0.;
    double ratio = std::fabs(raising - falling) / (0.5 * nEntries);

    int result = (int) (100. * (1. - ratio));
    if (result <= 0) result = 0;
    if (result >= 100) result = 100;

    // change scale by factor 6, starting from 100
    // idea to expand range [90..100] to [70..100] where 70 is visible boundary on histogram
    result = 100 - (100-result) * 3;
    if (result < 0) result = 0; else if (result > 99) result = 99;

    double resultEntries = (nEntries >= 100) ? 0.99 : (nEntries / 100.);
    double dresult = 1.*result + resultEntries;
    return dresult;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Test fine time

double hadaq::TdcProcessor::DoTestFineTimeH2(int iCh, base::H2handle h)
{
    int nBins1, nBins2;
    GetH2NBins(h, nBins1, nBins2);
    const int binSizeRebin = 32;
    const int nBinsRebin = nBins2 / binSizeRebin + 1;

    double hRebin[nBinsRebin];
    int nEntries = 0;
    for (int i = 0; i < nBinsRebin; i++){
        hRebin[i] = 0;
        for (int j = i * binSizeRebin; j < (i + 1) * binSizeRebin; j++){
            if (j >= nBins2) break;
            hRebin[i] += GetH2Content(h, iCh, j);
        }
        nEntries += hRebin[i];
    }
    return DoTestFineTime(hRebin, nBinsRebin, nEntries);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Test fine time

double hadaq::TdcProcessor::DoTestFineTime(double hRebin[], int nBinsRebin, int nEntries)
{
    if (nEntries < 50) return 0.;

    // shrink zero Edges
    int indMinRebin = 0;
    int indMaxRebin = 0;
    for (int i = 0; i < nBinsRebin; i++){
        if (hRebin[i] != 0){
            indMinRebin = i;
            break;
        }
    }
    for (int i = nBinsRebin - 1; i >= 0; i--){
        if (hRebin[i] != 0){
            indMaxRebin = i;
            break;
        }
    }
    int nBinsRebinShrinked = indMaxRebin - indMinRebin + 1;

    double mean = 0.;
    for (int i = indMinRebin; i <= indMaxRebin; i++) {
        mean += hRebin[i];
    }
    mean = mean / (double)nBinsRebinShrinked;
    if (mean == 0.) return 0.;

    //double mad = 0.; // Mean Absolute Deviation
    double stDev = 0.; //standard deviation
    for (int i = indMinRebin; i <= indMaxRebin; i++) {
        double d = hRebin[i] - mean;
        //mad += std::abs(d);
        stDev += d * d;
    }
    //mad /= nBinsRebinShrinked;
    stDev = std::sqrt(stDev / nBinsRebinShrinked);

    double k = 0.3;
    double ratio = (stDev/mean > 1.)?1.:k * stDev/mean;
    //double ratio = k * stDev/mean;
    if (ratio >= 1.) ratio = 1.;
    if ((double)nBinsRebinShrinked/(double)nBinsRebin < 0.5) ratio = 1.;

    int result = (int) (100. * (1. - ratio));
    if (result <= 0) result = 0;
    if (result >= 99) result = 99;

    double resultEntries = (nEntries >= 100) ? 0.99 : (nEntries / 100.);
    double dresult = 1.*result + resultEntries;
    return dresult;
}

///////////////////////////////////////////////////////////////////////////////////////
/// Method will be called by TRB processor if SYNC message was found
///  One should change 4 first bytes in the last buffer in the queue

void hadaq::TdcProcessor::AppendTrbSync(uint32_t syncid)
{
   if (fQueue.size() == 0) {
      printf("something went wrong!!!\n");
      exit(765);
   }

   memcpy(fQueue.back().ptr(), &syncid, 4);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Configure linear calibration for the channel

void hadaq::TdcProcessor::SetLinearCalibration(unsigned nch, unsigned finemin, unsigned finemax)
{
   if (nch < NumChannels())
      fCh[nch].SetLinearCalibr(finemin, finemax);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Calibrate channel

double hadaq::TdcProcessor::CalibrateChannel(unsigned nch, bool rising, const std::vector<uint32_t> &statistic, std::vector<float> &calibr, bool use_linear, bool preliminary)
{
   double sum = 0., limits = use_linear ? 100 : 1000;
   unsigned finemin = 0, finemax = 0;
   std::vector<double> integral(fNumFineBins, 0.);
   for (unsigned n=0;n<fNumFineBins;n++) {
      if (statistic[n]) {
         if (sum == 0.) finemin = n; else finemax = n;
      }

      sum += statistic[n];
      integral[n] = sum;
   }

/*   if (sum <= limits) {
      if (sum>0)
         printf("%s Ch:%u Too few counts %5.0f for calibration of fine counter, use predefined\n", GetName(), nch, sum);
   } else {
      printf("%s Ch:%u Cnts: %7.0f produce %s calibration min:%u max:%u\n", GetName(), nch, sum, (use_linear ? "linear" : "normal"),  finemin, finemax);
   }
*/

   double quality = 1.;

   std::string name_prefix = std::string(GetName()) + (rising ? "_rch" : "_fch") + std::to_string(nch);
   std::string err_log;

   if (!preliminary && (finemin > 50)) {
      std::string log_finemin = std::string("_BadFineMin_") + std::to_string(finemin);
      err_log.append(log_finemin);
      if (quality > 0.4) quality = 0.4;
      if (fCalibrQuality > 0.4) {
         fCalibrStatus = name_prefix + log_finemin;
         fCalibrQuality = 0.4;
      }
   }

   if (!preliminary && (finemax < (f400Mhz ? 200 : 400))) {
      std::string log_finemax = std::string("_BadFineMax_") + std::to_string(finemax);
      err_log.append(log_finemax);
      if (quality > 0.4) quality = 0.4;
      if (fCalibrQuality > 0.4) {
         fCalibrStatus = name_prefix + log_finemax;
         fCalibrQuality = 0.4;
      }
   }

   double coarse_unit = hadaq::TdcMessage::CoarseUnit();
   if (f400Mhz) coarse_unit *= 200. / fCustomMhz;
   if (fVersion4) coarse_unit = hadaq::TdcMessage::CoarseUnit280();

   if (sum <= limits) {

      if (quality > 0.15) quality = 0.15;
      err_log.append("_LowStat");

      if ((fCalibrQuality > 0.15) && !preliminary)  {
         fCalibrStatus = name_prefix + "_LowStat";
         fCalibrQuality = 0.15;
      }

      calibr.resize(5);

      calibr[0] = 2; // 2 values
      calibr[1] = hadaq::TdcMessage::GetFineMinValue();
      calibr[2] = 0.;
      calibr[3] = hadaq::TdcMessage::GetFineMaxValue();
      calibr[4] = coarse_unit;

      fCalibrLog.push_back(name_prefix + err_log);

      return quality;
   }

   if (use_linear) {

      calibr.resize(1 + fLinearNumPoints*2);
      calibr[0] = fLinearNumPoints; // how many points, minimum 2
      calibr[1] = finemin;
      calibr[2] = 0.;
      int indx = 1, fine_last = finemin; // current point to use

//      printf("%s first %f %g\n", GetName(), calibr[indx], calibr[indx+1]);

      for (int pnt = 1; pnt < fLinearNumPoints-1; pnt++) {

         int fine_next = finemin + (int) std::round((finemax-finemin+0.)*pnt/(fLinearNumPoints-1.));
         double ksum1 = 0., ksum2 = 0.;

         for (int fine = fine_last+1; fine <= fine_next; fine++) {
            double exact = ((integral[fine] - statistic[fine]/2) / sum) * coarse_unit;
            ksum1 = (fine - fine_last) * (exact - calibr[indx+1]); // sum (dx*dy)
            ksum2 = (fine - fine_last) * (fine - fine_last);       // sum (dx*dx)
         }

         double k = ksum1 / ksum2;

         float value_next = calibr[indx+1] + k*(fine_next-fine_last);

         indx+=2;
         calibr[indx] = fine_next;
         calibr[indx+1] = value_next;
         fine_last = fine_next;

//         printf("%s next %f %g k = %g\n", GetName(), calibr[indx], calibr[indx+1], k);
      }

      // last point is finemax
      indx += 2;
      calibr[indx] = finemax;
      calibr[indx+1] = coarse_unit;

//      printf("%s last %f %g\n", GetName(), calibr[indx], calibr[indx+1]);

/*
      // now check several points
      for (int fine = 10; fine < 580; fine += 50) {
         int segm = -1, pnt = -1;
         if ((fine > finemin) && (fine < finemax)) {
            pnt = calibr.size() - 2;
            segm = std::ceil((fine - calibr[1]) / (calibr[pnt] - calibr[1]) * (calibr[0] - 1)); // segment in linear approx
            pnt = 1 + segm*2; // first segment should have pnt = 3, second segment pnt = 5 and so on
         }
         printf("%s fine %d nsegm = %d pnt %d value %g\n", GetName(), fine, segm, pnt, ExtractCalibrDirect(calibr, fine));
      }
*/

   } else {
      calibr.resize(fNumFineBins);

      double sum1 = 0., sum2 = 0., linear = 0, exact = 0.;

      // exclude some cases when several wrong finecounter values measured, which confuses algorithm
      while ((finemax > finemin) && (integral[finemax] > 0.999*sum)) finemax--;

      double scale_value = (integral[finemax] - statistic[finemax]/2) / sum;

      if (!preliminary && (finemax < (f400Mhz ? 175 : 350))) {
         std::string log_finemax = std::string("_BadFineMax_") + std::to_string(finemax);
         err_log.append(log_finemax);
         if (quality > 0.4) quality = 0.4;
         if (fCalibrQuality > 0.4) {
            fCalibrStatus = name_prefix + log_finemax;
            fCalibrQuality = 0.4;
         }
      }

      for (unsigned n = 0; n < fNumFineBins; n++) {
         exact = (integral[n] - statistic[n]/2) / sum;
         calibr[n] = exact * coarse_unit;

         if ((n > finemin) && (n < finemax)) {
            linear = (n - finemin) / (0. + finemax - finemin) * scale_value;

            sum1 += 1.;
            sum2 += (exact - linear) * (exact - linear);
         }
      }

      if (!preliminary && (sum1 > 100)) {
         double dev = sqrt(sum2/sum1); // average deviation
         printf("%s ch %u cnts %5.0f deviation %5.4f\n", GetName(), nch, sum, dev);
         if (dev > 0.05) {
            err_log.append("_NonLinear");
            if (quality > 0.6) quality = 0.6;
            if (fCalibrQuality > 0.6) {
               fCalibrStatus = name_prefix + "_NonLinear";
               fCalibrQuality = 0.6;
            }
         }
      }
   }

   // add problematic channels to the full list
   if (!err_log.empty()) fCalibrLog.push_back(name_prefix + err_log);

   return quality;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Calibrate ToT

bool hadaq::TdcProcessor::CalibrateTot(unsigned nch, std::vector<uint32_t> &hist, float &tot_shift, float &tot_dev, float cut)
{
   int left = 0, right = TotBins;
   double sum0 = 0., sum1 = 0., sum2 = 0.;
   tot_dev = 0.;

   std::string name_prefix = std::string(GetName()) + "_ch" + std::to_string(nch) + "_ToT";
   std::string err_log;

   for (int n = left; n < right; n++) sum0 += hist[n];
   if (sum0 < fTotStatLimit) {
      printf("%s Ch:%u TOT failed - not enough statistic %5.0f\n", GetName(), nch, sum0);

      if (fCalibrQuality > 0.4) {
         fCalibrStatus = name_prefix + "_lowstat";
         fCalibrQuality = 0.4;
      }
      fCalibrLog.push_back(name_prefix + "_lowstat_cnt" + std::to_string((int) sum0));

      return false; // no statistic for small number of counts
   }

   if ((cut > 0) && (cut < 0.3)) {
      double suml = 0., sumr = 0.;

      while ((left < right-3) && (suml < sum0*cut))
         suml += hist[left++];

      while ((right > left+3) && (sumr < sum0*cut))
         sumr += hist[--right];
   }

   sum0 = 0;

   for (int n = left; n < right; n++) {
      double x = fToThmin + (n + 0.5) / TotBins * (fToThmax-fToThmin); // x coordinate of center bin

      sum0 += hist[n];
      sum1 += hist[n]*x;
      sum2 += hist[n]*x*x;
   }

   double mean = sum1/sum0;
   double rms = sum2/sum0 - mean*mean;
   if (rms < 0) {
      printf("%s Ch:%u TOT failed - error in RMS calculation  mean: %5.3f rms2: %5.3f \n", GetName(), nch, mean, rms);

      if (fCalibrQuality > 0.4) {
         fCalibrStatus = name_prefix + "_negativerms";
         fCalibrQuality = 0.4;
      }
      fCalibrLog.push_back(name_prefix + "_negativerms");
      return false;
   }
   rms = sqrt(rms);
   tot_dev = rms;

   if (rms > fTotRMSLimit) {
      printf("%s Ch:%u TOT failed - RMS %5.3f too high\n", GetName(), nch, rms);

      if (fCalibrQuality > 0.4) {
         fCalibrStatus = name_prefix + "_highrms";
         fCalibrQuality = 0.4;
      }

      char sbuf[100];
      snprintf(sbuf, sizeof(sbuf), "%5.3fns", rms);

      fCalibrLog.push_back(name_prefix + "_highrms_" + sbuf);
      return false;
   }

   tot_shift = mean - fToTvalue;

   printf("%s Ch:%u TOT: %6.3f rms: %5.3f offset %6.3f\n", GetName(), nch, mean, rms, tot_shift);

   return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Copy calibration

void hadaq::TdcProcessor::CopyCalibration(const std::vector<float> &calibr, base::H1handle hcalibr, unsigned ch, base::H2handle h2calibr)
{
   ClearH1(hcalibr);

   for (unsigned n=0;n<fNumFineBins;n++) {
      SetH1Content(hcalibr, n, ExtractCalibrDirect(calibr, n)*1e12);
      SetH2Content(h2calibr, ch, n, ExtractCalibrDirect(calibr,n)*1e12);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// For expert use - produce calibration

void hadaq::TdcProcessor::ProduceCalibration(bool clear_stat, bool use_linear, bool dummy, bool preliminary)
{
   std::string log_msg;
   if (!preliminary) {
      if (fCalibrProgress >= 1) {
         fCalibrStatus = "Ready";
         fCalibrQuality = 1.;
      } else if (fCalibrProgress >= 0.3) {
         fCalibrStatus = log_msg = std::string(GetName()) + "_LowStat";
         fCalibrQuality = 0.5;
      } else if (fCalibrProgress > 0) {
         fCalibrStatus = log_msg = std::string(GetName()) + "_BadStat";
         fCalibrQuality = 0.2;
      } else {
         fCalibrStatus = log_msg = std::string(GetName()) + "_NoData";
         fCalibrQuality = 0.51; // mark such situation as warning, LowStat is worse
      }
   }

   if (dummy) return;

   fCalibrLog.clear();
   if (!log_msg.empty()) {
      if (fCalibrProgress > 0) {
         log_msg.append("_");
         log_msg.append(std::to_string((int) (fCalibrProgress * 100.)));
         log_msg.append("pcnt");
      }
      fCalibrLog.push_back(log_msg);
   }

   if (!preliminary)
      printf("%s produce %s calibrations \n", GetName(), (use_linear ? "linear" : "normal"));

   if (fCalibrTempSum0 > 5) {
      double mean = fCalibrTempSum1/fCalibrTempSum0;
      double rms = fCalibrTempSum2 / fCalibrTempSum0 - mean*mean;
      if (rms>0) rms = sqrt(rms); else rms = (rms >=-1e-8) ? 0 : -1;
      printf("   temp %5.2f +- %3.2f during calibration\n", mean, rms);
      if ((rms>0) && (rms<3)) fCalibrTemp = mean;
   }

   fCalibrTempSum0 = 0;
   fCalibrTempSum1 = 0;
   fCalibrTempSum2 = 0;

   if (!preliminary) {
      ClearH2(fRisingCalibr);
      ClearH2(fFallingCalibr);
      ClearH1(fTotShifts);
   }

   for (unsigned ch=0;ch<NumChannels();ch++) {

      ChannelRec &rec = fCh[ch];

      if (!preliminary) {
         rec.calibr_stat_rising = rec.calibr_stat_falling = 0;
         rec.calibr_quality_rising = rec.calibr_quality_falling = -1.;
      }

      if (rec.docalibr) {

         rec.check_calibr = false; // reset flag, used in auto calibration

         // special case - use common statistic
         if (fEdgeMask == edge_CommonStatistic) {
            rec.all_rising_stat += rec.all_falling_stat;
            if (fCalHitsPerBrd) DefFillH2(*fCalHitsPerBrd, fSeqeunceId, ch, rec.all_falling_stat); // add all falling edges
            rec.all_falling_stat = 0;
            for (unsigned n=0;n<fNumFineBins;n++) {
               rec.rising_stat[n] += rec.falling_stat[n];
               rec.falling_stat[n] = 0;
            }
         }

         // printf("%s Ch:%d do: %d %d stat: %ld %ld mask %d\n", GetName(), ch, DoRisingEdge(), DoFallingEdge(), rec.all_rising_stat, rec.all_falling_stat, fEdgeMask);

         bool res = false;

         if (DoRisingEdge() && (rec.all_rising_stat > 0)) {
            rec.calibr_quality_rising = CalibrateChannel(ch, true, rec.rising_stat, rec.rising_calibr, use_linear, preliminary);
            rec.calibr_stat_rising = rec.all_rising_stat;
            res = (rec.calibr_quality_rising > 0.5);
         }

         if (DoFallingEdge() && (rec.all_falling_stat > 0) && (fEdgeMask == edge_BothIndepend)) {
            rec.calibr_quality_falling = CalibrateChannel(ch, false, rec.falling_stat, rec.falling_calibr, use_linear, preliminary);
            rec.calibr_stat_falling = rec.all_falling_stat;
            if (rec.calibr_quality_falling <= 0.5) res = false;
         }

         printf("%s:%u Check Tot dofalling: %d tot0d_cnt:%ld prelim:%d tot0d_hist:%d \n", GetName(), ch, DoFallingEdge(), rec.tot0d_cnt, preliminary, (int) rec.tot0d_hist.size());

         if ((ch > 0) && DoFallingEdge() && !preliminary) {

            std::string name_prefix = std::string(GetName()) + "_ch" + std::to_string(ch) + "_ToT";

            if ((rec.tot0d_cnt > 100)  && !rec.tot0d_hist.empty()) {

               CalibrateTot(ch, rec.tot0d_hist, rec.tot_shift, rec.tot_dev, 0.05);

               if (!rec.fTot0D && (HistFillLevel() > 2)) {
                  SetSubPrefix2("Ch", ch);
                  rec.fTot0D = MakeH1("Tot0D", "Time over threshold with 0xD trigger", TotBins, fToThmin, fToThmax, "ns");
                  SetSubPrefix2();
               }

               if (rec.fTot0D)
                  for (unsigned n=0;n<TotBins;n++) {
                     double x = fToThmin + (n + 0.1) / TotBins * (fToThmax-fToThmin);
                     DefFillH1(rec.fTot0D, x, rec.tot0d_hist[n]);
                  }

               if (rec.tot0d_misscnt > 0.5*rec.tot0d_cnt) {
                  printf("%s Ch:%u TOT problem - much values %ld missed histogram range\n", GetName(), ch, rec.tot0d_misscnt);
                  if (fCalibrQuality > 0.6) {
                     fCalibrStatus = name_prefix + "_miss_hrange";
                     fCalibrQuality = 0.6;
                  }
                  fCalibrLog.push_back(name_prefix + "_miss_hrange");
               }
            } else if (rec.tot0d_misscnt > 100) {
               printf("%s Ch:%u TOT failure - too much values %ld missed histogram range\n", GetName(), ch, rec.tot0d_misscnt);
               if (fCalibrQuality > 0.4) {
                  fCalibrStatus = name_prefix + "_err_hrange";
                  fCalibrQuality = 0.4;
               }
               fCalibrLog.push_back(name_prefix + "_err_hrange");
            }
         }

         //
         if ((ch > 0) && fToTPerBrd)
            SetH2Content(*fToTPerBrd, fSeqeunceId, ch-1, DoFallingEdge() ? rec.tot_shift : 0.);

         rec.hascalibr = res;

         if ((fEdgeMask == edge_CommonStatistic) || (fEdgeMask == edge_ForceRising)) {
            rec.falling_calibr = rec.rising_calibr;
            rec.calibr_stat_falling = rec.calibr_stat_rising;
            rec.calibr_quality_falling = rec.calibr_quality_rising;
         }

         if (clear_stat && !preliminary)
            ClearChannelStat(ch);
      }

      if (!preliminary) {
         if (DoRisingEdge())
            CopyCalibration(rec.rising_calibr, rec.fRisingCalibr, ch, fRisingCalibr);
         if (DoFallingEdge())
            CopyCalibration(rec.falling_calibr, rec.fFallingCalibr, ch, fFallingCalibr);
         DefFillH1(fTotShifts, ch, rec.tot_shift);
       }
    }


}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Fill ToT histogram

void hadaq::TdcProcessor::FillToTHistogram()
{
   if (fToTPerBrd)
      for (unsigned ch=1;ch<NumChannels();ch++) {
         ChannelRec &rec = fCh[ch];
         SetH2Content(*fToTPerBrd, fSeqeunceId, ch-1, DoFallingEdge() ? rec.tot_shift : 0.);
      }

}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Clear channel statistic used for calibrations

void hadaq::TdcProcessor::ClearChannelStat(unsigned ch)
{
   for (unsigned n=0;n<fNumFineBins;n++) {
      fCh[ch].falling_stat[n] = 0;
      fCh[ch].rising_stat[n] = 0;
   }
   fCh[ch].all_falling_stat = 0;
   fCh[ch].all_rising_stat = 0;
   fCh[ch].tot0d_cnt = 0;
   fCh[ch].tot0d_misscnt = 0;
   fCh[ch].ReleaseToTHist();
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// For expert use - store calibration in the file

void hadaq::TdcProcessor::StoreCalibration(const std::string& fprefix, unsigned fileid)
{
   if (fprefix.empty()) return;

   if (fileid == 0) fileid = GetID();
   char fname[1024];
   snprintf(fname, sizeof(fname), "%s%04x.cal", fprefix.c_str(), fileid);

   FILE* f = fopen(fname,"w");
   if (!f) {
      printf("%s Cannot open file %s for writing calibration\n", GetName(), fname);
      return;
   }

   uint64_t num = NumChannels();

   fwrite(&num, sizeof(num), 1, f);

   // calibration curves
   for (unsigned ch=0;ch<NumChannels();ch++) {
      fwrite(fCh[ch].rising_calibr.data(), sizeof(float)*fCh[ch].rising_calibr.size(), 1, f);
      fwrite(fCh[ch].falling_calibr.data(), sizeof(float)*fCh[ch].falling_calibr.size(), 1, f);
   }

   // tot shifts
   for (unsigned ch=0;ch<NumChannels();ch++) {
      fwrite(&(fCh[ch].tot_shift), sizeof(fCh[ch].tot_shift), 1, f);
   }

   // temperature
   fwrite(&fCalibrTemp, sizeof(fCalibrTemp), 1, f);
   fwrite(&fCalibrTempCoef, sizeof(fCalibrTempCoef), 1, f);

   for (unsigned ch=0;ch<NumChannels();ch++) {
      ChannelRec &rec = fCh[ch];
      fwrite(&rec.time_shift_per_grad, sizeof(rec.time_shift_per_grad), 1, f);
      fwrite(&rec.trig0d_coef, sizeof(rec.trig0d_coef), 1, f);
      fwrite(&rec.calibr_quality_rising, sizeof(rec.calibr_quality_rising), 1, f);
      fwrite(&rec.calibr_quality_falling, sizeof(rec.calibr_quality_falling), 1, f);
   }

   fclose(f);

   printf("%s storing calibration in %s\n", GetName(), fname);

   snprintf(fname, sizeof(fname), "%s%04x.cal.info", fprefix.c_str(), fileid);
   f = fopen(fname,"w");

   if (!f) {
      printf("%s Cannot open file %s for writing calibration info\n", GetName(), fname);
      return;
   }
   fprintf(f,"ch qrising    stat  fmin  fmax   qfalling  stat  fmin  fmax   ToT  Dev\n");
   for (unsigned ch=0;ch<NumChannels();ch++) {
      ChannelRec &rec = fCh[ch];
      int fmin1 = 10, fmax1 = 400, fmin2 = 10, fmax2 = 400;
      FindFMinMax(rec.rising_calibr, fNumFineBins, fmin1, fmax1);
      FindFMinMax(rec.falling_calibr, fNumFineBins, fmin2, fmax2);
      fprintf(f,"%2u   %5.2f  %6ld   %3d   %3d    %5.2f  %6ld   %3d   %3d  %7.3f %5.3f\n", ch,
                 rec.calibr_quality_rising, rec.calibr_stat_rising, fmin1, fmax1,
                 rec.calibr_quality_falling, rec.calibr_stat_falling, fmin2, fmax2,
                 rec.tot_shift, rec.tot_dev );
   }

   fclose(f);

   printf("%s storing calibration info %s\n", GetName(), fname);

   if (gStoreCalibrTables && fVersion4) {
      snprintf(fname, sizeof(fname), "%s%04x.cal.table", fprefix.c_str(), fileid);
      f = fopen(fname,"w");

      if (!f) {
         printf("%s Cannot open file %s for writing calibration tables\n", GetName(), fname);
         return;
      }

      for (unsigned ch=0;ch<NumChannels();ch++) {

         uint32_t table[256];

         CreateV4CalibrTable(ch, table);

         fprintf(f,"## calibration table for channel %u\n", ch);
         for(int n=0;n<256;n++) {
            fprintf(f, " %08x", (unsigned) table[n]);
            if ((n % 8) == 7) fprintf(f, "\n");
         }

      }

      fclose(f);

      printf("%s store calibration table %s\n", GetName(), fname);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Set V4 table values

void hadaq::TdcProcessor::SetTable(uint32_t *table, unsigned addr, uint32_t value)
{
   if (addr >= 0x200) return;
   unsigned indx = addr / 2;

   value = value & 0x1FF;

   if (addr % 2 == 0)
      table[indx] = (table[indx] & 0xFFFFFE00) | value;
   else
      table[indx] = (table[indx] & 0xFFFC01FF) | (value << 9);
}

#define CRC16 0x8005

uint16_t gen_crc16(const uint32_t *data, unsigned size, unsigned last_bitlen)
{
    uint16_t out = 0;
    int bits_read = 0, bit_flag, bitlen = 32;

    /* Sanity check: */
    if(!data)
        return 0;

    while(size > 0)
    {
        bit_flag = out >> 15;

        /* Get next bit: */
        out <<= 1;
        out |= (*data >> bits_read) & 1; // item a) work from the least significant bits

        /* Increment bit counter: */
        bits_read++;
        if(bits_read >= bitlen)
        {
            bits_read = 0;
            data++;
            size--;
            if (size == 1) bitlen = last_bitlen;
        }

        /* Cycle check: */
        if(bit_flag)
            out ^= CRC16;
    }

    // item b) "push out" the last 16 bits
    int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if(bit_flag)
            out ^= CRC16;
    }

    // item c) reverse the bits
    uint16_t crc = 0;
    i = 0x8000;
    int j = 0x0001;
    for (; i != 0; i >>=1, j <<= 1) {
        if (i & out) crc |= j;
    }

    return crc;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Create V4 calibration table

void hadaq::TdcProcessor::CreateV4CalibrTable(unsigned ch, uint32_t *table)
{
   ChannelRec &rec = fCh[ch];

   for (int i = 0; i < 256; i++)
      table[i] = 0;

   // user data 0x00 .. 0x1F empty for the moment

   double corse_unit = hadaq::TdcMessage::CoarseUnit280();

   // calibration curve
   for (unsigned fine = 0x20; fine <= 0x1DF; ++fine) {
      double value = rec.rising_calibr[fine];
      // convert into range 0..508
      long newfine = std::lround(value / corse_unit * 508);
      if (newfine < 0)
         newfine = 0;
      else if (newfine > 508)
         newfine = 508;

      SetTable(table, fine, newfine);
   }

   unsigned tableid = 0;
   SetTable(table, 0x1E0, tableid & 0x1FF);
   SetTable(table, 0x1E1, (tableid >> 9) & 0x1FF);
   SetTable(table, 0x1E2, (tableid >> 18) & 0x1FF);

   SetTable(table, 0x1E3, (ch > 0) ? ch-1 : 0x1FF); // channel id

   uint64_t serial_number = 0;

   SetTable(table, 0x1E4, serial_number & 0x1FF);
   SetTable(table, 0x1E5, (serial_number >> 9) & 0x1FF);
   SetTable(table, 0x1E6, (serial_number >> 18) & 0x1FF);
   SetTable(table, 0x1E7, (serial_number >> 27) & 0x1FF);

   timespec tm0;
   clock_gettime(CLOCK_REALTIME, &tm0);
   time_t src = tm0.tv_sec;
   struct tm res;
   localtime_r(&src, &res);

   SetTable(table, 0x1E8, res.tm_sec);    /* Seconds (0-60) */
   SetTable(table, 0x1E9, res.tm_min);    /* Minutes (0-59) */
   SetTable(table, 0x1EA, res.tm_hour);   /* Hours (0-23) */
   SetTable(table, 0x1EB, res.tm_mday);   /* Day of the month (1-31) */
   SetTable(table, 0x1EC, res.tm_mon);    /* Month (0-11) */
   SetTable(table, 0x1ED, res.tm_year-100);  /* Year - 1900 */

   uint16_t crc16 = gen_crc16(table, 0xF8, 2);

   SetTable(table, 0x1EF, (crc16 & 0x3F) << 2);  // first 7 bits of CRC16
   SetTable(table, 0x1EF, (crc16 >> 7) & 0x1FF);  // higher 9 bits of CRC16

   // default values on top
   for (unsigned addr = 0x1F0; addr <= 0x1FF; ++addr)
      SetTable(table, addr, addr);
}


//////////////////////////////////////////////////////////////////////////////////////////////
/// Load calibration from the file

bool hadaq::TdcProcessor::LoadCalibration(const std::string& fprefix)
{
   if (fprefix.empty()) return false;

   char fname[1024];

   snprintf(fname, sizeof(fname), "%s%04x.cal", fprefix.c_str(), GetID());

   FILE* f = fopen(fname,"r");
   if (!f) {
      printf("Cannot open file %s for reading calibration\n", fname);
      return false;
   }

   uint64_t num(0);

   fread(&num, sizeof(num), 1, f);

   ClearH2(fRisingCalibr);
   ClearH2(fFallingCalibr);
   ClearH1(fTotShifts);

   if (num!=NumChannels()) {
      printf("%s in file %s mismatch of channels number in calibrations  %u and in processor %u\n", GetName(), fname, (unsigned) num, NumChannels());
   }

   for (unsigned ch=0;ch<num;ch++) {
     fCh[ch].hascalibr = false;

     if (ch>=NumChannels()) {
        fseek(f, 2*sizeof(float)*fNumFineBins, SEEK_CUR);
        continue;
     }

     fCh[ch].rising_calibr.clear();
     fCh[ch].falling_calibr.clear();

     float val0 = 0.;
     if (fread(&val0, sizeof(float), 1, f) == 1) {
        if (val0) {
           fCh[ch].rising_calibr.resize(1 + ((int)val0)*2);
        } else{
           fCh[ch].rising_calibr.resize(fNumFineBins);
        }
        fCh[ch].rising_calibr[0] = val0;
        fread(fCh[ch].rising_calibr.data()+1, sizeof(float)*(fCh[ch].rising_calibr.size()-1), 1, f);
     }

     if (fread(&val0, sizeof(float), 1, f) == 1) {
        if (val0) {
           fCh[ch].falling_calibr.resize(1 + ((int)val0)*2);
        } else{
           fCh[ch].falling_calibr.resize(fNumFineBins);
        }
        fCh[ch].falling_calibr[0] = val0;
        fread(fCh[ch].falling_calibr.data()+1, sizeof(float)*(fCh[ch].falling_calibr.size()-1), 1, f);
     }

     fCh[ch].hascalibr = (fCh[ch].rising_calibr.size() > 4) && (fCh[ch].falling_calibr.size() > 4);

     CopyCalibration(fCh[ch].rising_calibr, fCh[ch].fRisingCalibr, ch, fRisingCalibr);

     CopyCalibration(fCh[ch].falling_calibr, fCh[ch].fFallingCalibr, ch, fFallingCalibr);
   }

   if (!feof(f)) {
      for (unsigned ch=0;ch<num;ch++) {
         if (ch>=NumChannels())
            fseek(f, sizeof(fCh[0].tot_shift), SEEK_CUR);
         else
            fread(&(fCh[ch].tot_shift), sizeof(fCh[ch].tot_shift), 1, f);

         DefFillH1(fTotShifts, ch, fCh[ch].tot_shift);

      }

      if (!feof(f)) {
         fread(&fCalibrTemp, sizeof(fCalibrTemp), 1, f);
         fread(&fCalibrTempCoef, sizeof(fCalibrTempCoef), 1, f);

         for (unsigned ch=0;ch<NumChannels();ch++)
            if (!feof(f)) {
               fread(&(fCh[ch].time_shift_per_grad), sizeof(fCh[ch].time_shift_per_grad), 1, f);
               fread(&(fCh[ch].trig0d_coef), sizeof(fCh[ch].trig0d_coef), 1, f);
               fread(&(fCh[ch].calibr_quality_rising), sizeof(fCh[ch].calibr_quality_rising), 1, f);
               fread(&(fCh[ch].calibr_quality_falling), sizeof(fCh[ch].calibr_quality_falling), 1, f);

               // old files with bubble coefficients
               if ((fabs(fCh[ch].calibr_quality_rising-20.)<0.01) && (fabs(fCh[ch].calibr_quality_falling-1.06)<0.01)) {
                  fCh[ch].calibr_quality_rising = fCh[ch].calibr_quality_falling = 1.;
               }
            }
      }
   }

   fclose(f);

   // workaround for testing, remove later !!!
   if (strstr(fname, "cal/global_"))
      switch (GetID()) {
         case 0x941: fTempCorrection = -1.58; break;
         case 0x942: fTempCorrection = -1.68; break;
         case 0x943: fTempCorrection = -1.32; break;
         default: fTempCorrection = 0; break;
      }

   printf("%s reading calibration from %s, tcorr:%5.1f uset:%d done\n", GetName(), fname, fTempCorrection, fCalibrUseTemp);

   fCalibrStatus = "CalibrFile";

   fCalibrQuality = 0.99;

   return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// For expert use - artificially set calibration statistic

void hadaq::TdcProcessor::IncCalibration(unsigned ch, bool rising, unsigned fine, unsigned value)
{
   if ((ch>=NumChannels()) || (fine>=fNumFineBins)) return;

   if (rising && DoRisingEdge()) {
      fCh[ch].rising_stat[fine] += value;
      fCh[ch].all_rising_stat += value;
   }

   if (!rising && DoFallingEdge()) {
      fCh[ch].falling_stat[fine] += value;
      fCh[ch].all_falling_stat += value;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Create TTree branch

void hadaq::TdcProcessor::CreateBranch(TTree*)
{
   switch(GetStoreKind()) {
      case 1:
         pStoreVect = &fDummyVect;
         mgr()->CreateBranch(GetName(), "std::vector<hadaq::TdcMessageExt>", (void**) &pStoreVect);
         break;
      case 2:
         pStoreFloat = &fDummyFloat;
         mgr()->CreateBranch(GetName(), "std::vector<hadaq::MessageFloat>", (void**) &pStoreFloat);
         break;
      case 3:
         pStoreDouble = &fDummyDouble;
         mgr()->CreateBranch(GetName(), "std::vector<hadaq::MessageDouble>", (void**) &pStoreDouble);
         break;
      default:
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Store event

void hadaq::TdcProcessor::Store(base::Event* ev)
{
   // in case of triggered analysis all pointers already set
   if (!ev || IsTriggeredAnalysis()) return;

   base::SubEvent* sub0 = ev->GetSubEvent(GetName());
   if (!sub0) return;

   switch (GetStoreKind()) {
      case 1: {
         hadaq::TdcSubEvent* sub = dynamic_cast<hadaq::TdcSubEvent*> (sub0);
         // when subevent exists, use directly pointer on messages vector
         pStoreVect = sub ? sub->vect_ptr() : &fDummyVect;
         break;
      }
      case 2: {
         hadaq::TdcSubEventFloat* sub = dynamic_cast<hadaq::TdcSubEventFloat*> (sub0);
         // when subevent exists, use directly pointer on messages vector
         pStoreFloat = sub ? sub->vect_ptr() : &fDummyFloat;
         break;
      }
      case 3: {
         hadaq::TdcSubEventDouble* sub = dynamic_cast<hadaq::TdcSubEventDouble*> (sub0);
         // when subevent exists, use directly pointer on messages vector
         pStoreDouble = sub ? sub->vect_ptr() : &fDummyDouble;
         break;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// reset store

void hadaq::TdcProcessor::ResetStore()
{
   pStoreVect = &fDummyVect;
   pStoreFloat = &fDummyFloat;
   pStoreDouble = &fDummyDouble;
}

