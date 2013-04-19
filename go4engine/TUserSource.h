#ifndef TUSERSOURCE_H
#define TUSERSOURCE_H

#include "TGo4EventSource.h"
#include <fstream>

#include "TGo4MbsEvent.h"

class TGo4UserSourceParameter;

class TUserSource : public TGo4EventSource {

   protected:
      Bool_t fbIsOpen;

      /** Optional argument string */
      TString fxArgs;

      /** Optional port number  */
      Int_t fiPort;

      /** file that contains the data in ascii format. */
      std::ifstream* fxFile;

      /** true if next buffer needs to be read from file */
      // Bool_t fbNeedNewBuffer;

      /** setup subevent ids for mbs container */
      TGo4SubEventHeader10 fxSubevHead;

      /** working buffer */
      Char_t* fxBuffer;

      /** actual size of bytes read into buffer most recently */
      std::streamsize fxBufsize;

      /** cursor in buffer */
      Short_t* fxCursor;

      /** points to end of buffer */
      Short_t* fxBufEnd;

      /** read next buffer from file into memory */
      Bool_t NextBuffer();

      /** Fill next xsys event from buffer location into output structure */
      Bool_t NextEvent(TGo4MbsEvent* target);

      /** Read from input file and check */
      std::streamsize ReadFile(Char_t* dest, size_t len);

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
