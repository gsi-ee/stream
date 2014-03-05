
// example of post-processor, which is called once at the analysis end

#include <stdio.h>

#include "TTree.h"
#include "TH1.h"

#include "base/EventProc.h"
#include "base/Event.h"
#include "hadaq/TdcSubEvent.h"
#include "hadaq/TrbProcessor.h"
#include "hadaq/TdcProcessor.h"


class PostProcessor : public base::EventProc {
   public:

      hadaq::TrbProcessor* fTRB;

      base::H2handle  hCorr;  //!< correlation between mean and rms

      PostProcessor(hadaq::TrbProcessor* trb) :
         base::EventProc("PostProcessor"),
         fTRB(trb)
      {
         hCorr = MakeH2("Corr","Correlation between mean and rms", 100, -10., 10., 100, 0., 5., "mean, ns;rms, ns");
      }

      virtual void UserPostLoop()
      {
         printf("UserPostLoop trb = %p\n", fTRB);
         if (fTRB==0) return;

         for (unsigned ntdc=0xC000;ntdc<0xC005;ntdc++) {

            hadaq::TdcProcessor* tdc = fTRB->GetTDC(ntdc);
            if (tdc==0) continue;

            for (unsigned  nch=1;nch<tdc->NumChannels(); nch++) {
               TH1* hist = (TH1*) tdc->GetChannelRefHist(nch);

               if (hist==0) continue;

               double mean = hist->GetMean();
               double rms = hist->GetRMS();

               FillH2(hCorr, mean, rms);
               // tdc->ClearChannelRefHist(nch);


               printf("  %s ch %u mean:%5.2f rms:%5.2f\n", tdc->GetName(), nch, mean, rms);
            }

         }

      }
};


void second()
{
   hadaq::TrbProcessor* trb3 = dynamic_cast<hadaq::TrbProcessor*> (base::ProcMgr::instance()->FindProc("TRB_0000"));

   // processor used only to invoke operation at the end of analysis
   new PostProcessor(trb3);

}


