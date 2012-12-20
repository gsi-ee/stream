#include "TGemProc.h"


#include "TH2.h"

#include "TGo4Log.h"

#include "go4/TStreamEvent.h"

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



TGemProc::TGemProc(const char* name) : TUserProcessor(name)
{
   TGo4Log::Info("TGemProc: Create instance %s", name);

   fMappingGSI_GEM1 = MakeTH2('I', "GEM/MappingGSI_GEM1", "MAPPING GSI for station 1 ", 20, 0., 19., 20, 0., 19., "Mapping_X", "Mapping_Y");
   fMappingGSI_GEM2 = MakeTH2('I', "GEM/MappingGSI_GEM2", "MAPPING GSI for station 2 ", 20, 0., 19., 20, 0., 19., "Mapping_X", "Mapping_Y");
   fMappingGSI_GEM3 = MakeTH2('I', "GEM/MappingGSI_GEM3", "MAPPING GSI for station 3 ", 20, 0., 19., 20, 0., 19., "Mapping_X", "Mapping_Y");

   gem1Roc = 2;  //! ROCid for first GEM
   gem2Roc = 3;  //! ROCid for second GEM
   gem3Roc = 4;  //! ROCid for third GEM
}


void TGemProc::Process(TStreamEvent* evnt)
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
         //cout << "roc::MSG_HIT" << endl;

         // extraxt message information
         // UInt_t dataROC = msg.GetRocNumber();

         UInt_t nxid = msg.getNxNumber();  // 0 or 2
         UInt_t nxch = msg.getNxChNum();   // 0..127
         Int_t dataHitADCraw = msg.getNxAdcValue();

         Int_t dataHitADCcor = extmsg.GetCorrectedNxADC();
         Double_t dataHitTime = extmsg.GetGlobalTime() - evnt->GetTriggerTime();

         int z = 0;

         switch (nGEM) {
            case 0:
               if(nxid==0) {
                  z = 1;
               }
               else if(nxid ==2)
               {
                  z =0;
               }
               fMappingGSI_GEM1->Fill((17-xGSI[z][nxch]),yGSI[z][nxch]);
               break;

            case 1:
               if(nxid ==0)
               {
                  z = 1;
               }
               else if(nxid ==2)
               {
                  z = 0;
               }
               fMappingGSI_GEM2->Fill((17-xGSI[z][nxch]),yGSI[z][nxch]);
               break;

            case 2:
               if(nxid ==0)
               {
                  z = 1;
               }
               else if(nxid ==2)
               {
                  z = 0;
               }
               fMappingGSI_GEM3->Fill((17-xGSI[z][nxch]),yGSI[z][nxch]);
               break;
         }
      }
   }
}
