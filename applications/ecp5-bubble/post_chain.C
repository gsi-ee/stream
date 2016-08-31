TH1* GetHist(TFile* f, const char* fullname) {
   TObject* obj = f->Get(fullname);

   TH1* h1 = dynamic_cast<TH1*> (obj);
   if (h1!=0) h1->SetDirectory(0);

   return h1;
}

void post_chain() {
   TFile* f1 = TFile::Open("res/scan.root");
   //TFile* f1 = TFile::Open("res/152255.root");

   double sum0,sum1,sum2, mean[33], rms[33];

   for (int nch=1;nch<4;nch++) {

      TString name;
      name.Form("Histograms/TDC_1202/Ch%d/TDC_1202_Ch%d_RisingRef", nch, nch);

      TH1* h1 = GetHist(f1, name.Data());

      if (h1==0) {
         printf("Did not find histogram %s\n", name.Data());
         return;
      }

      int nbins = h1->GetNbinsX();

      int maxbin = h1->GetMaximumBin();

      int left = maxbin - nbins/6,
            right = maxbin + nbins/6;

      if (left<1) left = 1;
      if (right > nbins-2) right = nbins+2;

      sum0 = 0; sum1 = 0; sum2 = 0;
      for (int i=left;i<=right;++i) {
         double x = h1->GetBinCenter(i),
               w = h1->GetBinContent(i);
         sum0 += w;
         sum1 += x*w;
         sum2 += x*x*w;
      }

      mean[nch] = 0; rms[nch] = 1;
      if (sum0>10) {
         mean[nch] = sum1/sum0;
         rms[nch] = TMath::Sqrt(sum2/sum0 - mean[nch]*mean[nch]);
      }
   }

   printf("Entries %7.0f  RMS: %7.6f %7.6f %7.6f  Mean1: %7.6f\n", sum0, rms[1], rms[2], rms[3], mean[1]);

   delete f1;

}
