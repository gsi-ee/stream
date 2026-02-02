 #ifndef HADAQ_SCALERPROCESSOR_H
#define HADAQ_SCALERPROCESSOR_H

#include "hadaq/SubProcessor.h"

#include "hadaq/ScalerSubEvent.h"

namespace hadaq {

class TrbProcessor;

  /** This is specialized sub-processor for scaler data.
    * It should be used together with TrbProcessor,
    * which the only can provide data  **/

class ScalerProcessor : public SubProcessor {

   friend class TrbProcessor;

protected:

   void CreateBranch(TTree *) override;
   void Store(base::Event* ev) override;

   ScalerSubEvent fDummy;              ///<! data
   ScalerSubEvent *pStore = nullptr;  ///<! pointer on store vector

   bool AddCTSData(hadaqs::RawSubevent* sub, unsigned ix, unsigned datalen);

public:

   ScalerProcessor(TrbProcessor *trb, unsigned subid);
   ~ScalerProcessor() override {}

   /** Scan all messages, find reference signals
     * if returned false, buffer has error and must be discarded */
   bool FirstBufferScan(const base::Buffer&) override;
};

} // namespace hadaq

#endif
