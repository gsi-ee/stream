#include "hadaq/StartProcessor.h"

#include "hadaq/definess.h"
#include "hadaq/TrbIterator.h"
#include "hadaq/TdcIterator.h"

const double EPOCHLEN = 5e-9*0x800;


hadaq::StartProcessor::StartProcessor() :
   base::StreamProc("START", 0, false)
{
   mgr()->RegisterProc(this, base::proc_TRBEvent, 0);

   fEvType = MakeH1("EvType", "Event type", 16, 0, 16, "id");
   fEvSize = MakeH1("EvSize", "Event size", 500, 0, 500000, "bytes");
   fSubevSize = MakeH1("SubevSize", "Subevent size", 500, 0, 5000, "bytes");

   fEpochDiff = MakeH1("EpochDiff", "Difference in epochs", 100, -50, 50, "epoch");

   fStartId = 0x8880;

   fTdcMin = 0x5000;
   fTdcMax = 0x5010;

}

hadaq::StartProcessor::~StartProcessor()
{
}

/** Return time difference between epochs in seconds */
double hadaq::StartProcessor::EpochTmDiff(unsigned ep1, unsigned ep2)
{
   return EpochDiff(ep1, ep2) * EPOCHLEN;
}


bool hadaq::StartProcessor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

   hadaq::TrbIterator iter(buf().buf, buf().datalen);

   hadaqs::RawEvent* ev = 0;

   unsigned evcnt = 0;

   hadaq::TdcMessage calibr;
   unsigned ncalibr(20); // position in calibr

   while ((ev = iter.nextEvent()) != 0) {

      evcnt++;

      DefFillH1(fEvType, (ev->GetId() & 0xf), 1.);
      DefFillH1(fEvSize, ev->GetPaddedSize(), 1.);

      hadaqs::RawSubevent* sub = 0;

      while ((sub = iter.nextSubevent()) != 0) {

         if (sub->GetId() != fStartId) continue;

         DefFillH1(fSubevSize, sub->GetSize(), 1.);

         unsigned ix = 0;           // cursor
         unsigned trbSubEvSize = sub->GetSize() / 4 - 4;

         while (ix < trbSubEvSize) {
            //! Extract data portion from the whole packet (in a loop)
            uint32_t data = sub->Data(ix++);

            unsigned datalen = (data >> 16) & 0xFFFF;
            unsigned dataid = data & 0xFFFF;

            // ignore HUB HEADER
            // if (dataid == hubid) continue;

            bool istdc = (dataid>=fTdcMin) && (dataid<fTdcMax);

            unsigned lastepoch = 0;


            if (istdc) {

               TdcIterator iter;
               iter.assign(sub, ix, datalen);

               hadaq::TdcMessage &msg = iter.msg();

               while (iter.next()) {
                  if (msg.isCalibrMsg()) {
                     ncalibr = 0;
                     calibr = msg;
                  } else if (msg.isHitMsg()) {
                     unsigned chid = msg.getHitChannel();
                     // unsigned coarse = msg.getHitTmCoarse();
                     bool isrising = msg.isHitRisingEdge();
                     double corr = 0., localtm = iter.getMsgTimeCoarse();

                     if (ncalibr < 2) {
                         // use correction from special message
                         corr = calibr.getCalibrFine(ncalibr++)*5e-9/0x3ffe;
                         if (!isrising) corr *= 10.; // range for falling edge is 50 ns.
                     } else {
                        unsigned fine = msg.getHitTmFine();
                        corr = hadaq::TdcMessage::SimpleFineCalibr(fine);
                     }

                     // this is corrected absolute time stamp for hit message
                     localtm -= corr;

                  } else if (msg.isEpochMsg()) {
                     unsigned epoch = iter.getCurEpoch();

                     if (lastepoch) {
                        int diff = EpochDiff(lastepoch, epoch);
                        if (diff>45) diff = 45; else if (diff < -45) diff = 45;
                        DefFillH1(fEpochDiff, diff, 1.);
                     }

                     lastepoch = epoch;
                  }
               }
            }

            ix+=datalen;

         } // while (ix < trbSubEvSize)

      } // subevents

   } // events

   return true;
}
