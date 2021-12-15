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

      virtual Bool_t BuildEvent(TGo4EventElement*);

      virtual bool InternalHistFormat() const { return false; }

      virtual void SetSortedOrder(bool = true);
      virtual bool IsSortedOrder();

      virtual base::H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = 0);
      virtual bool GetH1NBins(base::H1handle h1, int &nbins);
      virtual void FillH1(base::H1handle h1, double x, double weight = 1.);
      virtual double GetH1Content(base::H1handle h1, int nbin);
      virtual void SetH1Content(base::H1handle h1, int bin, double v = 0.);
      virtual void ClearH1(base::H1handle h1);
      virtual void CopyH1(base::H1handle tgt, base::H1handle src);
      virtual void SetH1Title(base::H1handle h1, const char* title);

      virtual base::H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = 0);
      virtual bool GetH2NBins(base::H2handle h2, int &nbins1, int &nbins2);
      virtual void FillH2(base::H2handle h2, double x, double y, double weight = 1.);
      virtual double GetH2Content(base::H2handle h2, int bin1, int bin2);
      virtual void SetH2Content(base::H2handle h2, int bin1, int bin2, double v = 0.);
      virtual void ClearH2(base::H2handle h2);
      virtual void SetH2Title(base::H2handle h2, const char* title);

      virtual void ClearAllHistograms();

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

