#ifndef HADAQ_ADCPROCESSOR_H
#define HADAQ_ADCPROCESSOR_H

#include "hadaq/SubProcessor.h"

#include "hadaq/AdcMessage.h"
#include "hadaq/AdcSubEvent.h"

namespace hadaq {

   class TrbProcessor;

   /** This is specialized sub-processor for ADC addon.
    * Normally it should be used together with TrbProcessor,
    * which the only can provide data
    **/

   class AdcProcessor : public SubProcessor {

      friend class TrbProcessor;

      protected:

         struct ChannelRec {
            base::H1handle fValues;          //! histogram of values distribution in channel
            base::H2handle fWaveform;        //! histogram of integrated raw waveform of channel (debug)
            base::H1handle fCFDIntegral;     //! histogram of integrals from CFD feature extraction
            base::H1handle fCFDCoarseTiming; //! histogram of coarse timings from CFD feature extraction
            base::H1handle fCFDFineTiming;   //! histogram of fine timings from CFD feature extraction
            ChannelRec() :
               fValues(0),
               fWaveform(0)
            {}
         };


         base::H1handle fKinds;      //! kinds of messages
         base::H1handle fChannels;   //! histogram with messages per channel
         std::vector<ChannelRec>  fCh; //! histogram for individual channels

         std::vector<hadaq::AdcMessage>   fStoreVect; //! dummy empty vector
         std::vector<hadaq::AdcMessage>  *pStoreVect; //! pointer on store vector

         virtual void CreateBranch(TTree*);

      public:

         AdcProcessor(TrbProcessor* trb, unsigned subid, unsigned numchannels = 48);
         virtual ~AdcProcessor();

         inline unsigned NumChannels() const { return fCh.size(); }

         /** Scan all messages, find reference signals
          * if returned false, buffer has error and must be discarded */
         virtual bool FirstBufferScan(const base::Buffer& buf);

         /** Scan buffer for selecting messages inside trigger window */
         virtual bool SecondBufferScan(const base::Buffer& buf);

         virtual void Store(base::Event*);
   };

}

#endif
