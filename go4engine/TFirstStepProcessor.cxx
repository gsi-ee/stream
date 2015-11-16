#include "TFirstStepProcessor.h"

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
#include "TUrl.h"

#include "TGo4WinCond.h"
#include "TGo4MbsEvent.h"
#include "TGo4MbsSubEvent.h"
#include "TGo4Parameter.h"
#include "TGo4Picture.h"
#include "TGo4Analysis.h"

#include "TStreamEvent.h"

#include "TGo4EventErrorException.h"


TFirstStepProcessor::TFirstStepProcessor():
   TGo4EventProcessor(),
   TRootProcMgr()
{
   // streamer dummy constructor
}


TFirstStepProcessor::TFirstStepProcessor(const char* name) :
   TGo4EventProcessor(name),
   TRootProcMgr()
{
   TGo4Log::Info("Create TFirstStepProcessor %s", name);

   if (gSystem->AccessPathName("first.C") == 0) {
      if (ExecuteScript("first.C") == -1) {
         TGo4Log::Error("Cannot setup analysis with first.C script");
         throw TGo4EventErrorException(this);
      }
   }

   if (gSystem->AccessPathName("second.C") == 0) {

      if (gSystem->Getenv("STREAMSYS")==0) {
         TGo4Log::Error("STREAMSYS shell variable not configured");
         throw TGo4EventErrorException(this);
      }

      gROOT->ProcessLine(".include $STREAMSYS/include");
      gROOT->ProcessLine(".include $GO4SYS/include");

      if (ExecuteScript("second.C+") == -1) {
         TGo4Log::Error("Cannot setup analysis with second.C script");
         throw TGo4EventErrorException(this);
      }
   }

   fTotalDataSize = 0;
   fNumInpBufs = 0;
   fNumOutEvents = 0;
}

TFirstStepProcessor::~TFirstStepProcessor()
{
   TGo4Log::Info("Input %ld  Output %ld  Total processed size = %ld", fNumInpBufs, fNumOutEvents, fTotalDataSize);
}


void TFirstStepProcessor::UserPreLoop()
{
   TRootProcMgr::UserPreLoop();

   // register tree in Go4 browser
   if (fTree!=0) TGo4Analysis::Instance()->AddTree(fTree);
}

void TFirstStepProcessor::UserPostLoop()
{
   if (fTree!=0) TGo4Analysis::Instance()->RemoveTree(fTree);

   TRootProcMgr::UserPostLoop();
}


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

         buf.makecopyof(psubevt->GetDataField(), psubevt->GetByteLen());

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

base::H1handle TFirstStepProcessor::MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle)
{
   // we will use xtitle arguments to deliver different optional arguments
   // syntax will be like: arg_name:arg_value;arg2_name:arg2_value;
   // For instance, labels for each bin:
   //      xbin:EPOCH,HIT,SYNC,AUX,,,SYS;

   TString newxtitle, newytitle;
   TString xbins;

   char kind = 'I';
   Bool_t useexisting = kFALSE;

   TObjArray* arr = (xtitle!=0) && strlen(xtitle)!=0 ? TString(xtitle).Tokenize(";") : 0;

   for (int n=0; n <= (arr ? arr->GetLast() : -1); n++) {
      TString part = arr->At(n)->GetName();
      if (part.Index("xbin:")==0) { xbins = part; xbins.Remove(0, 5); } else
      if (part.Index("kind:")==0) { kind = part[5]; } else
      if (part.Index("reuse")==0) { useexisting = kTRUE; } else
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

   return (base::H1handle) histo1;
}

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
   Bool_t useexisting = kFALSE;

   for (int n=0; n<= (arr ? arr->GetLast() : -1);n++) {
      TString part = arr->At(n)->GetName();
      if (part.Index("xbin:")==0) { xbins = part; xbins.Remove(0, 5); } else
      if (part.Index("ybin:")==0) { ybins = part; ybins.Remove(0, 5); } else
      if (part.Index("kind:")==0) { kind = part[5]; } else
      if (part.Index("reuse")==0) { useexisting = kTRUE; } else
      if (xtitle.Length()==0) xtitle = part;
                         else ytitle = part;
   }
   delete arr;

   SetMakeWithAutosave(useexisting);

   TH2* histo2 = MakeTH2(kind, name, title, nbins1, left1, right1, nbins2, left2, right2, xtitle.Data(), ytitle.Data());

   if (xbins.Length()>0) {
      arr = xbins.Tokenize(",");
      for (int n=0; n<=(arr ? arr->GetLast():-1); n++)
         histo2->GetXaxis()->SetBinLabel(1 + n, arr->At(n)->GetName());
      delete arr;
   }

   if (ybins.Length()>0) {
      arr = ybins.Tokenize(",");
      for (int n=0; n<=(arr ? arr->GetLast():-1); n++)
         histo2->GetYaxis()->SetBinLabel(1 + n, arr->At(n)->GetName());
      delete arr;
   }

   return (base::H2handle) histo2;
}

void TFirstStepProcessor::FillH1(base::H1handle h1, double x, double weight)
{
   if (h1==0) return;

   TH1* histo1 = (TH1*) h1;

   if (weight!=1.)
      histo1->Fill(x, weight);
   else
      histo1->Fill(x);
}

double TFirstStepProcessor::GetH1Content(base::H1handle h1, int nbin)
{
   if (h1==0) return 0.;

   TH1* histo1 = (TH1*) h1;

   return histo1->GetBinContent(nbin+1);
}

void TFirstStepProcessor::ClearH1(base::H1handle h1)
{
   if (h1==0) return;

   TH1* histo1 = (TH1*) h1;
   histo1->Reset();
}


void TFirstStepProcessor::FillH2(base::H1handle h2, double x, double y, double weight)
{
   if (h2==0) return;

   TH2* histo2 = (TH2*) h2;

   if (weight!=1.)
      histo2->Fill(x, y, weight);
   else
      histo2->Fill(x, y);
}

void TFirstStepProcessor::ClearH2(base::H2handle h2)
{
   if (h2==0) return;

   TH2* histo2 = (TH2*) h2;
   histo2->Reset();
}


base::C1handle TFirstStepProcessor::MakeC1(const char* name, double left, double right, base::H1handle h1)
{
   TH1* histo1 = (TH1*) h1;

   TGo4WinCond* cond = MakeWinCond(name, left, right, histo1 ? histo1->GetName() : 0);

   return (base::C1handle) cond;
}

void TFirstStepProcessor::ChangeC1(base::C1handle c1, double left, double right)
{
   TGo4WinCond* cond = (TGo4WinCond*) c1;
   if (cond) cond->SetValues(left, right);
}

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

double TFirstStepProcessor::GetC1Limit(base::C1handle c1, bool isleft)
{
   TGo4WinCond* cond = (TGo4WinCond*) c1;
   if (cond==0) return 0.;
   return isleft ? cond->GetXLow() : cond->GetXUp();
}

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
