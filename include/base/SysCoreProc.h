#ifndef BASE_SYSCOREPROC_H
#define BASE_SYSCOREPROC_H

#include "base/StreamProc.h"

namespace base {

   class OpticSplitter;

   /** SysCoreProc is base class for many processors of data,
    *  provided by SysCore2/3 boards.
    *  Main idea is to collect common settings/methods for SysCore variants like:
    *     nXYTER, GET4, Spadic1 and probably STS-XYTER
    *  Main common part is 250 MHz clock and SYNC/AUX messages which could be used
    *  for time synchronization. */

   class SysCoreProc : public base::StreamProc {

      friend class OpticSplitter;

      protected:
         unsigned  fSyncSource;    // 0,1: SYNC0,1 used for synchronization,  >=2: local time used for time stamp
         unsigned  fTriggerSignal; // 0 .. 3 is AUXs, 10-11 is SYNCs

         int fNumPrintMessages;      //! number of messages to be printed
         double fPrintLeft;          //! left border to start printing
         double fPrintRight;         //! right border to stop printing
         bool fAnyPrinted;           //! true when any message was printed

         base::H1handle fMsgPerBrd;  //! common histogram for all boards with similar prefix
         base::H1handle fALLt;       //! histogram for all messages times
         base::H1handle fHITt;       //! histogram for hit messages only
         base::H1handle fAUXt[4];    //! histogram for AUX times
         base::H1handle fSYNCt[2];   //! histogram for SYNC times

         static unsigned fMaxBrdId;  //! maximum allowed board id, used for histogramming

         // this part is dedicated for OpticSplitter and should not be touched
         Buffer    fSplitBuf;         //! temporary buffer for splitting
         uint64_t* fSplitPtr;         //! current position for split data


         /** Returns true when processor used to select trigger signal
          * In subclass one could have alternative ways of trigger or ROI selections */
         virtual bool doTriggerSelection() const { return (fTriggerSignal < 4) || (fTriggerSignal==10) || (fTriggerSignal==11); }

         void CreateBasicHistograms();

         void FillMsgPerBrdHist(unsigned cnt) { FillH1(fMsgPerBrd, GetBoardId(), cnt); }

         bool CheckPrint(double msgtm, double safetymargin = 1e-6);

      public:

         SysCoreProc(const char* name, unsigned brdid, OpticSplitter* spl = 0);
         virtual ~SysCoreProc();

         /** Set signal id, used for time synchronization
          *  One could use SYNC0 (id=0) or SYNC1 (id=1) for synchronization with other components
          *  if other id specified, local time stamp will be used and no any sync will be used */
         void SetSyncSource(unsigned id)
         {
            fSyncSource = id;
            SetSynchronisationRequired(fSyncSource < 2);
         }

         /** This declares that local time stamp will be used.
          * Should work with single ROC setup or with optical-based readout,
          * where all time counters are synchronized */
         void SetNoSyncSource() { SetSyncSource(0xff); }

         /** Set signal kind which should be used for data selection
          *  Values 0..3 are for AUX0 - AUX3
          *  Values 10,11 are for SYNC0, SYNC1
          *  Other values can be used in derived classed */
         void SetTriggerSignal(unsigned id) { fTriggerSignal = id; }

         /** Disable any trigger generation */
         void SetNoTriggerSignal() { fTriggerSignal = 99; }


         /** Set number of messages to be printed
           * Optionally, one could specify time window where messages should be taken
           * Time is same as time on histogram like AUX0t or SYNC1 (truncated to 1000 s) */
         void SetPrint(int nummsg, double left = -1., double right = -1.)
         {
            fNumPrintMessages = nummsg;
            fPrintLeft = left;
            fPrintRight = right;
            fAnyPrinted = false;
         }

         /** Set maximum board number, used for histograming */
         static void SetMaxBrdId(unsigned max) { fMaxBrdId = max; }

   };
}

#endif
