#include "TUserSource.h"

#include "TClass.h"
#include "TList.h"
#include "TString.h"
#include "TObjString.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

#include "TGo4EventErrorException.h"
#include "TGo4EventEndException.h"
#include "TGo4EventTimeoutException.h"
#include "TGo4UserSourceParameter.h"
#include "TGo4FileSource.h"
#include "TGo4MbsEvent.h"
#include "TGo4Log.h"
#include "TGo4Analysis.h"

#include "base/defines.h"

#include "hadaq/definess.h"

#define Trb_BUFSIZE 0x80000


///////////////////////////////////////////////////////////////////////////////////////////////////
/// constructor

TUserSource::TUserSource() :
   TGo4EventSource("default Trb source")
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// constructor

TUserSource::TUserSource(const char* name, const char* args, Int_t port) :
   TGo4EventSource(name),
   fxArgs(args),
   fiPort(port)
{
   Open();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// constructor

TUserSource::TUserSource(TGo4UserSourceParameter* par) :
   TGo4EventSource(" ")
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

///////////////////////////////////////////////////////////////////////////////////////////////////
/// destructor

TUserSource::~TUserSource()
{
   TGo4Log::Info("Close of TUserSource");
   fxFile.Close();
   fxDogmaFile.Close();

   if (fxBuffer) {
      delete [] fxBuffer;
      fxBuffer = nullptr;
   }

   if (fxDatFile) {
      fclose(fxDatFile);
      fxDatFile = nullptr;
   }

   if (fNames) {
      fNames->Delete();
      delete fNames;
      fNames = nullptr;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// check event class

Bool_t TUserSource::CheckEventClass(TClass* cl)
{
  return cl->InheritsFrom(TGo4MbsEvent::Class());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// build dat event

Bool_t TUserSource::BuildDatEvent(TGo4MbsEvent* evnt)
{
   char buf[100];

   if (!fxDatFile || feof(fxDatFile))
      if (!OpenNextFile()) return kFALSE;

   int cnt = 0;

   uint32_t arr[1000];
   arr[cnt++] = 0xaaaa0000; // mark all data as coming from get4 0

   while (!feof(fxDatFile) && (cnt<1000)) {
      if (fgets(buf, sizeof(buf), fxDatFile)==0) {
         fclose(fxDatFile);
         fxDatFile = nullptr;
         break;
      }

      if (strlen(buf) < 10) { printf("VERY SHORT LINE %s!!!\n", buf); break; }


      unsigned value;
      long tm;

      if (sscanf(buf,"%ld ps %X", &tm, &value)!=2) break;

      // printf("value = %8X buf = %s", value, buf);

      arr[cnt++] = value;
   }

   if (cnt == 0) return kFALSE;

   uint32_t bufsize = cnt*sizeof(uint32_t);

   TGo4SubEventHeader10 fxSubevHead;
   memset((void *) &fxSubevHead, 0, sizeof(fxSubevHead));
   fxSubevHead.fsProcid = 1;

   evnt->AddSubEvent(fxSubevHead.fiFullid, (Short_t*) arr, bufsize/sizeof(Short_t) + 2, kTRUE);

   evnt->SetCount(fEventCounter++);

   // set total MBS event length, which must include MBS header itself
   evnt->SetDlen(bufsize/sizeof(Short_t) + 2 + 6);

   return kTRUE; // event is ready
}


///////////////////////////////////////////////////////////////////////////////////////////////////
/// build HADAQ event

Bool_t TUserSource::BuildHldEvent(TGo4MbsEvent *evnt)
{
   uint32_t bufsize = Trb_BUFSIZE;

   Bool_t trynext = kFALSE;
   if (!fxFile.isOpened() || fxFile.eof())
      trynext = kTRUE;
   else if (!fxFile.ReadBuffer(fxBuffer, &bufsize, true))
      trynext = kTRUE;

   if (trynext) {
      bufsize = Trb_BUFSIZE;
      Bool_t isok = OpenNextFile();
      if (isok)
         isok = fxFile.ReadBuffer(fxBuffer, &bufsize, true);
      if (!isok) {
         SetCreateStatus(1);
         SetErrMess("End of HLD input");
         SetEventStatus(1);
         throw TGo4EventEndException(this);
         return kFALSE;
      }
   }

   TGo4SubEventHeader10 fxSubevHead;
   memset((void *) &fxSubevHead, 0, sizeof(fxSubevHead));
   fxSubevHead.fsProcid = base::proc_TRBEvent; // mark to be processed by TTrbProc

   evnt->AddSubEvent(fxSubevHead.fiFullid, (Short_t*) fxBuffer, bufsize/sizeof(Short_t) + 2, kTRUE);

   evnt->SetCount(((hadaqs::RawEvent*) fxBuffer)->GetSeqNr());

   // set total MBS event length, which must include MBS header itself
   evnt->SetDlen(bufsize/sizeof(Short_t) + 2 + 6);

   return kTRUE; // event is ready
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// build DOGMA event

Bool_t TUserSource::BuildDogmaEvent(TGo4MbsEvent *evnt)
{
   Bool_t is_empty_event = kTRUE;
   Int_t back_counter = 1000;
   uint32_t bufsize;
   dogma::DogmaEvent *devnt = (dogma::DogmaEvent *) fxBuffer;

   while (is_empty_event && (back_counter-- > 0)) {
      bufsize = Trb_BUFSIZE;

      Bool_t trynext = kFALSE;
      if (!fxDogmaFile.isOpened() || fxDogmaFile.eof())
         trynext = kTRUE;
      else if (!fxDogmaFile.ReadBuffer(fxBuffer, &bufsize, true))
         trynext = kTRUE;

      if (trynext) {
         bufsize = Trb_BUFSIZE;
         Bool_t isok = OpenNextFile();
         if (isok)
            isok = fxDogmaFile.ReadBuffer(fxBuffer, &bufsize, true);
         if (!isok) {
            SetCreateStatus(1);
            SetErrMess("End of DOGMA input");
            SetEventStatus(1);
            throw TGo4EventEndException(this);
            return kFALSE;
         }
      }

      is_empty_event = devnt->GetSeqId() == 0 && devnt->GetEventLen() == 16;
   }

   TGo4SubEventHeader10 fxSubevHead;
   memset((void *) &fxSubevHead, 0, sizeof(fxSubevHead));
   fxSubevHead.fsProcid = base::proc_DOGMAEvent; // mark to be processed by TDogmaProc

   if (is_empty_event) {
      evnt->SetCount(0);
      evnt->SetDlen(2 + 6); // empty event, not need to be processed
   } else {
      evnt->SetCount(devnt->GetSeqId());
      evnt->AddSubEvent(fxSubevHead.fiFullid, (Short_t*) fxBuffer, bufsize/sizeof(Short_t) + 2, kTRUE);
      // set total MBS event length, which must include MBS header itself
      evnt->SetDlen(bufsize/sizeof(Short_t) + 2 + 6);
   }

   return kTRUE; // event is ready
}


///////////////////////////////////////////////////////////////////////////////////////////////////
/// build event

Bool_t TUserSource::BuildEvent(TGo4EventElement* dest)
{
   TGo4MbsEvent* evnt = dynamic_cast<TGo4MbsEvent*> (dest);
   if (!evnt || !fxBuffer)
      return kFALSE;

   if (fIsHLD)
      return BuildHldEvent(evnt);

   if (fIsDOGMA)
      return BuildDogmaEvent(evnt);

   return BuildDatEvent(evnt);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// open file

Int_t TUserSource::Open()
{
   TString fname = GetName();

   fIsHLD = kTRUE;
   fIsDOGMA = kFALSE;
   if (fname.EndsWith(".dat"))
      fIsHLD = kFALSE;
   else if (fname.EndsWith(".dld")) {
      fIsHLD = kFALSE;
      fIsDOGMA = kTRUE;
   }

   if(fname.Contains("*") || fname.Contains("?")) {
      // name indicates wildcard expression
      fNames = TGo4FileSource::ProducesFilesList(fname.Data());
   } else if (fname.EndsWith(".hll")) {
      fIsHLD = kTRUE;
      fNames = new TList;
      std::ifstream filein(fname.Data());
      std::string line;
      while(std::getline(filein, line)) {
         if (!line.empty() && (line[0] != '#')) {
            TString name1 = line.c_str();
            fNames->Add(new TObjString(name1));
            if (name1.EndsWith(".dld")) {
               fIsHLD = kFALSE;
               fIsDOGMA = kTRUE;
            }
         }
      }
   } else {
      fNames = new TList;
      fNames->Add(new TObjString(fname.Data()));
   }

   fxBuffer = new Char_t[Trb_BUFSIZE];

   TGo4Log::Info("%s user source contains %d files", (fIsHLD ? "HLD" : "GET4"), fNames ? fNames->GetSize() : 0);

   TGo4Analysis::Instance()->SetInputFileName(fname.Data());

   return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// open next file

Bool_t TUserSource::OpenNextFile()
{
   if (!fNames || (fNames->GetSize() == 0)) {
      SetCreateStatus(1);
      SetErrMess("End of file input");
      SetEventStatus(1);
      throw TGo4EventEndException(this);
      return kFALSE;
   }

   printf("Open next file %s\n", fNames->At(0)->GetName());

   TObject* obj = fNames->First();
   TString nextname = obj->GetName();
   fNames->Remove(fNames->FirstLink());
   delete obj;

   TGo4Analysis* ana = TGo4Analysis::Instance();
   ana->SetNewInputFile(kTRUE);
   ana->SetInputFileName(nextname.Data());

   if (fIsHLD) {
      if(fxFile.isOpened())
         fxFile.Close();

      ///<! Open HLD file
      if(!fxFile.OpenRead(nextname.Data())) {
         SetCreateStatus(1);
         SetErrMess(Form("Error opening HLD file: %s", nextname.Data()));
         throw TGo4EventErrorException(this);
      }

      TGo4Log::Info("Open HLD file %s", nextname.Data());
   } else if (fIsDOGMA) {
      if(fxDogmaFile.isOpened())
         fxDogmaFile.Close();

      ///<! Open DOGMA file
      if(!fxDogmaFile.OpenRead(nextname.Data())) {
         SetCreateStatus(1);
         SetErrMess(Form("Error opening DOGMA file: %s", nextname.Data()));
         throw TGo4EventErrorException(this);
      }

      TGo4Log::Info("Open DOGMA file %s", nextname.Data());
   } else {
      if (fxDatFile) {
         fclose(fxDatFile);
         fxDatFile = nullptr;
      }

      fxDatFile = fopen(nextname.Data(), "r");
      if (!fxDatFile) {
         SetCreateStatus(1);
         SetErrMess(Form("Error opening DAT file: %s", nextname.Data()));
         throw TGo4EventErrorException(this);
      }

      char sbuf[100];

      fgets(sbuf, 100, fxDatFile);

      TGo4Log::Info("Open DAT file %s line %s", nextname.Data(), sbuf);
   }

   return kTRUE;
}
