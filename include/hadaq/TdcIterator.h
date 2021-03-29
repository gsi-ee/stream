#ifndef HADAQ_TDCITERATOR_H
#define HADAQ_TDCITERATOR_H

#include "hadaq/definess.h"

#include "hadaq/TdcMessage.h"

namespace hadaq {

   class TdcIterator {
      protected:

         enum { DummyEpoch = 0xffffffff };

         uint32_t*  fBuf;        //! pointer on raw data
         uint32_t*  fLastBuf;    //! pointer on last extracted message
         unsigned   fBuflen;     //! length of raw data
         bool       fSwapped;    //! true if raw data are swapped

         hadaq::TdcMessage fMsg; //! current message
         uint32_t  fCurEpoch;    //! current epoch

         base::LocalStampConverter  fConv;   //! use to covert time stamps in seconds

      public:

         TdcIterator(unsigned epochbitlen = 28) :
            fBuf(0),
            fLastBuf(0),
            fBuflen(0),
            fSwapped(false),
            fMsg(),
            fCurEpoch(DummyEpoch),
            fConv()
         {
            // we have 11 bits for coarse stamp and 28 bits for epoch
            // each bin is 5 ns
            fConv.SetTimeSystem(epochbitlen + 11, hadaq::TdcMessage::CoarseUnit());
         }

         void assign(uint32_t* buf, unsigned len, bool swapped = true)
         {
            fBuf = buf;
            fLastBuf = 0;
            fBuflen = len;
            fSwapped = swapped;
            fMsg.assign(0);

            if (fBuflen == 0) fBuf = 0;
            fCurEpoch = DummyEpoch;
         }

         void assign(hadaqs::RawSubevent* subev, unsigned indx, unsigned datalen)
         {
            if (subev!=0)
               assign(subev->GetDataPtr(indx), datalen, subev->IsSwapped());
         }

         /** One should call method to set current reference epoch */
         void setRefEpoch(uint32_t epoch)
         {
            fConv.MoveRef(((uint64_t) epoch) << 11);
         }

         bool next()
         {
            if (!fBuf) return false;

            if (fSwapped)
               fMsg.assign((((uint8_t *) fBuf)[0] << 24) | (((uint8_t *) fBuf)[1] << 16) | (((uint8_t *) fBuf)[2] << 8) | (((uint8_t *) fBuf)[3]));
            else
               fMsg.assign(*fBuf);

            if (fMsg.isEpochMsg()) fCurEpoch = fMsg.getEpochValue();

            fLastBuf = fBuf++;
            if (--fBuflen == 0) fBuf = nullptr;

            return true;
         }

         bool next4()
         {
            if (!fBuf) return false;

            if (fSwapped)
               fMsg.assign((((uint8_t *) fBuf)[0] << 24) | (((uint8_t *) fBuf)[1] << 16) | (((uint8_t *) fBuf)[2] << 8) | (((uint8_t *) fBuf)[3]));
            else
               fMsg.assign(*fBuf);

            if (fMsg.isEPOC()) fCurEpoch = fMsg.getEPOC();

            fLastBuf = fBuf++;
            if (--fBuflen == 0) fBuf = nullptr;

            return true;
         }

         bool lookForwardMsg(TdcMessage &msg)
         {
            if (fBuf==0) return false;
            if (fSwapped)
               msg.assign((((uint8_t *) fBuf)[0] << 24) | (((uint8_t *) fBuf)[1] << 16) | (((uint8_t *) fBuf)[2] << 8) | (((uint8_t *) fBuf)[3]));
            else
               msg.assign(*fBuf);
            return true;
         }

         /** Returns 39-bit value, which combines epoch and coarse counter.
          * Time bin is 5 ns  */
         uint64_t getMsgStamp() const
         { return (isCurEpoch() ? ((uint64_t) fCurEpoch) << 11 : 0) | (fMsg.isHitMsg() ? fMsg.getHitTmCoarse() : 0); }

         inline double getMsgTimeCoarse() const
         { return fConv.ToSeconds(getMsgStamp()); }

         // return fine time value for current message
         inline double getMsgTimeFine() const
         {
           if (fMsg.isHit0Msg() || fMsg.isHit2Msg()) return hadaq::TdcMessage::SimpleFineCalibr(fMsg.getHitTmFine());
           if (fMsg.isHit1Msg()) return hadaq::TdcMessage::CoarseUnit()*fMsg.getHitTmFine()*0.001;
           return 0;
         }

         hadaq::TdcMessage& msg() { return fMsg; }

         /** Returns true, if current epoch was assigned */
         bool isCurEpoch() const { return fCurEpoch != DummyEpoch; }

         /** Clear current epoch value */
         void clearCurEpoch() { fCurEpoch = DummyEpoch; }

         /** Set value of current epoch */
         void setCurEpoch(uint32_t val) { fCurEpoch = val; }

         /** Return value of current epoch */
         uint32_t getCurEpoch() const { return fCurEpoch; }

         void printmsg()
         {
            double tm = -1.;
            if (msg().isHitMsg() || msg().isEpochMsg())
               tm = getMsgTimeCoarse() - getMsgTimeFine();
            msg().print(tm);
         }

         void printall4()
         {
            uint32_t ttype = 0;
            while (next4())
              msg().print4(ttype);
         }
   };
}

#endif
