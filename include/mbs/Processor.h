#ifndef MBS_PROCESSOR_H
#define MBS_PROCESSOR_H

#include "base/StreamProc.h"


namespace mbs {

   /** This could be generic processor for data, coming from MBS
    * For the moment it is rather CERN beamtime-specific code  */

   class Processor : public base::StreamProc {

      protected:

         base::LocalStampConverter  fConv1; ///<! use converter to emulate local time scale

         unsigned fLastSync1; ///<! last sync id in first scan
         unsigned fLastSync2; ///<! last sync id in second scan

         /** Returns true when processor used to select trigger signal
          * TRB3 not yet able to perform trigger selection */
         bool doTriggerSelection() const override { return false; }

         /** This is maximum disorder time for MBS
          * TODO: derive this value from sub-items */
         double MaximumDisorderTm() const override { return 1e-6; }

      public:

         Processor();

         virtual ~Processor();

         /** Scan all messages, find reference signals */
         bool FirstBufferScan(const base::Buffer& buf) override;

         bool SecondBufferScan(const base::Buffer& buf) override;

   };
}


#endif
