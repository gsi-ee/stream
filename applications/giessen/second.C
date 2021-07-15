#include <cstdio>

#include "TTree.h"

#include "base/EventProc.h"
#include "base/Event.h"
#include "hadaq/TdcSubEvent.h"

class RawPandaDircProc : public base::EventProc {
   protected:

      std::string fTdcId;    ///< tdc id where channels will be selected "TDC_8a00"
      unsigned    fFirstId;  ///< first channel

      double      fHits[16]; ///< 16 channel, last hit in every channel

      base::H1handle  hNumHits; ///< histogram with hits number

   public:
      RawPandaDircProc(const char* procname, const char* _tdcid, unsigned _firstid = 1) :
         base::EventProc(procname),
         fTdcId(_tdcid),
         fFirstId(_firstid),
         hNumHits(0)
      {
         printf("Create %s for %s\n", GetName(), fTdcId.c_str());

         hNumHits = MakeH1("NumHits","Number of hits", 100, 0, 100, "number");

         // enable storing already in constructor
         SetStoreEnabled();
      }


      virtual void CreateBranch(TTree* t)
      {
         t->Branch(GetName(), fHits, "hits[16]/D");
      }

      virtual bool Process(base::Event* ev)
      {
         for (unsigned n=0;n<16;n++) fHits[n] = 0.;

         hadaq::TdcSubEvent* sub =
               dynamic_cast<hadaq::TdcSubEvent*> (ev->GetSubEvent(fTdcId));

         // printf("%s process sub %p %s\n", GetName(), sub, fTdcId.c_str());

         if (sub==0) return false;

         double num = 0;

         for (unsigned cnt=0;cnt<sub->Size();cnt++) {
            const hadaq::TdcMessageExt& ext = sub->msg(cnt);
            unsigned chid = ext.msg().getHitChannel();
            double tm = ext.GetGlobalTime() /* - ev->GetTriggerTime() */;

            if ((chid>=fFirstId) && (chid<fFirstId+16)) {

               // account only new channels
               if (fHits[chid - fFirstId] == 0.) num += 1.;

               fHits[chid - fFirstId] = tm;

               // exclude all channels which has nothing to do with Padiwa
               // printf("   HIT ch %u time %12.9f\n", chid, tm);
            }

         }

         FillH1(hNumHits, num);

         return true;
      }
};


void second()
{
   // uncomment line to create tree for the events storage
   base::ProcMgr::instance()->CreateStore("file.root");

   new RawPandaDircProc("DIRC1", "TDC_0c10", 1);

   new RawPandaDircProc("DIRC2", "TDC_0c10", 17);

   new RawPandaDircProc("DIRC3", "TDC_0c10", 33);

   new RawPandaDircProc("DIRC4", "TDC_0c10", 39);

   new RawPandaDircProc("DIRC5", "TDC_0c30", 1);
}
