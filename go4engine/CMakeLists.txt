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
   INCLUDE_DIRS
      ${CMAKE_SOURCE_DIR}/include
)
