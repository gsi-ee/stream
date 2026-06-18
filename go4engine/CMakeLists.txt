include(${GO4_USE_FILE})

if(NOT TARGET stream::Stream)
   find_package(Stream REQUIRED PATHS $ENV{STREAMSYS})
endif()

if(Go4_VERSION AND ${Go4_VERSION} VERSION_GREATER "6.4.5")
   set(extra_defs "GO4_WITH_USER_SKIP")
endif()

GO4_USER_ANALYSIS(
   LINKDEF
      LinkDef.h
   HEADERS
      TStreamEvent.h
      TUserSource.h
      TFirstStepProcessor.h
   DEFINITIONS
      ${extra_defs}
   SOURCES
      TUserSource.cxx
      TFirstStepProcessor.cxx
   LIBRARIES
      stream::Stream stream::StreamDict
   INCLUDE_DIRS
      ${CMAKE_SOURCE_DIR}/include
   TARGET_DIR
      ${CMAKE_BINARY_DIR}/lib
)

STREAM_INSTALL_HEADERS(go4engine TStreamEvent.h TUserSource.h TFirstStepProcessor.h)

install(
   TARGETS Go4UserAnalysis
   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
   PUBLIC_HEADER DESTINATION ${STREAM_INSTALL_INCLUDEDIR})


install(FILES ${CMAKE_BINARY_DIR}/lib/libGo4UserAnalysis.rootmap ${CMAKE_BINARY_DIR}/lib/libGo4UserAnalysis_rdict.pcm
        DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES TStreamEvent.h TUserSource.h TFirstStepProcessor.h
        DESTINATION ${STREAM_INSTALL_INCLUDEDIR})
