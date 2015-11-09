#ifndef TROOTPROCMGR_H
#define TROOTPROCMGR_H

#include "base/ProcMgr.h"

class TFile;
class TTree;

class TRootProcMgr : public base::ProcMgr {

   protected:
      TFile                   *fFile;            //!< file with the tree, if 0 - tree is external
   public:
      TRootProcMgr();
      virtual ~TRootProcMgr();

      virtual bool ProcessEvent(base::Event* evt);

      virtual bool CreateStore(const char* storename);
      virtual bool CloseStore();

      virtual bool CreateBranch(TTree* t, const char* name, const char* class_name, void** obj);
      virtual bool CreateBranch(TTree* t, const char* name, void* member, const char* kind);

      bool CallFunc(const char* funcname, void* arg);
};

#endif

