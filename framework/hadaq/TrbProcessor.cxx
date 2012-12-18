#include "hadaq/TrbProcessor.h"

#include "base/defines.h"
#include "base/ProcMgr.h"

#include "hadaq/TrbIterator.h"
#include "hadaq/TdcProcessor.h"

hadaq::TrbProcessor::TrbProcessor(unsigned brdid) :
   base::StreamProc("TRB", brdid),
   fBrdId(brdid),
   fMap()
{
   mgr()->RegisterProc(this, base::proc_TRBEvent, brdid);

   fEvSize = MakeH1("EvSize", "Event size", 500, 0, 5000, "bytes");
   fSubevSize = MakeH1("SubevSize", "Subevent size", 500, 0, 5000, "bytes");

   fLostRate = MakeH1("LostRate", "Relative number of lost packets", 1000, 0, 1., "data lost");

   fLastTriggerId = 0;
   fLostTriggerCnt = 0;
   fTakenTriggerCnt = 0;
}

hadaq::TrbProcessor::~TrbProcessor()
{
}

void hadaq::TrbProcessor::AddSub(TdcProcessor* tdc, unsigned id)
{
   fMap[id] = tdc;
}


void hadaq::TrbProcessor::SetTriggerWindow(double left, double right)
{
   base::StreamProc::SetTriggerWindow(left, right);

   for (SubProcMap::iterator iter = fMap.begin(); iter != fMap.end(); iter++)
      iter->second->SetTriggerWindow(left, right);
}


void hadaq::TrbProcessor::AccountTriggerId(unsigned subid)
{
   if (fLastTriggerId==0) {
      fLostTriggerCnt = 0;
      fTakenTriggerCnt = 0;
   } else {
      unsigned diff = (subid - fLastTriggerId) & 0xffff;
      if ((diff > 0x8000) || (diff==0)) {
         // ignore this, most probable it is some mistake
      } else
      if (diff==1) {
         fTakenTriggerCnt++;
      } else {
         fLostTriggerCnt+=(diff-1);
      }
   }

   fLastTriggerId = subid;

   if (fTakenTriggerCnt + fLostTriggerCnt > 1000) {
      double lostratio = 1.*fLostTriggerCnt / (fTakenTriggerCnt + fLostTriggerCnt + 0.);
      FillH1(fLostRate, lostratio);
      fTakenTriggerCnt = 0;
      fLostTriggerCnt = 0;
   }
}


bool hadaq::TrbProcessor::FirstBufferScan(const base::Buffer& buf)
{
   if (buf.null()) return false;

//   buf().format;
//   buf().boardid;

//   buf().buf

//   buf().datalen


//   printf("*********** Start TRB processing len %u ************ \n", buf().datalen);

   hadaq::TrbIterator iter(buf().buf, buf().datalen);


   hadaq::RawEvent* ev = 0;

   while ((ev = iter.nextEvent()) != 0) {

//      printf("************ Find event len %u *********** \n", ev->GetSize());

      FillH1(fEvSize, ev->GetSize());

      hadaq::RawSubevent* sub = 0;

      while ((sub = iter.nextSubevent()) != 0) {

//         printf("************ Find subevent len %u ********** \n", sub->GetSize());

         FillH1(fSubevSize, sub->GetSize());

         // use only 16-bit in trigger number while CTS make a lot of errors in higher 8 bits
         AccountTriggerId((sub->GetTrigNr() >> 8) & 0xffff);

         ProcessTDCV3(sub);

      }

//      printf("Finish event processing\n");

   }

   return true;
}

void hadaq::TrbProcessor::ProcessTDCV3(hadaq::RawSubevent* sub)
{

   unsigned ix = 0;           // cursor

   uint32_t data;             // 32 bits

   unsigned trbSubEvSize = sub->GetSize() / 4 - 4;

   while (ix < trbSubEvSize) {
      //! Extract data portion from the whole packet (in a loop)
      data = sub->Data(ix++);

      // ===========  this is header for TDC, build inside the TRB3 =================
      if (((data >> 8) & 0xFF) == 0xB0) {
         // do that ever you want
         unsigned curBrd = (data  & 0xFF);
         unsigned tdcLen = ((data >> 16) & 0xFFFF);
         // EPRINT ("***  --- tdc header: 0x%08x, TRB3-buildin TDC-FPGA brd=%d, size=%d  IGNORED\n", data, curBrd, tdcLen);
         ix+=tdcLen;
         continue;
      }

      //! ================= RICH-FEE TDC header ========================
      if (((data >> 8) & 0xFF) == 0xC0) {

         unsigned tdcid = (data  & 0xFF);

         unsigned tdcLen = ((data >> 16) & 0xFFFF);

         TdcProcessor* subproc = fMap[tdcid];

         if (subproc)
            subproc->FirstSubScan(sub, ix, tdcLen);
         else {
            printf("Did not find processor for TDC %u - skip data %u\n", tdcid, tdcLen);
         }
         ix+=tdcLen;

         continue; // go to next block
      }  // end of if TDC header


      //! ==================== CTS header and inside ================
      if ((data & 0xFFFF) == 0x8000) {
         unsigned centHubLen = ((data >>16) & 0xFFFF);
         //EPRINT ("***  --- CTS header: 0x%x, size=%d\n", data, centHubLen);
         //hTrbTriggerCount->Fill(5);          //! TRB - CTS
         //hTrbTriggerCount->Fill(0);          //! TRB TOTAL


         data = sub->Data(ix++);
         unsigned trigtype = (data & 0xFFFF);
         unsigned trignum = (data >> 16) & 0xF;
         // printf("***  --- CTS trigtype: 0x%x %x, trignum=0x%x\n", trigtype, trigtype & 0x3, trignum);
         //fCurrentTriggerType = trigtype;

         // printf("trigtype = %x trignum\n", trigtype & 0x3);
         centHubLen--;

         while (centHubLen-- > 0) {
            //! Look only for the CTS data
            data = sub->Data(ix++);
            // printf("***  --- CTS word: 0x%x, skipping\n", data);
            //hTrbTriggerCount->Fill(5);       //! TRB - CTS
            //hTrbTriggerCount->Fill(0);       //! TRB TOTAL
         }
         //! Last CTS word - SYNC number
         // data = hadsubevent->Data(++ix);

         unsigned syncNum = (data & 0xFFFFFF);

         // if ((trigtype & 0x3) == 1) printf("Find SYNC %u\n", syncNum);

         // printf("***  --- CTS word - SYNC number: 0x%x, num=%d\n", data, syncNum);
         //hTrbTriggerCount->Fill(5);       //! TRB - CTS
         //hTrbTriggerCount->Fill(0);       //! TRB TOTAL
         continue;
      }

      //! ==================  Dummy header and inside ==========================
      if ((data & 0xFFFF) == 0x5555) {
         unsigned centHubLen = ((data >>16) & 0xFFFF);
         //EPRINT ("***  --- dummy header: 0x%x, size=%d\n", data, centHubLen);
         //hTrbTriggerCount->Fill(4);          //! TRB - DUMMY
         //hTrbTriggerCount->Fill(0);          //! TRB TOTAL
         while (centHubLen-- > 0) {
            //! In theory here must be only one word - termination package with the status
            data = sub->Data(ix++);
            //EPRINT ("***  --- status word: 0x%x\n", data);
            //hTrbTriggerCount->Fill(4);       //! TRB - DUMMY
            //hTrbTriggerCount->Fill(0);       //! TRB TOTAL
            uint32_t fSubeventStatus = data;
            if (fSubeventStatus != 0x00000001) {
               // hTrbTriggerCount->Fill(1);    //! TRB - INVALID
            }
         }
      }
   }

}


bool hadaq::TrbProcessor::SecondBufferScan(const base::Buffer& buf)
{
   return true;
}
