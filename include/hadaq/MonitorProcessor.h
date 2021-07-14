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

         unsigned fMonitorProcess;

         std::vector<hadaq::MessageMonitor>  fDummyVect; //! dummy empty vector
         std::vector<hadaq::MessageMonitor> *pStoreVect; //! pointer on store vector

         /** Scan FPGA-TDC data, distribute over sub-processors */
         virtual void ScanSubEvent(hadaqs::RawSubevent* sub, unsigned trb3runid, unsigned trb3seqid);

         virtual void CreateBranch(TTree* t);

      public:

         MonitorProcessor(unsigned brdid = 0, HldProcessor *hld = nullptr, int hfill = -1);
         virtual ~MonitorProcessor() {}

         void SetNWords(unsigned nwords = 3) { fMonitorProcess = nwords; }
         unsigned GetNWords() const { return fMonitorProcess; }

         virtual void Store(base::Event*);
         virtual void ResetStore();
   };
}

#endif
