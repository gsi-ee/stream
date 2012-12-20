#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclasses;

// here is base classes
#pragma link C++ class base::Buffer+;
#pragma link C++ class base::SubEvent+;
#pragma link C++ class base::Event+;
#pragma link C++ class base::Message+;
#pragma link C++ class base::Iterator+;
#pragma link C++ class base::StreamProc+;
#pragma link C++ class base::SysCoreProc+;
#pragma link C++ class base::ProcMgr+;
#pragma link C++ class base::SyncMarker+;
#pragma link C++ class base::LocalTriggerMarker+;
#pragma link C++ class base::GlobalTriggerMarker+;
#pragma link C++ typedef base::H1handle;
#pragma link C++ typedef base::H2handle;
#pragma link C++ typedef base::C1handle;


// here is classes for nXYTER processing
#pragma link C++ namespace nx;
#pragma link C++ class nx::Message+;
#pragma link C++ class nx::Iterator+;
#pragma link C++ class nx::Processor+;
#pragma link C++ class nx::SubEvent+;
#pragma link C++ class nx::MessageExtended+;
#pragma link C++ class std::vector<nx::MessageExtended>+;

// GET4 processing
#pragma link C++ namespace get4;
#pragma link C++ class get4::Message+;
#pragma link C++ class get4::Iterator+;
#pragma link C++ class get4::Processor+;
#pragma link C++ class get4::SubEvent+;
#pragma link C++ class get4::MessageExtended+;
#pragma link C++ class std::vector<get4::MessageExtended>+;

// HADAQ processing
#pragma link C++ namespace hadaq;
#pragma link C++ class hadaq::TrbProcessor+;
#pragma link C++ class hadaq::TrbIterator+;
#pragma link C++ class hadaq::TdcMessage+;
#pragma link C++ class hadaq::TdcMessageExt+;
#pragma link C++ class hadaq::TdcSubEvent+;
#pragma link C++ class hadaq::TdcIterator+;
#pragma link C++ class hadaq::TdcProcessor+;


#endif
