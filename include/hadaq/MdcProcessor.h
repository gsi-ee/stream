#ifndef HADAQ_MDCPROCESSOR_H
#define HADAQ_MDCPROCESSOR_H

#include "hadaq/SubProcessor.h"

#include "hadaq/MdcSubEvent.h"

namespace hadaq {

class TrbProcessor;

  /** This is specialized sub-processor for custom data.
    * It should be used together with TrbProcessor,
    * which the only can provide data
    **/

class MdcProcessor : public SubProcessor {

   friend class TrbProcessor;

public:

  enum { TDCCHANNELS = 32 };

protected:

   base::H2handle HitsPerBinRising = nullptr;
   base::H2handle HitsPerBinFalling = nullptr;
   base::H2handle ToT = nullptr;
   base::H2handle deltaT = nullptr;
   base::H1handle Errors = nullptr;
   base::H1handle alldeltaT = nullptr;
   base::H1handle allToT = nullptr;
   base::H2handle deltaT_ToT = nullptr;
   base::H2handle T0_T1 = nullptr;
   base::H1handle tot_h[TDCCHANNELS+1];
   base::H1handle t1_h[TDCCHANNELS+1];
   base::H1handle potato_h[TDCCHANNELS+1];

   unsigned fHldId = 0;
   base::H1handle *hHitsHld = nullptr;
   base::H1handle *hErrsHld = nullptr;
   base::H2handle *hChToTHld = nullptr;
   base::H2handle *hChDeltaTHld = nullptr;

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

   void AssignPerHldHistos(unsigned id, base::H1handle *hHits, base::H1handle *hErrs,
                                        base::H2handle *hChToT, base::H2handle *hChDeltaT)
   {
      fHldId = id;
      hHitsHld = hHits;
      hErrsHld = hErrs;
      hChToTHld = hChToT;
      hChDeltaTHld = hChDeltaT;
   }

};

} // namespace hadaq

#endif
