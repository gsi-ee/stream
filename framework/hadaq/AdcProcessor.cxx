#include "hadaq/AdcProcessor.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbProcessor.h"

#ifdef WITH_ROOT
#include "TTree.h"
#endif

hadaq::AdcProcessor::AdcProcessor(TrbProcessor* trb, unsigned subid, unsigned numchannels) :
   SubProcessor(trb, "ADC_%04x", subid),
   fStoreVect(),
   pStoreVect(0)
{
   fChannels = 0;

   if (HistFillLevel() > 1) {
      fKinds = MakeH1("Kinds", "Messages kinds", 16, 0, 16, "kinds");
      fChannels = MakeH1("Channels", "Messages per channels", numchannels, 0, numchannels, "ch");
   }

   for (unsigned ch=0;ch<numchannels;ch++) {
      ChannelRec rec;
      SetSubPrefix("Ch", ch);
      rec.fValues = MakeH1("Values","Distribution of values (unsigned)", 1<<10, 0, 1<<10, "value");
      rec.fWaveform = MakeH2("Waveform", "Integrated Waveform", 512, 0, 512, 1<<10, 0, 1<<10, "sample;value");
      SetSubPrefix();
      fCh.push_back(rec);
   }
}


hadaq::AdcProcessor::~AdcProcessor()
{
}


bool hadaq::AdcProcessor::FirstBufferScan(const base::Buffer& buf)
{
   unsigned len = buf.datalen()/4;
   uint32_t* arr = (uint32_t*) buf.ptr();

   // printf("First scan len %u\n", len);

   // BeforeFill(); // optional

   // use iterator only if context is important
   uint32_t nSample = 0; // number of msg from the same ADC channel, aka Sample
   uint32_t lastCh = 0;
   for (unsigned n=0;n<len;n++) {
      AdcMessage msg(arr[n]);

      uint32_t kind = msg.getKind();
      FillH1(fKinds, kind);

      // kind==0 is verbose data word
      if(kind == 0) {
         uint32_t ch = msg.getCh();
         uint32_t value = msg.getValue();

         FillH1(fChannels, ch);
         FillH1(fCh[ch].fValues, value);

         // check if msg still belongs to same ADC channel
         // catch first msg as special case
         if(n==0 || ch != lastCh) {
            lastCh = ch;
            nSample = 0;
         }
         else {
            nSample++;
         }

         FillH2(fCh[ch].fWaveform, nSample, value);
      }
      // other kinds unsupported for now
   }

   // if (!fCrossProcess) AfterFill(); // optional


   return true;

}

bool hadaq::AdcProcessor::SecondBufferScan(const base::Buffer& buf)
{
   unsigned len = buf.datalen()/4;
   uint32_t* arr = (uint32_t*) buf.ptr();

   // printf("Second scan len %u\n", len);

   // use iterator only if context is important
   for (unsigned n=0;n<len;n++) {
      AdcMessage msg(arr[n]);

      // ignore all other kinds
      if (msg.getKind()!=1) continue;

      // we test hits, but do not allow to close events
      // this is normal procedure
      // unsigned indx = TestHitTime(0., true, false);
      unsigned indx = 0; // index 0 is event index in triggered-based analysis

      if (indx < fGlobalMarks.size()) {
         AddMessage(indx, (hadaq::AdcSubEvent*) fGlobalMarks.item(indx).subev, msg);
      }
   }

   return true;
}


void hadaq::AdcProcessor::CreateBranch(TTree* t)
{
#ifdef WITH_ROOT
   pStoreVect = &fStoreVect;
   t->Branch(GetName(), "std::vector<hadaq::AdcMessage>", &pStoreVect);
#endif
}

void hadaq::AdcProcessor::Store(base::Event* ev)
{
   fStoreVect.clear();

   hadaq::AdcSubEvent* sub =
         dynamic_cast<hadaq::AdcSubEvent*> (ev->GetSubEvent(GetName()));

   // when subevent exists, use directly pointer on messages vector
   if (sub!=0)
      pStoreVect = sub->vect_ptr();
   else
      pStoreVect = &fStoreVect;
}
