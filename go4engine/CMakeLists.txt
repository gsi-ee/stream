include(${GO4_USE_FILE})

GO4_USER_ANALYSIS(
   LINKDEF
      LinkDef.h
   HEADERS
      TStreamEvent.h
      TUserSource.h
      TFirstStepProcessor.h
   SOURCES
      TUserSource.cxx
      TFirstStepProcessor.cxx
   LIBRARIES
      Stream StreamDict
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
