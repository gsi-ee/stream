// $Id$

// Copy of source from DABC Profiler

#ifndef BASE_PROFILER_H
#define BASE_PROFILER_H

#include <string>
#include <vector>

namespace base {

   class ProfilerGuard;

   class Profiler {

      friend class ProfilerGuard;

      typedef unsigned long long clock_t;

      bool fActive{true};

      clock_t GetClock()
      {
         unsigned low, high;
         asm volatile ("rdtsc" : "=a" (low), "=d" (high));
         return (clock_t(high) << 32) | low;
      }

      clock_t fLast{0};

      struct Entry {
         clock_t fSum{0};           // sum of used time
         double fRatio{0.};          // rel time for this slot
         std::string fName;
      };

      std::vector<Entry>  fEntries;

   public:

      Profiler() { Reserve(10); }

      void Reserve(unsigned num = 10)
      {
         while (fEntries.size() < num)
            fEntries.emplace_back();
      }

      void SetActive(bool on = true) { fActive = on; }

      void MakeStatistic();

      std::string Format();

   };

   class ProfilerGuard {
      Profiler &fProfiler;
      unsigned fCnt{0};
      Profiler::clock_t fLast{0};

   public:
      ProfilerGuard(Profiler &prof, const char *name = nullptr, unsigned lvl = 0) : fProfiler(prof), fCnt(lvl)
      {
         if (!fProfiler.fActive || (fCnt >= fProfiler.fEntries.size()))
            return;

         fLast = fProfiler.GetClock();

         if (name && fProfiler.fEntries[fCnt].fName.empty())
            fProfiler.fEntries[fCnt].fName = name;
      }

      ~ProfilerGuard()
      {
         Next();
      }

      void Next(const char *name = nullptr, unsigned lvl = 0)
      {
         if (!fProfiler.fActive || (fCnt >= fProfiler.fEntries.size()))
            return;

         auto now = fProfiler.GetClock();

         fProfiler.fEntries[fCnt].fSum += (now - fLast);

         fLast = now;

         if (lvl) fCnt = lvl; else fCnt++;

         if (name && (fCnt < fProfiler.fEntries.size()) && fProfiler.fEntries[fCnt].fName.empty())
            fProfiler.fEntries[fCnt].fName = name;
      }

   };

} // namespace dabc

#endif
