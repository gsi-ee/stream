#ifndef BASE_PROCESSOR_H
#define BASE_PROCESSOR_H

#include <string>

#include "base/ProcMgr.h"

#define DefFillH1(h1, x, w) {                                        \
  if (h1 && fIntHistFormat) {                                        \
     double* arr = (double*) h1;                                     \
     int nbin = (int) arr[0];                                        \
     int bin = (int) (nbin * (x - arr[1]) / (arr[2] - arr[1]));      \
     if (bin<0) arr[3]+=w; else                                      \
     if (bin>=nbin) arr[4+nbin]+=w; else arr[4+bin]+=w;              \
  } else {                                                           \
     if (h1) mgr()->FillH1(h1, x, w);                                \
  }                                                                  \
}

#define DefFastFillH1(h1,x,weight) {              \
    if (h1) {                                     \
      if (fIntHistFormat)                         \
        ((double*) h1)[4+(x)] += weight;          \
     else                                         \
        mgr()->FillH1(h1, (x), weight);           \
     }                                            \
}

#define DefFillH2(h2,x,y,weight) {               \
  if (h2 && fIntHistFormat) {                    \
  double* arr = (double*) h2;                    \
  int nbin1 = (int) arr[0];                      \
  int nbin2 = (int) arr[3];                      \
  int bin1 = (int) (nbin1 * (x - arr[1]) / (arr[2] - arr[1]));  \
  int bin2 = (int) (nbin2 * (y - arr[4]) / (arr[5] - arr[4]));  \
  if (bin1<0) bin1 = -1; else if (bin1>nbin1) bin1 = nbin1;     \
  if (bin2<0) bin2 = -1; else if (bin2>nbin2) bin2 = nbin2;     \
  arr[6 + (bin1+1) + (bin2+1)*(nbin1+2)]+=weight;               \
} else {                                                        \
  if (h2) mgr()->FillH2(h2, x, y, weight);                      \
} }

#define DefFastFillH2(h2,x,y) {                                            \
  if (h2 && fIntHistFormat) {                                              \
     ((double*) h2)[6 + (x+1) + (y+1) * ((int) *((double*)h2) + 2)] += 1.; \
   } else {                                                                \
     if (h2) mgr()->FillH2(h2, x, y, 1.);                                  \
   }                                                                       \
}



namespace base {

   /** \brief Abstract processor
    *
    * \ingroup stream_core_classes
    *
    * Class base::Processor is abstract processor.
    * Provides service functions to create histograms and conditions */

   class Processor {
      friend class ProcMgr;

      private:

         void SetManager(base::ProcMgr* m);


      protected:

         enum { DummyBrdId = 0xffffffff };

         std::string   fName;                     //!< processor name, used for event naming
         unsigned      fID;                       //!< identifier, used mostly for debugging
         ProcMgr*      fMgr;                      //!< direct pointer on manager
         std::string   fPathPrefix;               //!< histogram path prefix, used for histogram folder name
         std::string   fPrefix;                   //!< prefix, used for histogram names
         std::string   fSubPrefixD;               //!< sub-prefix for histogram directory
         std::string   fSubPrefixN;               //!< sub-prefix for histogram names
         int           fHistFilling;              //!< level of histogram filling
         unsigned      fStoreKind;                //!< if >0, store will be enabled for processor
         bool          fIntHistFormat;            //!< if true, internal histogram format is used

         /** Make constructor protected - no way to create base class instance */
         Processor(const char* name = "", unsigned brdid = DummyBrdId);

         void SetBoardId(unsigned id) { fID = id; }

         void SetPathPrefix(const std::string &prefix) { fPathPrefix = prefix; }

         /** Set subprefix for histograms and conditions */
         void SetSubPrefix(const char* subname = "", int indx = -1, const char* subname2 = "", int indx2 = -1);

         /** Set subprefix for histograms and conditions, index uses 2 symbols */
         void SetSubPrefix2(const char* subname = "", int indx = -1, const char* subname2 = "", int indx2 = -1);

         H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = 0);

         /** Fill 1-D histogram */
         inline void FillH1(H1handle h1, double x, double weight = 1.)
           { DefFillH1(h1,x,weight); }

         /** \brief Fast fill 1-D histogram
          * \details Can only be used where index is same as x itself, no range checks are performed */
         inline void FastFillH1(H1handle h1, int x, double weight = 1.)
            { DefFastFillH1(h1,x, weight); }

         /** Get bin content of 1-D histogram */
         inline double GetH1Content(H1handle h1, int nbin)
         {
            return h1 ? mgr()->GetH1Content(h1, nbin) : 0.;
         }

         /** Set bin content of 1-D histogram */
         inline void SetH1Content(H1handle h1, int nbin, double v = 0.)
         {
            if (h1) mgr()->SetH1Content(h1, nbin, v);
         }

         /** Get bins numbers for 1-D histogram */
         inline int GetH1NBins(H1handle h1)
         {
            int nbins = 0;
            bool isGood = mgr()->GetH1NBins(h1, nbins);
            return isGood ? nbins : 0;
         }

         /** Clear 1-D histogram */
         inline void ClearH1(H1handle h1)
         {
            if (h1) mgr()->ClearH1(h1);
         }

         /** Copy 1-D histogram from src to tgt */
         inline void CopyH1(H1handle tgt, H1handle src)
         {
            mgr()->CopyH1(tgt, src);
         }

         /** Set 1-D histogram title */
         inline void SetH1Title(H1handle h1, const char *title)
         {
            mgr()->SetH1Title(h1, title);
         }

         H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = 0);

         inline void FillH2(H1handle h2, double x, double y, double weight = 1.)
         { DefFillH2(h2,x,y,weight); }

         /** Can only be used where index is same as x and y themself, no range checks are performed */
         inline void FastFillH2(H1handle h2, int x, int y)
         { DefFastFillH2(h2,x,y); }

         inline void SetH2Content(H2handle h2, int nbin1, int nbin2, double v = 0.)
         {
            if (h2) mgr()->SetH2Content(h2, nbin1, nbin2, v);
         }

         inline double GetH2Content(H2handle h2, int bin1, int bin2)
         {
            return h2 ? mgr()->GetH2Content(h2, bin1, bin2) : 0.;
         }

         inline bool GetH2NBins(H2handle h2, int &nBins1, int &nBins2)
         {
            bool isGood = mgr()->GetH2NBins(h2, nBins1, nBins2);
            return isGood;
         }

         inline void ClearH2(base::H2handle h2)
         {
            if (h2) mgr()->ClearH2(h2);
         }

         inline void SetH2Title(H2handle h2, const char* title)
         {
            mgr()->SetH2Title(h2, title);
         }


         C1handle MakeC1(const char* name, double left, double right, H1handle h1 = 0);

         void ChangeC1(C1handle c1, double left, double right);

         inline int TestC1(C1handle c1, double value, double* dist = 0)
         {
            return mgr()->TestC1(c1, value, dist);
         }

         double GetC1Limit(C1handle c1, bool isleft = true);

         virtual void CreateBranch(TTree*) {}

         virtual bool RegisterObject(TObject* tobj, const char* subfolder = 0)
         {
            return mgr()->RegisterObject(tobj, subfolder);
         }


      public:

         virtual ~Processor();

         ProcMgr* mgr() const { return fMgr; }

         const char* GetName() const { return fName.c_str(); }
         unsigned GetID() const { return fID; }

         inline void SetHistFilling(int lvl = 99) { fHistFilling = lvl; }
         inline bool IsHistFilling() const { return fHistFilling > 0; }
         inline int  HistFillLevel() const { return fHistFilling; }

         unsigned GetStoreKind() const { return fStoreKind; }
         bool IsStoreEnabled() const { return GetStoreKind()!=0; }

         virtual void SetStoreKind(unsigned kind = 1) { fStoreKind = kind; }
         void SetStoreEnabled(bool on = true) { SetStoreKind(on ? 1 : 0); }

         virtual void UserPreLoop() {}

         virtual void UserPostLoop() {}
   };


}

#endif
