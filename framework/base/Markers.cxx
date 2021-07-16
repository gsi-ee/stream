#include "base/Markers.h"

#include <cstdio>
#include <cstdlib>

///////////////////////////////////////////////////////////////////////////
/// set interval

void base::GlobalMarker::SetInterval(double left, double right)
{
   if (left>right) {
      printf("left > right in time interval - failure\n");
      exit(7);
   }

   lefttm = globaltm + left;
   righttm = globaltm + right;
}

///////////////////////////////////////////////////////////////////////////
/// test hit time
///
/// be aware that condition like [left, right) is tested
/// therefore if left==right, hit will never be assigned to such condition

int base::GlobalMarker::TestHitTime(const GlobalTime_t& hittime, double* dist)
{

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


