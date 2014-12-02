#include "hadaq/AdcProcessor.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbProcessor.h"
#include "hadaq/TdcSubEvent.h"

#ifdef WITH_ROOT
#include "TTree.h"
#endif


#define RAWPRINT( args ...) if(IsPrintRawData()) printf( args )

hadaq::AdcProcessor::AdcProcessor(TrbProcessor* trb, unsigned subid, unsigned numchannels) :
   SubProcessor(trb, "ADC_%04x", subid)
{
   fChannels = 0;

   if (HistFillLevel() > 1) {
      fChannels = MakeH1("Channels", "Messages per ADC channels", numchannels, 0, numchannels, "ch");
   }

   for (unsigned ch=0;ch<numchannels;ch++) {
      fCh.push_back(ChannelRec());
   }
}


hadaq::AdcProcessor::~AdcProcessor()
{
}


