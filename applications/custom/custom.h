#ifndef CustomProcessor_h
#define CustomProcessor_h

#include "hadaq/SubProcessor.h"


namespace hadaq {

class TrbProcessor;

  /** This is specialized sub-processor for custom data.
    * It should be used together with TrbProcessor,
    * which the only can provide data
    **/

class CustomProcessor : public SubProcessor {

   friend class TrbProcessor;

protected:

   unsigned fNumChannels;
   base::H1handle fChannels;     ///<! histogram with messages per channel

public:

   CustomProcessor(TrbProcessor* trb, unsigned subid, unsigned numchannels = 33);
   virtual ~CustomProcessor() {}

   unsigned NumChannels() const { return fNumChannels; }

   /** Scan all messages, find reference signals
     * if returned false, buffer has error and must be discarded */
   virtual bool FirstBufferScan(const base::Buffer&);

   /** Scan buffer for selecting messages inside trigger window */
   virtual bool SecondBufferScan(const base::Buffer&) { return true; }
};

}



hadaq::CustomProcessor::CustomProcessor(TrbProcessor* trb, unsigned subid, unsigned numchannels) :
   hadaq::SubProcessor(trb, "CUST_%04X", subid),
   fNumChannels(numchannels)
{
   fChannels = MakeH1("CustomChannels", "Messages per channels", numchannels, 0, numchannels, "ch");
}

bool hadaq::CustomProcessor::FirstBufferScan(const base::Buffer &buf)
{
   unsigned len = buf.datalen()/4;
   uint32_t* arr = (uint32_t*) buf.ptr();

   // printf("Process sub data len %u\n", len);

   // process subsubeven
   for (unsigned n=0;n<len;n++) {

      uint32_t data = arr[n];

      // from here dummy code, just to fill histogram
      unsigned chid = data & 0xff;
      if (chid >= fNumChannels)
         chid = chid % fNumChannels;
      FillH1(fChannels, chid);
   }

   return true;
}


#endif
