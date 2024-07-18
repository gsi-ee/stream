void scan_hist(unsigned trb = 0xc004, unsigned tdc = 0x0044)
{
   if (!gFile)
     TFile::Open("Go4AutoSave.root");

   if (!gFile) {
      printf("Fail to open ROOT file\n");
      return;
   }

   const unsigned int NumCh = 32;

   TString  path = TString::Format("Histograms/TRB_%04X/TDC_%04X/", trb, tdc);

   // scan all Tot histograms

   for (unsigned nch = 0; nch < NumCh; ++nch) {

      TString hname = path + TString::Format("Ch%02u/TDC_%04X_Ch%02u_Tot", nch, tdc, nch);

      TH1 *hist = nullptr;
      gFile->GetObject(hname.Data(), hist);

      if (hist) printf("hist: %s mean: %5.3f rms: %5.3f\n", hist->GetName(), hist->GetMean(), hist->GetRMS());

      delete hist;
   }

   // scan all RisingRef histograms

   for (unsigned nch = 0; nch < NumCh; ++nch) {

      TString hname = path + TString::Format("Ch%02u/TDC_%04X_Ch%02u_RisingRef", nch, tdc, nch);

      TH1 *hist = nullptr;
      gFile->GetObject(hname.Data(), hist);

      if (hist) printf("hist: %s mean: %5.3f rms: %5.3f\n", hist->GetName(), hist->GetMean(), hist->GetRMS());

      delete hist;
   }
}
