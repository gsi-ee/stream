#include "root/TRootProcMgr.h"

#include "TTree.h"
#include "TFile.h"
#include "TROOT.h"
#include "TUrl.h"
#include "TInterpreter.h"

TRootProcMgr::TRootProcMgr() :
   base::ProcMgr()
{
}

TRootProcMgr::~TRootProcMgr()
{
   CloseStore();
}

bool TRootProcMgr::StoreEvent()
{
   if (fTree==0) return false;

   fTree->Fill();

   return true;
}

bool TRootProcMgr::CreateStore(const char* fname)
{
   if (fTree) return true;
   TUrl url(fname);

   int maxsize = -1;

   if (url.HasOption("maxsize")) {
      maxsize = url.GetIntValueFromOptions("maxsize");
      if (url.GetFile()!=0) fname = url.GetFile();
   }
   if (maxsize>0) TTree::SetMaxTreeSize(maxsize*1024*1024);
   TFile* f = TFile::Open(fname, "RECREATE","Store for stream events");
   if (f==0) return false;
   fTree = new TTree("T", "Tree with stream data");
   return true;
}

bool TRootProcMgr::CloseStore()
{
   if (fTree) {
      TFile* f = fTree->GetCurrentFile();
      f->cd();
      fTree->Write();
      delete fTree;
      fTree = 0;
      delete f;
   }
   return true;
}

bool TRootProcMgr::CreateBranch(const char* name, const char* class_name, void** obj)
{
   if (fTree==0) return false;
   fTree->Branch(name, class_name, obj);
   return true;
}

bool TRootProcMgr::CreateBranch(const char* name, void* member, const char* kind)
{
   if (fTree==0) return false;
   fTree->Branch(name, member, kind);
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


