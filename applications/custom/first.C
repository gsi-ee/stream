/// This is example of custom processor of HADAQ sub-event
///
/// In file custom.h new processor is defined
/// And in first.C processor is configured

#include <cstdlib>

#include "hadaq/TdcProcessor.h"
#include "hadaq/TrbProcessor.h"
#include "hadaq/HldProcessor.h"
#include "base/ProcMgr.h"
#include "custom.h"


void first()
{
  //base::ProcMgr::instance()->SetRawAnalysis(true);
    base::ProcMgr::instance()->SetTriggeredAnalysis(true);

   // configure ToT calibration parameters
   // first - minimal number of counts in ToT histogram
   // second - maximal RMS value
    hadaq::TdcProcessor::SetToTCalibr(100, 1);



   // all new instances get this value
    base::ProcMgr::instance()->SetHistFilling(4);

   // this limits used for liner calibrations when nothing else is available
//   hadaq::TdcMessage::SetFineLimits(16, 500);

   // First is range of fine time
   // Second parameter determines the range of the ToT histograms in ns
   // Third is reduction factor of bins for 2D histogram
   hadaq::TdcProcessor::SetDefaults(600, 200, 1);

   // default channel numbers and edges mask
   // 1 - use only rising edge, falling edge is ignore
   // 2   - falling edge enabled and fully independent from rising edge
   // 3   - falling edge enabled and uses calibration from rising edge
   // 4   - falling edge enabled and common statistic is used for calibration
   // default channel numbers and edges mask
   hadaq::TrbProcessor::SetDefaults(53, 2);
   // hadaq::TdcProcessor::SetDefaults(1024);
   hadaq::TdcProcessor::SetTriggerDWindow(-10,70);
   hadaq::TdcProcessor::SetAllHistos(true);

   // [min..max] range for TDC ids
   //hadaq::TrbProcessor::SetTDCRange(0x610, 0x613);
   hadaq::TrbProcessor::SetTDCRange(0x0000, 0x5fff);

   // [min..max] range for HUB ids
   hadaq::TrbProcessor::SetHUBRange(0x8000, 0x8fff);

   // when first argument true - TRB/TDC will be created on-the-fly
   // second parameter is function name, called after elements are created
   hadaq::HldProcessor* hld = new hadaq::HldProcessor(true, "after_create");


   //--------------------------------------------------------------------------------------

   // create TRB processor which holds custom data
   hadaq::TrbProcessor* trb3 = new hadaq::TrbProcessor(0x83CC, hld);
   // enable auto-creation only if TDCs expected on that TRB
   trb3->SetAutoCreate(true);
   // create custom processor
   hadaq::CustomProcessor *custom = new hadaq::CustomProcessor(trb3, 0x8143, 32);

   //--------------------------------------------------------------------------------------



   const char* calname = getenv("CALNAME");
   if ((calname==0) || (*calname==0)) calname = "test_";
   const char* calmode = getenv("CALMODE");
   int cnt = (calmode && *calmode) ? atoi(calmode) : 100000;
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
   hld->ConfigureCalibration(calname, cnt, (1 << trig) | use_temp);


   // only accept trigger type 0x1 when storing file
   //new hadaq::HldFilter(0x1);

   // create ROOT file store
   //base::ProcMgr::instance()->CreateStore("td.root");

   // 0 - disable store
   // 1 - std::vector<hadaq::TdcMessageExt> - includes original TDC message
   // 2 - std::vector<hadaq::MessageFloat>  - compact form, without channel 0, stamp as float (relative to ch0)
   // 3 - std::vector<hadaq::MessageDouble> - compact form, with channel 0, absolute time stamp as double
    base::ProcMgr::instance()->SetStoreKind(1);


   // when configured as output in DABC, one specifies:
   // <OutputPort name="Output2" url="stream://file.root?maxsize=5000&kind=3"/>


}

// extern "C" required by DABC to find function from compiled code
//TH1* h1;
extern "C" void after_create(hadaq::HldProcessor* hld)
{
   printf("Called after all sub-components are created\n");

   if (hld==0) return;

   for (unsigned k=0;k<hld->NumberOfTRB();k++) {
      hadaq::TrbProcessor* trb = hld->GetTRB(k);
      if (trb==0) continue;

//trb->DisableCalibrationFor(0,8);

      printf("Configure %s!\n", trb->GetName());
      trb->SetPrintErrors(10);
   }

   unsigned firsttdc = 0;

   for (unsigned k=0;k<hld->NumberOfTDC();k++) {
      hadaq::TdcProcessor* tdc = hld->GetTDC(k);
      if (tdc==0) continue;
      if (firsttdc == 0) firsttdc = tdc->GetID();

      printf("Configure %s!\n", tdc->GetName());

      tdc->SetUseLastHit(false);
      for (int i=2;i<52;i++) {
         tdc->SetRefChannel(i,i-1, tdc->GetID(), 30000, -50, 50); // LED DIFF
       }

       if (tdc->GetID() == 0x0507) {
          tdc->SetToTRange(20, 15., 60.);
       }
   }

}
