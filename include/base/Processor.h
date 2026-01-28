#ifndef BASE_PROCESSOR_H
#define BASE_PROCESSOR_H

#include <string>

#include "base/ProcMgr.h"

#define DefFillH1(h1, x, w) {                                            \
  if (h1 && fIntHistFormat) {                                            \
     double* __arr = (double*) h1;                                       \
     int __nbin = (int) __arr[0];                                        \
     int __bin = (int) (__nbin * (x - __arr[1]) / (__arr[2] - __arr[1]));\
     if (__bin < 0) __arr[3]+=w; else                                    \
     if (__bin>=__nbin) __arr[4+__nbin]+=w; else __arr[4+__bin]+=w;      \
  } else {                                                               \
     if (h1) mgr()->FillH1(h1, x, w);                                    \
  }                                                                      \
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
  double* __arr = (double*) h2;                  \
  int __nbin1 = (int) __arr[0];                  \
  int __nbin2 = (int) __arr[3];                  \
  int __bin1 = (int) (__nbin1 * (x - __arr[1]) / (__arr[2] - __arr[1]));  \
  int __bin2 = (int) (__nbin2 * (y - __arr[4]) / (__arr[5] - __arr[4]));  \
  if (__bin1<0) __bin1 = -1; else if (__bin1>__nbin1) __bin1 = __nbin1;   \
  if (__bin2<0) __bin2 = -1; else if (__bin2>__nbin2) __bin2 = __nbin2;   \
  __arr[6 + (__bin1+1) + (__bin2+1)*(__nbin1+2)]+=weight;                 \
} else {                                                                  \
  if (h2) mgr()->FillH2(h2, x, y, weight);                                \
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

         std::string   fName;                     ///< processor name, used for event naming
         unsigned      fID;                       ///< identifier, used mostly for debugging
         ProcMgr*      fMgr;                      ///< direct pointer on manager
         std::string   fPathPrefix;               ///< histogram path prefix, used for histogram folder name
         std::string   fPrefix;                   ///< prefix, used for histogram names
         std::string   fSubPrefixD;               ///< sub-prefix for histogram directory
         std::string   fSubPrefixN;               ///< sub-prefix for histogram names
         int           fHistFilling;              ///< level of histogram filling
         unsigned      fStoreKind;                ///< if >0, store will be enabled for processor
         bool          fIntHistFormat;            ///< if true, internal histogram format is used

         /** Make constructor protected - no way to create base class instance */
         Processor(const char* name = "", unsigned brdid = DummyBrdId);

         /** Set board id */
         void SetBoardId(unsigned id) { fID = id; }

         /** Set path prefix for histogramsid */
         void SetPathPrefix(const std::string &prefix) { fPathPrefix = prefix; }

         /** Set subprefix for histograms and conditions */
         void SetSubPrefix(const char* subname = "", int indx = -1, const char* subname2 = "", int indx2 = -1);

         /** Set subprefix for histograms and conditions, index uses 2 symbols */
         void SetSubPrefix2(const char* subname = "", int indx = -1, const char* subname2 = "", int indx2 = -1);

         H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = nullptr);

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

         H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = nullptr);

         /** Fill 2-D histogram */
         inline void FillH2(H1handle h2, double x, double y, double weight = 1.)
         { DefFillH2(h2,x,y,weight); }

         /** \brief Fast fill 2-D histogram
           * \details  Can only be used where index is same as x and y themself, no range checks are performed */
         inline void FastFillH2(H1handle h2, int x, int y)
         { DefFastFillH2(h2,x,y); }

         /** Set bin content of 2-D histogram */
         inline void SetH2Content(H2handle h2, int nbin1, int nbin2, double v = 0.)
         {
            if (h2) mgr()->SetH2Content(h2, nbin1, nbin2, v);
         }

         /** Get bin content of 2-D histogram */
         inline double GetH2Content(H2handle h2, int bin1, int bin2)
         {
            return h2 ? mgr()->GetH2Content(h2, bin1, bin2) : 0.;
         }

         /** Get number of bins for 2-D histogram */
         inline bool GetH2NBins(H2handle h2, int &nBins1, int &nBins2)
         {
            bool isGood = mgr()->GetH2NBins(h2, nBins1, nBins2);
            return isGood;
         }

         /** Clear 2-D histogram */
         inline void ClearH2(base::H2handle h2)
         {
            if (h2) mgr()->ClearH2(h2);
         }

         /** Change title of 2-D histogram */
         inline void SetH2Title(H2handle h2, const char* title)
         {
            mgr()->SetH2Title(h2, title);
         }

         C1handle MakeC1(const char* name, double left, double right, H1handle h1 = nullptr);

         void ChangeC1(C1handle c1, double left, double right);

         /** Test condition */
         inline int TestC1(C1handle c1, double value, double* dist = nullptr)
         {
            return mgr()->TestC1(c1, value, dist);
         }

         double GetC1Limit(C1handle c1, bool isleft = true);

         /** Create branch */
         virtual void CreateBranch(TTree*) {}

         /** Register object */
         virtual bool RegisterObject(TObject* tobj, const char* subfolder = nullptr)
         {
            return mgr()->RegisterObject(tobj, subfolder);
         }

      public:

         virtual ~Processor();

         /** Return manager instance */
         ProcMgr* mgr() const { return fMgr; }

         /** Get processor name */
         const char* GetName() const { return fName.c_str(); }
         /** Get processor ID */
         unsigned GetID() const { return fID; }

         /** Set histogram filling level */
         inline void SetHistFilling(int lvl = 99) { fHistFilling = lvl; }
         /** Is histogram filling enabled */
         inline bool IsHistFilling() const { return fHistFilling > 0; }
         /** Get histogram filling level */
         inline int  HistFillLevel() const { return fHistFilling; }

         /** Get store kind */
         unsigned GetStoreKind() const { return fStoreKind; }
         /** Is store enabled */
         bool IsStoreEnabled() const { return GetStoreKind() != 0; }

         /** Set store kind */
         virtual void SetStoreKind(unsigned kind = 1) { fStoreKind = kind; }
         /** Enable store - set store kind 1 */
         void SetStoreEnabled(bool on = true) { SetStoreKind(on ? 1 : 0); }

         /** pre loop */
         virtual void UserPreLoop() {}

         /** post loop */
         virtual void UserPostLoop() {}
   };

}

#endif
