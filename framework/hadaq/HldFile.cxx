/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "hadaq/HldFile.h"


// dabc::Object* dabc::FileInterface::fmatch(const char* fmask) { return 0; }



hadaq::HldFile::HldFile() :
   dabc::BasicFile(),
   fRunNumber(0),
   fEOF(true)
{
}

hadaq::HldFile::~HldFile()
{
   Close();
}

bool hadaq::HldFile::OpenWrite(const char* fname, uint32_t runid)
{
   if (isOpened()) return false;

   if (fname==0 || *fname==0) {
      fprintf(stderr, "file name not specified\n");
      return false;
   }

   CheckIO();

   fd = io->fopen(fname, "w");
   if (fd==0) {
      fprintf(stderr, "File open failed %s for writing\n", fname);
      return false;
   }

   // put here a dummy event into file:

   hadaqs::RawEvent evnt;
   evnt.Init(0, runid, hadaqs::EvtId_runStart);
   if(!WriteBuffer(&evnt, sizeof(evnt))) {
      CloseBasicFile();
      return false;
   }

   fReadingMode = false;
   fRunNumber = runid;

   return true;
}

bool hadaq::HldFile::OpenRead(const char* fname)
{
   if (isOpened()) return false;

   if (fname==0 || *fname==0) {
      fprintf(stderr, "file name not specified\n");
      return false;
   }

   CheckIO();

   fd = io->fopen(fname,  "r");
   if (fd==0) {
      fprintf(stderr, "File open failed %s for reading\n", fname);
      return false;
   }
   fReadingMode = true;

//   DOUT0("Open HLD file %s for reading", fname);

   hadaqs::RawEvent evnt;
   uint32_t size = sizeof(hadaqs::RawEvent);

   // printf("starts reading into buf %u isreading %u \n", (unsigned) size, (unsigned)isReading());

   if (!ReadBuffer(&evnt, &size, true)) {
      fprintf(stderr,"Cannot read starting event from file\n");
      CloseBasicFile();
      return false;
   }

   if ((size!=sizeof(hadaqs::RawEvent)) || (evnt.GetId() != hadaqs::EvtId_runStart)) {
      fprintf(stderr,"Did not found start event at the file beginning\n");
      CloseBasicFile();
      return false;
   }

//   DOUT0("Find start event at the file begin");

   fRunNumber = evnt.GetRunNr();
   fEOF = false;

   return true;
}

void hadaq::HldFile::Close()
{
  if (isWriting()) {
      // need to add empty terminating event:
      hadaqs::RawEvent evnt;
      evnt.Init(0, fRunNumber, hadaqs::EvtId_runStop);
      WriteBuffer(&evnt, sizeof(evnt));
   }

  CloseBasicFile();

  fRunNumber=0;
  fEOF = true;
}



bool hadaq::HldFile::WriteBuffer(void* buf, uint32_t bufsize)
{
   if (!isWriting() || (buf==0) || (bufsize==0)) return false;

   if (io->fwrite(buf, bufsize, 1, fd)!=1) {
      fprintf(stderr, "fail to write buffer payload of size %u\n", (unsigned) bufsize);
      CloseBasicFile();
      return false;
   }

   return true;
}

bool hadaq::HldFile::ReadBuffer(void* ptr, uint32_t* sz, bool onlyevent)
{
   if (!isReading() || (ptr==0) || (sz==0) || (*sz < sizeof(hadaqs::HadTu))) return false;

   uint64_t maxsz = *sz; *sz = 0;

   size_t readsz = io->fread(ptr, 1, (onlyevent ? sizeof(hadaqs::HadTu) : maxsz), fd);

   //printf("Read HLD portion of data %u max %u\n", (unsigned) readsz, (unsigned) maxsz);

   if (readsz < sizeof(hadaqs::HadTu)) {
      if (!io->feof(fd)) fprintf(stderr, "Fail to read next portion while no EOF detected\n");
      fEOF = true;
      return false;
   }

   hadaqs::HadTu* hdr = (hadaqs::HadTu*) ptr;


   if (onlyevent) {

      if (hdr->GetPaddedSize() > maxsz) {
         fprintf(stderr, "Buffer %u too small to read next event %u from hld file\n", (unsigned) maxsz, (unsigned)hdr->GetPaddedSize());
         return false;
      }

      // printf("Expect next event of size %u\n", (unsigned) hdr->GetPaddedSize());

      readsz = io->fread((char*) ptr + sizeof(hadaqs::HadTu), 1, hdr->GetPaddedSize() - sizeof(hadaqs::HadTu), fd);

      // printf("Read size %u expects %u \n", (unsigned) readsz, (unsigned) (hdr->GetPaddedSize() - sizeof(hadaqs::HadTu)));

      // not possible to read event completely
      if ( readsz != (hdr->GetPaddedSize() - sizeof(hadaqs::HadTu))) {
         fprintf(stderr, "HLD file reading problem: readsz %u not matches with padded size %u, abort reading\n", readsz, hdr->GetPaddedSize() - sizeof(hadaqs::HadTu));
         fEOF = true;
         return false;
      }

      if ((hdr->GetPaddedSize() == sizeof(hadaqs::RawEvent)) && (((hadaqs::RawEvent*)hdr)->GetId() == hadaqs::EvtId_runStop)) {
         // we are not deliver such stop event to the top

         // printf("Find stop event at the file end\n");
         fEOF = true;
         return false;
      }

      *sz = hdr->GetPaddedSize();
      return true;
   }

   size_t checkedsz = 0;


   while (checkedsz < readsz) {
      // special case when event was read not completely
      // or we want to provide only event

      size_t restsize = readsz - checkedsz;
      if (restsize >= sizeof(hadaqs::HadTu)) restsize = hdr->GetPaddedSize();

//      printf("tu padded size = %u\n", (unsigned) restsize);

      if ((restsize == sizeof(hadaqs::RawEvent)) && (((hadaqs::RawEvent*)hdr)->GetId() == hadaqs::EvtId_runStop)) {
         // we are not deliver such stop event to the top

         // printf("Find stop event at the file end\n");
         fEOF = true;
         break;
      }

      bool not_enough_place_for_next_event = checkedsz + restsize > readsz;

      if (not_enough_place_for_next_event || (onlyevent && (checkedsz>0))) {

         if (not_enough_place_for_next_event && (readsz<maxsz)) fEOF = true;

         // return file pointer to the begin of event
         io->fseek(fd, -(readsz - checkedsz), true);

         break;
      }
      checkedsz += hdr->GetPaddedSize();
      hdr = (hadaqs::HadTu*) ((char*) hdr + hdr->GetPaddedSize());
   }

   // detect end of file by such method
   if ((readsz<maxsz) && (checkedsz == readsz) && !fEOF) fEOF = true;

   *sz = checkedsz;

//   DOUT0("Return size %u", (unsigned) checkedsz);

   return checkedsz>0;
}
