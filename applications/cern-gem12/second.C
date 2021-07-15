#include <cstdio>

#include "base/EventProc.h"
#include "base/Event.h"
#include "base/SubEvent.h"
#include "base/EventStore.h"

#include "nx/SubEvent.h"

int xGSI[2][128] = {   {8,6,4,2,9,11,13,15,8,6,4,2,9,11,13,15,8,6,4,2,9,11,13,15,8,6,4,2,9,11,13,15,8,6,4,2,9,11,13,15,8,6,4,2,9,11,13,15,8,6,4,2,9,11,13,15,8,6,4,2,9,11,13,15,
                13,15,9,11,4,2,8,6,13,15,9,11,4,2,8,6,13,15,9,11,4,2,8,6,13,15,9,11,4,2,8,6,13,15,9,11,4,2,8,6,13,15,9,11,4,2,8,6,13,15,9,11,4,2,8,6,13,15,9,11,4,2,8,6},
                {7,5,3,1,10,12,14,16,7,5,3,1,10,12,14,16,7,5,3,1,10,12,14,16,7,5,3,1,10,12,14,16,7,5,3,1,10,12,14,16,7,5,3,1,10,12,14,16,7,5,3,1,10,12,14,16,7,5,3,1,10,12,14,16,
                14,16,10,12,3,1,7,5,14,16,10,12,3,1,7,5,14,16,10,12,3,1,7,5,14,16,10,12,3,1,7,5,14,16,10,12,3,1,7,5,14,16,10,12,3,1,7,5,14,16,10,12,3,1,7,5,14,16,10,12,3,1,7,5}  };
int yGSI[2][128] = {   {16,16,16,16,16,16,16,16,15,15,15,15,15,15,15,15,14,14,14,14,14,14,14,14,13,13,13,13,13,13,13,13,12,12,12,12,12,12,12,12,11,11,11,11,11,11,11,11,
                10,10,10,10,10,10,10,10,9,9,9,9,9,9,9,9,8,8,8,8,8,8,8,8,7,7,7,7,7,7,7,7,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,2,2,2,2,2,2,2,2,
                1,1,1,1,1,1,1,1},
                {16,16,16,16,16,16,16,16,15,15,15,15,15,15,15,15,14,14,14,14,14,14,14,14,13,13,13,13,13,13,13,13,12,12,12,12,12,12,12,12,11,11,11,11,11,11,11,11,
                10,10,10,10,10,10,10,10,9,9,9,9,9,9,9,9,8,8,8,8,8,8,8,8,7,7,7,7,7,7,7,7,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,2,2,2,2,2,2,2,2,
                1,1,1,1,1,1,1,1}  };

class TGemProc : public base::EventProc {
   protected:

      base::H2handle fMappingGSI_GEM1;  ///<!
      base::H2handle fMappingGSI_GEM2;  ///<!
      base::H2handle fMappingGSI_GEM3;  ///<!

   public:

      unsigned gem1Roc;  ///<! ROCid for first GEM
      unsigned gem2Roc;  ///<! ROCid for second GEM
      unsigned gem3Roc;  ///<! ROCid for third GEM

      TGemProc(const char* name) :
         base::EventProc(name)
      {
         // printf("Create %s for %s ch%u\n", GetName(), fTdcId.c_str(), fChId);

         fMappingGSI_GEM1 = MakeH2("GEM/MappingGSI_GEM1", "MAPPING GSI for station 1 ", 20, 0., 19., 20, 0., 19., "Mapping_X;Mapping_Y");
         fMappingGSI_GEM2 = MakeH2("GEM/MappingGSI_GEM2", "MAPPING GSI for station 2 ", 20, 0., 19., 20, 0., 19., "Mapping_X;Mapping_Y");
         fMappingGSI_GEM3 = MakeH2("GEM/MappingGSI_GEM3", "MAPPING GSI for station 3 ", 20, 0., 19., 20, 0., 19., "Mapping_X;Mapping_Y");

         gem1Roc = 2;  ///<! ROCid for first GEM
         gem2Roc = 3;  ///<! ROCid for second GEM
         gem3Roc = 4;  ///<! ROCid for third GEM
      }


      virtual void CreateBranch(TTree*)
      {
      }

      virtual bool Process(base::Event* evnt)
      {
         for (int nGEM=0; nGEM<3; nGEM++) {
           // cout << "  Processing ROC "<< RICHROC << endl;
            // Loop over all ROCs which deliver RICH data

            unsigned rocid(0);
            if (nGEM == 0) rocid = gem1Roc; else
            if (nGEM == 1) rocid = gem2Roc; else
            if (nGEM == 2) rocid = gem3Roc;

            nx::SubEvent* sub = dynamic_cast<nx::SubEvent*> (evnt->GetSubEvent("ROC", rocid));
            if (sub==0) {
               // there is no data for the ROC
               // TGo4Log::Warn("No data for ROC%u found", rocid);
               continue;
            }

            for (unsigned cnt=0; cnt < sub->Size(); cnt++) {

               const nx::MessageExt& extmsg = sub->msg(cnt);
               const nx::Message& msg = extmsg.msg();

               if (!msg.isHitMsg()) continue;  // only interested in HIT messages

               // extract message information
               // UInt_t dataROC = msg.GetRocNumber();

               UInt_t nxid = msg.getNxNumber();  // 0 or 2
               UInt_t nxch = msg.getNxChNum();   // 0..127

               //Int_t dataHitADCraw = msg.getNxAdcValue();
               //Int_t dataHitADCcor = extmsg.GetCorrectedNxADC();
               //Double_t dataHitTime = extmsg.GetGlobalTime() - evnt->GetTriggerTime();

               int z = (nxid==0) ? 1 : 0;

               switch (nGEM) {
                  case 0:
                     FillH2(fMappingGSI_GEM1,(17-xGSI[z][nxch]),yGSI[z][nxch]);
                     break;

                  case 1:
                     FillH2(fMappingGSI_GEM2, (17-xGSI[z][nxch]),yGSI[z][nxch]);
                     break;

                  case 2:
                     FillH2(fMappingGSI_GEM3, (17-xGSI[z][nxch]),yGSI[z][nxch]);
                     break;
               }
            }
         }

         return true;
      }
};



void second() {

   TGemProc* p = new TGemProc("GEM");

   p->gem1Roc = 2;  ///<! ROCid for first GEM
   p->gem2Roc = 3;  ///<! ROCid for second GEM
   p->gem3Roc = 4;  ///<! ROCid for third GEM
}
