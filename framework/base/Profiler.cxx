// $Id$

#include "base/Profiler.h"

void base::Profiler::MakeStatistic()
{

   clock_t now = GetClock();

   clock_t diff = fLast ? now - fLast : 0;

   if (diff) {
      for (auto &entry : fEntries) {
         entry.fRatio = 1.*entry.fSum/diff;
         entry.fSum = 0;
      }
   }

   fLast = now;
}


std::string base::Profiler::Format()
{
   std::string res;

   double total{0.};
   int cnt{0};

   for (auto &entry : fEntries) {
      if (entry.fRatio > 0) {
         if (!res.empty()) res.append(" ");
         total += entry.fRatio;
         if (entry.fName.empty())
            res.append(std::to_string(cnt));
         else
            res.append(entry.fName);
         res.append(":");
         res.append(std::to_string((int)(entry.fRatio*1000)));
      }
      cnt++;
   }

   if (total){
      res.append(" total:");
      res.append(std::to_string((int)(total*1000)));
   }

   return res;
}
