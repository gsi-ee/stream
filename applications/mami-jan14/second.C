#include <stdio.h>

#include "TTree.h"

#include "base/EventProc.h"
#include "base/Event.h"
#include "base/SubEvent.h"

#include "hadaq/TdcSubEvent.h"

class PadiwaProc : public base::EventProc {
   protected:
      
      std::string fTdcId;    //!< tdc id with padiwa asic 
      unsigned    fChId;     //!< first channel
      double      fHits[4];  //!< record time 
      
      base::H1handle  hFast;  //!< histogram of fast channel
      base::H1handle  hSlow;  //!< histogram of slow channel
      base::H2handle  hCorr;  //!< correlation between fast and slow
      
   public:
      PadiwaProc(unsigned padiwaid, const char* _tdcid, unsigned _chid) : 
         base::EventProc("Padiwa", padiwaid),
         fTdcId(_tdcid),
         fChId(_chid)
         
      {
         // printf("Create %s for %s ch%u\n", GetName(), fTdcId.c_str(), fChId);
         ResetHits();
         
         hFast = MakeH1("Fast","Width of fast channel", 1000, 0, 100, "ns");
         hSlow = MakeH1("Slow","Width of slow channel", 1000, 0, 200, "ns");
         hCorr = MakeH2("Corr","Correlation between fast and slow", 100, 0., 100., 100, 0., 200., "fast, ns;slow, ns");
      }
      
      
      virtual void CreateBranch(TTree* t)
      {
         t->Branch(GetName(), fHits, "hits[4]/D");
      }
      
      void ResetHits() 
      {
         for (unsigned n=0;n<4;n++) fHits[n] = 0;
      }
      
      bool IsComplete() 
      {
         for (unsigned n=0;n<4;n++) 
            if (fHits[n] == 0) return false;
         return true;
      }
      
      double GetHit(unsigned n) const { return fHits[n]; } 
      
      double GetFast() const { return fHits[1] - fHits[0]; }
      double GetSlow() const { return fHits[3] - fHits[2]; }
      
      virtual bool Process(base::Event* ev) 
      {
         ResetHits();
         
         hadaq::TdcSubEvent* sub = 
               dynamic_cast<hadaq::TdcSubEvent*> (ev->GetSubEvent(fTdcId));
         
         if (sub==0) return false;
         // printf("%s process sub %p\n", GetName(), sub);
         
         for (unsigned cnt=0;cnt<sub->Size();cnt++) {
            const hadaq::TdcMessageExt& ext = sub->msg(cnt);
            
            // exclude all channels which has nothing to do with Padiwa
            if ((ext.msg().getHitChannel()<=fChId) ||
                (ext.msg().getHitChannel()>fChId+4)) continue;
            
            unsigned id = ext.msg().getHitChannel() - fChId - 1;
            
            fHits[id] = ext.GetGlobalTime() - ev->GetTriggerTime();
            // printf("   %s id %u time %12.9f\n", GetName(), id, fHits[id]);
         }
         
         if (IsComplete()) {
            FillH1(hFast, GetFast()*1e9);
            FillH1(hSlow, GetSlow()*1e9);
            FillH2(hCorr, GetFast()*1e9, GetSlow()*1e9);
         }
         
         return true;
      }
};

class TestProc : public base::EventProc {
   protected:
      
      PadiwaProc* fProc1;     //!< first processor
      PadiwaProc* fProc2;     //!< second processor
      base::H2handle  hCorr;  //!< correlation between padiwas
      double fX, fY;          //!< calcualted coordinates 
      
   public:
      TestProc(PadiwaProc* proc1, PadiwaProc* proc2) : 
         base::EventProc("TEST"),
         fProc1(proc1),
         fProc2(proc2),
         fX(0), fY(0)
      {
         hCorr = MakeH2("Corr","Correlation between detector and trigger", 80, -10., 30., 500, 0., 500., "trigger, ns;detector, ns");
         
         SetStoreEnabled();
      }

      virtual void CreateBranch(TTree* t)
      {
         t->Branch("testx", &fX, "X/D");
         t->Branch("testy", &fY, "Y/D");
      }
      
      virtual bool Process(base::Event*) 
      {
         if ((fProc2->GetHit(2)!=0) && (fProc1->GetHit(3)!=0)) {
            fX = (fProc2->GetHit(2) - fProc2->GetHit(0))*1e9;
            fY = (fProc1->GetHit(3) - fProc1->GetHit(0))*1e9;
            FillH2(hCorr, fX, fY);
            return (fX>0) && (fY>0);
         }
         
         fX = 0; fY = 0;
         // event is not complete and should not be stored
         return false;
      }
      
};


void second() 
{
   base::ProcMgr::instance()->CreateStore("file.root");
   
   PadiwaProc* proc1 = new PadiwaProc(0, "TDC0", 48);
   proc1->SetStoreEnabled();
   
   PadiwaProc* proc2 = new PadiwaProc(1, "TDC3", 0);
   proc2->SetStoreEnabled();
   
   new TestProc(proc1, proc2);
}
