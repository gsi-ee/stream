#ifndef TSECONDSTEPPROCESSOR_H
#define TSECONDSTEPPROCESSOR_H

#include "TGo4EventProcessor.h"

#include "TObjArray.h"

class TUserProcessor;

class TSecondStepProcessor : public TGo4EventProcessor {

   protected:

      static TString fDfltSetupScript;  //!  name of setup script

      static TSecondStepProcessor* fInstance;  //!  pointer on object singelton

      TObjArray  fProc;

   public:

      TSecondStepProcessor(const char* name = 0);
      virtual ~TSecondStepProcessor();

      static TSecondStepProcessor* Instance() { return fInstance; }

      static void SetDfltScript(const char* name) { fDfltSetupScript = name; }
      static const char* GetDfltScript() { return fDfltSetupScript.Data(); }

      /** Add subprocessor, it will be automatically deleted at the end */
      void AddProc(TUserProcessor* proc);

      /** Can be overwritten by subclass, but is not recommended! use ProcessEvent or ProcessSubevent instead*/
      virtual Bool_t BuildEvent(TGo4EventElement*);


   ClassDef(TSecondStepProcessor,1)
};

#endif

