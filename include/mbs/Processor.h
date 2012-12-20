#ifndef MBS_PROCESSOR_H
#define MBS_PROCESSOR_H

#include "base/StreamProc.h"


namespace mbs {

   /** This could be generic processor for data, coming from MBS
    * For the moment it is rather CERN beamtime-specific code  */

   class Processor : public base::StreamProc {

      protected:

         unsigned fLastSync1; //! last sync id in first scan
         unsigned fLastSync2; //! last sync id in second scan

         /** Returns true when processor used to select trigger signal
          * TRB3 not yet able to perform trigger selection */
         virtual bool doTriggerSelection() const { return false; }

         /** This is maximum disorder time for MBS
          * TODO: derive this value from sub-items */
         virtual double MaximumDisorderTm() const { return 1000.; }




      public:

         Processor();

         virtual ~Processor();

         /** Scan all messages, find reference signals */
         virtual bool FirstBufferScan(const base::Buffer& buf);

         virtual bool SecondBufferScan(const base::Buffer& buf);

   };
}


#endif
