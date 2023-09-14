#include "base/ProcMgr.h"
#include "hadaq/HldProcessor.h"
#include "hadaq/TrbProcessor.h"
#include "hadaq/TdcProcessor.h"

#include "TSystem.h"


// Converter of calibration files into ROOT histogram

void root_converter(const char *prefix = "local", const char *fname = "calibr.root")
{
   // important - number 600 is number of bins in calibration
   hadaq::TdcProcessor::SetDefaults(600, 200, 1);

   auto mgr = new base::ProcMgr;

   auto hld = new hadaq::HldProcessor();
   auto trb3 = new hadaq::TrbProcessor(0x8000, hld);

   //hadaq::TdcProcessor::CreateFromCalibr(trb3, "local5000.cal");
   //hadaq::TdcProcessor::CreateFromCalibr(trb3, "local5001.cal");
   //hadaq::TdcProcessor::CreateFromCalibr(trb3, "test_1000.cal");
   //hadaq::TdcProcessor::CreateFromCalibr(trb3, "test_0050.cal");

   auto dir = gSystem->OpenDirectory(".");

   while (auto entry = gSystem->GetDirEntry(dir)) {
      std::string s = entry;
      if ((s.length() < 8) || (s.find(".cal") != s.length() - 4))
         continue;
      if (prefix && *prefix && s.find(prefix) != 0)
         continue;
      printf("Entry: %s\n", entry);

      auto tdc = hadaq::TdcProcessor::CreateFromCalibr(trb3, s);

   }

   gSystem->FreeDirectory(dir);

}
