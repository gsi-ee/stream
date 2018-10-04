#ifndef HADAQ_SUBPROCESSOR_H
#define HADAQ_SUBPROCESSOR_H

#include "base/StreamProc.h"

#include "hadaq/definess.h"

#include <map>


namespace hadaq {

   class SubProcessor;
   class TrbProcessor;

   typedef std::map<unsigned,SubProcessor*> SubProcMap;

   /** This is specialized sub-processor for ADC addon.
    * Normally it should be used together with TrbProcessor,
    * which the only can provide data
    **/

   class SubProcessor : public base::StreamProc {

      friend class TrbProcessor;

      protected:

         TrbProcessor *fTrb{nullptr};//! pointer on TRB processor
         unsigned fSeqeunceId{0};    //! sequence number of processor in TRB
         bool fIsTDC{false};         //! indicate when it is TDC, to avoid dynamic_cast

         base::H1handle *fMsgPerBrd{nullptr};  //! messages per board - from TRB
         base::H1handle *fErrPerBrd{nullptr};  //! errors per board - from TRB
         base::H1handle *fHitsPerBrd{nullptr}; //! data hits per board - from TRB

         bool      fNewDataFlag{false};       //! flag used by TRB processor to indicate if new data was added
         bool      fPrintRawData{false};      //! if true, raw data will be printed
         bool      fCrossProcess{false};      //! if true, AfterFill will be called by Trb processor

         SubProcessor(TrbProcessor *trb, const char* nameprefix, unsigned subid);

         /** These methods used to fill different raw histograms during first scan */
         virtual void BeforeFill() {}
         virtual void AfterFill(SubProcMap* = nullptr) {}

         /** Method will be called by TRB processor if SYNC message was found
          * One should change 4 first bytes in the last buffer in the queue */
         virtual void AppendTrbSync(uint32_t) {}

         /** Methods used by TrbProcessor */
         void SetNewDataFlag(bool on) { fNewDataFlag = on; }
         bool IsNewDataFlag() const { return fNewDataFlag; }

         void AssignPerBrdHistos(TrbProcessor *trb, unsigned seqid);

      public:

         virtual ~SubProcessor() {}

         virtual void UserPreLoop();

         inline bool IsTDC() const { return fIsTDC; }

         void SetPrintRawData(bool on = true) { fPrintRawData = on; }
         bool IsPrintRawData() const { return fPrintRawData; }

   };

}

#endif
