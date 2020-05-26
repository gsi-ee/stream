STREAM_INSTALL_HEADERS(base
   Buffer.h
   defines.h
   Event.h
   EventProc.h
   Iterator.h
   Markers.h
   Message.h
   OpticSplitter.h
   Processor.h
   ProcMgr.h
   Profiler.h
   Queue.h
   StreamProc.h
   SubEvent.h
   SysCoreProc.h
   TimeStamp.h
)

STREAM_INSTALL_HEADERS(get4
   Iterator.h
   MbsProcessor.h
   Message.h
   Processor.h
   SubEvent.h
)

STREAM_INSTALL_HEADERS(dabc
   BinaryFile.h
)

STREAM_INSTALL_HEADERS(hadaq
   AdcMessage.h
   AdcProcessor.h
   AdcSubEvent.h
   definess.h
   HldFile.h
   HldProcessor.h
   SpillProcessor.h
   StartProcessor.h
   SubProcessor.h
   TdcIterator.h
   TdcMessage.h
   TdcProcessor.h
   TdcSubEvent.h
   TrbIterator.h
   TrbProcessor.h
)

STREAM_INSTALL_HEADERS(mbs
   Processor.h
   SubEvent.h
)

STREAM_INSTALL_HEADERS(nx
   Iterator.h
   Message.h
   Processor.h
   SubEvent.h
)

# ================== Produce Stream headers ==========

get_property(__allHeaders GLOBAL PROPERTY STREAM_HEADER_TARGETS)
add_custom_target(move_headers ALL DEPENDS ${__allHeaders})

STREAM_LINK_LIBRARY(Stream
   SOURCES
   base/Buffer.cxx
   base/Event.cxx
   base/EventProc.cxx
   base/Iterator.cxx
   base/Markers.cxx
   base/Message.cxx
   base/OpticSplitter.cxx
   base/Processor.cxx
   base/ProcMgr.cxx
   base/Profiler.cxx
   base/StreamProc.cxx
   base/SysCoreProc.cxx
   get4/Iterator.cxx
   get4/MbsProcessor.cxx
   get4/Message.cxx
   get4/Processor.cxx
   hadaq/AdcProcessor.cxx
   hadaq/definess.cxx
   hadaq/HldFile.cxx
   hadaq/HldProcessor.cxx
   hadaq/SpillProcessor.cxx
   hadaq/StartProcessor.cxx
   hadaq/SubProcessor.cxx
   hadaq/TdcMessage.cxx
   hadaq/TdcProcessor.cxx
   hadaq/TrbIterator.cxx
   hadaq/TrbProcessor.cxx
   mbs/Processor.cxx
   nx/Iterator.cxx
   nx/Message.cxx
   nx/Processor.cxx   
)
