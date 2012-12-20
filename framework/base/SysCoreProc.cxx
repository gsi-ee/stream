#include "base/SysCoreProc.h"

#include <stdio.h>

#include "base/ProcMgr.h"


unsigned base::SysCoreProc::fMaxBrdId = 16;

base::SysCoreProc::SysCoreProc(const char* name, unsigned brdid) :
   base::StreamProc(name, brdid),
   fSyncSource(0),
   fTriggerSignal(0xff),
   fNumPrintMessages(0),
   fPrintLeft(-1.),
   fPrintRight(-1.),
   fAnyPrinted(false)
{
   char sbuf1[100], sbuf2[100];
   sprintf(sbuf1, "MsgPer%s", name);
   sprintf(sbuf2, "Number of messages per %s", name);

   fMsgPerBrd = mgr()->MakeH1(sbuf1, sbuf2, fMaxBrdId, 0, fMaxBrdId, "brdid");
}

base::SysCoreProc::~SysCoreProc()
{
}


void base::SysCoreProc::CreateBasicHistograms()
{
   fALLt = MakeH1("ALL_t", "Time distribution of all messages", 10000, 0., 1000., "s");

   fAUXt[0] = MakeH1("AUX0_t", "Time distribution of AUX0 signal", 10000, 0., 1000., "s");
   fAUXt[1] = MakeH1("AUX1_t", "Time distribution of AUX1 signal", 10000, 0., 1000., "s");
   fAUXt[2] = MakeH1("AUX2_t", "Time distribution of AUX2 signal", 10000, 0., 1000., "s");
   fAUXt[3] = MakeH1("AUX3_t", "Time distribution of AUX3 signal", 10000, 0., 1000., "s");

   fSYNCt[0] = MakeH1("SYNC0_t", "Time distribution of SYNC0 signal", 10000, 0., 1000., "s");
   fSYNCt[1] = MakeH1("SYNC1_t", "Time distribution of SYNC1 signal", 10000, 0., 1000., "s");
}


bool base::SysCoreProc::CheckPrint(double msgtm, double safetymargin)
{
   if (fNumPrintMessages <= 0) return false;

   bool doprint = true;

   if ((fPrintLeft>0) && (fPrintLeft < fPrintRight)) {
      if (!fAnyPrinted) safetymargin = 0.;
      doprint = (msgtm >= fPrintLeft-safetymargin) && (msgtm <= fPrintRight+safetymargin);
      if (!doprint && fAnyPrinted) {
         fNumPrintMessages = 0;
         printf("Stop printing while message time is %9.6f\n", msgtm);
         return false;
      }
   }

   if (doprint) {
      fNumPrintMessages--;
      fAnyPrinted = true;
   }

   return doprint;
}
