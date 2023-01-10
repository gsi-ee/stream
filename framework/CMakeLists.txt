set(base_hdrs
   base/Buffer.h
   base/defines.h
   base/Event.h
   base/EventProc.h
   base/Iterator.h
   base/Markers.h
   base/Message.h
   base/OpticSplitter.h
   base/Processor.h
   base/ProcMgr.h
   base/Profiler.h
   base/Queue.h
   base/StreamProc.h
   base/SubEvent.h
   base/SysCoreProc.h
   base/TimeStamp.h
)

STREAM_INSTALL_HEADERS(base ${base_hdrs})

set(get4_hdrs
   get4/Iterator.h
   get4/MbsProcessor.h
   get4/Message.h
   get4/Processor.h
   get4/SubEvent.h
)

STREAM_INSTALL_HEADERS(get4 ${get4_hdrs})

set(dabc_hdrs
   dabc/BinaryFile.h
)

STREAM_INSTALL_HEADERS(dabc ${dabc_hdrs})

set(hadaq_hdrs
   hadaq/AdcMessage.h
   hadaq/AdcProcessor.h
   hadaq/AdcSubEvent.h
   hadaq/definess.h
   hadaq/HldFile.h
   hadaq/HldProcessor.h
   hadaq/SpillProcessor.h
   hadaq/StartProcessor.h
   hadaq/SubProcessor.h
   hadaq/TdcIterator.h
   hadaq/TdcMessage.h
   hadaq/TdcProcessor.h
   hadaq/TdcSubEvent.h
   hadaq/TrbIterator.h
   hadaq/TrbProcessor.h
   hadaq/MonitorSubEvent.h
   hadaq/MonitorProcessor.h
)

STREAM_INSTALL_HEADERS(hadaq ${hadaq_hdrs})

set(mbs_hdrs
   mbs/Processor.h
   mbs/SubEvent.h
)

STREAM_INSTALL_HEADERS(mbs ${mbs_hdrs})

set(nx_hdrs
   nx/Iterator.h
   nx/Message.h
   nx/Processor.h
   nx/SubEvent.h
)

STREAM_INSTALL_HEADERS(nx ${nx_hdrs})

# ================== Produce Stream headers ==========

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
   hadaq/MonitorProcessor.cxx
   mbs/Processor.cxx
   nx/Iterator.cxx
   nx/Message.cxx
   nx/Processor.cxx
)

if(ROOT_FOUND)

   set(root_hdrs
       root/THookProc.h
       root/TRootProcMgr.h
   )

   STREAM_INSTALL_HEADERS(root ${root_hdrs})

   include(${ROOT_USE_FILE})

   STREAM_LINK_LIBRARY(StreamDict
      SOURCES
         root/THookProc.cxx
         root/TRootProcMgr.cxx
      LIBRARIES
         Stream
      NOWARN
   )

   ROOT_GENERATE_DICTIONARY(G__StreamDict ${base_hdrs} ${dabc_hdrs} ${get4_hdrs} ${hadaq_hdrs} ${mbs_hdrs} ${nx_hdrs} ${root_hdrs}
                            LINKDEF LinkDef.h
                            MODULE StreamDict
                            DEPENDENCIES Stream
                            NOINSTALL)

endif()
