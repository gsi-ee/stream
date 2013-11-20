#ifndef TUSERPROCESSOR_H
#define TUSERPROCESSOR_H

#include "TGo4EventProcessor.h"

class TStreamEvent;

class TUserProcessor : public TGo4EventProcessor {

   protected:

      long fTotalDataSize;

      static TString fDfltSetupScript;

   public:
      /** Default constructor */
      TUserProcessor();

      /** Normal constructor */
      TUserProcessor(const char* name);

      virtual ~TUserProcessor();

      /* Must be overwritten by subclass */
      virtual void Process(TStreamEvent*) = 0;


   ClassDef(TUserProcessor,1)
};

#endif

