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

      virtual bool CreateStore(const char* storename);
      virtual bool CloseStore();

      virtual bool CreateBranch(const char* name, const char* class_name, void** obj);
      virtual bool CreateBranch(const char* name, void* member, const char* kind);

      virtual bool StoreEvent();

      bool CallFunc(const char* funcname, void* arg);
};

#endif

