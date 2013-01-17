#ifndef TGet4TestProc_H
#define TGet4TestProc_H

#include "go4/TUserProcessor.h"

#include <map>

struct TGet4TestRec {
   unsigned rocid;
   unsigned get4id;
   unsigned chid;

   TH1* fWidth;

   TH1* fDiffRising;

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

class TGet4TestProc : public TUserProcessor {

   protected:

      TH1* fMultipl;   //!
      TH1* fEvPerCh;   //!  events per channel
      TH1* fBadPerCh;  //!  bad events per channel
      TH1* fHitPerCh;  //!  bad events per channel

      TGet4TestMap fMap; //! all channels which should be present

   public:

      TGet4TestProc();

      TGet4TestProc(const char* name);

      inline unsigned code(unsigned rocid, unsigned get4id, unsigned chid) const { return rocid*1000 + get4id*10 + chid; }

      void Add(unsigned rocid, unsigned get4id)
      {
         for (unsigned indx=0;indx<4;indx++)
            Add(rocid, get4id, indx);
      }

      /** Register specific get4 channel for testing */
      void Add(unsigned rocid, unsigned get4id, unsigned chid);

      /** Create common histograms for all channels */
      void MakeHistos();

      virtual void Process(TStreamEvent*);



   ClassDef(TGet4TestProc, 1)
};


#endif
