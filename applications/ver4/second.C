#include <cstdio>

#include "TTree.h"
#include "TH1.h"

#include "base/EventProc.h"
#include "base/Event.h"
#include "hadaq/TdcSubEvent.h"
#include "hadaq/TdcProcessor.h"

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


class PrintProc : public base::EventProc {
   protected:

      std::string fTdcId;    //!< tdc id where channels will be selected "TDC_8a00"

      int fCounter;

   public:
      PrintProc(const char* procname, const char* _tdcid) :
         base::EventProc(procname),
         fTdcId(_tdcid)
      {
         printf("Create %s for %s\n", GetName(), fTdcId.c_str());

         fCounter = 0;

         // enable storing already in constructor
         SetStoreEnabled();
      }


      virtual bool Process(base::Event* ev)
      {
         fCounter++;
         if (fCounter % 10000 == 0) {
            int kRisingRefId = 2, kTotId = 5;

            hadaq::TdcProcessor *tdc = dynamic_cast<hadaq::TdcProcessor *>(mgr()->FindProc(fTdcId.c_str()));
            TH1 *hist = tdc ? (TH1 *) tdc->GetHist(1, kRisingRefId) : 0; // rising ref for channel 1
            if (hist) printf("%s channel 1 ref mean %f rms %f\n", fTdcId.c_str(), hist->GetMean(), hist->GetRMS());
            hist = tdc ? (TH1 *) tdc->GetHist(1, kTotId) : 0; // Tot for channel 1
            if (hist) printf("%s channel 1 ToT mean %f rms %f\n", fTdcId.c_str(), hist->GetMean(), hist->GetRMS());
            hist = tdc ? (TH1 *) tdc->GetHist(5, kRisingRefId) : 0; // rising ref for channel 5
            if (hist) printf("%s channel 5 ref mean %f rms %f\n", fTdcId.c_str(), hist->GetMean(), hist->GetRMS());
            hist = tdc ? (TH1 *) tdc->GetHist(5, kTotId) : 0; // Tot for channel 5
            if (hist) printf("%s channel 5 ToT mean %f rms %f\n", fTdcId.c_str(), hist->GetMean(), hist->GetRMS());
            //hist = tdc ? (TH1 *) tdc->GetHist(14, kRisingRefId) : 0; // rising ref for channel 14
            //if (hist) printf("TDC %s channel 14 mean %f rms %f\n", fTdcId.c_str(), hist->GetMean(), hist->GetRMS());
         }

         return true;
      }
};


void second()
{
   // uncomment line to create tree for the events storage
   // base::ProcMgr::instance()->CreateStore("file.root");

   // new DebugProc("Debug1", "TDC_16F7");

   new PrintProc("Print", "TDC_16F7");
}
