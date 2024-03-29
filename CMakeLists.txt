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

# ====== check ROOT and its c++ standard ==========

if(ROOT_FOUND AND NOT ROOT_CXX_FLAGS STREQUAL "")
   if(APPLE)
      string(FIND ${ROOT_CXX_FLAGS} "-std=c++11" _check_c11)
      string(FIND ${ROOT_CXX_FLAGS} "-std=c++1y" _check_c14)
      string(FIND ${ROOT_CXX_FLAGS} "-std=c++1z" _check_c17)
      string(FIND ${ROOT_CXX_FLAGS} "-std=c++20" _check_c20)
   else()
      string(FIND ${ROOT_CXX_FLAGS} "-std=c++11" _check_c11)
      string(FIND ${ROOT_CXX_FLAGS} "-std=c++14" _check_c14)
      string(FIND ${ROOT_CXX_FLAGS} "-std=c++17" _check_c17)
      string(FIND ${ROOT_CXX_FLAGS} "-std=c++20" _check_c20)
   endif()
   if(NOT ${_check_c11} EQUAL -1)
      set(CMAKE_CXX_STANDARD 11)
   elseif(NOT ${_check_c14} EQUAL -1)
      set(CMAKE_CXX_STANDARD 14)
   elseif(NOT ${_check_c17} EQUAL -1)
      set(CMAKE_CXX_STANDARD 17)
   elseif(NOT ${_check_c20} EQUAL -1)
      set(CMAKE_CXX_STANDARD 20)
   endif()
   message("FOUND ROOT with flags ${ROOT_CXX_FLAGS} stream take standard ${CMAKE_CXX_STANDARD}")
endif()

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
endif()


configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/StreamConfig.cmake.in
               ${CMAKE_BINARY_DIR}/StreamConfig.cmake COPYONLY)

configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/streamlogin.in
               ${CMAKE_BINARY_DIR}/streamlogin  @ONLY NEWLINE_STYLE UNIX)


add_subdirectory(framework)

if(Go4_FOUND)
   add_subdirectory(go4engine)
endif()

get_property(__allHeaders GLOBAL PROPERTY STREAM_HEADER_TARGETS)
add_custom_target(move_headers ALL DEPENDS ${__allHeaders})
