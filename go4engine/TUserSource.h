#ifndef TUSERSOURCE_H
#define TUSERSOURCE_H

#include "TGo4EventSource.h"

#include "hadaq/HldFile.h"

#include <fstream>

class TGo4UserSourceParameter;
class TGo4MbsEvent;

class TUserSource : public TGo4EventSource {

   protected:
      /** Optional argument string */
      TString fxArgs;

      /** Optional port number  */
      Int_t fiPort;

      /** file that contains the data in ascii format. */
      hadaq::HldFile fxFile;

      /** working buffer */
      Char_t* fxBuffer;

   public:

      TUserSource();

      /** Create source specifying values directly */
      TUserSource(const char* name, const char* args, Int_t port);

      /** Creat source from setup within usersource parameter object */
      TUserSource(TGo4UserSourceParameter* par);

      virtual ~TUserSource();

      /** Open the file or connection. */
      virtual Int_t Open();

      /** Close the file or connection. */
      virtual Int_t Close();

      /** This method checks if event class is suited for the source */
      virtual Bool_t CheckEventClass(TClass* cl);

      /** This methods actually fills the target event class which is passed as pointer.
       * Uses the latest event which is referenced
       * by fxEvent or fxBuffer. Does _not_ fetch a new event
       * from source, therefore one source event may be used
       * to fill several TTrbEvent classes. To get a new
       * event call NextEvent() before this method.*/
      virtual Bool_t BuildEvent(TGo4EventElement* dest);

      const char* GetArgs() const { return fxArgs.Data(); }
      void SetArgs(const char* arg) { fxArgs=arg; }

      Int_t GetPort() const { return fiPort; }
      void SetPort(Int_t val) { fiPort=val; }

   ClassDef(TUserSource, 1)
};

#endif
