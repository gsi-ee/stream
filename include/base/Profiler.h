// Copy of source from DABC Profiler

#ifndef BASE_PROFILER_H
#define BASE_PROFILER_H

#include <string>
#include <vector>

namespace base {

   class ProfilerGuard;

   /** Performance profiler */

   class Profiler {

      friend class ProfilerGuard;

      typedef unsigned long long clock_t;

      bool fActive{true};

      clock_t GetClock()
      {

#if defined(STREAM_WINDOWS)
         return 0;
#elif defined(__MACH__)
         uint64_t virtual_timer_value;
         asm volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
         return virtual_timer_value;
#else
         unsigned low, high;
         asm volatile ("rdtsc" : "=a" (low), "=d" (high));
         return (clock_t(high) << 32) | low;
#endif
      }

      clock_t fLast{0};

      /** profiler entry */
      struct Entry {
         clock_t fSum{0};           ///< sum of used time
         double fRatio{0.};         ///< rel time for this slot
         std::string fName;         ///< name
      };

      std::vector<Entry>  fEntries;  ///< entries

   public:

      /** constructor */
      Profiler() { Reserve(10); }

      /** reserve */
      void Reserve(unsigned num = 10)
      {
         while (fEntries.size() < num)
            fEntries.emplace_back();
      }

      /** set active */
      void SetActive(bool on = true) { fActive = on; }

      void MakeStatistic();

      std::string Format();

   };

   /** Guard class to use \ref base::Profiler */

   class ProfilerGuard {
      Profiler &fProfiler;       ///< profiler
      unsigned fCnt{0};          ///< counter
      Profiler::clock_t fLast{0}; ///< clock

   public:
      /** constructor */
      ProfilerGuard(Profiler &prof, const char *name = nullptr, unsigned lvl = 0) : fProfiler(prof), fCnt(lvl)
      {
         if (!fProfiler.fActive || (fCnt >= fProfiler.fEntries.size()))
            return;

         fLast = fProfiler.GetClock();

         if (name && fProfiler.fEntries[fCnt].fName.empty())
            fProfiler.fEntries[fCnt].fName = name;
      }

      /** destructor */
      ~ProfilerGuard()
      {
         Next();
      }

      /** next ? */
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
