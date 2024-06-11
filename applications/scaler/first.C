#include "base/ProcMgr.h"
#include "hadaq/HldProcessor.h"
#include "hadaq/TdcProcessor.h"
#include "hadaq/TrbProcessor.h"
#include "hadaq/ScalerProcessor.h"

void first()
{
   // base::ProcMgr::instance()->SetRawAnalysis(true);
   base::ProcMgr::instance()->SetTriggeredAnalysis(true);

   // all new instances get this value
   base::ProcMgr::instance()->SetHistFilling(4);

   // configure bubbles
   //hadaq::TdcProcessor::SetBubbleMode(3, 18);

   // this limits used for linear calibrations when nothing else is available
   hadaq::TdcMessage::SetFineLimits(19, 500);

   // default channel numbers and edges mask
   // 1 - only rising edge
   // 2 - falling edge fully independent
   // 3 - falling uses calibration from rising edge
   // 4 - use common statistic for both rising and falling edge
   hadaq::TrbProcessor::SetDefaults(49, 2);

   hadaq::TdcProcessor::SetTriggerDWindow(-5, 120);
//   hadaq::TdcProcessor::SetToTRange(30, 50, 80);

   // [min..max] range for TDC ids
   hadaq::TrbProcessor::SetTDCRange(0x2700, 0x2800);


   // [min..max] range for HUB ids
   hadaq::TrbProcessor::SetHUBRange(0x8000, 0x8100);

   // Histogramming for ToT: first: nr. of bins, 2nd: TOT range
   hadaq::TdcProcessor::SetDefaults(600, 100);

   // when first argument true - TRB/TDC will be created on-the-fly
   // second parameter is function name, called after elements are created
   hadaq::HldProcessor* hld = new hadaq::HldProcessor(true, "after_create");


   const char* calname = getenv("CALNAME");
   if ((calname==0) || (*calname==0)) calname = "test_";
   const char* calmode = getenv("CALMODE");
   int cnt = (calmode && *calmode) ? atoi(calmode) : 0;
   // cnt = 0;
   const char* caltrig = getenv("CALTRIG");
   unsigned trig = (caltrig && *caltrig) ? atoi(caltrig) : 0xd;
   const char* uset = getenv("USETEMP");
   unsigned use_temp =  0; // 0x80000000;
   if ((uset!=0) && (*uset!=0) && (strcmp(uset,"1")==0)) use_temp = 0x80000000;

   printf("TDC CALIBRATION MODE %d, cal trigger: %x\n", cnt, trig);

   //printf("HLD configure calibration calfile:%s  cnt:%d trig:%X temp:%X\n", calname, cnt, trig, use_temp);

   // first parameter if filename  prefix for calibration files
   //     and calibration mode (empty string - no file I/O)
   // second parameter is hits count for autocalibration
   //     0 - only load calibration
   //    -1 - accumulate data and store calibrations only at the end
   //    -77 - accumulate data and store linear calibrations only at the end
   //    >0 - automatic calibration after N hits in each active channel
   //         if value ends with 77 like 10077 linear calibration will be calculated
   //    >1000000000 - automatic calibration after N hits only once, 1e9 excluding
   // third parameter is trigger type mask used for calibration
   //   (1 << 0xD) - special 0XD trigger with internal pulser, used also for TOT calibration
   //    0x3FFF - all kinds of trigger types will be used for calibration (excluding 0xE and 0xF)
   //   0x80000000 in mask enables usage of temperature correction
   hld->ConfigureCalibration(calname, cnt, (1 << trig));

   // only accept trigger type 0x1 when storing file
   // new hadaq::HldFilter(0x1);

   // when configured as output in DABC, one specifies:
   // <OutputPort name="Output2" url="stream://file.root?maxsize=5000&kind=3"/>


   hadaq::TdcProcessor::SetUseDTrigForRef(true);
   // hadaq::TdcProcessor::(true);

   // 0 - disable store
   // 1 - std::vector<hadaq::TdcMessageExt> - includes original TDC message
   // 2 - std::vector<hadaq::MessageFloat>  - compact form, without channel 0, stamp as float (relative to ch0)
   // 3 - std::vector<hadaq::MessageDouble> - compact form, with channel 0, absolute time stamp as double
   base::ProcMgr::instance()->SetStoreKind(2);

   // create ROOT file store
   base::ProcMgr::instance()->CreateStore("td.root");

}

// extern "C" required by DABC to find function from compiled code

extern "C" void after_create(hadaq::HldProcessor* hld)
{
   printf("Called after all sub-components are created\n");

   if (!hld) return;

   for (unsigned k = 0; k < hld->NumberOfTRB(); k++) {
      hadaq::TrbProcessor *trb = hld->GetTRB(k);
      if (!trb)
        continue;
      printf("Configure %s!\n", trb->GetName());
      trb->SetPrintErrors(10);

      if (trb->GetID() == 0xc001) {
         auto scaler = new hadaq::ScalerProcessor(trb, 0x3800);
         scaler->SetStoreKind(trb->GetStoreKind());
         trb->mgr()->UserPreLoop(scaler); // while loop already running, call it once again for new processor
     }
   }

   for (unsigned k = 0; k < hld->NumberOfTDC(); k++) {
     hadaq::TdcProcessor *tdc = hld->GetTDC(k);
     if (!tdc)
       continue;

     printf("Configure %s isver4 %d\n", tdc->GetName(), tdc->IsVersion4());

     // tdc->SetCustomMhz(200);

     // tdc->SetToTRange(30, 50, 110);
     // tdc->SetUseLastHit(false);

     // tdc->SetRefChannel(6, 5, 0xffff, 4000, -20., 20.);
     // tdc->SetRefChannel(5, 6, 0xffff, 4000, -20., 20.);
   }

}
