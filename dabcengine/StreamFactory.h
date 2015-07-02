// $Id: Factory.h 2935 2015-01-23 13:15:45Z linev $

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

#ifndef DABC_StreamFactory
#define DABC_StreamFactory

#ifndef DABC_Factory
#include "dabc/Factory.h"
#endif

/** \brief Support for stream framework in DABC  */

namespace dabc {

   /** \brief %Factory for HADAQ classes  */

   class StreamFactory : public dabc::Factory {
      public:
         StreamFactory(const std::string& name) : dabc::Factory(name) {}

         virtual dabc::Module* CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd);
   };

}

#endif
