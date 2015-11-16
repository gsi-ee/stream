#ifndef TFIRSTSTEPPROCESSOR_H
#define TFIRSTSTEPPROCESSOR_H

#include "TGo4EventProcessor.h"
#include "root/TRootProcMgr.h"

class TFirstStepProcessor : public TGo4EventProcessor,
                            public TRootProcMgr {

   protected:

      long fTotalDataSize;
      long fNumInpBufs;
      long fNumOutEvents;

   public:

      TFirstStepProcessor();
      TFirstStepProcessor(const char* name);
      virtual ~TFirstStepProcessor();

      /* Can be overwritten by subclass, but is not recommended! use ProcessEvent or ProcessSubevent instead*/
      virtual Bool_t BuildEvent(TGo4EventElement*);

      virtual bool InternalHistFormat() const { return false; }

      virtual base::H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = 0);
      virtual void FillH1(base::H1handle h1, double x, double weight = 1.);
      virtual double GetH1Content(base::H1handle h1, int nbin);
      virtual void ClearH1(base::H1handle h1);

      virtual base::H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = 0);
      virtual void FillH2(base::H1handle h2, double x, double y, double weight = 1.);
      virtual void ClearH2(base::H2handle h2);

      virtual base::C1handle MakeC1(const char* name, double left, double right, base::H1handle h1 = 0);
      virtual void ChangeC1(base::C1handle c1, double left, double right);
      virtual int TestC1(base::C1handle c1, double value, double* dist = 0);
      virtual double GetC1Limit(base::C1handle c1, bool isleft = true);

      virtual bool RegisterObject(TObject* tobj, const char* subfolder = 0);

      virtual void UserPreLoop();
      virtual void UserPostLoop();

   ClassDef(TFirstStepProcessor,1)
};

#endif

