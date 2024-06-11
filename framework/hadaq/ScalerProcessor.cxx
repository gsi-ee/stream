#include "hadaq/ScalerProcessor.h"


hadaq::ScalerProcessor::ScalerProcessor(TrbProcessor* trb, unsigned subid) :
   hadaq::SubProcessor(trb, "SCALER_%04x", subid)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Create TTree branch

void hadaq::ScalerProcessor::CreateBranch(TTree*)
{
   if(GetStoreKind() > 0) {
      pStore = &fDummy;
      mgr()->CreateBranch(GetName(), "hadaq::ScalerSubEvent", (void**) &pStore);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Store event

void hadaq::ScalerProcessor::Store(base::Event* ev)
{
   // always set to dummy
   pStore = &fDummy;

   // in case of triggered analysis all pointers already set
   if (!ev /*|| IsTriggeredAnalysis()*/) return;

   auto sub0 = ev->GetSubEvent(GetName());
   if (!sub0) return;

   if (GetStoreKind() > 0) {
      auto sub = dynamic_cast<hadaq::ScalerSubEvent *> (sub0);
      // when subevent exists, use directly pointer on messages vector
      if (sub) {
         pStore = sub;
      }
   }
}


bool hadaq::ScalerProcessor::FirstBufferScan(const base::Buffer &buf)
{
   uint32_t *arr = (uint32_t *)buf.ptr();
   unsigned len = buf.datalen() / 4;

   if (len != 4) {
      fprintf(stderr, "%s has wrong data length %u, expected 5\n", GetName(), len);
      return false;
   }

   if ((arr[0] >> 16 != 0xaaa) || (arr[2] >> 16 != 0x1bbb)) {
      fprintf(stderr, "%s has wrong scaler headers 0x%x 0x%x\n", GetName(), arr[0] >> 16, arr[2] >> 16);
      return false;
   }

   uint64_t scaler1 = ((uint64_t) arr[0] & 0xFFFF) << 32 | arr[1];
   uint64_t scaler2 = ((uint64_t) arr[2] & 0xFFFF) << 32 | arr[3];

   if (IsPrintRawData())
      printf("%s scalers %lu %lu\n", GetName(), (long unsigned) scaler1, (long unsigned) scaler2);

   if (IsTriggeredAnalysis() && IsStoreEnabled() && (GetStoreKind() > 0)) {
      auto subevnt = new hadaq::ScalerSubEvent(scaler1, scaler2);
      mgr()->AddToTrigEvent(GetName(), subevnt);
      pStore = subevnt;
   }

   return true;
}
