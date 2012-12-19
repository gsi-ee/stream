#include "TFirstStepProcessor.h"

#include "TGo4Log.h"
#include "TH1.h"
#include "TH2.h"
#include "TObjArray.h"

#include "TGo4WinCond.h"
#include "TGo4MbsEvent.h"
#include "TGo4MbsSubEvent.h"

#include "go4/TStreamEvent.h"

#include "TGo4EventErrorException.h"
//#include "TGo4EventTimeoutException.h"
//#include "TGo4EventEndException.h"


TString TFirstStepProcessor::fDfltSetupScript = "first.C";


TFirstStepProcessor::TFirstStepProcessor():
   TGo4EventProcessor(),
   base::ProcMgr()
{
} // streamer dummy


TFirstStepProcessor::TFirstStepProcessor(const char* name) :
   TGo4EventProcessor(name),
   base::ProcMgr()
{
   TGo4Log::Info("Create TFirstStepProcessor %s", name);

   if (ExecuteScript(fDfltSetupScript.Data()) == -1) {
      TGo4Log::Error("Cannot setup analysis with %s script", fDfltSetupScript.Data());
      throw TGo4EventErrorException(this);
   }

   fTotalDataSize = 0;
   fNumMbsBufs = 0;
   fNumCbmEvents = 0;
}

TFirstStepProcessor::~TFirstStepProcessor()
{
   TGo4Log::Info("Input %ld  Output %ld  Total processed size = %ld", fNumMbsBufs, fNumCbmEvents, fTotalDataSize);
}

Bool_t TFirstStepProcessor::BuildEvent(TGo4EventElement* outevnt)
{
//   TGo4Log::Info("Start processing!!!");

   TGo4MbsEvent* mbsev = (TGo4MbsEvent*) GetInputEvent();
   base::Event* cbmev = (TStreamEvent*) outevnt;

   if ((mbsev==0) || (cbmev==0))
      throw TGo4EventErrorException(this);

   if (!IsKeepInputEvent()) {
      // if keep input was not specified before,
      // framework has delivered new MBS event, it should be processed

      // TGo4Log::Info("Accept new event %d", mbsev->GetIntLen()*4);

      fTotalDataSize += mbsev->GetIntLen()*4;
      fNumMbsBufs++;

      TGo4MbsSubEvent* psubevt = 0;
      mbsev->ResetIterator();
      while((psubevt = mbsev->NextSubEvent()) != 0) {
         // loop over subevents

         base::Buffer buf;

         buf.makecopyof(psubevt->GetDataField(), psubevt->GetByteLen());

         buf().kind = psubevt->GetProcid();
         buf().boardid = psubevt->GetSubcrate();
         buf().format = psubevt->GetControl();

         //      TGo4Log::Info("  find subevent kind %2u brd %2u fmt %u len %d", buf().kind, buf().boardid, buf().format, buf().datalen);

         ProvideRawData(buf);
      }

      //TGo4Log::Info("Start scanning");

      // scan new data
      ScanNewData();

      if (IsRawAnalysis()) {

         //TGo4Log::Info("Skip data");

         SkipAllData();

      } else {

         //TGo4Log::Info("Analyze data");

         // analyze new sync markers
         if (AnalyzeSyncMarkers()) {

            printf("Collect new triggers\n");

            // get and redistribute new triggers
            CollectNewTriggers();

            // scan for new triggers
            ScanDataForNewTriggers();
         }
      }
   } else {
    //  TGo4Log::Info("Keep event %d", mbsev->GetIntLen()*4);
   }

   if (ProduceNextEvent(cbmev)) {

      // TGo4Log::Info("Produce event");

      SetKeepInputEvent(kTRUE);
      outevnt->SetValid(kTRUE);

      fNumCbmEvents++;

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

   TString newxtitle(xtitle ? xtitle : "");
   TString xbins;

   TObjArray* arr = newxtitle.Length() > 0 ? newxtitle.Tokenize(";") : 0;

   for (int n=0; n<= (arr ? arr->GetLast() : -1);n++) {
      TString part = arr->At(n)->GetName();
      if (part.Index("xbin:")==0) { xbins = part; xbins.Remove(0, 5); }
       else newxtitle = part;
   }
   delete arr;

   TH1* histo1 = MakeTH1('I', name, title, nbins, left, right, newxtitle.Data());

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

   for (int n=0; n<= (arr ? arr->GetLast() : -1);n++) {
      TString part = arr->At(n)->GetName();
      if (part.Index("xbin:")==0) { xbins = part; xbins.Remove(0, 5); } else
      if (part.Index("ybin:")==0) { ybins = part; ybins.Remove(0, 5); } else
      if (xtitle.Length()==0) xtitle = part;
                         else ytitle = part;
   }
   delete arr;

   TH2* histo2 = MakeTH2('I', name, title, nbins1, left1, right1, nbins2, left2, right2, xtitle.Data(), ytitle.Data());

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

void TFirstStepProcessor::FillH2(base::H1handle h2, double x, double y, double weight)
{
   if (h2==0) return;

   TH2* histo2 = (TH2*) h2;

   if (weight!=1.)
      histo2->Fill(x, y, weight);
   else
      histo2->Fill(x, y);
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
