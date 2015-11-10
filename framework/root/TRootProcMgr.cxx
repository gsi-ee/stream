#include "root/TRootProcMgr.h"

#include "TTree.h"
#include "TFile.h"
#include "TROOT.h"
#include "TInterpreter.h"

TRootProcMgr::TRootProcMgr() :
   base::ProcMgr(),
   fFile(0)
{
}

TRootProcMgr::~TRootProcMgr()
{
   CloseStore();
}


bool TRootProcMgr::ProcessEvent(base::Event* evt)
{
   if (!base::ProcMgr::ProcessEvent(evt)) return false;

   if (fTree) fTree->Fill();

   return true;
}

bool TRootProcMgr::CreateStore(const char* fname)
{
   if (fTree) return true;
   fFile = TFile::Open(fname, "RECREATE","Store for stream events");
   if (fFile==0) return false;
   fTree = new TTree("T", "Tree with stream data");
   return true;
}

bool TRootProcMgr::CloseStore()
{
   if (fTree && fFile) {
      fFile->cd();
      fTree->Write();
      delete fTree;
      fTree = 0;
      delete fFile;
      fFile = 0;
   }
   return true;
}

bool TRootProcMgr::CreateBranch(TTree* t, const char* name, const char* class_name, void** obj)
{
   if (t==0) return false;
   t->Branch(name, class_name, obj);
   return true;
}

bool TRootProcMgr::CreateBranch(TTree* t, const char* name, void* member, const char* kind)
{
   if (t==0) return false;
   t->Branch(name, member, kind);
   return true;
}

bool TRootProcMgr::CallFunc(const char* funcname, void* arg)
{
   if (funcname==0) return false;

   TString callargs;
   callargs.Form("%p", arg);

   int err = 0;
   gInterpreter->Execute(funcname, callargs.Data(), &err);

   printf("CINT: call %s(%s) err = %d\n", funcname,callargs.Data(), err);

   return err == 0;
}


