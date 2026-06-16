set(LIBS_BASESET ${ROOT_LIBRARIES} ${ROOT_XMLIO_LIBRARY})

# Detect Raspberry Pi
function(detect_raspberry_pi)
    set(IS_RASPBERRY_PI FALSE PARENT_SCOPE)

    # Check for /proc/cpuinfo (Linux only)
    if(EXISTS "/proc/cpuinfo")
        file(READ "/proc/cpuinfo" CPUINFO)
        if(CPUINFO MATCHES "Raspberry Pi" OR CPUINFO MATCHES "BCM2")
            set(IS_RASPBERRY_PI TRUE PARENT_SCOPE)
            return()
        endif()
    endif()

    # Check for /sys/firmware/devicetree/base/model
    if(EXISTS "/sys/firmware/devicetree/base/model")
        file(READ "/sys/firmware/devicetree/base/model" DT_MODEL)
        if(DT_MODEL MATCHES "Raspberry Pi")
            set(IS_RASPBERRY_PI TRUE PARENT_SCOPE)
        endif()
    endif()
endfunction()

detect_raspberry_pi()

if(IS_RASPBERRY_PI)
    message(STATUS "Building for Raspberry Pi")
    # Add Pi-specific flags, definitions, etc.
endif()


if(APPLE)
  set(libprefix ${CMAKE_SHARED_LIBRARY_PREFIX})
  if(CMAKE_PROJECT_NAME STREQUAL Stream)
    set(libsuffix .so)
  else()
    set(libsuffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
  endif()
else()
  set(libprefix ${CMAKE_SHARED_LIBRARY_PREFIX})
  set(libsuffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
endif()

set(STREAM_LIBRARY_PROPERTIES
    SUFFIX ${libsuffix}
    PREFIX ${libprefix}
    IMPORT_PREFIX ${libprefix})

#---------------------------------------------------------------------------------------------------
#---STREAM_INSTALL_HEADERS(subdir [hdr1 hdr2 ...])
#---------------------------------------------------------------------------------------------------
function(STREAM_INSTALL_HEADERS subdir)
  cmake_parse_arguments(ARG "" "" "" ${ARGN})
  foreach(include_file ${ARG_UNPARSED_ARGUMENTS})
    if(subdir STREQUAL go4engine)
       set(src ${CMAKE_SOURCE_DIR}/${subdir}/${include_file})
       set(dst ${CMAKE_BINARY_DIR}/include/${include_file})
    else()
       set(src ${CMAKE_SOURCE_DIR}/include/${include_file})
       set(dst ${CMAKE_BINARY_DIR}/include/${include_file})
    endif()
    add_custom_command(
      OUTPUT ${dst}
      COMMAND ${CMAKE_COMMAND} -E copy ${src} ${dst}
      COMMENT "Copying header ${include_file} to ${CMAKE_BINARY_DIR}/include"
      DEPENDS ${src})
    list(APPEND dst_list ${dst})
  endforeach()
  add_custom_target(move_headers_${subdir} DEPENDS ${dst_list})
  set_property(GLOBAL APPEND PROPERTY STREAM_HEADER_TARGETS move_headers_${subdir})
endfunction()

#---------------------------------------------------------------------------------------------------
#---STREAM_LINK_LIBRARY(libname
#                       SOURCES src1 src2          :
#                       LIBRARIES lib1 lib2        : direct linked libraries
#                       DEFINITIONS def1 def2      : library definitions
#                       OPTIONS opt1 opt2          : custom compile options
#)
function(STREAM_LINK_LIBRARY libname)
   cmake_parse_arguments(ARG "NOEXPORT;NOWARN" "" "SOURCES;LIBRARIES;OPTIONS;DEFINITIONS;DEPENDENCIES" ${ARGN})

   add_library(${libname} SHARED ${ARG_SOURCES})
   add_library(stream::${libname} ALIAS ${libname})

   set_target_properties(${libname} PROPERTIES ${STREAM_LIBRARY_PROPERTIES})

   target_compile_definitions(${libname} PUBLIC ${ARG_DEFINITIONS})

   if(NOT ARG_NOWARN)
      target_compile_options(${libname} PRIVATE -Wall $<$<CXX_COMPILER_ID:GNU>:-Wsuggest-override>)
   endif()

   if(ARG_OPTIONS)
      target_compile_options(${libname} PRIVATE ${ARG_OPTIONS})
   endif()

   target_link_libraries(${libname} PUBLIC ${ARG_LIBRARIES})

   # add_dependencies(${libname} move_headers ${ARG_DEPENDENCIES})

   target_include_directories(${libname}
      PUBLIC
         $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${STREAM_INSTALL_INCLUDEDIR}>
   )

   install(
    TARGETS ${libname}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${STREAM_INSTALL_INCLUDEDIR})


endfunction()
