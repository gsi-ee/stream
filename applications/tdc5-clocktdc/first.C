/// This is example of automatic TDC creation

#include "base/ProcMgr.h"
#include "hadaq/HldProcessor.h"
#include "hadaq/TdcProcessor.h"
#include "hadaq/TrbProcessor.h"

void first()
{
   base::ProcMgr::instance()->SetRawAnalysis(true);
   // base::ProcMgr::instance()->SetTriggeredAnalysis(true);

   // all new instances get this value
   base::ProcMgr::instance()->SetHistFilling(4);

   // default for TDC processor
   // first value - range for fine counter
   // second - ToT histogram range
   // third - reduce factor for 2D histogram, not needed for low number of fine counters
   hadaq::TdcProcessor::SetDefaults(16, 50, 1);

   // this limits used for liner calibrations when nothing else is available
   hadaq::TdcMessage::SetFineLimits(0, 15);

   // configure ToT calibration parameter
   // first  - minimal number of counts
   // second - RMS in ns to reject produced ToT value
   hadaq::TdcProcessor::SetToTCalibr(1000, 0.5);

   // use D trigger also for normal ref histograms
   hadaq::TdcProcessor::SetUseDTrigForRef(true);

   // default channel numbers and edges mask
   // 1 - use only rising edge, falling edge is ignore
   // 2   - falling edge enabled and fully independent from rising edge
   // 3   - falling edge enabled and uses calibration from rising edge
   // 4   - falling edge enabled and common statistic is used for calibration
   hadaq::TrbProcessor::SetDefaults(32, 2);

   // [min..max] range for TDC ids
   hadaq::TrbProcessor::SetTDCRange(0x1000, 0x7FFF);

   // true indicates that dogma data are expected
   auto proc = new hadaq::TrbProcessor(0, nullptr, 4, true);

   auto vect =  proc->CreateTDC5(0xd060);

   for (auto tdc : vect) {

      tdc->SetCustomMhz(150.);
      tdc->SetToTRange(19.2, 15., 45.);

      for (int nch = 1; nch < tdc->NumChannels(); nch++)
         tdc->SetRefChannel(nch, nch - 1, 0xffffff, 160,  -20., 20.);
   }

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
   proc->ConfigureCalibration("dogmacal", -1, 1 << 0xD /* 0x3fff */);

   // only accept trigger type 0x1 when storing file
   // new hadaq::HldFilter(0x1);

   // create ROOT file store
   // base::ProcMgr::instance()->CreateStore("td.root");

   // 0 - disable store
   // 1 - std::vector<hadaq::TdcMessageExt> - includes original TDC message
   // 2 - std::vector<hadaq::MessageFloat>  - compact form, without channel 0, stamp as float (relative to ch0)
   // 3 - std::vector<hadaq::MessageDouble> - compact form, with channel 0, absolute time stamp as double
   base::ProcMgr::instance()->SetStoreKind(0);
}
