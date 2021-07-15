#include <cstdio>

#include "base/EventProc.h"
#include "base/Event.h"
#include "base/SubEvent.h"

#include "get4/SubEvent.h"

#include "TString.h"

#include <map>

struct TGet4TestRec {
   unsigned rocid;
   unsigned get4id;
   unsigned chid;

   base::H1handle fWidth;
   base::H1handle fDiffRising;

   unsigned lastrising;
   unsigned lastfalling;
   unsigned hitcnt;

   bool isrising;
   bool isfalling;

   TGet4TestRec() : rocid(0), get4id(0), chid(0), fWidth(0), fDiffRising(0) {}

   void reset()
   {
      hitcnt = 0;
      resetedges();
   }

   void resetedges()
   {
      lastrising = 0; isrising = false;
      lastfalling = 0; isfalling = false;
   }

   void set_edge(unsigned edge, unsigned ts)
   {

      if (isrising && isfalling) resetedges();

      if (edge == 1) {
         lastrising = ts;
         isrising = true;
      } else {
         lastfalling = ts;
         isfalling = true;
      }
   }

   double calc_width()
   {
      if (!isrising || !isfalling) return 0.;

      return (lastrising < lastfalling) ? (lastfalling - lastrising)*50. : (lastrising - lastfalling)*-50.;
   }

};

typedef std::map<unsigned,TGet4TestRec> TGet4TestMap;


class TGet4TestProc : public base::EventProc {
   protected:

      base::H1handle fMultipl;   ///<!
      base::H1handle fEvPerCh;   ///<!  events per channel
      base::H1handle fBadPerCh;  ///<!  bad events per channel
      base::H1handle fHitPerCh;  ///<!  bad events per channel

      TGet4TestMap fMap; ///<! all channels which should be present

   public:

      TGet4TestProc(const char* name) :
         base::EventProc(name)
      {
         fMultipl = MakeH1("GET4TEST/Get4TestMultipl", "Number of messages in event", 32, 0., 32.);
         fEvPerCh = 0;
         fBadPerCh = 0;
         fHitPerCh = 0;
      }

      inline unsigned code(unsigned rocid, unsigned get4id, unsigned chid) const
      { return rocid*1000 + get4id*10 + chid; }

      void Add(unsigned rocid, unsigned get4id)
      {
         for (unsigned indx=0;indx<4;indx++)
            Add(rocid, get4id, indx);
      }

      /** Register specific get4 channel for testing */
      void Add(unsigned rocid, unsigned get4id, unsigned chid)
      {
         unsigned id = code(rocid, get4id, chid);

         TGet4TestRec& rec = fMap[id];

         rec.rocid = rocid;
         rec.get4id = get4id;
         rec.chid = chid;

         TString prefix = Form("GET4TEST/ROC%u_GET%u_CH%u/HROC%u_GET%u_CH%u_", rocid, get4id, chid, rocid, get4id, chid);
         TString info = Form("ROC%u GET4 %u Channel %u", rocid, get4id, chid);

         rec.fWidth = MakeH1((prefix+"Width").Data(), (TString("Signal width on ") + info).Data(), 4000, -2000*50., 2000*50., "ps");
         rec.fDiffRising = MakeH1((prefix+"DiffRising").Data(), (TString("Rising difference on ") + info).Data(), 4000, -2000*50., 2000*50., "ps");
      }

      void MakeHistos()
      {
         if ((fEvPerCh!=0) || (fMap.size()==0)) return;

         TString xbin;
         for (TGet4TestMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
            TString lbl = Form("%u_%u_%u", iter->second.rocid, iter->second.get4id, iter->second.chid);
            if (xbin.Length()==0) xbin = "xbin:"; else xbin.Append(",");
            xbin.Append(lbl);
         }

         fEvPerCh = MakeH1("GET4TEST/EvPerCh", "Number of good events per channel", fMap.size(), 0, fMap.size(), xbin.Data());
         fBadPerCh = MakeH1("GET4TEST/BadPerCh", "Number of bad events per channel", fMap.size(), 0, fMap.size(), xbin.Data());
         fHitPerCh = MakeH1("GET4TEST/HitPerCh", "Number of hits per channel", fMap.size(), 0, fMap.size(), xbin.Data());
      }

      virtual void CreateBranch(TTree*)
      {
      }

      virtual bool Process(base::Event* evnt)
      {
         for (TGet4TestMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
            iter->second.reset();
         }

          get4::SubEvent* sub0 = dynamic_cast<get4::SubEvent*> (evnt->GetSubEvent("ROC0"));

          if (sub0!=0) {
             //TGo4Log::Info("Find GET4 data for ROC0 size %u  trigger: %10.9f", sub0->Size(), ev->GetTriggerTime());
             FillH1(fMultipl, sub0->Size());

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
                if (width!=0.) FillH1(rec.fWidth, width);
             }

          } else {
             printf("Not found GET4 data for ROC0 in event %10.9f s\n", evnt->GetTriggerTime());
          }

          for (TGet4TestMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {

             TGet4TestMap::iterator next = iter;
             next++;
             if (next == fMap.end()) next = fMap.begin();

             TGet4TestRec& rec1 = iter->second;
             TGet4TestRec& rec2 = next->second;

             if (rec1.isrising && rec2.isrising) {

                double diff = rec2.lastrising*50. - rec1.lastrising*50.;

                FillH1(rec1.fDiffRising, diff);
             }
          }

          unsigned indx=0;
          if (fEvPerCh && fBadPerCh)
             for (TGet4TestMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++) {
                TGet4TestRec& rec = iter->second;

                // TODO: once can configure more complex conditions
                if (rec.hitcnt == 2)
                   FillH1(fEvPerCh, indx);
                else
                   FillH1(fBadPerCh, indx);

                FillH1(fHitPerCh, rec.hitcnt);

                indx++;
             }

         return true;
      }
};


void second() {
   printf("Create get4 test classes\n");

   // gSystem->Load("libGet4Test");

   TGet4TestProc* p = new TGet4TestProc("Get4Test");

   for (int get4=0;get4<5;get4++)
     p->Add(0,get4); // add all channels of get4 0..5 on ROC0

   p->MakeHistos(); // make summary histograms
}
