#include "TFirstStepProcessor.h"

#include <fstream>
#include <iterator>
#include <regex>

#include "TGo4Log.h"
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TObjArray.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TTimer.h"
#include "TSystem.h"
#include "TInterpreter.h"
#include "TUrl.h"

#include "TGo4WinCond.h"
#include "TGo4MbsEvent.h"
#include "TGo4MbsSubEvent.h"
#include "TGo4Parameter.h"
#include "TGo4Picture.h"
#include "TGo4Status.h"
#include "TGo4Analysis.h"

#include "TStreamEvent.h"

#include "TGo4EventErrorException.h"

//////////////////////////////////////////////////////////////////////////////////////////////
/// constructor

TFirstStepProcessor::TFirstStepProcessor():
   TGo4EventProcessor(),
   TRootProcMgr()
{
   // streamer dummy constructor
}

//////////////////////////////////////////////////////////////////////////////////////////////
///  constructor

TFirstStepProcessor::TFirstStepProcessor(const char* name) :
   TGo4EventProcessor(name),
   TRootProcMgr()
{
   TGo4Log::Info("Create TFirstStepProcessor %s", name);

   SetSortedOrder(true);

   std::string fcode = ReadMacroCode("first.C");
   if (fcode.empty()) {
      TGo4Log::Error("Cannot load content of first.C script");
      throw TGo4EventErrorException(this);
   }

   static std::string gFirstCode;
   static std::string gFirstName;
   static std::string gSecondCode;
   static std::string gSecondName;
   static int gFirstCnt = 0;

   if(gFirstCode.empty()) {
      gFirstCode = fcode;
      gFirstName = "first";
      if (!gInterpreter->LoadText(gFirstCode.c_str())) {
         TGo4Log::Error("Cannot parse code of first.C script");
         throw TGo4EventErrorException(this);
      }
   } else if (gFirstCode != fcode) {
      // load modified function
      gFirstName = std::string("first") + std::to_string(gFirstCnt++);
      gFirstCode = std::regex_replace(fcode, std::regex("first"), gFirstName);
      gFirstCode = std::regex_replace(gFirstCode, std::regex("after_create"), std::string("after_create_") + std::to_string(gFirstCnt));
      if (!gInterpreter->LoadText(gFirstCode.c_str())) {
         TGo4Log::Error("Cannot parse code of %s.C script", gFirstName.c_str());
         throw TGo4EventErrorException(this);
      }
   }

   TString exec = TString::Format("%s()", gFirstName.c_str());
   TGo4Log::Info("Execute first.C script with command %s", exec.Data());
   gROOT->ProcessLine(exec.Data());

   std::string second_name = GetSecondName();
   if (!second_name.empty() && (gSystem->AccessPathName(second_name.c_str()) == 0)) {

      std::string scode = ReadMacroCode(second_name), exec2, func;
      if (scode.empty()) {
         TGo4Log::Error("Cannot load content of %s script", second_name.c_str());
         throw TGo4EventErrorException(this);
      }

      if (gSecondCode.empty()) {

         if (gSystem->Getenv("STREAMSYS")==0) {
            TGo4Log::Error("STREAMSYS shell variable not configured");
            throw TGo4EventErrorException(this);
         }

         gROOT->ProcessLine(".include $STREAMSYS/include");
         gROOT->ProcessLine(".include $GO4SYS/include");

         gSecondName = second_name;
         gSecondCode = scode;

         exec2 = second_name + "+";
      } else if (gSecondName == second_name) {
         if (gSecondCode != scode) {
            TGo4Log::Error("Not allowed to change content of %s file", gSecondName.c_str());
            throw TGo4EventErrorException(this);
         } else {
            std::string::size_type pos = second_name.find(".");
            func = second_name.substr(0,pos);
            pos = func.rfind("/");
            if (pos != std::string::npos) func = func.substr(pos+1, func.length()-pos-1);
         }
      } else {
         gSecondName = second_name;
         gSecondCode = scode;
         exec2 = second_name + "+";
      }

      if (!func.empty()) {
         TString func_exec = TString::Format("%s()", func.c_str());
         TGo4Log::Info("Execute function %s", func_exec.Data());
         gROOT->ProcessLine(func_exec.Data());
      } else if (ExecuteScript(exec2.c_str()) == -1) {
         TGo4Log::Error("Cannot setup analysis with %s script", second_name.c_str());
         throw TGo4EventErrorException(this);
      }
   }

   fTotalDataSize = 0;
   fNumInpBufs = 0;
   fNumOutEvents = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// destructor

TFirstStepProcessor::~TFirstStepProcessor()
{
   TGo4Log::Info("Input %ld  Output %ld  Total processed size = %ld", fNumInpBufs, fNumOutEvents, fTotalDataSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// read macro code

std::string TFirstStepProcessor::ReadMacroCode(const std::string &fname)
{
   std::ifstream t(fname);
   std::string str((std::istreambuf_iterator<char>(t)), {});
   return str;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// pre loop

void TFirstStepProcessor::UserPreLoop()
{
   TRootProcMgr::UserPreLoop();

   // register tree in Go4 browser
   if (fTree!=0) TGo4Analysis::Instance()->AddTree(fTree);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// post loop

void TFirstStepProcessor::UserPostLoop()
{
   if (fTree!=0) TGo4Analysis::Instance()->RemoveTree(fTree);

   TRootProcMgr::UserPostLoop();
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// build event - main entry point

Bool_t TFirstStepProcessor::BuildEvent(TGo4EventElement* outevnt)
{
//   TGo4Log::Info("Start processing!!!");

   gSystem->ProcessEvents();

   TGo4MbsEvent* mbsev = (TGo4MbsEvent*) GetInputEvent();
   base::Event* event = (TStreamEvent*) outevnt;

   if ((mbsev==0) || (event==0))
      throw TGo4EventErrorException(this);

   Bool_t filled_event = kFALSE;

   if (!IsKeepInputEvent()) {
      // if keep input was not specified before,
      // framework has delivered new MBS event, it should be processed

      // TGo4Log::Info("Accept new event %d", mbsev->GetIntLen()*4);

      fTotalDataSize += mbsev->GetIntLen()*4;
      fNumInpBufs++;

      TGo4MbsSubEvent* psubevt = 0;
      mbsev->ResetIterator();
      while((psubevt = mbsev->NextSubEvent()) != 0) {
         // loop over subevents

         base::Buffer buf;

         buf.makereferenceof(psubevt->GetDataField(), psubevt->GetByteLen());

         buf().kind = psubevt->GetProcid();
         buf().boardid = psubevt->GetSubcrate();
         buf().format = psubevt->GetControl();

//         TGo4Log::Info("  find subevent kind %2u brd %2u fmt %u len %d", buf().kind, buf().boardid, buf().format, buf().datalen);

         TRootProcMgr::ProvideRawData(buf);
      }

      //TGo4Log::Info("Start scanning");

      filled_event = TRootProcMgr::AnalyzeNewData(event);
   } else {
    //  TGo4Log::Info("Keep event %d", mbsev->GetIntLen()*4);
   }

   if (!filled_event)
      filled_event = TRootProcMgr::ProduceNextEvent(event);

   if (filled_event) {

      Bool_t store = TRootProcMgr::ProcessEvent(event);

      // only for stream analysis we need possibility to produce as much events as possible
      if (TRootProcMgr::IsStreamAnalysis())
         SetKeepInputEvent(kTRUE);

      // printf("Store event %s\n", store ? "true" : "false");

      outevnt->SetValid(store);

      fNumOutEvents++;

      return kTRUE;
   }

   SetKeepInputEvent(kFALSE);

   outevnt->SetValid(kFALSE);

   return kFALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// set sorted order for created histogram folders

void TFirstStepProcessor::SetSortedOrder(bool on)
{
   TGo4Analysis::Instance()->SetSortedOrder(on);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// is sorted order for created histogram folders

bool TFirstStepProcessor::IsSortedOrder()
{
   return TGo4Analysis::Instance()->IsSortedOrder();
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// make H1
/// see \ref base::ProcMgr::MakeH1

base::H1handle TFirstStepProcessor::MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle)
{
   // we will use xtitle arguments to deliver different optional arguments
   // syntax will be like: arg_name:arg_value;arg2_name:arg2_value;
   // For instance, labels for each bin:
   //      xbin:EPOCH,HIT,SYNC,AUX,,,SYS;

   TString newxtitle, newytitle;
   TString xbins;

   char kind = 'I';
   Bool_t useexisting = kFALSE, clear_protect = kFALSE;

   TObjArray* arr = xtitle && strlen(xtitle) ? TString(xtitle).Tokenize(";") : nullptr;

   for (int n=0; n <= (arr ? arr->GetLast() : -1); n++) {
      TString part = arr->At(n)->GetName();
      if (part.Index("xbin:")==0) { xbins = part; xbins.Remove(0, 5); } else
      if (part.Index("kind:")==0) { kind = part[5]; } else
      if (part.Index("reuse")==0) { useexisting = kTRUE; } else
      if (part.Index("clear_protect")==0) { clear_protect = kTRUE; } else
      if (newxtitle.Length()>0) newytitle = part;
                           else newxtitle = part;
   }
   delete arr;

   SetMakeWithAutosave(useexisting);

   TH1* histo1 = MakeTH1(kind, name, title, nbins, left, right, newxtitle.Data(), newytitle.Data());

   if (xbins.Length()>0) {
      arr = xbins.Tokenize(",");
      for (int n=0; n<=(arr ? arr->GetLast() : -1);n++)
         histo1->GetXaxis()->SetBinLabel(1 + n, arr->At(n)->GetName());
      delete arr;
   }

   if (clear_protect) histo1->SetBit(TGo4Status::kGo4NoReset);

   return (base::H1handle) histo1;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// Get H1 bins numbers

bool TFirstStepProcessor::GetH1NBins(base::H1handle h1, int &nbins)
{
   if (h1==0) return false;

   TH1* histo1 = (TH1*) h1;
   nbins = histo1->GetNbinsX();
   return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// fill H1

void TFirstStepProcessor::FillH1(base::H1handle h1, double x, double weight)
{
   if (h1==0) return;

   TH1* histo1 = (TH1*) h1;

   if (weight!=1.)
      histo1->Fill(x, weight);
   else
      histo1->Fill(x);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// get h1 content

double TFirstStepProcessor::GetH1Content(base::H1handle h1, int nbin)
{
   if (h1==0) return 0.;

   TH1* histo1 = (TH1*) h1;

   return histo1->GetBinContent(nbin+1);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// set h1 content

void TFirstStepProcessor::SetH1Content(base::H1handle h1, int nbin, double v)
{
   TH1* histo1 = (TH1*) h1;

   if (histo1) histo1->SetBinContent(nbin+1, v);
}


//////////////////////////////////////////////////////////////////////////////////////////////
/// clear h1

void TFirstStepProcessor::ClearH1(base::H1handle h1)
{
   if (!h1) return;

   TH1* histo1 = (TH1*) h1;
   histo1->Reset();
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// copy h1

void TFirstStepProcessor::CopyH1(base::H1handle tgt, base::H1handle src)
{
   if (!tgt || !src) return;

   TH1 *htgt = (TH1*) tgt;
   TH1 *hsrc = (TH1*) src;
   htgt->Reset();
   htgt->Add(hsrc);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// set h1 title

void TFirstStepProcessor::SetH1Title(base::H1handle h1, const char* title)
{
   if (!h1) return;

   TH1* histo1 = (TH1*) h1;
   histo1->SetTitle(title);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// make H2
/// see \ref base::ProcMgr::MakeH2

base::H2handle TFirstStepProcessor::MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options)
{
   // we will use title arguments to deliver different optional arguments
   // syntax will be like: arg_name:arg_value;arg2_name:arg2_value;
   // For instance, labels for each bin:
   //      xbin:EPOCH,HIT,SYNC,AUX,,,SYS;
   // Default arguments are xtitle;ytitle

   TString xbins, ybins, xtitle, ytitle;

   TObjArray* arr = (options && *options) ? TString(options).Tokenize(";") : 0;

   char kind = 'I';
   Bool_t useexisting = kFALSE, clear_protect = kFALSE;

   for (int n = 0; n <= (arr ? arr->GetLast() : -1); n++) {
      TString part = arr->At(n)->GetName();
      if (part.Index("xbin:")==0) { xbins = part; xbins.Remove(0, 5); } else
      if (part.Index("ybin:")==0) { ybins = part; ybins.Remove(0, 5); } else
      if (part.Index("kind:")==0) { kind = part[5]; } else
      if (part.Index("reuse")==0) { useexisting = kTRUE; } else
      if (part.Index("clear_protect")==0) { clear_protect = kTRUE; } else
      if (xtitle.Length()==0) xtitle = part;
                         else ytitle = part;
   }
   delete arr;

   SetMakeWithAutosave(useexisting);

   TH2* histo2 = MakeTH2(kind, name, title, nbins1, left1, right1, nbins2, left2, right2, xtitle.Data(), ytitle.Data());

   if (xbins.Length() > 0) {
      arr = xbins.Tokenize(",");
      for (int n=0; n<=(arr ? arr->GetLast():-1); n++)
         histo2->GetXaxis()->SetBinLabel(1 + n, arr->At(n)->GetName());
      delete arr;
   }

   if (ybins.Length() > 0) {
      arr = ybins.Tokenize(",");
      for (int n=0; n<=(arr ? arr->GetLast():-1); n++)
         histo2->GetYaxis()->SetBinLabel(1 + n, arr->At(n)->GetName());
      delete arr;
   }

   if (clear_protect) histo2->SetBit(TGo4Status::kGo4NoReset);

   return (base::H2handle) histo2;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// get h2 bins number

bool TFirstStepProcessor::GetH2NBins(base::H2handle h2, int &nbins1, int &nbins2)
{
   if (h2==0) return false;
   TH2* histo2 = (TH2*) h2;
   nbins1 = histo2->GetNbinsX();
   nbins2 = histo2->GetNbinsY();
   return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// fill h2

void TFirstStepProcessor::FillH2(base::H2handle h2, double x, double y, double weight)
{
   if (h2==0) return;

   TH2* histo2 = (TH2*) h2;

   if (weight != 1.)
      histo2->Fill(x, y, weight);
   else
      histo2->Fill(x, y);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// get h2 content

double TFirstStepProcessor::GetH2Content(base::H2handle h2, int bin1, int bin2)
{
   if (h2==0) return 0.;

   TH2* histo2 = (TH2*) h2;

   return histo2->GetBinContent(bin1+1, bin2+1);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// set h2 content

void TFirstStepProcessor::SetH2Content(base::H2handle h2, int bin1, int bin2, double v)
{
   if (h2==0) return;

   TH2* histo2 = (TH2*) h2;

   return histo2->SetBinContent(bin1+1, bin2+1, v);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// clear h2

void TFirstStepProcessor::ClearH2(base::H2handle h2)
{
   if (h2==0) return;

   TH2* histo2 = (TH2*) h2;
   histo2->Reset();
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// set h2 title

void TFirstStepProcessor::SetH2Title(base::H2handle h2, const char* title)
{
   if (!h2) return;

   TH2* histo2 = (TH2*) h2;
   histo2->SetTitle(title);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// clear all histograms

void TFirstStepProcessor::ClearAllHistograms()
{
   TGo4Analysis* an = TGo4Analysis::Instance();
   if (an)
      an->ClearObjects("Histograms");
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// make condition

base::C1handle TFirstStepProcessor::MakeC1(const char* name, double left, double right, base::H1handle h1)
{
   TH1* histo1 = (TH1*) h1;

   TGo4WinCond* cond = MakeWinCond(name, left, right, histo1 ? histo1->GetName() : 0);

   return (base::C1handle) cond;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// change condition

void TFirstStepProcessor::ChangeC1(base::C1handle c1, double left, double right)
{
   TGo4WinCond* cond = (TGo4WinCond*) c1;
   if (cond) cond->SetValues(left, right);
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// test condition

int TFirstStepProcessor::TestC1(base::C1handle c1, double value, double* dist)
{
   TGo4WinCond* cond = (TGo4WinCond*) c1;
   if (dist) *dist = 0.;
   if (cond==0) return 0;
   if (value < cond->GetXLow()) {
      if (dist) *dist = value - cond->GetXLow();
      return -1;
   } else
   if (value > cond->GetXUp()) {
      if (dist) *dist = value - cond->GetXUp();
      return 1;
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// get condition limit

double TFirstStepProcessor::GetC1Limit(base::C1handle c1, bool isleft)
{
   TGo4WinCond* cond = (TGo4WinCond*) c1;
   if (cond==0) return 0.;
   return isleft ? cond->GetXLow() : cond->GetXUp();
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// register object

bool TFirstStepProcessor::RegisterObject(TObject* tobj, const char* subfolder)
{
   if (tobj==0) return false;

   if (tobj->InheritsFrom(TH1::Class()))
      TGo4Analysis::Instance()->AddHistogram((TH1*)tobj, subfolder);

   if (tobj->InheritsFrom(TTree::Class()))
      TGo4Analysis::Instance()->AddTree((TTree*)tobj);

   if (tobj->InheritsFrom(TGo4Condition::Class()))
      TGo4Analysis::Instance()->AddAnalysisCondition((TGo4Condition*)tobj, subfolder);

   if (tobj->InheritsFrom(TGo4Parameter::Class()))
      TGo4Analysis::Instance()->AddParameter((TGo4Parameter*)tobj, subfolder);

   if (tobj->InheritsFrom(TGo4Picture::Class()))
      TGo4Analysis::Instance()->AddPicture((TGo4Picture*)tobj, subfolder);

   if (tobj->InheritsFrom(TCanvas::Class()))
      TGo4Analysis::Instance()->AddCanvas((TCanvas*)tobj, subfolder);

   if (tobj->InheritsFrom(TNamed::Class()))
      return TGo4Analysis::Instance()->AddObject((TNamed*)tobj, subfolder);

   return false;
}

// =============================================================================

/*

// unfortunately, this does not work
// one should configure include path BEFORE loading user library

class StreamLoadHandler {
public:
   StreamLoadHandler()
   {
      const char *stream_sys = gSystem->Getenv("STREAMSYS");
      if (stream_sys && *stream_sys) {
         TString path = stream_sys;
         if (path[path.Length()-1]!='/') path.Append("/");
         path.Append("include/");
         printf("ADD INCLUDE PATH %s\n",path.Data());
         gInterpreter->AddIncludePath(path.Data());
      }
   }
};

static StreamLoadHandler load_handler;

*/
