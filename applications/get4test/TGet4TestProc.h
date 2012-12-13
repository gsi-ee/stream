#ifndef TGet4TestProc_H
#define TGet4TestProc_H

#include "go4/TUserProcessor.h"

class TGet4TestProc : public TUserProcessor {
   public:

      TGet4TestProc();

      TGet4TestProc(const char* name);

      virtual void Process(TStreamEvent*);

   ClassDef(TGet4TestProc, 1)
};


#endif
