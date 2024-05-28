#include "base/Processor.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "base/ProcMgr.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
/// constructor

base::Processor::Processor(const char* name, unsigned brdid) :
   fName(name),
   fID(0),
   fMgr(nullptr),
   fPrefix(),
   fSubPrefixD(),
   fSubPrefixN(),
   fHistFilling(99),
   fStoreKind(0),
   fIntHistFormat(false)
{
   if (brdid != DummyBrdId) {
      char sbuf[100];
      unsigned mask = strstr(name, "%06X") ? 0xffffff : 0xffff;
      snprintf(sbuf, sizeof(sbuf), name, brdid & mask);
      fName = sbuf;
      fID = brdid;
   }

   fPathPrefix = fName;
   fPrefix = fName;

   SetManager(base::ProcMgr::instance());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/// destructor

base::Processor::~Processor()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/// set manager instance

void base::Processor::SetManager(base::ProcMgr* m)
{
   fMgr = m;

   if (fMgr) {
      fIntHistFormat = fMgr->InternalHistFormat();
      fHistFilling = fMgr->fDfltHistLevel;
      fStoreKind = fMgr->fDfltStoreKind;
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/// set sub-prefixes

void base::Processor::SetSubPrefix(const char* name, int indx, const char* subname2, int indx2)
{
   if (!name || (*name==0)) {
      fSubPrefixD.clear();
      fSubPrefixN.clear();
      return;
   }

   fSubPrefixN = name;
   fSubPrefixD = name;
   if (indx>=0) {
      char sbuf[100];
      snprintf(sbuf, sizeof(sbuf), "%d", indx);
      fSubPrefixD.append(sbuf);
      fSubPrefixN.append(sbuf);
   }
   fSubPrefixD.append("/");
   fSubPrefixN.append("_");

   if (subname2 && (*subname2 != 0)) {
      fSubPrefixD.append(subname2);
      fSubPrefixN.append(subname2);
      if (indx2>=0) {
         char sbuf[100];
         snprintf(sbuf,sizeof(sbuf), "%d", indx2);
         fSubPrefixD.append(sbuf);
         fSubPrefixN.append(sbuf);
      }
      fSubPrefixD.append("/");
      fSubPrefixN.append("_");
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/// Set sub-prefixes, use 2 digits for indicies

void base::Processor::SetSubPrefix2(const char* name, int indx, const char* subname2, int indx2)
{
   if (!name || (*name==0)) {
      fSubPrefixD.clear();
      fSubPrefixN.clear();
      return;
   }

   fSubPrefixN = name;
   fSubPrefixD = name;
   if (indx>=0) {
      char sbuf[100];
      snprintf(sbuf, sizeof(sbuf), "%02d", indx);
      fSubPrefixD.append(sbuf);
      fSubPrefixN.append(sbuf);
   }
   fSubPrefixD.append("/");
   fSubPrefixN.append("_");

   if (subname2 && (*subname2 != 0)) {
      fSubPrefixD.append(subname2);
      fSubPrefixN.append(subname2);
      if (indx2>=0) {
         char sbuf[100];
         snprintf(sbuf,sizeof(sbuf), "%02d", indx2);
         fSubPrefixD.append(sbuf);
         fSubPrefixN.append(sbuf);
      }
      fSubPrefixD.append("/");
      fSubPrefixN.append("_");
   }
}

/////////////////////////////////////////////////////////////////////////
/// Adds processor prefix to histogram name and calls \ref base::ProcMgr::MakeH1 method

base::H1handle base::Processor::MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle)
{
   if (!mgr() || !IsHistFilling()) return nullptr;

   std::string hname = fPathPrefix + "/";
   if (!fSubPrefixD.empty()) hname += fSubPrefixD;
   hname += fPrefix + "_";
   if (!fSubPrefixN.empty()) hname += fSubPrefixN;
   hname.append(name);

   std::string htitle = fName;
   htitle.append(" ");
   if (!fSubPrefixN.empty()) htitle += fSubPrefixN.substr(0, fSubPrefixN.length()-1) + " ";
   htitle.append(title);

   auto iter = mgr()->fCustomBinning.find(name);
   if (iter != mgr()->fCustomBinning.end()) {
      nbins = iter->second.nbins;
      left = iter->second.left;
      right = iter->second.right;
   }

   return mgr()->MakeH1(hname.c_str(), htitle.c_str(), nbins, left, right, xtitle);
}


/////////////////////////////////////////////////////////////////////////
/// Adds processor prefix to histogram name and calls \ref base::ProcMgr::MakeH2 method

base::H2handle base::Processor::MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options)
{
   if (!mgr() || !IsHistFilling()) return nullptr;

   std::string hname = fPathPrefix + "/";
   if (!fSubPrefixD.empty()) hname += fSubPrefixD;
   hname += fPrefix + "_";
   if (!fSubPrefixN.empty()) hname += fSubPrefixN;
   hname.append(name);

   std::string htitle = fName;
   htitle.append(" ");
   if (!fSubPrefixN.empty()) htitle += fSubPrefixN.substr(0, fSubPrefixN.length()-1) + " ";
   htitle.append(title);

   auto iter = mgr()->fCustomBinning.find(name);
   if (iter != mgr()->fCustomBinning.end()) {
      nbins2 = iter->second.nbins;
      left2 = iter->second.left;
      right2 = iter->second.right;
   }

   return mgr()->MakeH2(hname.c_str(), htitle.c_str(), nbins1, left1, right1, nbins2, left2, right2, options);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/// Create condition

base::C1handle base::Processor::MakeC1(const char* name, double left, double right, H1handle h1)
{
   if (!mgr()) return nullptr;

   std::string cname = fPathPrefix + "/";
   if (!fSubPrefixD.empty()) cname += fSubPrefixD;
   cname += fPrefix + "_";
   if (!fSubPrefixN.empty()) cname += fSubPrefixN;
   cname.append(name);

   return mgr()->MakeC1(cname.c_str(), left, right, h1);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/// Change condition limits

void base::Processor::ChangeC1(C1handle c1, double left, double right)
{
   if (mgr()) mgr()->ChangeC1(c1, left, right);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/// Get condition limit

double base::Processor::GetC1Limit(C1handle c1, bool isleft)
{
   return mgr() ? mgr()->GetC1Limit(c1, isleft) : 0.;
}
