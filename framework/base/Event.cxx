#include "base/Event.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

base::SubEvent* base::Event::GetSubEvent(const std::string& name, unsigned subindx) const
{
   char sbuf[200];

   if (name.size()>sizeof(sbuf)-10) {
      printf("Fix - size problem\n");
      exit(77);
   }

   snprintf(sbuf, sizeof(sbuf), "%s%u", name.c_str(), subindx);

   return GetSubEvent(sbuf);
}
