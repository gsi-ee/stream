#include "TUserSource.h"

#include "TClass.h"
#include "TList.h"

#include <stdlib.h>
#include <string.h>

#include "TGo4EventErrorException.h"
#include "TGo4EventEndException.h"
#include "TGo4EventTimeoutException.h"
#include "TGo4UserSourceParameter.h"
#include "TGo4FileSource.h"
#include "TGo4MbsEvent.h"
#include "TGo4Log.h"

#include "base/defines.h"

#include "hadaq/definess.h"

#define Trb_BUFSIZE 0x80000


TUserSource::TUserSource() :
   TGo4EventSource("default Trb source"),
   fxArgs(""),
   fiPort(0),
   fNames(0),
   fIsHLD(kTRUE),
   fxFile(),
   fxBuffer(0),
   fxDatFile(0),
   fEventCounter(0)
{
}

TUserSource::TUserSource(const char* name, const char* args, Int_t port) :
   TGo4EventSource(name),
   fxArgs(args),
   fiPort(port),
   fNames(0),
   fIsHLD(kTRUE),
   fxFile(),
   fxBuffer(0),
   fxDatFile(0),
   fEventCounter(0)
{
   Open();
}

TUserSource::TUserSource(TGo4UserSourceParameter* par) :
   TGo4EventSource(" "),
   fxArgs(""),
   fiPort(0),
   fNames(0),
   fIsHLD(kTRUE),
   fxFile(),
   fxBuffer(0),
   fxDatFile(0),
   fEventCounter(0)
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
   TGo4Log::Info("Close of TUserSource");
   fxFile.Close();

   if (fxBuffer) {
      delete [] fxBuffer;
      fxBuffer = 0;
   }

   if (fxDatFile) {
      fclose(fxDatFile);
      fxDatFile = 0;
   }

   if (fNames) {
      fNames->Delete();
      delete fNames;
      fNames = 0;
   }
}

Bool_t TUserSource::CheckEventClass(TClass* cl)
{
  return cl->InheritsFrom(TGo4MbsEvent::Class());
}

Bool_t TUserSource::BuildDatEvent(TGo4MbsEvent* evnt)
{
   char buf[100];

   if ((fxDatFile==0) || feof(fxDatFile))
      if (!OpenNextFile()) return kFALSE;

   int cnt=0;

   uint32_t arr[1000];
   arr[cnt++] = 0xaaaa0000; // mark all data as coming from get4 0

   while (!feof(fxDatFile) && (cnt<1000)) {
      if (fgets(buf, sizeof(buf), fxDatFile)==0) {
         fclose(fxDatFile);
         fxDatFile = 0;
         break;
      }

      if (strlen(buf)<10) { printf("VERY SHORT LINE %s!!!\n", buf); break; }


      unsigned value; long tm;

      if (sscanf(buf,"%ld ps %X", &tm, &value)!=2) break;

      // printf("value = %8X buf = %s", value, buf);

      arr[cnt++] = value;
   }

   if (cnt==0) return kFALSE;

   uint32_t bufsize = cnt*sizeof(uint32_t);

   TGo4SubEventHeader10 fxSubevHead;
   memset(&fxSubevHead, 0, sizeof(fxSubevHead));
   fxSubevHead.fsProcid = 1;

   evnt->AddSubEvent(fxSubevHead.fiFullid, (Short_t*) arr, bufsize/sizeof(Short_t) + 2, kTRUE);

   evnt->SetCount(fEventCounter++);

   // set total MBS event length, which must include MBS header itself
   evnt->SetDlen(bufsize/sizeof(Short_t) + 2 + 6);

   return kTRUE; // event is ready
}

Bool_t TUserSource::BuildEvent(TGo4EventElement* dest)
{
   TGo4MbsEvent* evnt = dynamic_cast<TGo4MbsEvent*> (dest);
   if ((evnt==0) || (fxBuffer==0)) return kFALSE;

   if (!fIsHLD) return BuildDatEvent(evnt);

   uint32_t bufsize = Trb_BUFSIZE;

   Bool_t trynext(kFALSE);
   if (!fxFile.isOpened() || fxFile.eof()) trynext = kTRUE; else
   if (!fxFile.ReadBuffer(fxBuffer, &bufsize, true)) trynext = kTRUE;

   if (trynext) {
      bufsize = Trb_BUFSIZE;
      Bool_t isok = OpenNextFile();
      if (isok) isok = fxFile.ReadBuffer(fxBuffer, &bufsize, true);
      if (!isok) {
         SetCreateStatus(1);
         SetErrMess("End of HLD input");
         SetEventStatus(1);
         throw TGo4EventEndException(this);
         return kFALSE;
      }
   }

   TGo4SubEventHeader10 fxSubevHead;
   memset(&fxSubevHead, 0, sizeof(fxSubevHead));
   fxSubevHead.fsProcid = base::proc_TRBEvent; // mark to be processed by TTrbProc

   evnt->AddSubEvent(fxSubevHead.fiFullid, (Short_t*) fxBuffer, bufsize/sizeof(Short_t) + 2, kTRUE);

   evnt->SetCount(((hadaqs::RawEvent*) fxBuffer)->GetSeqNr());

   // set total MBS event length, which must include MBS header itself
   evnt->SetDlen(bufsize/sizeof(Short_t) + 2 + 6);

   return kTRUE; // event is ready
}

Int_t TUserSource::Open()
{
   TString fname = GetName();

   if(fname.Contains("*") || fname.Contains("?")) {
      // name indicates wildcard expression
      fNames = TGo4FileSource::ProducesFilesList(fname.Data());
   } else {
      fNames = new TList;
      fNames->Add(new TObjString(fname.Data()));
   }

   fIsHLD = kTRUE;

   if ((fname.Index(".dat") != kNPOS) && (fname.Index(".dat")>0)) fIsHLD = kFALSE;


   fxBuffer = new Char_t[Trb_BUFSIZE];
   
   TGo4Log::Info("%s user source contains %d files", (fIsHLD ? "HLD" : "GET4"), fNames->GetSize());

   return 0;
}


Bool_t TUserSource::OpenNextFile()
{
   if ((fNames==0) || (fNames->GetSize()==0)) {
      SetCreateStatus(1);
      SetErrMess("End of file input");
      SetEventStatus(1);
      throw TGo4EventEndException(this);
      return kFALSE;
   }

   TObject* obj = fNames->First();
   TString nextname = obj->GetName();
   fNames->Remove(fNames->FirstLink());
   delete obj;

   if (fIsHLD) {
      if(fxFile.isOpened()) fxFile.Close();

      //! Open connection/file
      if(!fxFile.OpenRead(nextname.Data())) {
         SetCreateStatus(1);
         SetErrMess(Form("Eror opening user file:%s", nextname.Data()));
         throw TGo4EventErrorException(this);
      }

      TGo4Log::Info("Open HLD file %s", nextname.Data());
   } else {

      if (fxDatFile) { fclose(fxDatFile); fxDatFile = 0; }

      fxDatFile = fopen(nextname.Data(), "r");
      if (fxDatFile == 0) {
         SetCreateStatus(1);
         SetErrMess(Form("Eror opening user file:%s", nextname.Data()));
         throw TGo4EventErrorException(this);
      }

      char sbuf[100];

      fgets(sbuf, 100, fxDatFile);

      TGo4Log::Info("Open DAT file %s line %s", nextname.Data(), sbuf);
   }

   return kTRUE;
}
