#ifndef TUSERSOURCE_H
#define TUSERSOURCE_H

#include "TGo4EventSource.h"

#include "hadaq/HldFile.h"

#include "dogma/DogmaFile.h"

#include <cstdio>

class TGo4UserSourceParameter;
class TGo4MbsEvent;
class TList;

/** Custom user source to read files formats supported by stream framework */

class TUserSource : public TGo4EventSource {

   protected:
      /** Optional argument string */
      TString fxArgs;

      /** Optional port number  */
      Int_t fiPort = 0;

      /** list of all files names */
      TList* fNames = nullptr;

      /** indicates if HLD file will be read */
      Bool_t fIsHLD = kTRUE;

      /** current HLD file */
      hadaq::HldFile fxFile;

      /** indicates if DOGMA file will be read */
      Bool_t fIsDOGMA = kFALSE;

      /** current DOGMA file */
      dogma::DogmaFile fxDogmaFile;

      /** working buffer */
      Char_t* fxBuffer = nullptr;

      /** current DAT file */
      FILE* fxDatFile = nullptr;

      /** event counter */
      Int_t fEventCounter = 0;


      Bool_t OpenNextFile();

      /** Open the file or connection. */
      Int_t Open();

      Bool_t BuildDatEvent(TGo4MbsEvent *dest);
      Bool_t BuildHldEvent(TGo4MbsEvent *dest);
      Bool_t BuildDogmaEvent(TGo4MbsEvent *dest);

   public:

      TUserSource();

      /** Create source specifying values directly */
      TUserSource(const char* name, const char* args, Int_t port);

      /** Create source from setup within usersource parameter object */
      TUserSource(TGo4UserSourceParameter* par);

      virtual ~TUserSource();

      /** This method checks if event class is suited for the source */
      virtual Bool_t CheckEventClass(TClass* cl);

      /** This methods actually fills the target event class which is passed as pointer.
       * Uses the latest event which is referenced
       * by fxEvent or fxBuffer. Does _not_ fetch a new event
       * from source, therefore one source event may be used
       * to fill several TTrbEvent classes. To get a new
       * event call NextEvent() before this method.*/
      virtual Bool_t BuildEvent(TGo4EventElement* dest);

      /** get args */
      const char* GetArgs() const { return fxArgs.Data(); }
      /** set args */
      void SetArgs(const char* arg) { fxArgs=arg; }

      /** get port */
      Int_t GetPort() const { return fiPort; }
      /** set port */
      void SetPort(Int_t val) { fiPort=val; }

   ClassDef(TUserSource, 1)
};

#endif
