#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclasses;

// here is base classes
#pragma link C++ namespace base;
#pragma link C++ class base::Buffer+;
#pragma link C++ class base::SubEvent+;
#pragma link C++ class base::LocalStampConverter+;
#pragma link C++ class base::Event+;
#pragma link C++ class base::Message+;
#pragma link C++ class base::Iterator+;
#pragma link C++ class base::Processor+;
#pragma link C++ class base::EventProc+;
#pragma link C++ class base::StreamProc+;
#pragma link C++ class base::SysCoreProc+;
#pragma link C++ class base::OpticSplitter+;
#pragma link C++ class base::ProcMgr+;
#pragma link C++ class base::SyncMarker+;
#pragma link C++ class base::LocalTimeMarker+;
#pragma link C++ class base::GlobalMarker+;

// here is dabc classes
#pragma link C++ namespace dabc;
#pragma link C++ class dabc::BasicFile+;
#pragma link C++ class dabc::FileInterface+;
#pragma link C++ class dabc::BinaryFile+;

// here is classes for nXYTER processing
#pragma link C++ namespace nx;
#pragma link C++ class nx::Message+;
#pragma link C++ class nx::Iterator+;
#pragma link C++ class nx::Processor+;
#pragma link C++ class nx::SubEvent+;
#pragma link C++ class base::MessageExt<nx::Message>+;
#pragma link C++ class nx::MessageExt+;
#pragma link C++ class std::vector<nx::MessageExt>+;

// GET4 processing
#pragma link C++ namespace get4;
#pragma link C++ class get4::Message+;
#pragma link C++ class get4::Iterator+;
#pragma link C++ class get4::Processor+;
#pragma link C++ class get4::SubEvent+;
#pragma link C++ class base::MessageExt<get4::Message>+;
#pragma link C++ class get4::MessageExt+;
#pragma link C++ class std::vector<get4::MessageExt>+;
#pragma link C++ class get4::MbsProcessor+;

// HADAQ processing
#pragma link C++ namespace hadaqs;
#pragma link C++ class hadaqs::HadTu+;
#pragma link C++ class hadaqs::HadTuId+;
#pragma link C++ class hadaqs::RawEvent+;
#pragma link C++ class hadaqs::RawSubevent+;
#pragma link C++ namespace hadaq;
#pragma link C++ class hadaq::HldFile+;
#pragma link C++ class hadaq::TrbIterator+;
#pragma link C++ class hadaq::TdcMessage+;
#pragma link C++ class base::MessageExt<hadaq::TdcMessage>+;
#pragma link C++ class hadaq::TdcMessageExt+;
#pragma link C++ class std::vector<hadaq::TdcMessageExt>+;
#pragma link C++ class hadaq::TdcSubEvent+;
#pragma link C++ class hadaq::TdcIterator+;
#pragma link C++ class hadaq::AdcMessage+;
#pragma link C++ class std::vector<hadaq::AdcMessage>+;
#pragma link C++ class hadaq::AdcSubEvent+;
#pragma link C++ class hadaq::SubProcessor+;
#pragma link C++ class hadaq::AdcProcessor+;
#pragma link C++ class hadaq::TdcProcessor+;
#pragma link C++ class hadaq::TrbMessage+;
#pragma link C++ class hadaq::TrbProcessor+;
#pragma link C++ class hadaq::HldMessage+;
#pragma link C++ class hadaq::HldSubEvent+;
#pragma link C++ class hadaq::HldFilter+;
#pragma link C++ class hadaq::HldProcessor+;
#pragma link C++ class hadaq::SpillProcessor+;
#pragma link C++ struct hadaq::MessageFloat+;
#pragma link C++ class std::vector<hadaq::MessageFloat>+;
#pragma link C++ struct hadaq::MessageDouble+;
#pragma link C++ class std::vector<hadaq::MessageDouble>+;

// MBS data processing
#pragma link C++ namespace mbs;
#pragma link C++ class mbs::SubEvent+;
#pragma link C++ class mbs::Processor+;

// ROOT classes
#pragma link C++ class THookProc+;
#pragma link C++ class TRootProcMgr+;

#endif
