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

#include "StreamFactory.h"

#include "StreamModule.h"

dabc::FactoryPlugin streamfactory(new dabc::StreamFactory("stream"));

dabc::Module* dabc::StreamFactory::CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
{
   if (classname == "dabc::StreamModule")
      return new dabc::StreamModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}
