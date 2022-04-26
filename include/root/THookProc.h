#ifndef THOOKPROC_H
#define THOOKPROC_H

#include "base/StreamProc.h"

#include "TInterpreter.h"
#include "TStopwatch.h"

/** Hook processor to regularly execute code during events processing */

class THookProc : public base::StreamProc {

   protected:
      TString fCmd;           ///< command to execute
      Double_t fPeriod{0};    ///< period
      TStopwatch fWatch;      ///< time measuring

   public:
      THookProc(const char* cmd, Double_t period = 0.);

      bool ScanNewBuffers() override;
};

#endif

