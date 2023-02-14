// this is example for

#include "TTree.h"
#include "TFile.h"
#include "TSystem.h"
#include "TString.h"
#include "TH1.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TGo4AnalysisObjectManager.h"

#include "base/EventProc.h"
#include "base/Event.h"
#include "hadaq/TdcSubEvent.h"

#include "base/ProcMgr.h"
#include "hadaq/HldProcessor.h"
#include "hadaq/TdcProcessor.h"
#include "hadaq/TrbProcessor.h"
#include "hadaq/MdcProcessor.h"

#include <cstdlib>

#define TDCRANGE_START 0x0350
#define TDCRANGE_STOP 0x1823


void first()
{
  //base::ProcMgr::instance()->SetRawAnalysis(true);
    base::ProcMgr::instance()->SetTriggeredAnalysis(true);

   // all new instances get this value
   base::ProcMgr::instance()->SetHistFilling(4);

   // this limits used for liner calibrations when nothing else is available
   hadaq::TdcMessage::SetFineLimits(10, 490);

   // default channel numbers and edges mask
   hadaq::TrbProcessor::SetDefaults(53, 0x2);
// hadaq::TrbProcessor::SetDefaults(17, 0x2);
//    hadaq::TdcProcessor::SetDefaults(1000);
   //hadaq::TdcProcessor::DisableCalibrationFor(0,8);
   // [min..max] range for TDC ids
   // hadaq::TrbProcessor::SetTDCRange(0x0350, 0x1900);
   hadaq::TrbProcessor::SetTDCRange(TDCRANGE_START, TDCRANGE_STOP);

   // configure ToT calibration parameters
   // first - minimal number of counts in ToT histogram
   // second - maximal RMS value
   hadaq::TdcProcessor::SetToTCalibr(100, 0.2);

   //hadaq::T
   // [min..max] range for HUB ids
    //hadaq::TrbProcessor::SetHUBRange(HUBRANGE_START, HUBRANGE_STOP);
    hadaq::TrbProcessor::SetHUBRange(0x8000, 0x8400);

   // when first argument true - TRB/TDC will be created on-the-fly
   // second parameter is function name, called after elements are created
   hadaq::HldProcessor* hld = new hadaq::HldProcessor(true, "after_create");
   // create TRB processor which holds custom data
   hadaq::TrbProcessor* trb = new hadaq::TrbProcessor(0xc035, hld);
   //trb->SetAutoCreate(true);
//trb->AddHadaqHUBId(0xc035);
   trb->AddHadaqHUBId(0x8352);
   trb->AddHadaqHUBId(0x8353);

   unsigned TDClist[14] = { 0x1000, 0x1001, 0x1010, 0x1011, 0x10A0, 0x10A1, 0x10b0, 0x10b1, 0x10c0, 0x10c1, 0x10D0, 0x10D1, 0x1020, 0x1021 };
   // create custom processor
    hadaq::MdcProcessor *mdc[14];

   for (int itdc = 0 ; itdc < int(sizeof(TDClist)/sizeof(*TDClist)) ; itdc++)
   {
     mdc[itdc] = new hadaq::MdcProcessor(trb, TDClist[itdc]);
   }

//
   const char* calname = getenv("CALNAME");
   if ((calname==0) || (*calname==0)) calname = "test_";
   const char* calmode = getenv("CALMODE");
   int cnt = (calmode && *calmode) ? atoi(calmode) : 0;
   const char* caltrig = getenv("CALTRIG");
   unsigned trig = (caltrig && *caltrig) ? atoi(caltrig) : 0xd;
   const char* uset = getenv("USETEMP");
   unsigned use_temp = 0; // 0x80000000;
   if ((uset!=0) && (*uset!=0) && (strcmp(uset,"1")==0)) use_temp = 0x80000000;

   printf("HLD configure calibration calfile:%s  cnt:%d trig:%X temp:%X\n", calname, cnt, trig, use_temp);

   // first parameter if filename  prefix for calibration files
   //     and calibration mode (empty string - no file I/O)
   // second parameter is hits count for autocalibration
   //     0 - only load calibration
   //    -1 - accumulate data and store calibrations only at the end
   //    >0 - automatic calibration after N hits in each active channel
   // third parameter is trigger type mask used for calibration
   //   (1 << 0xD) - special 0XD trigger with internal pulser, used also for TOT calibration
   //    0x3FFF - all kinds of trigger types will be used for calibration (excluding 0xE and 0xF)
   //   0x80000000 in mask enables usage of temperature correction
//   hld->ConfigureCalibration(calname, cnt, /*(1 << trig) | use_temp*/ 0x3fff);
   hld->ConfigureCalibration(calname, cnt, (1 << trig));


   // only accept trigger type 0x1 when storing file
   //new hadaq::HldFilter(0x0);

   // create ROOT file store
   base::ProcMgr::instance()->CreateStore("mdc.root");

   // 0 - disable store
   // 1 - std::vector<hadaq::TdcMessageExt> - includes original TDC message
   // 2 - std::vector<hadaq::MessageFloat>  - compact form, without channel 0, stamp as float (relative to ch0)
   // 3 - std::vector<hadaq::MessageDouble> - compact form, with channel 0, absolute time stamp as double
   base::ProcMgr::instance()->SetStoreKind(2);

}

// extern "C" required by DABC to find function from compiled code

extern "C" void after_create(hadaq::HldProcessor* hld)
{
   printf("Called after all sub-components are created\n");

   if (hld==0) return;

   for (unsigned k=0;k<hld->NumberOfTRB();k++) {
      hadaq::TrbProcessor* trb = hld->GetTRB(k);
      if (trb==0) continue;
      printf("Configure %s!\n", trb->GetName());
      trb->SetPrintErrors(10);
   }
}


