#ifndef BASE_PROCESSOR_H
#define BASE_PROCESSOR_H

#include <string>

#include "base/ProcMgr.h"

namespace base {

   /** Class base::Processor is abstract processor.
    * Provides service functions to create histograms and conditions */

   class Processor {
      friend class ProcMgr;

      protected:

         enum { DummyBrdId = 0xffffffff };

         std::string   fName;                     //!< processor name, used for event naming
         unsigned      fBoardId;                  //!< identifier, used mostly for debugging
         ProcMgr*      fMgr;                      //!< direct pointer on manager
         std::string   fPrefix;                   //!< prefix, used for histogram names
         std::string   fSubPrefixD;               //!< sub-prefix for histogram directory
         std::string   fSubPrefixN;               //!< sub-prefix for histogram names
         int           fHistFilling;              //!< level of histogram filling
         bool          fStoreEnabled;             //!< if true, store will be enabled for processor

         /** Make constructor protected - no way to create base class instance */
         Processor(const char* name = "", unsigned brdid = DummyBrdId);

         void SetBoardId(unsigned id) { fBoardId = id; }

         /** Set subprefix for histograms and conditions */
         void SetSubPrefix(const char* subname = "", int indx = -1, const char* subname2 = "", int indx2 = -1);

         H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = 0);

         inline void FillH1(H1handle h1, double x, double weight = 1.)
         {
            if (IsHistFilling() && (h1!=0)) mgr()->FillH1(h1, x, weight);
         }

         inline double GetH1Content(H1handle h1, int nbin)
         {
            return mgr()->GetH1Content(h1, nbin);
         }

         inline void ClearH1(base::H1handle h1)
         {
            if (IsHistFilling()) mgr()->ClearH1(h1);
         }

         H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = 0);

         inline void FillH2(H1handle h2, double x, double y, double weight = 1.)
         {
            if (IsHistFilling() && (h2!=0)) mgr()->FillH2(h2, x, y, weight);
         }

         C1handle MakeC1(const char* name, double left, double right, H1handle h1 = 0);

         void ChangeC1(C1handle c1, double left, double right);

         inline int TestC1(C1handle c1, double value, double* dist = 0)
         {
            return mgr()->TestC1(c1, value, dist);
         }

         double GetC1Limit(C1handle c1, bool isleft = true);

         virtual void CreateBranch(TTree*) {}

      public:

         virtual ~Processor();

         ProcMgr* mgr() const { return fMgr; }

         const char* GetName() const { return fName.c_str(); }
         unsigned GetBoardId() const { return fBoardId; }

         void SetHistFilling(int lvl = 99) { fHistFilling = lvl; }
         bool IsHistFilling() const { return fHistFilling > 0; }
         int  HistFillLevel() const { return fHistFilling; }

         void SetStoreEnabled(bool on = true) { fStoreEnabled = on; }
         bool IsStoreEnabled() const { return fStoreEnabled; }

         virtual void UserPreLoop() {}

         virtual void UserPostLoop() {}
   };

}

#endif
