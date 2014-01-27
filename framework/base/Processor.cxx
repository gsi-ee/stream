#include "base/Processor.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "base/ProcMgr.h"

base::Processor::Processor(const char* name, unsigned brdid) :
   fName(name),
   fID(0),
   fMgr(0),
   fPrefix(),
   fSubPrefixD(),
   fSubPrefixN(),
   fHistFilling(99),
   fStoreEnabled(false)
{
   if (brdid != DummyBrdId) {
      char sbuf[100];
      snprintf(sbuf, sizeof(sbuf), name, brdid);
      fName = sbuf;
      fID = brdid;
   }

   fPrefix = fName;

   fMgr = base::ProcMgr::instance();
}


base::Processor::~Processor()
{
}

void base::Processor::SetSubPrefix(const char* name, int indx, const char* subname2, int indx2)
{
   if ((name==0) || (*name==0)) {
      fSubPrefixD.clear();
      fSubPrefixN.clear();
      return;
   }

   fSubPrefixN = name;
   fSubPrefixD = name;
   if (indx>=0) {
      char sbuf[100];
      snprintf(sbuf,sizeof(sbuf), "%d", indx);
      fSubPrefixD.append(sbuf);
      fSubPrefixN.append(sbuf);
   }
   fSubPrefixD.append("/");
   fSubPrefixN.append("_");

   if ((subname2!=0) && (*subname2!=0)) {
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

base::H1handle base::Processor::MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle)
{
   if ((mgr()==0) || !IsHistFilling()) return 0;

   std::string hname = fPrefix + "/";
   if (!fSubPrefixD.empty()) hname += fSubPrefixD;
   hname += fPrefix + "_";
   if (!fSubPrefixN.empty()) hname += fSubPrefixN;
   hname.append(name);

   std::string htitle = fName;
   htitle.append(" ");
   if (!fSubPrefixN.empty()) htitle += fSubPrefixN.substr(0, fSubPrefixN.length()-1) + " ";
   htitle.append(title);

   return mgr()->MakeH1(hname.c_str(), htitle.c_str(), nbins, left, right, xtitle);
}


base::H2handle base::Processor::MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options)
{
   if ((mgr()==0) ||!IsHistFilling()) return 0;

   std::string hname = fPrefix + "/";
   if (!fSubPrefixD.empty()) hname += fSubPrefixD;
   hname += fPrefix + "_";
   if (!fSubPrefixN.empty()) hname += fSubPrefixN;
   hname.append(name);

   std::string htitle = fName;
   htitle.append(" ");
   if (!fSubPrefixN.empty()) htitle += fSubPrefixN.substr(0, fSubPrefixN.length()-1) + " ";
   htitle.append(title);

   return mgr()->MakeH2(hname.c_str(), htitle.c_str(), nbins1, left1, right1, nbins2, left2, right2, options);
}


base::C1handle base::Processor::MakeC1(const char* name, double left, double right, H1handle h1)
{
   if (mgr()==0) return 0;

   std::string cname = fPrefix + "/";
   if (!fSubPrefixD.empty()) cname += fSubPrefixD;
   cname += fPrefix + "_";
   if (!fSubPrefixN.empty()) cname += fSubPrefixN;
   cname.append(name);

   return mgr()->MakeC1(cname.c_str(), left, right, h1);
}


void base::Processor::ChangeC1(C1handle c1, double left, double right)
{
   if (mgr()) mgr()->ChangeC1(c1, left, right);
}

double base::Processor::GetC1Limit(C1handle c1, bool isleft)
{
   return mgr() ? mgr()->GetC1Limit(c1, isleft) : 0.;
}
