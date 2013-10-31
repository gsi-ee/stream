#ifndef THOOKPROC_H
#define THOOKPROC_H

#include "base/StreamProc.h"
#include "TInterpreter.h"
#include "TStopwatch.h"

class THookProc : public base::StreamProc {

   protected:
      TString fCmd;
      Double_t fPeriod;
      TStopwatch fWatch;

   public:
      THookProc(const char* cmd, Double_t period = 0.) :
         base::StreamProc("Hook"),
         fCmd(cmd),
         fPeriod(period),
         fWatch()
      {
         SetRawScanOnly(true);
         fWatch.Start();
      }

      virtual bool ScanNewBuffers()
      {
         if (fPeriod>0.) {
            if (fWatch.RealTime() > fPeriod) {
               fWatch.Start(kTRUE);
            } else {
               fWatch.Start(kFALSE);
               return true;
            }
         }

         gInterpreter->ProcessLine(fCmd.Data());

         return true;
      }
};

#endif

