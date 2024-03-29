#ifndef BASE_OPTICSPLITTER_H
#define BASE_OPTICSPLITTER_H

#include "base/SysCoreProc.h"

#include "base/defines.h"

#include <map>

namespace base {

   /** This is splitter of raw data, recorded by the optic readout
    * Current ABB firmware mix data from all ROCs together, therefore
    * OpticSplitter required to resort raw data per buffer.  */

   class OpticSplitter : public base::StreamProc {

      friend class SysCoreProc;

      /** map of processors */
      typedef std::map<unsigned,base::SysCoreProc*> SysCoreMap;


      protected:

         SysCoreMap fMap;   ///< map of processors

         /** Returns true when processor used to select trigger signal
          * TRB3 not yet able to perform trigger selection */
         bool doTriggerSelection() const override  { return false; }

         void AddSub(SysCoreProc* tdc, unsigned id);

      public:

         OpticSplitter(unsigned brdid = 0xff);
         virtual ~OpticSplitter();

         bool FirstBufferScan(const base::Buffer& buf) override ;

   };
}

#endif
