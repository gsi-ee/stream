#include "base/EventProc.h"
#include "hadaq/TdcSubEvent.h"
#include "hadaq/HldProcessor.h"

//#include "base/Event.h"
//#include "base/TimeStamp.h"
// #include "hadaq/HldProcessor.h"

#define MAXCH 64

class SecondProc : public base::EventProc {
protected:
  std::string fTdcId;      //!< tdc id where channels will be selected
  base::H1handle  hToT;    //!< histogram with hits number
  base::H2handle  hToTCh;  //!< histogram with hits number
  base::H2handle  hFineCh;
  base::H2handle  hFineRiseCh;
  base::H2handle  hFineFallCh;

public:
	SecondProc(const char* procname, const char* _tdcid);
   virtual void CreateBranch(TTree* t);
   virtual bool Process(base::Event* ev);
};



SecondProc::SecondProc(const char* procname, const char* _tdcid) :
    base::EventProc(procname),
    fTdcId(_tdcid)
{
   hToT = MakeH1("ToT","ToT distribution", 1000, -1000, 1000, "ns");
   hToTCh = MakeH2("ToTch","ToT distribution channels", MAXCH, 0, MAXCH, 1000, -1000, 1000, "ch;ns");
   hFineCh = MakeH2("Finech","Fine distribution channels", MAXCH, 0, MAXCH, 600, 0, 600, "");
   hFineRiseCh = MakeH2("FineRisech","Fine rising", MAXCH, 0, MAXCH, 600, 0, 600, "");
   hFineFallCh = MakeH2("FineFallch","Fine falling", MAXCH, 0, MAXCH, 600, 0, 600, "");
}



void SecondProc::CreateBranch(TTree* t)
{
  // only called when tree is created in first.C
  // one can ignore
  // t->Branch(GetName(), fHits, "hits[8]/D");
}

bool SecondProc::Process(base::Event* ev)
{
   // nothing to do
   if (ev->NumSubEvents() == 0)
      return false;

   static int dcnt = 0;

   if (++dcnt < 5)
      for (auto &entry : ev->GetEventsMap())
         printf("Name %s Instance %p\n", entry.first.c_str(), entry.second);

   hadaq::TdcSubEvent* sub =
         dynamic_cast<hadaq::TdcSubEvent*> (ev->GetSubEvent(fTdcId));
  if (!sub) {
     printf("Fail to find %s\n", fTdcId.c_str());
     return false;
  }

   hadaq::HldSubEvent *hld = dynamic_cast<hadaq::HldSubEvent*> (ev->GetSubEvent("HLD"));
//    if (hld)
//       printf("HLD: type %u seq %u run %u\n", hld->fMsg.trig_type,
//                   hld->fMsg.seq_nr, hld->fMsg.run_nr);

  unsigned trigtype = hld->fMsg.trig_type;

  double last_rising[64];
  for (int n=0;n<64;n++) last_rising[n] = 0;

  double ch0tm = 0;

  for (unsigned ihit=0;ihit<sub->Size();ihit++) {

      hadaq::TdcMessageExt &ext = sub->msg(ihit);

      unsigned ch = ext.msg().getHitChannel();
      unsigned edge = ext.msg().getHitEdge();
      unsigned fine = ext.msg().getHitTmFine();
      unsigned coarse = ext.msg().getHitTmCoarse();
      double stamp = ext.GetGlobalTime(); // here in seconds


      FillH2(hFineCh, ch, fine);

      if (ch==0) { ch0tm = stamp; }  // ch0 has absolute time, all other channels relative to ch0

      if(edge == 0)
        FillH2(hFineFallCh, ch, fine);
      if(edge == 1) {
        // printf("%0.10f %0.10f\n",stamp,ch0tm);
        if( (stamp < 50e-9 && stamp > -10e-9) || ch == 0 || trigtype != 0xd)
          FillH2(hFineRiseCh, ch, fine);

        }

      // failure, but just keep it here
//       if (ch>=MAXCH) continue;

      // printf("ch %u edge %u stamp %f\n", ch, edge, stamp*1e9);

      if (edge==0) {
         last_rising[ch] = stamp;

         }
//       } else if (!last_rising[ch]) {
//          // printf(" ToT %f\n", stamp - last_rising[ch]);
//          FillH1(hToT, stamp - last_rising[ch]);
//          FillH2(hToTCh, ch, stamp - last_rising[ch]);
//          last_rising[ch] = 0;
//       }
    }

  return true;
}

class SecondCreator : public base::EventProc {
public:

   bool fCreated{false};

   SecondCreator(const char *name) : base::EventProc(name) {}

   virtual bool Process(base::Event* ev)
   {
      if (fCreated)
         return true;

      if (ev->NumSubEvents() == 0)
         return false;

      for (auto &entry : ev->GetEventsMap()) {

         if (entry.first.compare(0,3,"TDC") == 0) {
            std::string procname = std::string("x") + entry.first.substr(4);
            new SecondProc(procname.c_str(), entry.first.c_str());
            printf("CREATE SECOND for %s\n", entry.first.c_str());
            fCreated = true;
         }
      }

      return true;
   }

};


void second()
{
   new SecondCreator("Creator");
  //new SecondProc("x0050", "TDC_0050");
  //new SecondProc("x0507", "TDC_0507");
  //new SecondProc("x0303", "TDC_0303");
}
