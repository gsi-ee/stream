#ifndef TGEMPROC_H
#define TGEMPROC_H

#include "go4/TUserProcessor.h"

class TGemProc : public TUserProcessor {
 protected:


   TH2* fMappingGSI_GEM1;  //!
   TH2* fMappingGSI_GEM2;  //!
   TH2* fMappingGSI_GEM3;  //!

 public:
    unsigned gem1Roc;  //! ROCid for first GEM
    unsigned gem2Roc;  //! ROCid for second GEM
    unsigned gem3Roc;  //! ROCid for third GEM

    TGemProc(const char* name = 0);

    virtual void Process(TStreamEvent*);

    ClassDef(TGemProc,1)
};

#endif

