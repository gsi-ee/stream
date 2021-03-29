#include <cstdio>

#include "TTree.h"

#include "base/EventProc.h"
#include "base/Event.h"
#include "hadaq/TdcSubEvent.h"

const int NumChannels = 33;

class DebugProc : public base::EventProc {
   protected:

      std::string fTdcId;    //!< tdc id where channels will be selected "TDC_8a00"

      double      fHits[NumChannels]; //!< 16 channel, last hit in every channel

      base::H1handle  hNumHits; //!< histogram with hits number

      base::H1handle  hRefHist; //!< histogram with hits number

   public:
      DebugProc(const char* procname, const char* _tdcid) :
         base::EventProc(procname),
         fTdcId(_tdcid)
      {
         printf("Create %s for %s\n", GetName(), fTdcId.c_str());

         hNumHits = MakeH1("NumHits","Number of hits", 100, 0, 100, "number");

         hRefHist = MakeH1("RefHist", "Difference between channels", 1000, -5, 5, "ns");

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

         hadaq::TdcSubEventFloat* sub =
               dynamic_cast<hadaq::TdcSubEventFloat*> (ev->GetSubEvent(fTdcId));

         if (!sub) {
            printf("Not found subevent %s\n", fTdcId.c_str());
            return false;
         }

         // printf("%s process sub %p %s\n", GetName(), sub, fTdcId.c_str());

         double num = 0;

         for (unsigned cnt=0;cnt<sub->Size();cnt++) {
            const hadaq::MessageFloat& msg = sub->msg(cnt);

            float tm = msg.getStamp();
            unsigned chid = msg.getCh();
            bool isrising = msg.isRising();

            if ((chid < NumChannels) && isrising)
               fHits[chid] = tm;

            num++;
         }

         FillH1(hNumHits, num);

         if (fHits[14] && fHits[12])
            FillH1(hRefHist, fHits[14] - fHits[12]);

         return true;
      }
};


void second()
{
   // uncomment line to create tree for the events storage
   // base::ProcMgr::instance()->CreateStore("file.root");

   new DebugProc("Debug1", "TDC_16F7");

}
