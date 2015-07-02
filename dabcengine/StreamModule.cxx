// $Id$

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

#include "StreamModule.h"

#include "hadaq/Iterator.h"

#include "dabc/Manager.h"
#include "dabc/Factory.h"

#include <stdlib.h>

#include "base/Buffer.h"
#include "base/StreamProc.h"
#include "dabc/ProcMgr.h"

// ==================================================================================

typedef void* entryfunc();

dabc::StreamModule::StreamModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fProcMgr(0),
   fTotalSize(0),
   fTotalEvnts(0),
   fTotalOutEvnts(0)
{
   // we need one input and no outputs
   EnsurePorts(1, 0);


   const char* dabcsys = getenv("DABCSYS");
   const char* streamsys = getenv("STREAMSYS");
   if (dabcsys==0) {
      EOUT("DABCSYS variable not set, cannot run stream framework");
      dabc::mgr.StopApplication();
      return;
   }

   if (streamsys==0) {
      EOUT("STREAMSYS variable not set, cannot run stream framework");
      dabc::mgr.StopApplication();
      return;
   }

   fWorkerHierarchy.Create("Worker");

   fProcMgr = new dabc::ProcMgr;

   fProcMgr->fTop = fWorkerHierarchy;

   // base::StreamProc::SetMarksQueueCapacity(10000);
   // base::StreamProc::SetBufsQueueCapacity(100);

   bool second = system("ls second.C >/dev/null 2>/dev/null") == 0;

   std::string exec = dabc::format("g++ %s/dabcengine/stream_engine.cpp -O2 -fPIC -Wall -I. -I%s/include %s"
                                   "-shared -Wl,-soname,librunstream.so -Wl,--no-as-needed -Wl,-rpath,%s/lib -Wl,-rpath,%s/lib  -o librunstream.so",
                                   streamsys, streamsys, (second ? "-D_SECOND_ " : ""), dabcsys, streamsys);

   system("rm -f ./librunstream.so");

   DOUT0("Executing %s", exec.c_str());

   int res = system(exec.c_str());

   if (res!=0) {
      EOUT("Fail to compile first.C/second.C scripts. Abort");
      dabc::mgr.StopApplication();
      return;
   }

   if (!dabc::Factory::LoadLibrary("librunstream.so")) {
      EOUT("Fail to load generated librunstream.so library");
      dabc::mgr.StopApplication();
      return;
   }

   entryfunc* func = (entryfunc*) dabc::Factory::FindSymbol("stream_engine");
   if (func==0) {
      EOUT("Fail to find stream_engine function in librunstream.so library");
      dabc::mgr.StopApplication();
      return;
   }

   func();

   CreatePar("Events").SetRatemeter(false, 3.).SetUnits("Ev");

   // remove pointer, let other modules to create and use it
   base::ProcMgr::ClearInstancePointer();

   Publish(fWorkerHierarchy, dabc::format("$CONTEXT$/%s", GetName()));
}

dabc::StreamModule::~StreamModule()
{
   delete fProcMgr;
   fProcMgr = 0;
}


int dabc::StreamModule::ExecuteCommand(dabc::Command cmd)
{
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

void dabc::StreamModule::BeforeModuleStart()
{
   fProcMgr->UserPreLoop();
}

void dabc::StreamModule::AfterModuleStop()
{
   fProcMgr->UserPostLoop();

   DOUT0("STOP STREAM MODULE len %lu evnts %lu out %lu", fTotalSize, fTotalEvnts, fTotalOutEvnts);
}

bool dabc::StreamModule::ProcessRecv(unsigned port)
{
   if (CanRecv()) {
      dabc::Buffer buf = Recv();

      if (buf.GetTypeId() == dabc::mbt_EOF) return true;

      fTotalSize += buf.GetTotalSize();

      hadaq::ReadIterator iter(buf);

      while (iter.NextEvent()) {

         fTotalEvnts++;

         Par("Events").SetValue(1);

         // TODO - later we need to use DABC buffer here to allow more complex
         // analysis when many dabc buffers required at the same time to analyze data

         base::Buffer bbuf;

         bbuf.makereferenceof(iter.evnt(), iter.evntsize());

         bbuf().kind = base::proc_TRBEvent;
         bbuf().boardid = 0;
         bbuf().format = 0;

         fProcMgr->ProvideRawData(bbuf);

         // scan new data
         fProcMgr->ScanNewData();

         if (fProcMgr->IsRawAnalysis()) {

            fProcMgr->SkipAllData();

         } else {

            //TGo4Log::Info("Analyze data");

            // analyze new sync markers
            if (fProcMgr->AnalyzeSyncMarkers()) {

               // get and redistribute new triggers
               fProcMgr->CollectNewTriggers();

               // scan for new triggers
               fProcMgr->ScanDataForNewTriggers();
            }
         }

         base::Event* outevent = 0;

         // now producing events as much as possible
         while (fProcMgr->ProduceNextEvent(outevent)) {

            fTotalOutEvnts++;

            bool store = fProcMgr->ProcessEvent(outevent);

            if (store) {
               // implement events store later
            }
         }

         delete outevent;

         // fProcMgr->SkipAllData();
      }
   }


   return true;
}

