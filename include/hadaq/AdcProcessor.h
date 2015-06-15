#ifndef HADAQ_ADCPROCESSOR_H
#define HADAQ_ADCPROCESSOR_H

#include "hadaq/SubProcessor.h"
#include "hadaq/TdcProcessor.h"

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

class AdcProcessor : public TdcProcessor {
   
   friend class TrbProcessor;
   
protected:
   
   struct ChannelRec {
   public:
      ChannelRec() :
         fValues(0),
         fWaveform(0),
         fIntegral(0),
         fSamples(0),
         fCoarseTiming(0),
         fFineTiming(0),
         fDiffTiming(0),
         fPhaseVsFrac(0),
         fBeforeVsFrac(0),         
         fAfterVsFrac(0),                  
         fDiffCh(-1),
         fTiming(std::numeric_limits<double>::quiet_NaN())
      {}
      base::H1handle fValues;          //! histogram of values distribution in channel
      base::H2handle fWaveform;        //! histogram of integrated raw waveform of channel (debug)
      base::H1handle fIntegral;        //! histogram of integrals from CFD feature extraction
      base::H2handle fSamples;         //! histogram of fine timings from CFD feature extraction
      base::H1handle fCoarseTiming;    //! ADC samples since trigger detected
      base::H1handle fFineTiming;      //! histogram of timing of single channel to trigger (received by TDC)     
      base::H1handle fDiffTiming;      //! histogram of CFD fine time difference to other specified channel
      base::H2handle fPhaseVsFrac;
      base::H2handle fBeforeVsFrac;
      base::H2handle fAfterVsFrac;      
      int fDiffCh;                     //! if not <0, specifies channel for fDiffTiming
      double fTiming;                  //! the timing in ns, relative to trigger
   };
   
   const double fSamplingPeriod;
   
   static std::vector<double> storage;
   
   
   base::H1handle fKinds;        //! kinds of messages
   base::H1handle fChannels;     //! histogram with messages per channel
   base::H1handle fADCPhase;     //! histogram with ADC clock phase to trigger measured by TDC
   std::vector<ChannelRec>  fCh; //! histogram for individual channels
   
   std::vector<hadaq::AdcMessage>   fStoreVect; //! dummy empty vector
   std::vector<hadaq::AdcMessage>  *pStoreVect; //! pointer on store vector
   
   virtual void CreateBranch(TTree*);
   
public:
   
   AdcProcessor(TrbProcessor* trb, unsigned subid, unsigned numchannels = 48, 
                double samplingPeriod = 1000.0/80);
   virtual ~AdcProcessor();
   
   inline unsigned NumChannels() const { return fCh.size(); }
   void SetDiffChannel(unsigned ch, int diffch);
   
   /** Scan all messages, find reference signals
          * if returned false, buffer has error and must be discarded */
   virtual bool FirstBufferScan(const base::Buffer& buf);
   
   /** Scan buffer for selecting messages inside trigger window */
   virtual bool SecondBufferScan(const base::Buffer& buf);
   
   virtual void Store(base::Event*);
};

}

#endif
