#ifndef TFIRSTSTEPPROCESSOR_H
#define TFIRSTSTEPPROCESSOR_H

#include "TGo4EventProcessor.h"
#include "root/TRootProcMgr.h"

#include <string>

/** Handler for first.C in go4 */

class TFirstStepProcessor : public TGo4EventProcessor,
                            public TRootProcMgr {

   protected:

      long fTotalDataSize;  ///< processed data size
      long fNumInpBufs;     ///< processed number of buffers
      long fNumOutEvents;   ///< created number of output events

      std::string ReadMacroCode(const std::string &fname);

   public:

      TFirstStepProcessor();
      TFirstStepProcessor(const char* name);
      virtual ~TFirstStepProcessor();

      Bool_t BuildEvent(TGo4EventElement*) override;

      bool InternalHistFormat() const override { return false; }

      void SetSortedOrder(bool = true) override;
      bool IsSortedOrder() override;

      base::H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = nullptr) override;
      bool GetH1NBins(base::H1handle h1, int &nbins) override;
      void FillH1(base::H1handle h1, double x, double weight = 1.) override;
      double GetH1Content(base::H1handle h1, int nbin) override;
      void SetH1Content(base::H1handle h1, int bin, double v = 0.) override;
      void ClearH1(base::H1handle h1) override;
      void CopyH1(base::H1handle tgt, base::H1handle src) override;
      void SetH1Title(base::H1handle h1, const char* title) override;

      base::H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = nullptr) override;
      bool GetH2NBins(base::H2handle h2, int &nbins1, int &nbins2) override;
      void FillH2(base::H2handle h2, double x, double y, double weight = 1.) override;
      double GetH2Content(base::H2handle h2, int bin1, int bin2) override;
      void SetH2Content(base::H2handle h2, int bin1, int bin2, double v = 0.) override;
      void ClearH2(base::H2handle h2) override;
      void SetH2Title(base::H2handle h2, const char* title) override;

      void ClearAllHistograms() override;

      base::C1handle MakeC1(const char* name, double left, double right, base::H1handle h1 = nullptr) override;
      void ChangeC1(base::C1handle c1, double left, double right) override;
      int TestC1(base::C1handle c1, double value, double* dist = nullptr) override;
      double GetC1Limit(base::C1handle c1, bool isleft = true) override;

      bool RegisterObject(TObject* tobj, const char* subfolder = nullptr) override;

      void UserPreLoop() override;
      void UserPostLoop() override;

   ClassDefOverride(TFirstStepProcessor,1)
};

#endif

