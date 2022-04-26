#ifndef TROOTPROCMGR_H
#define TROOTPROCMGR_H

#include "base/ProcMgr.h"

/** Processors manager for using in ROOT environment */

class TRootProcMgr : public base::ProcMgr {
   public:
      TRootProcMgr();
      virtual ~TRootProcMgr();

      bool CreateStore(const char* storename) override;
      bool CloseStore() override;

      bool CreateBranch(const char* name, const char* class_name, void** obj) override;
      bool CreateBranch(const char* name, void* member, const char* kind) override;

      bool StoreEvent() override;

      bool CallFunc(const char* funcname, void* arg) override;
};

#endif

