#ifndef TROOTPROCMGR_H
#define TROOTPROCMGR_H

#include "base/ProcMgr.h"

/** Processors manager for using in ROOT environment */

class TRootProcMgr : public base::ProcMgr {
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

