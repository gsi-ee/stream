#ifndef HADAQ_MDCPROCESSOR_H
#define HADAQ_MDCPROCESSOR_H

#include "hadaq/SubProcessor.h"

#include "hadaq/MdcSubEvent.h"

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
   base::H2handle T0_T1;
   base::H1handle  tot_h[TDCCHANNELS+1];
   base::H1handle  t1_h[TDCCHANNELS+1];
   base::H1handle  potato_h[TDCCHANNELS+1];

   std::vector<hadaq::MdcMessage> fDummyFloat;  ///<! vector with messages
   std::vector<hadaq::MdcMessage> *pStoreFloat{nullptr}; ///<! pointer on store vector

   void CreateBranch(TTree *) override;
   void Store(base::Event* ev) override;

public:

   MdcProcessor(TrbProcessor *trb, unsigned subid);
   virtual ~MdcProcessor() {}

   /** Scan all messages, find reference signals
     * if returned false, buffer has error and must be discarded */
   bool FirstBufferScan(const base::Buffer&) override;
};

} // namespace hadaq

#endif
