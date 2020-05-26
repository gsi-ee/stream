cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

project(Stream)

#---Set paths where to put the libraries, executables and headers------------------------------
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


#--- where to search for cmake modules ----
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

#--- Load some basic macros ---
include(StreamMacros)

# looking for ROOT and Go4
find_package(Go4 QUIET)
if(NOT Go4_FOUND)
   find_package(ROOT QUIET)
endif()

configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/StreamConfig.cmake.in
               ${CMAKE_BINARY_DIR}/StreamConfig.cmake COPYONLY)

configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/streamlogin.in
               ${CMAKE_BINARY_DIR}/streamlogin  @ONLY NEWLINE_STYLE UNIX)


add_subdirectory(framework)

if(Go4_FOUND)
   add_subdirectory(go4engine)
endif()

