#ifndef HADAQ_ADCPROCESSOR_H
#define HADAQ_ADCPROCESSOR_H

#include "hadaq/SubProcessor.h"

#include "hadaq/AdcMessage.h"
#include "hadaq/AdcSubEvent.h"
#include <limits>
#include <iostream>
#include <vector>

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
   public:
      ChannelRec() :
         fHValues(0),
         fHWaveform(0),
         fHIntegral(0),
         fHSamples(0),
         fHCoarseTiming(0),
         fHFineTiming(0)
      {}
      base::H1handle fHValues;          //! histogram of values distribution in channel
      base::H2handle fHWaveform;        //! histogram of integrated raw waveform of channel (debug)
      base::H1handle fHIntegral;        //! histogram of integrals from CFD feature extraction
      base::H2handle fHSamples;         //! histogram of fine timings from CFD feature extraction
      base::H1handle fHCoarseTiming;    //! ADC samples since trigger detected
      base::H1handle fHFineTiming;      //! histogram of timing of single channel to trigger
   };

   const double fSamplingPeriod; // ADC sampling period in seconds

   static std::vector<double> storage;

   base::H1handle fKinds;        //! kinds of messages
   base::H1handle fChannels;     //! histogram with messages per channel
   std::vector<ChannelRec>  fCh; //! histogram for individual channels

   std::vector<hadaq::AdcMessage>   fStoreVect; //! dummy empty vector
   std::vector<hadaq::AdcMessage>  *pStoreVect; //! pointer on store vector

   virtual void CreateBranch(TTree*);

public:

   AdcProcessor(TrbProcessor* trb, unsigned subid, unsigned numchannels = 48,
                double samplingPeriod = 1000.0e-9/80);
   virtual ~AdcProcessor();

   inline unsigned NumChannels() const { return fCh.size(); }

   /** Scan all messages, find reference signals
          * if returned false, buffer has error and must be discarded */
   virtual bool FirstBufferScan(const base::Buffer& buf);

   /** Scan buffer for selecting messages inside trigger window */
   virtual bool SecondBufferScan(const base::Buffer& buf);

   virtual void Store(base::Event*);
   virtual void ResetStore();
};

}

#endif
