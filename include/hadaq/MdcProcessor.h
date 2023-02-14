#ifndef MdcProcessor_h
#define MdcProcessor_h

#include "hadaq/SubProcessor.h"

#define TDCCHANNELS 32

namespace hadaq {

class TrbProcessor;

  /** This is specialized sub-processor for custom data.
    * It should be used together with TrbProcessor,
    * which the only can provide data
    **/

class MdcProcessor : public SubProcessor {

   friend class TrbProcessor;

protected:

   base::H2handle HitsPerBinRising;
   base::H2handle HitsPerBinFalling;
   base::H2handle ToT;
   base::H2handle deltaT;
   base::H1handle Errors;
   base::H1handle alldeltaT;
   base::H1handle allToT;
   base::H2handle deltaT_ToT;
   base::H1handle  tot_h[TDCCHANNELS+1];
   base::H1handle  t1_h[TDCCHANNELS+1];
   base::H1handle  potato_h[TDCCHANNELS+1];

public:

   MdcProcessor(TrbProcessor *trb, unsigned subid);
   virtual ~MdcProcessor() {}

   /** Scan all messages, find reference signals
     * if returned false, buffer has error and must be discarded */
   bool FirstBufferScan(const base::Buffer&) override;
};

} // namespace hadaq

#endif
