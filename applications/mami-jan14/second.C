#include <stdio.h>

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
      
      virtual void Process(base::Event* ev) 
      {
         ResetHits();
         
         hadaq::TdcSubEvent* sub = 
               dynamic_cast<hadaq::TdcSubEvent*> (ev->GetSubEvent(fTdcId));
         
         if (sub==0) return;
         // printf("%s process sub %p\n", GetName(), sub);
         
         for (unsigned cnt=0;cnt<sub->Size();cnt++) {
            const hadaq::TdcMessageExt& ext = sub->msg(cnt);
            
            if ((ext.msg().getHitChannel()<=fChId) ||
                (ext.msg().getHitChannel()>fChId+4)) continue;
            
            unsigned id = ext.msg().getHitChannel() - fChId - 1;
            
            fHits[id] = ext.GetGlobalTime() - ev->GetTriggerTime();
            // printf("   %s id %u time %12.9f\n", GetName(), id, fHits[id]);
         }
         
         if (!IsComplete()) return;
         
         FillH1(hFast, GetFast()*1e9);
         FillH1(hSlow, GetSlow()*1e9);
         FillH2(hCorr, GetFast()*1e9, GetSlow()*1e9);
      }
};

class TestProc : public base::EventProc {
   protected:
      
      PadiwaProc* fProc1;     //!< first processor
      PadiwaProc* fProc2;     //!< second processor
      base::H2handle  hCorr;  //!< correlation between padiwas
      
   public:
      TestProc(PadiwaProc* proc1, PadiwaProc* proc2) : 
         base::EventProc("TEST"),
         fProc1(proc1),
         fProc2(proc2)
      {
         hCorr = MakeH2("Corr","Correlation between detector and trigger", 80, -10., 30., 500, 0., 500., "trigger, ns;detector, ns");
      }

      virtual void Process(base::Event*) 
      {
         if ((fProc2->GetHit(2)!=0) && (fProc1->GetHit(3)!=0)) {
            FillH2(hCorr, (fProc2->GetHit(2) - fProc2->GetHit(0))*1e9, (fProc1->GetHit(3) - fProc1->GetHit(0))*1e9);
         }
      }
      
};


void second() 
{
   PadiwaProc* proc1 = new PadiwaProc(0, "TDC0", 48);
   PadiwaProc* proc2 = new PadiwaProc(1, "TDC3", 0);
   
   new TestProc(proc1, proc2);
}
