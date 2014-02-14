#include <stdio.h>

#include "TTree.h"

#include "base/EventProc.h"
#include "base/Event.h"
#include "hadaq/TdcSubEvent.h"

class TdcDataProc : public base::EventProc {
   protected:
      
      std::string fTdcId;    //!< tdc id with padiwa asic like "TDC_8a00"
      double      fNumHits;  //!< number of hits, can be stored in the tree
      
      base::H1handle  hNumHits; //!< histogram with hits number
      
   public:
      TdcDataProc(unsigned procid, const char* _tdcid) :
         base::EventProc("Proc_%04x", procid),
         fTdcId(_tdcid),
         fNumHits(0)

      {
         printf("Create %s for %s\n", GetName(), fTdcId.c_str());

         hNumHits = MakeH1("NumHits","Number of hits", 100, 0, 100, "number");

         // enable storing already in constructor
         SetStoreEnabled();
      }


      virtual void CreateBranch(TTree* t)
      {
         t->Branch(GetName(), &fNumHits, "numhits/D");
      }

      virtual bool Process(base::Event* ev) 
      {
         fNumHits = 0;

         hadaq::TdcSubEvent* sub = 
               dynamic_cast<hadaq::TdcSubEvent*> (ev->GetSubEvent(fTdcId));

         printf("%s process sub %p %s\n", GetName(), sub, fTdcId.c_str());

         if (sub==0) return false;
         
         fNumHits = sub->Size();

         for (unsigned cnt=0;cnt<sub->Size();cnt++) {
            const hadaq::TdcMessageExt& ext = sub->msg(cnt);
            
            unsigned chid = ext.msg().getHitChannel();
            double tm = ext.GetGlobalTime() /* - ev->GetTriggerTime() */;

            // exclude all channels which has nothing to do with Padiwa
            printf("   HIT ch %u time %12.9f\n", chid, tm);
         }
         
         FillH1(hNumHits, fNumHits);
         
         return true;
      }
};


void second() 
{
   // uncomment line to create tree for the events storage
   base::ProcMgr::instance()->CreateStore("file.root");
   
   TdcDataProc* proc1 = new TdcDataProc(0xc10, "TDC_0c10");
   
   TdcDataProc* proc2 = new TdcDataProc(0xc30, "TDC_0c30");
}
