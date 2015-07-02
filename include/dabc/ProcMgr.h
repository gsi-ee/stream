#ifndef DABC_DABCPROCMGR_H
#define DABC_DABCPROCMGR_H

#include "base/ProcMgr.h"

#include "dabc/Hierarchy.h"

namespace dabc {

class ProcMgr : public base::ProcMgr {
   protected:

   public:
      dabc::Hierarchy fTop;

      ProcMgr() : base::ProcMgr(), fTop() {}
      virtual ~ProcMgr() {}

      // redefine only make procedure, fill and clear should work
      virtual base::H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = 0)
      {
         dabc::Hierarchy h = fTop.CreateHChild(name);
         if (h.null()) return 0;

         h.SetField("_kind","ROOT.TH1D");
         h.SetField("_title", title);
         h.SetField("_dabc_hist", true); // indicate for browser that it is DABC histogram
         h.SetField("nbins", nbins);
         h.SetField("left", left);
         h.SetField("right", right);

         std::vector<double> bins;
         bins.resize(nbins+3, 0.);
         bins[0] = nbins;
         bins[1] = left;
         bins[2] = right;
         h.SetField("bins", bins);

         return (base::H1handle) h.GetFieldPtr("bins")->GetDoubleArr();
      }

      virtual base::H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = 0)
      {
         dabc::Hierarchy h = fTop.CreateHChild(name);
         if (h.null()) return 0;

         h.SetField("_kind","ROOT.TH2D");
         h.SetField("_title", title);
         h.SetField("_dabc_hist", true); // indicate for browser that it is DABC histogram
         h.SetField("nbins1", nbins1);
         h.SetField("left1", left1);
         h.SetField("right1", right1);
         h.SetField("nbins2", nbins2);
         h.SetField("left2", left2);
         h.SetField("right2", right2);

         std::vector<double> bins;
         bins.resize(nbins1*nbins2+6, 0.);
         bins[0] = nbins1;
         bins[1] = left1;
         bins[2] = right1;
         bins[3] = nbins2;
         bins[4] = left2;
         bins[5] = right2;
         h.SetField("bins", bins);

         return h.GetFieldPtr("bins")->GetDoubleArr();
      }
};

}

#endif
