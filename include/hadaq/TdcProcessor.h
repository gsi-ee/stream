#ifndef HADAQ_TDCPROCESSOR_H
#define HADAQ_TDCPROCESSOR_H

#include "base/StreamProc.h"

#include "hadaq/defines.h"

#include "hadaq/TdcMessage.h"

namespace hadaq {

   class TrbProcessor;

   /** This is specialized sub-processor for FPGA-TDC.
    * It only can be used together with TrbProcessor  */

   class TdcProcessor : public base::StreamProc {

      protected:

         unsigned  fTdcId;          //! board id, which will be used for the filling

         base::H1handle fMsgPerBrd;  //! messages per board
         base::H1handle fErrPerBrd;  //! errors per board
         base::H1handle fHitsPerBrd; //! data hits per board

         base::H1handle fChannels;  //! histogram with messages per channel
         base::H1handle fMsgsKind;  //! messages kinds
         base::H1handle fAllFine;   //! histogram of all fine counters
         base::H1handle fAllCoarse; //! histogram of all coarse counters


         base::H1handle fCoarse[NumTdcChannels]; //!
         base::H1handle fFine[NumTdcChannels];   //!

         double fCh0Time;           //! value, which is found after first scan

         /** Returns true when processor used to select trigger signal
          * TDC not yet able to perform trigger selection */
         virtual bool doTriggerSelection() const { return false; }

         static unsigned fMaxBrdId;     //! maximum board id
         static unsigned fFineMinValue; //! minimum value for fine counter, used for simple linear approximation
         static unsigned fFineMaxValue; //! maximum value for fine counter, used for simple linear approximation

         double CoarseUnit() const { return 5.; }

         double SimpleFineCalibr(unsigned fine)
         {
            if (fine<=fFineMinValue) return 0.;
            if (fine>=fFineMaxValue) return CoarseUnit();
            return (CoarseUnit() * (fine - fFineMinValue)) / (fFineMaxValue - fFineMinValue);
         }

      public:

         TdcProcessor(TrbProcessor* trb, unsigned tdcid);
         virtual ~TdcProcessor();

         static void SetMaxBoardId(unsigned max) { fMaxBrdId = max; }

         /** Method set static limits, which are used for simple interpolation of time for fine counter */
         static void SetFineLimits(unsigned min, unsigned max)
         {
            fFineMinValue = min;
            fFineMaxValue = max;
         }

         /** Returns configured board id */
         unsigned GetBoardId() const { return fTdcId; }

         void BeforeFirstScan() { fCh0Time = 0.; }

         /** Method to scan data, which are related to that TDC */
         void FirstSubScan(hadaq::RawSubevent* sub, unsigned indx, unsigned datalen);

         /** Method will be called by TRB processor if SYNC message was found */
         double AddSyncIfFound(unsigned syncid);



   };
}

#endif
