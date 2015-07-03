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

#ifndef DABC_StreamModule
#define DABC_StreamModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

namespace dabc {

   /** \brief Runs code of stream framework
    *
    * Module used to run code, available in stream framework
    */

   class ProcMgr;



   class StreamModule : public dabc::ModuleAsync {

   protected:
      dabc::ProcMgr* fProcMgr;
      std::string  fAsf;
      long unsigned fTotalSize;
      long unsigned fTotalEvnts;
      long unsigned fTotalOutEvnts;
      virtual int ExecuteCommand(dabc::Command cmd);

   public:
      StreamModule(const std::string& name, dabc::Command cmd = 0);
      virtual ~StreamModule();

      virtual bool ProcessRecv(unsigned port);

      virtual void BeforeModuleStart();

      virtual void AfterModuleStop();
   };
}


#endif
