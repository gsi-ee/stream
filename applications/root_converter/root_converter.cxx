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

   auto dir = gSystem->OpenDirectory(".");

   auto f = TFile::Open(fname, "recreate");

   while (auto entry = gSystem->GetDirEntry(dir)) {
      std::string s = entry;
      if ((s.length() < 8) || (s.find(".cal") != s.length() - 4))
         continue;
      if (prefix && *prefix && s.find(prefix) != 0)
         continue;

      printf("File: %s\n", entry);

      auto tdc = hadaq::TdcProcessor::CreateFromCalibr(trb3, s);

      if (!tdc) continue;

      auto subdir = f->mkdir(tdc->GetName());

      for (unsigned nch = 0; nch < tdc->NumChannels(); nch++) {
         auto rising_name = TString::Format("%s_ch%02d_rising", tdc->GetName(), nch);
         auto rising_title = TString::Format("Calibration of channel %02d on TDC %04x, rising edge", nch, tdc->GetID());
         auto h_rising = new TH1F(rising_name, rising_title, tdc->NumFineBins(), 0, tdc->NumFineBins());
         auto fall_name = TString::Format("%s_ch%02d_falling", tdc->GetName(), nch);
         auto fall_title = TString::Format("Calibration of channel %02d on TDC %04x, falling edge", nch, tdc->GetID());
         auto h_fall = new TH1F(fall_name, fall_title, tdc->NumFineBins(), 0, tdc->NumFineBins());
         for (unsigned bin = 0; bin < tdc->NumFineBins(); ++bin) {
            h_rising->SetBinContent(bin+1, tdc->GetCalibrFunc(nch, true, bin));
            h_fall->SetBinContent(bin+1, tdc->GetCalibrFunc(nch, false, bin));
         }

         subdir->WriteTObject(h_rising, rising_name);
         subdir->WriteTObject(h_fall, fall_name);
      }

   }

   gSystem->FreeDirectory(dir);

   f->Close();
   delete f;

}
