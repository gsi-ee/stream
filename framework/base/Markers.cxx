#include "base/Markers.h"

#include <stdio.h>
#include <stdlib.h>

void base::GlobalTriggerMarker::SetInterval(double left, double right)
{
   if (left>right) {
      printf("left > right in time interval - failure\n");
      exit(7);
   }

   lefttm = globaltm + left;
   righttm = globaltm + right;
}

int base::GlobalTriggerMarker::TestHitTime(const GlobalTime_t& hittime, double* dist)
{
   // be aware that condition like [left, right) is tested
   // therefore if left==right, hit will never be assigned to such condition

   if (dist) *dist = 0.;
   if (hittime < lefttm) {
      if (dist) *dist = hittime - lefttm;
      return -1;
   } else
   if (hittime >= righttm) {
      if (dist) *dist = hittime - righttm;
      return 1;
   }
   return 0;
}


