#include "TTreeStore.h"

#include "TTree.h"
#include "TFile.h"

TTreeStore::TTreeStore(const char* fname) :
   base::EventStore(),
   fFile(0),
   fTree(0)
{
   fFile = TFile::Open(fname, "RECREATE","Store for stream events");
   if (fFile==0) return;
   fTree = new TTree("T", "Tree with stream data");
   // fTree->Write();
}

TTreeStore::~TTreeStore()
{
   if (fTree && fFile) {
      fFile->cd();
      fTree->Write();
      delete fTree;
      fTree = 0;
      delete fFile;
      fFile = 0;
   }
}

bool TTreeStore::DataBranch(const char* name, void* ptr, const char* options)
{
   if (fTree==0) return false;

   printf("Create branch %s ptr %p options %s\n", name, ptr, options);

   fTree->Branch(name, ptr, Form("%s%s", name, options));
   return true;
}

bool TTreeStore::EventBranch(const char* name, const char* classname, void* ptr)
{
   if (fTree==0) return false;
   fTree->Branch(name, classname, &ptr);
   return true;
}

void TTreeStore::Fill()
{
   if (fTree) fTree->Fill();
}
