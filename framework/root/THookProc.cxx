#include "root/THookProc.h"

THookProc::THookProc(const char* cmd, Double_t period) :
   base::StreamProc("Hook"),
   fCmd(cmd),
   fPeriod(period),
   fWatch()
{
   SetRawScanOnly(true);
   SetSynchronisationKind(sync_None);
   fWatch.Start();
}

bool THookProc::ScanNewBuffers()
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
