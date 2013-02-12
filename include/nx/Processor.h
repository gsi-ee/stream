#ifndef NX_PROCESSOR_H
#define NX_PROCESSOR_H

#include "base/SysCoreProc.h"

#include "nx/Iterator.h"

namespace nx {

   struct NxRec {
      bool used;

      base::H1handle fChannels;  //! histogram with system types
      base::H2handle fADCs;      //! ADC distribution
      base::H1handle fHITt;      //! hit time

      NxRec() : used(false), fChannels(0), fADCs(0), fHITt(0) {}
   };

   class Processor : public base::SysCoreProc {

      protected:
         nx::Iterator fIter1;  //! first iterator over all messages
         nx::Iterator fIter2;  //! second iterator over all messages

         base::H1handle fMsgsKind;   //! histogram with messages kinds
         base::H1handle fSysTypes;   //! histogram with system types

         std::vector<nx::NxRec> NX;      //! usage masks for nxyters

         int fNumHits;               //! total number of hits
         int fNumBadHits;            //! total number of bad hits
         int fNumCorrHits;           //! total number of corrected hits

         bool nx_in_use(unsigned id) { return id < NX.size() ? NX[id].used : false; }

         bool IsValidBufIndex(unsigned indx) const { return indx<fQueue.size(); }

         void AssignBufferTo(nx::Iterator& iter, const base::Buffer& buf);

         // this constant identify to which extend NX time can be disordered
         virtual double MaximumDisorderTm() const { return fNXDisorderTm; }

         static double fNXDisorderTm;
         static bool fLastEpochCorr;

      public:

         Processor(unsigned rocid, unsigned nxmask = 0x5, base::OpticSplitter* spl = 0);
         virtual ~Processor();

         /** Scan NX messages for SYNC, AUX and all other kind of messages */
         virtual bool FirstBufferScan(const base::Buffer& buf);

         /** Scan buffer for selecting them inside trigger */
         virtual bool SecondBufferScan(const base::Buffer& buf);

         static void SetDisorderTm(double v) { fNXDisorderTm = v; }
         static void SetLastEpochCorr(bool on) { fLastEpochCorr = on; }
         static bool IsLastEpochCorr() { return fLastEpochCorr; }
   };
}

#endif
