#include "root/THookProc.h"

///////////////////////////////////////////////////////////////////////////////
/// constructor

THookProc::THookProc(const char* cmd, Double_t period) :
   base::StreamProc("Hook"),
   fCmd(cmd),
   fPeriod(period),
   fWatch()
{
   SetRawScanOnly();
   SetSynchronisationKind(sync_None);
   fWatch.Start();
}

///////////////////////////////////////////////////////////////////////////////
/// check if hook command should be executed during data scan

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
