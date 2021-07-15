#include <cstdio>

#include "TTree.h"

#include "base/EventProc.h"
#include "base/Event.h"
#include "hadaq/MonitorSubEvent.h"

const int NumChannels = 16;

class DebugProc : public base::EventProc {
   protected:

      std::string fSubId;    ///< if which is tested

      double fHits[NumChannels];

      base::H1handle  hNumHits; ///< histogram with hits number

   public:
      DebugProc(const char* procname, const char* _subid) :
         base::EventProc(procname),
         fSubId(_subid)
      {
         printf("Create %s for %s\n", GetName(), fSubId.c_str());

         hNumHits = MakeH1("NumHits","Number of hits", 100, 0, 500, "number");

         // enable storing already in constructor
         SetStoreEnabled();
      }


      virtual void CreateBranch(TTree* t)
      {
         t->Branch(GetName(), fHits, "hits[16]/D");
      }

      virtual bool Process(base::Event* ev)
      {
         for (unsigned n=0;n<NumChannels;n++) fHits[n] = 0.;

         hadaq::MonitorSubEvent* sub =
               dynamic_cast<hadaq::MonitorSubEvent*> (ev->GetSubEvent(fSubId));

         // keep loop running, but it is not that we needed
         if (!sub) return true;

         for (unsigned cnt=0;cnt<sub->Size();cnt++) {

            const hadaq::MessageMonitor& msg = sub->msg(cnt);

            // printf("Data: %04x %04x = %08x\n", msg.addr0,  msg.addr, msg.value);
         }

         printf("%s process sub %s size %u\n", GetName(), fSubId.c_str(), sub->Size());

         FillH1(hNumHits, sub->Size());

         return true;
      }
};


void second()
{
   // uncomment line to create tree for the events storage
   // base::ProcMgr::instance()->CreateStore("file.root");

   new DebugProc("Debug1", "TRB_AAAA");

}
