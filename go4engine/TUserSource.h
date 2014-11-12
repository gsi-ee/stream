#ifndef TUSERSOURCE_H
#define TUSERSOURCE_H

#include "TGo4EventSource.h"

#include "hadaq/HldFile.h"

#include <stdio.h>

class TGo4UserSourceParameter;
class TGo4MbsEvent;
class TList;

class TUserSource : public TGo4EventSource {

   protected:
      /** Optional argument string */
      TString fxArgs;

      /** Optional port number  */
      Int_t fiPort;

      /** list of all files names */
      TList* fNames;

      /** indicates if HLD file will be read */
      Bool_t fIsHLD;

      /** current HLD file */
      hadaq::HldFile fxFile;

      /** working buffer */
      Char_t* fxBuffer;

      /** current DAT file */
      FILE* fxDatFile;

      /** event counter */
      Int_t fEventCounter;


      Bool_t OpenNextFile();

      /** Open the file or connection. */
      Int_t Open();

   public:

      TUserSource();

      /** Create source specifying values directly */
      TUserSource(const char* name, const char* args, Int_t port);

      /** Creat source from setup within usersource parameter object */
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

      virtual Bool_t BuildDatEvent(TGo4MbsEvent* dest);


      const char* GetArgs() const { return fxArgs.Data(); }
      void SetArgs(const char* arg) { fxArgs=arg; }

      Int_t GetPort() const { return fiPort; }
      void SetPort(Int_t val) { fiPort=val; }

   ClassDef(TUserSource, 1)
};

#endif
