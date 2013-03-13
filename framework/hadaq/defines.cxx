#include "hadaq/defines.h"

#include <stdio.h>

void hadaq::RawEvent::Dump()
{
   printf("*** Event #0x%06x fullid=0x%04x size %u *** \n",
         (unsigned) GetSeqNr(), (unsigned) GetId(), (unsigned) GetSize());
}


void hadaq::RawSubevent::Dump(bool print_raw_data)
{
   printf("   *** Subevent size %u decoding 0x%06x id 0x%04x trig 0x%08x %s align %u *** \n",
             (unsigned) GetSize(),
             (unsigned) GetDecoding(),
             (unsigned) GetId(),
             (unsigned) GetTrigNr(),
             IsSwapped() ? "swapped" : "not swapped",
             (unsigned) Alignment());

   if (!print_raw_data) return;

   bool newline = true;

   for (unsigned ix=0; ix < ((GetSize() - sizeof(RawSubevent)) / Alignment()); ix++)
   {
      if (ix % 8 == 0) printf("    "); newline = false;

      printf("  [%2u] %08x\t", ix, (unsigned) Data(ix));

      if (((ix + 1) % 8) == 0)  { printf("\n"); newline = true; }
   }

   if (!newline) printf("\n");
}
