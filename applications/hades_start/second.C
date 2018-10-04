#include <stdio.h>

#include "TTree.h"

#include "base/EventProc.h"
#include "base/Event.h"
#include "hadaq/TdcSubEvent.h"

class RawHadesProc : public base::EventProc {
   protected:
      
      std::string fTdcId[2];    //!< tdc1 id where channels will be selected "TDC_8a00"
      double lasttm[2];
      
      base::H1handle  hDiff;  //!< histogram with time diff between two events
      base::H1handle  hRatio; //!< histogram with time ratio
      base::H2handle  hDet[2]; //!< time distribution
      base::H1handle  hDetInt[2]; //!< time distribution
      
   public:
      RawHadesProc(const char* procname, const char* tdc1, const char* tdc2) :
         base::EventProc(procname),
         hDiff(0),
         hRatio(0)
      {
         printf("Create %s for %s %s\n", GetName(), tdc1, tdc2);

         fTdcId[0] = tdc1; lasttm[0] = 0;
         fTdcId[1] = tdc2; lasttm[1] = 0;

         hDiff = MakeH1("EvntDiff","Difference between two events", 400, -20., 20., "ns");
         hRatio = MakeH1("EvntRatio","Ratio between two events", 1000, 0.9999, 1.0001, "rel");

         for (int n=0;n<2;n++) {
            hDet[n] = MakeH2(Form("Det%d", n),"Detector time to trigger", 100, -1000., 0., 32, 1, 33, "ns;ch");
            hDetInt[n] = MakeH1(Form("DetInt%d", n),"Detector time to trigger", 1000, -1000., 0., "ns");
         }
      }


      virtual void CreateBranch(TTree*)
      {
         // t->Branch(GetName(), fHits, "hits[16]/D");
      }

      virtual bool Process(base::Event* ev) 
      {
         double currtm[2];

         for (int n=0;n<2;n++) {

            currtm[n] = 0;

            hadaq::TdcSubEvent* sub =
               dynamic_cast<hadaq::TdcSubEvent*> (ev->GetSubEvent(fTdcId[n]));

            if (sub==0) continue;

            for (unsigned cnt=0;cnt<sub->Size();cnt++) {
               const hadaq::TdcMessageExt& ext = sub->msg(cnt);
            
               unsigned chid = ext.msg().getHitChannel();
               double tm = ext.GetGlobalTime() /* - ev->GetTriggerTime() */;

               if (chid==0) {
                  currtm[n] = tm;
               } else {
                  FillH2(hDet[n], tm*1e9, chid);
                  FillH1(hDetInt[n], tm*1e9);
               }
            }
         }

         
         if ((lasttm[0]!=0) && (lasttm[1]!=0) && (currtm[0]!=0) && (currtm[1]!=0)) {
            double diff = (currtm[0] - lasttm[0]) - (currtm[1] - lasttm[1]);
            double ratio = (currtm[0] - lasttm[0]) / (currtm[1] - lasttm[1]);
            FillH1(hDiff, diff*1e9);
            FillH1(hRatio, ratio);
            // printf("diff %5.3f\n", diff*1e9);
         }

         for (int n=0;n<2;n++) lasttm[n] = currtm[n];

         return true;
      }
};


void second() 
{
   // uncomment line to create tree for the events storage
   // base::ProcMgr::instance()->CreateStore("file.root");
   
   new RawHadesProc("HADES1", "TDC_5010", "TDC_5000");
   new RawHadesProc("HADES2", "TDC_5011", "TDC_5001");
   new RawHadesProc("HADES3", "TDC_5012", "TDC_5002");
   new RawHadesProc("HADES4", "TDC_5013", "TDC_5003");
   new RawHadesProc("HADES5", "TDC_5000", "TDC_5003");
   new RawHadesProc("HADES6", "TDC_5010", "TDC_5013");
}
