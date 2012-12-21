#ifndef GET4_PROCESSOR_H
#define GET4_PROCESSOR_H

#include "base/SysCoreProc.h"

#include "get4/Iterator.h"

namespace get4 {

   enum { NumChannels = 4 };

   struct Get4Rec {
      bool used;

      base::H1handle fChannels;  //! histogram with system types

      base::H1handle fRisCoarseTm[NumChannels]; //! histograms of rising stamp for each channel
      base::H1handle fFalCoarseTm[NumChannels]; //! histograms of falling stamp for each channel
      base::H1handle fRisFineTm[NumChannels]; //! histograms of rising stamp for each channel
      base::H1handle fFalFineTm[NumChannels]; //! histograms of falling stamp for each channel

      Get4Rec();
   };

   class Processor : public base::SysCoreProc {

      protected:

         get4::Iterator fIter;  //! first iterator over all messages
         get4::Iterator fIter2;  //! second iterator over all messages

         base::H1handle fMsgsKind;   //! histogram with messages kinds
         base::H1handle fSysTypes;   //! histogram with system types

         std::vector<get4::Get4Rec> GET4;      //! usage masks for nxyters

         unsigned fRefChannelId;              //! set channel ID, which used as trigger

         bool get4_in_use(unsigned id) { return id < GET4.size() ? GET4[id].used : false; }

         bool IsValidBufIndex(unsigned indx) const { return indx<fQueue.size(); }

         void AssignBufferTo(get4::Iterator& iter, const base::Buffer& buf);

         // this constant identify to which extend NX time can be disordered
         virtual double MaximumDisorderTm() const { return 1e-6; }

         /** Returns true when processor used to select trigger signal */
         virtual bool doTriggerSelection() const { return (fTriggerSignal < 4) || (fTriggerSignal==10) || (fTriggerSignal==11); }

         int Get4TimeDiff(uint64_t t1, uint64_t t2) { return t1<=t2 ? t2-t1 : -1*((int) (t1-t2)); }

      public:

         Processor(unsigned rocid, unsigned nxmask = 0x5);
         virtual ~Processor();

         void setRefChannel(unsigned ref_get4, unsigned ref_ch, unsigned ref_edge = 0);

         /** Scan all messages, find reference signals */
         virtual bool FirstBufferScan(const base::Buffer& buf);

         /** Scan buffer for selecting them inside trigger */
         virtual bool SecondBufferScan(const base::Buffer& buf);
   };
}

#endif
