#ifndef NX_PROCESSOR_H
#define NX_PROCESSOR_H

#include "base/SysCoreProc.h"

#include "nx/Iterator.h"

namespace nx {

   class SubEvent;

   struct NxRec {
      bool used;

      base::H1handle fChannels;  //! histogram with system types
      base::H2handle fADCs;      //! ADC distribution
      base::H1handle fHITt;      //! hit time


      // these variables for wrong last-epoch bit detection
      // idea is simple - every readout costs 32 ns
      // if last epoch is appears, one need to have long readout history

/*      unsigned lasthittm;
      unsigned reversecnt;
      unsigned lasthit[128];
      unsigned lastch;
      unsigned lastepoch;
*/
      uint64_t lastfulltm1;
      uint64_t lastfulltm2;

      NxRec() : used(false), fChannels(0), fADCs(0), fHITt(0), lastfulltm1(0), lastfulltm2(0) {}

//      void corr_reset();
//      void corr_nextepoch(unsigned epoch);
//      int corr_nexthit_old(nx::Message& msg, bool docorr = false);
//      int corr_nexthit_next(nx::Message& msg, bool docorr = false);
      int corr_nexthit(nx::Message& msg, uint64_t fulltm, bool docorr = false, bool firstscan = true);
   };

   class Processor : public base::SysCoreProc {

      protected:
         nx::Iterator fIter;  //! first iterator over all messages
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

         /** Method should return measure for subevent multiplicity */
         virtual unsigned GetTriggerMultipl(unsigned indx);

         virtual void SortDataInSubEvent(base::SubEvent*);

         static double fNXDisorderTm;
         static bool fLastEpochCorr;

      public:

         Processor(unsigned rocid, unsigned nxmask = 0x5);
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
