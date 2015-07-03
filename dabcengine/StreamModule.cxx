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

#include "dabc/BinaryFile.h"

#include "StreamModule.h"

#include "hadaq/Iterator.h"

#include "dabc/Manager.h"
#include "dabc/Factory.h"
#include "dabc/Iterator.h"
#include "dabc/Buffer.h"

#include <stdlib.h>

#include "base/Buffer.h"
#include "base/StreamProc.h"
#include "dabc/ProcMgr.h"

// ==================================================================================

dabc::StreamModule::StreamModule(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fParallel(0),
   fInitFunc(0),
   fProcMgr(0),
   fTotalSize(0),
   fTotalEvnts(0),
   fTotalOutEvnts(0)
{
   fParallel = Cfg("parallel",cmd).AsInt(0);

   // we need one input and no outputs
   EnsurePorts(1, fParallel<0 ? 0 : fParallel);

   fInitFunc = cmd.GetPtr("initfunc");

   if ((fParallel>=0) && (fInitFunc==0)) {
      // first generate and load init func

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

      fInitFunc = dabc::Factory::FindSymbol("stream_engine");
      if (fInitFunc==0) {
         EOUT("Fail to find stream_engine function in librunstream.so library");
         dabc::mgr.StopApplication();
         return;
      }
   }


   if (fParallel>=0) {
      fAsf = Cfg("asf",cmd).AsStr();
      // do not autosave is specified, module will not stop when data source disappears
      if (fAsf.length()==0) SetAutoStop(false);
      CreatePar("Events").SetRatemeter(false, 3.).SetUnits("Ev");
   } else {
      // SetAutoStop(false);
   }

   fWorkerHierarchy.Create("Worker");

   Publish(fWorkerHierarchy, dabc::format("$CONTEXT$/%s", GetName()));
}

dabc::StreamModule::~StreamModule()
{
   if (fProcMgr) {
      delete fProcMgr;
      fProcMgr = 0;
   }
}

typedef void* entryfunc();



void dabc::StreamModule::OnThreadAssigned()
{
   if ((fInitFunc!=0) && (fParallel<=0)) {

      entryfunc* func = (entryfunc*) fInitFunc;

      fProcMgr = new dabc::ProcMgr;
      fProcMgr->fTop = fWorkerHierarchy;

      func();

      // remove pointer, let other modules to create and use it
      base::ProcMgr::ClearInstancePointer();
   } else
   if ((fParallel>0) && (fInitFunc!=0))  {
      for (int n=0;n<fParallel;n++) {
         std::string mname = dabc::format("%s%d", GetName(), n);
         dabc::CmdCreateModule cmd("dabc::StreamModule", mname);
         cmd.SetPtr("initfunc", fInitFunc);
         cmd.SetInt("parallel", -1);

         DOUT0("Create module %s", mname.c_str());

         dabc::mgr.Execute(cmd);

         dabc::ModuleRef m = dabc::mgr.FindModule(mname);

         DOUT0("Connect %s ->%s", OutputName(n,true).c_str(), m.InputName(0).c_str());
         dabc::Reference r = dabc::mgr.Connect(OutputName(n,true), m.InputName(0));
         DOUT0("Connect output %u connected %s", n, DBOOL(IsOutputConnected(n)));

         m.Start();
      }
   }
}


int dabc::StreamModule::ExecuteCommand(dabc::Command cmd)
{
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

void dabc::StreamModule::BeforeModuleStart()
{
   DOUT0("START STREAM MODULE %s inp %s", GetName(), DBOOL(IsInputConnected(0)));

   if (fProcMgr) fProcMgr->UserPreLoop();
}

void dabc::StreamModule::AfterModuleStop()
{
   if (fProcMgr) fProcMgr->UserPostLoop();

//   dabc::Iterator iter(fWorkerHierarchy);
//   while (iter.next())
//      DOUT0("Item %s", iter.fullname());

   DOUT0("STOP STREAM MODULE %s len %lu evnts %lu out %lu", GetName(), fTotalSize, fTotalEvnts, fTotalOutEvnts);

   if (fAsf.length()>0) {

      dabc::Buffer buf = fWorkerHierarchy.SaveToBuffer();
      DOUT0("store hierarchy size %d in temporary h.bin file", buf.GetTotalSize(), buf.NumSegments());
      {
         dabc::BinaryFile f;
         system("rm -f h.bin");
         if (f.OpenWriting("h.bin")) {
            if (f.WriteBufHeader(buf.GetTotalSize(), buf.GetTypeId()))
               for (unsigned n=0;n<buf.NumSegments();n++)
                  f.WriteBufPayload(buf.SegmentPtr(n), buf.SegmentSize(n));
            f.Close();
         }
      }

      std::string args("dabc_root -h h.bin -o ");
      args += fAsf;

      DOUT0("Calling: %s", args.c_str());

      int res = system(args.c_str());

      if (res!=0) EOUT("Fail to convert DABC histograms in ROOT file, check h.bin file");
             else system("rm -f h.bin");
   }
}

bool dabc::StreamModule::ProcessNextBuffer()
{
   dabc::Buffer buf = Recv();

   if (buf.GetTypeId() == dabc::mbt_EOF) return true;

   fTotalSize += buf.GetTotalSize();

   hadaq::ReadIterator iter(buf);

   while (iter.NextEvent()) {

      fTotalEvnts++;

      if (fParallel==0) Par("Events").SetValue(1);

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
   }

   return true;
}

bool dabc::StreamModule::RedistributeBuffer()
{
   if (!CanRecv()) return false;

   unsigned indx(0), max(0);
   for (unsigned n=0;n<NumOutputs();n++)
      if (NumCanSend(n)>max) {
         max = NumCanSend(n);
         indx = n;
      }

   if (max==0) return false;

   dabc::Buffer buf = Recv();

   if (buf.GetTypeId() == dabc::mbt_EOF) {
      DOUT0("END of FILE, DO SOMETHING");
      return false;
   }

   fTotalSize += buf.GetTotalSize();

   hadaq::ReadIterator iter(buf);

   int cnt = 0;
   while (iter.NextEvent()) cnt++;

   fTotalEvnts+=cnt;

   Par("Events").SetValue(cnt);

//   DOUT0("Send buffer to output %d\n", indx);

   Send(indx, buf);

   return true;
}

bool dabc::StreamModule::ProcessRecv(unsigned port)
{
   if (fParallel<=0) {
      if (CanRecv()) return ProcessNextBuffer();
      return true;
   }

   return RedistributeBuffer();
}

