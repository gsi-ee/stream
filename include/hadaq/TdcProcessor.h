#ifndef HADAQ_TDCPROCESSOR_H
#define HADAQ_TDCPROCESSOR_H

#include "base/StreamProc.h"

#include "hadaq/defines.h"

namespace hadaq {

   class TrbProcessor;

   enum { MaxTdc = 32 };       // maximum number of TDCs

   enum { NumTdcChannels = 65 }; // number of TDC channels

   /** This is specialized sub-processor for FPGA-TDC.
    * It only can be used together with TrbProcessor  */

   class TdcProcessor : public base::StreamProc {

      protected:

         unsigned  fTdcId;          //! board id, which will be used for the filling

         base::H1handle fChannels;  //! histogram with messages per channel

         base::H1handle fCoarse[NumTdcChannels]; //!
         base::H1handle fFine[NumTdcChannels];   //!

         /** Returns true when processor used to select trigger signal
          * TDC not yet able to perform trigger selection */
         virtual bool doTriggerSelection() const { return false; }

      public:

         TdcProcessor(TrbProcessor* trb, unsigned tdcid);
         virtual ~TdcProcessor();

         /** Returns configured board id */
         unsigned GetBoardId() const { return fTdcId; }

         /** Method to scan data, which are related to that TDC */
         void FirstSubScan(hadaq::RawSubevent* sub, unsigned indx, unsigned datalen);


   };
}

#endif
