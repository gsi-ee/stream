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
   fbIsOpen(kFALSE),
   fxArgs(""),
   fiPort(0),
   fxFile(0),
   fxBuffer(0),
   fxBufsize(0),
   fxCursor(0),
   fxBufEnd(0)
{
}

TUserSource::TUserSource(const char* name,
						       const char* args,
						       Int_t port) :
	TGo4EventSource(name),
	fbIsOpen(kFALSE),
	fxArgs(args),
	fiPort(port),
	fxFile(0),
	fxBuffer(0),
	fxBufsize(0),
	fxCursor(0),
	fxBufEnd(0)
{
   Open();
}

TUserSource::TUserSource(TGo4UserSourceParameter* par) :
	TGo4EventSource(" "),
	fbIsOpen(kFALSE),
	fxArgs(""),
	fiPort(0),
	fxFile(0),
	fxBuffer(0),
	fxBufsize(0),
	fxCursor(0),
	fxBufEnd(0)
{
	if(par)
	{
		SetName(par->GetName());
		SetPort(par->GetPort());
		SetArgs(par->GetExpression());
		Open();
	}
	else
	{
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

std::streamsize TUserSource::ReadFile(Char_t* dest, size_t len)
{
	fxFile->read(dest, len);
	if(fxFile->eof() || !fxFile->good())
	{
		SetCreateStatus(1);
		SetErrMess(Form("End of input file %s", GetName()));
		SetEventStatus(1);
		throw TGo4EventEndException(this);
	}
	//cout <<"ReadFile reads "<< (hex) << fxFile->gcount()<<" bytes to 0x"<< (hex) <<(int) dest<< endl;
	return fxFile->gcount();
}

Bool_t TUserSource::NextBuffer()
{
	//! Each buffer contains one full event:
	//! First read event header:
	ReadFile(fxBuffer, sizeof(hadaq::RawEvent));

	//! Then read rest of buffer from file
	size_t evlen = ((hadaq::RawEvent*) fxBuffer)->GetSize();
	
	//! Account padding of events to 8 byte boundaries:
	while((evlen % 8) != 0)
	{
		evlen++;
//		cout << "Next Buffer extends for padding the length to " << evlen << endl;
	}
//	cout << "Next Buffer reads event of size:" << evlen << endl;
	if(evlen > Trb_BUFSIZE)
	{
		SetErrMess(Form("Next event length 0x%x bigger than read buffer limit 0x%x", (unsigned) evlen, (unsigned) Trb_BUFSIZE));
		SetEventStatus(1);
		throw TGo4EventEndException(this);
	}
	ReadFile(fxBuffer + sizeof(hadaq::RawEvent), evlen - sizeof(hadaq::RawEvent));
	fxBufsize = evlen;
	fxBufEnd = (Short_t*)(fxBuffer+evlen);
	return kTRUE;

}

Bool_t TUserSource::NextEvent(TGo4MbsEvent* target)
{
	//! We fill complete hadaq event into one mbs subevent, not to lose header information
	//! decomposing hades subevents is left to the user analysis processor!

	fiEventLen = (fxBufsize/sizeof(Short_t)) ; // data payload of mbs is full hades event in buffer
	fxSubevHead.fsProcid = base::proc_TRBEvent; // mark to be processed by TTrbProc
	int evcounter = ((hadaq::RawEvent*) fxBuffer)->GetSeqNr();
//	cout <<"Next Event is filling mbs event, fullid:"<<fxSubevHead.fiFullid<<", nr:"<<evcounter<< endl;
	target->AddSubEvent(fxSubevHead.fiFullid, (Short_t*) fxBuffer, fiEventLen + 2, kTRUE);
	target->SetDlen(fiEventLen+2+4); // total length of pseudo mbs event
//	cout <<"Next Event setting event length:"<<fiEventLen+2+4<< endl;

	target->SetCount(evcounter);

//	return kFALSE; // this means we need another file buffer for this event, what is never the case since buffer contains just one event
	return kTRUE; // event is ready
}

Bool_t TUserSource::BuildEvent(TGo4EventElement* dest)
{
	TGo4MbsEvent* evnt = dynamic_cast<TGo4MbsEvent*> (dest);
	if (evnt==0) return kFALSE;
	// this generic loop is intended to handle buffers with several events. we keep it here,
	// although our "buffers" consists of single events.
	do	{
		NextBuffer(); // note: next buffer will throw exception on end of file
	}	while (!NextEvent(evnt));

	return kTRUE;
}

Int_t TUserSource::Open()
{
	if(fbIsOpen) return -1;
	TGo4Log::Info("Open of TUserSource");

	//! Open connection/file
	fxFile = new std::ifstream(GetName(), ios::binary);
	if((fxFile==0) || !fxFile->good()) {
		delete fxFile; fxFile = 0;
		SetCreateStatus(1);
		SetErrMess(Form("Eror opening user file:%s", GetName()));
		throw TGo4EventErrorException(this);
	}
	fxBuffer = new Char_t[Trb_BUFSIZE];

	fbIsOpen = kTRUE;
	return 0;
}

Int_t TUserSource::Close()
{
	if(!fbIsOpen) return -1;
	TGo4Log::Info("Close of TUserSource");
	delete [] fxBuffer;
	Int_t status = 0;	//! Closestatus of source
	delete fxFile;
	fbIsOpen = kFALSE;
	return status;
}
