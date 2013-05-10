#include "TUserSource.h"

#include "TClass.h"

#include <stdlib.h>

#include "TGo4EventErrorException.h"
#include "TGo4EventEndException.h"
#include "TGo4EventTimeoutException.h"
#include "TGo4UserSourceParameter.h"
#include "TGo4MbsEvent.h"
#include "TGo4Log.h"

#include "base/defines.h"

#include "hadaq/defines.h"

#define Trb_BUFSIZE 0x80000


TUserSource::TUserSource() :
   TGo4EventSource("default Trb source"),
   fxArgs(""),
   fiPort(0),
   fxFile(),
   fxBuffer(0)
{
}

TUserSource::TUserSource(const char* name,
       const char* args,
       Int_t port) :
   TGo4EventSource(name),
   fxArgs(args),
   fiPort(port),
   fxFile(),
   fxBuffer(0)
{
   Open();
}

TUserSource::TUserSource(TGo4UserSourceParameter* par) :
   TGo4EventSource(" "),
   fxArgs(""),
   fiPort(0),
   fxFile(),
   fxBuffer(0)
{
   if(par) {
      SetName(par->GetName());
      SetPort(par->GetPort());
      SetArgs(par->GetExpression());
      Open();
   } else {
      TGo4Log::Error("TUserSource constructor with zero parameter!");
   }
}

TUserSource::~TUserSource()
{
  Close();
}

Bool_t TUserSource::CheckEventClass(TClass* cl)
{
  return cl->InheritsFrom(TGo4MbsEvent::Class());
}

Bool_t TUserSource::BuildEvent(TGo4EventElement* dest)
{
   TGo4MbsEvent* evnt = dynamic_cast<TGo4MbsEvent*> (dest);
   if ((evnt==0) || (fxBuffer==0)) return kFALSE;

   uint32_t bufsize = Trb_BUFSIZE;

   if (fxFile.eof() || !fxFile.ReadBuffer(fxBuffer, &bufsize, true)) {
      SetCreateStatus(1);
      SetErrMess(Form("End of input file %s", GetName()));
      SetEventStatus(1);
      throw TGo4EventEndException(this);
      return kFALSE;
   }

   TGo4SubEventHeader10 fxSubevHead;
   memset(&fxSubevHead, 0, sizeof(fxSubevHead));
   fxSubevHead.fsProcid = base::proc_TRBEvent; // mark to be processed by TTrbProc

   evnt->AddSubEvent(fxSubevHead.fiFullid, (Short_t*) fxBuffer, bufsize/sizeof(Short_t) + 2, kTRUE);

   evnt->SetCount(((hadaq::RawEvent*) fxBuffer)->GetSeqNr());

   // set total MBS event length, which must include MBS header itself
   evnt->SetDlen(bufsize/sizeof(Short_t) + 2 + 6);

   return kTRUE; // event is ready
}

Int_t TUserSource::Open()
{
   if(fxFile.isOpened()) return -1;
   TGo4Log::Info("Open of TUserSource");

   //! Open connection/file
   if(!fxFile.OpenRead(GetName())) {
      SetCreateStatus(1);
      SetErrMess(Form("Eror opening user file:%s", GetName()));
      throw TGo4EventErrorException(this);
   }
   fxBuffer = new Char_t[Trb_BUFSIZE];
   return 0;
}


Int_t TUserSource::Close()
{
   if(!fxFile.isOpened()) return -1;
   TGo4Log::Info("Close of TUserSource");
   delete [] fxBuffer; fxBuffer = 0;
   fxFile.Close();
   return 0;
}
