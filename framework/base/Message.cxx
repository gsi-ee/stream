#include "base/Message.h"

uint32_t base::Message::RawSize(int fmt)
{
   switch (fmt) {
      case formatEth1:   return 6;
      case formatOptic1: return 8;
      case formatEth2:   return 6;
      case formatOptic2: return 8;
      case formatNormal: return 8;
   }
   return 8;
}
