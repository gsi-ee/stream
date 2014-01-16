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
      THookProc(const char* cmd, Double_t period = 0.);

      virtual bool ScanNewBuffers();
};

#endif

