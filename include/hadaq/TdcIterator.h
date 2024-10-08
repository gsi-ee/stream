#ifndef HADAQ_TDCITERATOR_H
#define HADAQ_TDCITERATOR_H

#include "hadaq/definess.h"

#include "hadaq/TdcMessage.h"

namespace hadaq {

/** \brief TDC iterator
  *
  * \ingroup stream_hadaq_classes
  *
  * Iterator over TDC messages */

   class TdcIterator {
      protected:

         enum { DummyEpoch = 0xffffffff };

         uint32_t*  fBuf{nullptr};     ///<! pointer on raw data
         uint32_t*  fLastBuf{nullptr}; ///<! pointer on last extracted message
         unsigned   fBuflen{0};        ///<! length of raw data
         bool       fSwapped{false};   ///<! true if raw data are swapped

         hadaq::TdcMessage fMsg;          ///<! current message
         uint32_t  fCurEpoch{DummyEpoch};  ///<! current epoch

         base::LocalStampConverter  fConv;   ///<! use to covert time stamps in seconds

      public:

         /** constructor */
         TdcIterator(unsigned epochbitlen = 28) {
            // we have 11 bits for coarse stamp and 28 bits for epoch
            // each bin is 5 ns
            fConv.SetTimeSystem(epochbitlen + 11, hadaq::TdcMessage::CoarseUnit());
         }

         unsigned remain_length() const { return fBuflen; }

         /** assign buffer */
         void assign(uint32_t* buf, unsigned len, bool swapped = true)
         {
            fBuf = buf;
            fLastBuf = nullptr;
            fBuflen = len;
            fSwapped = swapped;
            fMsg.assign(0);

            if (fBuflen == 0) fBuf = nullptr;
            fCurEpoch = DummyEpoch;
         }

         /** assign data from sub event */
         void assign(hadaqs::RawSubevent* subev, unsigned indx, unsigned datalen)
         {
            if (subev)
               assign(subev->GetDataPtr(indx), datalen, subev->IsSwapped());
         }

         /** One should call method to set current reference epoch */
         void setRefEpoch(uint32_t epoch)
         {
            fConv.MoveRef(((uint64_t) epoch) << 11);
         }

         /** Returns true if ref epoch was set */
         bool hasRefEpoch() const
         {
            return fConv.HasRef();
         }

         /** next TDC message */
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

         /** next TDC v4 message */
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

         /** try to check forward message - without shifting iterator */
         bool lookForwardMsg(TdcMessage &msg)
         {
            if (!fBuf)
               return false;
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

         /** get coarse time for the current message */
         inline double getMsgTimeCoarse() const
         { return fConv.ToSeconds(getMsgStamp()); }

         /** get coarse time for the current message */
         inline double convertTime(uint32_t epoch, uint32_t coarse) const
         { return fConv.ToSeconds((((uint64_t) epoch) << 11) | coarse); }


         /** return fine time value for current message */
         inline double getMsgTimeFine() const
         {
           if (fMsg.isHit0Msg() || fMsg.isHit2Msg()) return hadaq::TdcMessage::SimpleFineCalibr(fMsg.getHitTmFine());
           if (fMsg.isHit1Msg()) return hadaq::TdcMessage::CoarseUnit()*fMsg.getHitTmFine()*0.001;
           return 0;
         }

         /** get current message */
         hadaq::TdcMessage& msg() { return fMsg; }

         /** Returns true, if current epoch was assigned */
         bool isCurEpoch() const { return fCurEpoch != DummyEpoch; }

         /** Clear current epoch value */
         void clearCurEpoch() { fCurEpoch = DummyEpoch; }

         /** Set value of current epoch */
         void setCurEpoch(uint32_t val) { fCurEpoch = val; }

         /** Return value of current epoch */
         uint32_t getCurEpoch() const { return fCurEpoch; }

         /** print current message */
         void printmsg()
         {
            double tm = -1.;
            if (msg().isHitMsg() || msg().isEpochMsg())
               tm = getMsgTimeCoarse() - getMsgTimeFine();
            msg().print(tm);
         }

         /** print all v4 messages */
         void printall4()
         {
            uint32_t ttype = 0;
            while (next4())
              msg().print4(ttype);
         }
   };
}

#endif
