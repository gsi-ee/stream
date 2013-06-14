#include "TGet4TestProc.h"

#include "TH1.h"

#include "TGo4Log.h"

#include "get4/SubEvent.h"

#include "go4/TStreamEvent.h"

TGet4TestProc::TGet4TestProc() :
   TUserProcessor()
{
}

TGet4TestProc::TGet4TestProc(const char* name) :
   TUserProcessor(name)
{
   fMultipl = MakeTH1('I', "GET4TEST/Get4TestMultipl", "Number of messages in event", 32, 0., 32.);
   fEvPerCh = 0;
   fBadPerCh = 0;
}


void TGet4TestProc::Add(unsigned rocid, unsigned get4id, unsigned chid)
{
   unsigned id = code(rocid, get4id, chid);

   TGet4TestRec& rec = fMap[id];

   rec.rocid = rocid;
   rec.get4id = get4id;
   rec.chid = chid;

   TString prefix = Form("GET4TEST/ROC%u_GET%u_CH%u/HROC%u_GET%u_CH%u_", rocid, get4id, chid, rocid, get4id, chid);
   TString info = Form("ROC%u GET4 %u Channel %u", rocid, get4id, chid);

   rec.fWidth = MakeTH1('I', (prefix+"Width").Data(), (TString("Signal width on ") + info).Data(), 4000, -2000*50., 2000*50., "ps");
   rec.fDiffRising = MakeTH1('I', (prefix+"DiffRising").Data(), (TString("Rising difference on ") + info).Data(), 4000, -2000*50., 2000*50., "ps");
}

void TGet4TestProc::MakeHistos()
{
   if ((fEvPerCh!=0) || (fMap.size()==0)) return;

   fEvPerCh = MakeTH1('I', "GET4TEST/EvPerCh", "Number of good events per channel", fMap.size(), 0, fMap.size());
   fBadPerCh = MakeTH1('I', "GET4TEST/BadPerCh", "Number of bad events per channel", fMap.size(), 0, fMap.size());
   fHitPerCh = MakeTH1('I', "GET4TEST/HitPerCh", "Number of hits per channel", fMap.size(), 0, fMap.size());

   unsigned n = 0;
   for (TGet4TestMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      TString lbl = Form("%u_%u_%u", iter->second.rocid, iter->second.get4id, iter->second.chid);
      fEvPerCh->GetXaxis()->SetBinLabel(1 + n, lbl.Data());
      fBadPerCh->GetXaxis()->SetBinLabel(1 + n, lbl.Data());
      fHitPerCh->GetXaxis()->SetBinLabel(1 + n, lbl.Data());
      n++;
   }

}

void TGet4TestProc::Process(TStreamEvent* ev)
{

//   printf("new event\n");

   for (TGet4TestMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
      iter->second.reset();
   }

    get4::SubEvent* sub0 = dynamic_cast<get4::SubEvent*> (ev->GetSubEvent("ROC0"));

    if (sub0!=0) {
       //TGo4Log::Info("Find GET4 data for ROC0 size %u  trigger: %10.9f", sub0->Size(), ev->GetTriggerTime());
       fMultipl->Fill(sub0->Size());

       TGet4TestMap::iterator iter;

       for (unsigned cnt=0;cnt<sub0->Size();cnt++) {
          const get4::Message& msg = sub0->msg(cnt).msg();

          bool is32bit = msg.is32bitHit();

          unsigned indx = is32bit ? code (0, msg.getGet4V10R32ChipId(), msg.getGet4V10R32HitChan()) :
                  code(0, msg.getGet4Number(), msg.getGet4ChNum()) ;

          iter = fMap.find(indx);
          if (iter == fMap.end()) continue;

          TGet4TestRec& rec = iter->second;


          if (is32bit) {
             rec.hitcnt+=2;
             rec.set_edge(1, msg.getGet4V10R32HitTimeBin());
             rec.set_edge(0, msg.getGet4V10R32HitTimeBin() + msg.getGet4V10R32HitTot());
          } else {
             rec.hitcnt++;
             rec.set_edge(msg.getGet4Edge(), msg.getGet4Ts());
          }

          double width = rec.calc_width();
          if (width!=0.) rec.fWidth->Fill(width);
       }

    } else {
       TGo4Log::Error("Not found GET4 data for ROC0 in event %10.9f s", ev->GetTriggerTime());
    }

    for (TGet4TestMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {

       TGet4TestMap::iterator next = iter;
       next++;
       if (next == fMap.end()) next = fMap.begin();

       TGet4TestRec& rec1 = iter->second;
       TGet4TestRec& rec2 = next->second;

       if (rec1.isrising && rec2.isrising) {

          double diff = rec2.lastrising*50. - rec1.lastrising*50.;

          rec1.fDiffRising->Fill(diff);
       }
    }

    unsigned indx=0;
    if (fEvPerCh && fBadPerCh)
       for (TGet4TestMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
          TGet4TestRec& rec = iter->second;

          // TODO: once can configure more complex conditions
          if (rec.hitcnt == 2)
             fEvPerCh->Fill(indx);
          else
             fBadPerCh->Fill(indx);

          if (fHitPerCh) fHitPerCh->Fill(indx, rec.hitcnt);

          indx++;
       }

}
