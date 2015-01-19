#ifndef HADAQ_TDCITERATOR_H
#define HADAQ_TDCITERATOR_H

#include "hadaq/definess.h"

#include "hadaq/TdcMessage.h"

#include "base/TimeStamp.h"

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
            if (fBuf==0) return false;

            if (fSwapped)
               fMsg.assign((((uint8_t *) fBuf)[0] << 24) | (((uint8_t *) fBuf)[1] << 16) | (((uint8_t *) fBuf)[2] << 8) | (((uint8_t *) fBuf)[3]));
            else
               fMsg.assign(*fBuf);

            if (fMsg.isEpochMsg()) fCurEpoch = fMsg.getEpochValue();

            fLastBuf = fBuf++;
            if (--fBuflen == 0) fBuf = 0;

            return true;
         }

         /** Returns 39-bit value, which combines epoch and coarse counter.
          * Time bin is 5 ns  */
         uint64_t getMsgStamp() const
         { return (isCurEpoch() ? ((uint64_t) fCurEpoch) << 11 : 0) | (fMsg.isHitMsg() ? fMsg.getHitTmCoarse() : 0); }

         inline double getMsgTimeCoarse() const
         { return fConv.ToSeconds(getMsgStamp()); }

         inline double getMsgTimeFine() const
         { return fMsg.isHitMsg() ? hadaq::TdcMessage::SimpleFineCalibr(fMsg.getHitTmFine()) : 0.; }

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

         void setFineTime(uint32_t v) {
            if (fLastBuf==0) {
               printf("fLastBuf pointer is 0"); exit(1);
            }

            uint32_t buf = *fLastBuf;
            if (fSwapped) buf = (((uint8_t *) fLastBuf)[0] << 24) | (((uint8_t *) fLastBuf)[1] << 16) | (((uint8_t *) fLastBuf)[2] << 8) | (((uint8_t *) fLastBuf)[3]);

            buf = (buf & ~tdckind_Mask) | tdckind_Hit1; // mark as message 1
            if (v>=0x3ff) {
              buf |= (0x3ff << 12);
            } else {
              buf = (buf & ~(0x3ff << 12)) | (v << 12);
            }

            *fLastBuf = buf;
            if (fSwapped) *fLastBuf = (((uint8_t *) fLastBuf)[0] << 24) | (((uint8_t *) fLastBuf)[1] << 16) | (((uint8_t *) fLastBuf)[2] << 8) | (((uint8_t *) fLastBuf)[3]);
         }
   };
}

#endif
