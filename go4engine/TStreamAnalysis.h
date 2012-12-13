#ifndef TSTREAMANALYSIS_H
#define TSTREAMANALYSIS_H

#include "TGo4Analysis.h"

class TStreamAnalysis : public TGo4Analysis {
   public:
      TStreamAnalysis();
      TStreamAnalysis(int argc, char** argv);
      virtual ~TStreamAnalysis() ;

   private:

   ClassDef(TStreamAnalysis,1)
};

#endif //TANALYSIS_H
