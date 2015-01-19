#ifndef HADAQ_HLDPROCESSOR_H
#define HADAQ_HLDPROCESSOR_H

#include "base/StreamProc.h"

#include "hadaq/definess.h"

#include "hadaq/TrbProcessor.h"

namespace hadaq {


   /** This is generic processor for HLD data
    * Its only task - redistribute data over TRB processors  */

   typedef std::map<unsigned,TrbProcessor*> TrbProcMap;


   class HldProcessor : public base::StreamProc {

      friend class TrbProcessor;

      protected:

         TrbProcMap fMap;            ///< map of trb processors

         unsigned  fEventTypeSelect; ///< selection for event type (lower 4 bits in event id)

         bool fPrintRawData;         ///< true when raw data should be printed

         base::H1handle fEvType;     ///< HADAQ event type
         base::H1handle fEvSize;     ///< HADAQ event size
         base::H1handle fSubevSize;  ///< HADAQ subevent size

         /** Returns true when processor used to select trigger signal
          * TRB3 not yet able to perform trigger selection */
         virtual bool doTriggerSelection() const { return false; }

         /** Way to register trb processor */
         void AddTrb(TrbProcessor* trb, unsigned id);


      public:

         HldProcessor();
         virtual ~HldProcessor();

         /** Search for specified TDC in all subprocessors */
         TdcProcessor* FindTDC(unsigned tdcid) const;

         /** Set event type, only used in the analysis */
         void SetEventTypeSelect(unsigned evid) { fEventTypeSelect = evid; }

         /** Set trigger window not only for itself, bit for all subprocessors */
         virtual void SetTriggerWindow(double left, double right);

         /** Enable/disable store for HLD and all TRB processors */
         virtual void SetStoreEnabled(bool on = true);

         /** Scan all messages, find reference signals */
         virtual bool FirstBufferScan(const base::Buffer& buf);

         void SetPrintRawData(bool on = true);
         bool IsPrintRawData() const { return fPrintRawData; }


   };
}

#endif
