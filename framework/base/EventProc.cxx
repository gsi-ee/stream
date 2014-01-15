#include "base/EventProc.h"

base::EventProc::EventProc(const char* name, unsigned brdid) :
   base::Processor(name, brdid)
{
   fMgr = base::ProcMgr::AddProc(this);
}
