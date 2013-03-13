#ifndef TSTREAMANALYSIS_H
#define TSTREAMANALYSIS_H

#include "TGo4Analysis.h"

class TStreamAnalysis : public TGo4Analysis {
   public:
      TStreamAnalysis();
      TStreamAnalysis(int argc, char** argv);
      virtual ~TStreamAnalysis() ;

      virtual Int_t UserPreLoop();

      virtual Int_t UserPostLoop();


   ClassDef(TStreamAnalysis,1)
};

#endif //TANALYSIS_H
