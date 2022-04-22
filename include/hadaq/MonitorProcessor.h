#ifndef HADAQ_MONITORROCESSOR_H
#define HADAQ_MONITORROCESSOR_H

#include "hadaq/TrbProcessor.h"
#include "hadaq/MonitorSubEvent.h"

#include <vector>

namespace hadaq {

   /** \brief Processor of monitored data
     *
     * \ingroup stream_hadaq_classes
     *
     * This is specialized processor for data produced with hadaq::MonitorModule
     * Normally one requires specific sub-processor for frontend like TDC or any other
     * Idea that TrbProcessor can interpret HADAQ event/subevent structures and
     * will distribute data to sub-processors.  */

   class MonitorProcessor : public TrbProcessor {

      protected:

         unsigned fMonitorProcess{0};                    ///< counter

         std::vector<hadaq::MessageMonitor>  fDummyVect; ///<! dummy empty vector
         std::vector<hadaq::MessageMonitor> *pStoreVect{nullptr}; ///<! pointer on store vector

         void ScanSubEvent(hadaqs::RawSubevent* sub, unsigned trb3runid, unsigned trb3seqid) override;

         void CreateBranch(TTree* t) override;

      public:

         MonitorProcessor(unsigned brdid = 0, HldProcessor *hld = nullptr, int hfill = -1);
         virtual ~MonitorProcessor() {}

         /** set number of words */
         void SetNWords(unsigned nwords = 3) { fMonitorProcess = nwords; }
         /** get number of words */
         unsigned GetNWords() const { return fMonitorProcess; }

         void Store(base::Event*) override;
         void ResetStore() override;
   };
}

#endif
