#ifndef TTREESTORE_H
#define TTREESTORE_H

#include "base/EventStore.h"

class TTree;
class TFile;

class TTreeStore : public base::EventStore {

   protected:
      TFile* fFile;
      TTree* fTree;

   public:

      TTreeStore(const char* fname);

      virtual ~TTreeStore();

      virtual void* GetHandle() { return fTree; }

      virtual bool DataBranch(const char* name, void* ptr, const char* options);

      virtual bool EventBranch(const char* name, const char* classname, void* ptr);

      virtual void Fill();

};

#endif
