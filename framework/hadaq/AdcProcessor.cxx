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
      fCh.push_back(ChannelRec());
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
   for (unsigned n=0;n<len;n++) {
      AdcMessage msg(arr[n]);

      FillH1(fKinds, msg.getKind());

      switch (msg.getKind()) {
         case 0: FillH1(fChannels, msg.getCh()); break;
         case 1: break;
         default: break;
      }
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

