project("ebbrt-filesystem" CXX)

cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
include(CMakePackageConfigHelpers)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g3")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -flto -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g3")

add_compile_options(-std=gnu++14 -Wall -Werror)

find_package(Capnp REQUIRED)
# Platform-specific package dependencies
if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  find_package(EbbRT REQUIRED)
endif()
if( ${CMAKE_SYSTEM_NAME} STREQUAL "EbbRT")
  find_library(EbbRT REQUIRED NO_SYSTEM_ENVIRONMENT_PATH)
endif()

set(capnp_sources
  ${CMAKE_SOURCE_DIR}/FileSystem.capnp
)

include_directories(${CAPNP_INCLUDE_DIRS})
include_directories(${EBBRT_INCLUDE_DIRS})

add_definitions(${CAPNP_DEFINITIONS})
set(CAPNPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(CAPNPC_SRC_PREFIX ${CMAKE_SOURCE_DIR})
include_directories(${CAPNPC_OUTPUT_DIR})
capnp_generate_cpp(CapnpSources CapnpHeaders ${capnp_sources})

add_library(ebbrt-filesystem
  FileSystem.cc
  FileSystem.h
  ${CapnpSources}
  ${CapnpHeaders}
  )

install(TARGETS ebbrt-filesystem
  ARCHIVE DESTINATION "lib"
  LIBRARY DESTINATION "lib"
  RUNTIME DESTINATION "bin"
)

install(FILES FileSystem.h
        DESTINATION "include/ebbrt-filesystem")

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH
  "Installation directory for header files")
if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_CMAKE_DIR CMake)
else()
  set(DEF_INSTALL_CMAKE_DIR lib/cmake/EbbRTFilesystem)
endif()
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH
  "Installation directory for CMake files")

configure_package_config_file(EbbRTFilesystemConfig.cmake.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/EbbRTFilesystemConfig.cmake"
  INSTALL_DESTINATION ${INSTALL_CMAKE_DIR}
  PATH_VARS INSTALL_INCLUDE_DIR INSTALL_LIB_DIR
)

install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/EbbRTFilesystemConfig.cmake"
  DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)
