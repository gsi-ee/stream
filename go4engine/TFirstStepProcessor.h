#ifndef TFIRSTSTEPPROCESSOR_H
#define TFIRSTSTEPPROCESSOR_H

#include "TGo4EventProcessor.h"
#include "base/ProcMgr.h"

class TFirstStepProcessor : public TGo4EventProcessor,
                            public base::ProcMgr {

   protected:

      long fTotalDataSize;
      long fNumMbsBufs;
      long fNumCbmEvents;

      static TString fDfltSetupScript;

   public:

      TFirstStepProcessor();
      TFirstStepProcessor(const char* name);
      virtual ~TFirstStepProcessor();

      static void SetDfltScript(const char* name) { fDfltSetupScript = name; }

      /* Can be overwritten by subclass, but is not recommended! use ProcessEvent or ProcessSubevent instead*/
      virtual Bool_t BuildEvent(TGo4EventElement*);

      virtual base::H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = 0);
      virtual void FillH1(base::H1handle h1, double x, double weight = 1.);
      virtual double GetH1Content(base::H1handle h1, int nbin);
      virtual void ClearH1(base::H1handle h1);

      virtual base::H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = 0);
      virtual void FillH2(base::H1handle h2, double x, double y, double weight = 1.);

      virtual base::C1handle MakeC1(const char* name, double left, double right, base::H1handle h1 = 0);
      virtual void ChangeC1(base::C1handle c1, double left, double right);
      virtual int TestC1(base::C1handle c1, double value, double* dist = 0);
      virtual double GetC1Limit(base::C1handle c1, bool isleft = true);


   private:

      ClassDef(TFirstStepProcessor,1)
};

#endif

