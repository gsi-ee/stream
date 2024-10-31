#include "root/TRootProcMgr.h"

#include "TTree.h"
#include "TFile.h"
#include "TROOT.h"
#include "TUrl.h"
#include "TInterpreter.h"

///////////////////////////////////////////////////////////////////////////////
/// constructor

TRootProcMgr::TRootProcMgr() :
   base::ProcMgr()
{
}

///////////////////////////////////////////////////////////////////////////////
/// destructor

TRootProcMgr::~TRootProcMgr()
{
   CloseStore();
}

///////////////////////////////////////////////////////////////////////////////
/// store event

bool TRootProcMgr::StoreEvent()
{
   if (!fTree) return false;

   fTree->Fill();

   return true;
}

///////////////////////////////////////////////////////////////////////////////
/// create store

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

///////////////////////////////////////////////////////////////////////////////
/// close store

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

///////////////////////////////////////////////////////////////////////////////
/// create branch

bool TRootProcMgr::CreateBranch(const char* name, const char* class_name, void** obj)
{
   if (fTree==0) return false;
   fTree->Branch(name, class_name, obj);
   return true;
}

///////////////////////////////////////////////////////////////////////////////
/// create branch

bool TRootProcMgr::CreateBranch(const char* name, void* member, const char* kind)
{
   if (fTree==0) return false;
   fTree->Branch(name, member, kind);
   return true;
}

///////////////////////////////////////////////////////////////////////////////
/// call function

bool TRootProcMgr::CallFunc(const char* funcname, void* arg)
{
   if (funcname==0) return false;

   typedef void (*myfunc)(void*);

   void *symbol = gInterpreter->FindSym(funcname);

   if (symbol) {

      printf("direct func call %s\n", funcname);

      myfunc func = (myfunc) symbol;

      func(arg);

      return true;

   }

   TString callargs;
   callargs.Form("%s((void*)%p);", funcname, arg);

   int err = 0;

   gROOT->ProcessLine(callargs.Data(), &err);

   // gInterpreter->Execute(funcname, callargs.Data(), &err);

   printf("ProcessLine: %s err = %d\n", callargs.Data(), err);

   return err == 0;
}
