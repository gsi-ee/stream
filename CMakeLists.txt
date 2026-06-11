cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(Stream)

#---Set paths where to put the libraries, executables and headers------------------------------
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


#--- where to search for cmake modules ----
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

#--- Set install dirs ---
include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
set(STREAM_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/stream)

#--- Load some basic macros ---
include(StreamMacros)

# looking for ROOT and Go4
find_package(Go4 QUIET)
if(NOT Go4_FOUND)
   find_package(ROOT QUIET)
endif()

# ====== check ROOT and its c++ standard ==========

if(ROOT_FOUND AND ROOT_CXX_STANDARD)
   set(CMAKE_CXX_STANDARD ${ROOT_CXX_STANDARD})
   message("FOUND ROOT with c++ standard ${CMAKE_CXX_STANDARD}")
endif()

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()


if(APPLE)
  set(_ldvname_ DYLD_LIBRARY_PATH)
else()
  set(_ldvname_ LD_LIBRARY_PATH)
endif()

include(CMakePackageConfigHelpers)

configure_package_config_file(
  ${CMAKE_SOURCE_DIR}/cmake/scripts/StreamConfig.cmake.in
  ${CMAKE_BINARY_DIR}/StreamConfig.cmake
  INSTALL_DESTINATION lib/cmake/Stream
)

set(_streamsys_ ${CMAKE_BINARY_DIR})
set(_streaminc_ ${CMAKE_BINARY_DIR}/include)
set(_streamlib_ ${CMAKE_BINARY_DIR}/lib)
configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/streamlogin.in
               ${CMAKE_BINARY_DIR}/streamlogin  @ONLY NEWLINE_STYLE UNIX)

# --- for install use case ----
set(_streamsys_ ${CMAKE_INSTALL_PREFIX})
set(_streaminc_ ${CMAKE_INSTALL_PREFIX}/${STREAM_INSTALL_INCLUDEDIR})
set(_streamlib_ ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
configure_file(${CMAKE_SOURCE_DIR}/cmake/scripts/streamlogin.in
                ${CMAKE_BINARY_DIR}/macros/streamlogin @ONLY NEWLINE_STYLE UNIX)
# -----------------------------

install(PROGRAMS ${PROJECT_BINARY_DIR}/macros/streamlogin
        DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES ${CMAKE_BINARY_DIR}/StreamConfig.cmake
        DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
)

add_subdirectory(framework)

if(Go4_FOUND)
   add_subdirectory(go4engine)
endif()

get_property(__allHeaders GLOBAL PROPERTY STREAM_HEADER_TARGETS)
add_custom_target(move_headers ALL DEPENDS ${__allHeaders})

export(
  EXPORT ${PROJECT_NAME}Targets
  NAMESPACE stream::
  FILE ${PROJECT_NAME}Targets.cmake)

install(
  EXPORT ${PROJECT_NAME}Targets
  NAMESPACE stream::
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR})

