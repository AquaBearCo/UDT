# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.16)
project(udt VERSION 4.11 LANGUAGES CXX)

# Options/Defaults
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(_DEBUG)
endif()

# Sources
set(SOURCES
  src/api.cpp
  src/buffer.cpp
  src/cache.cpp
  src/ccc.cpp
  src/channel.cpp
  src/core.cpp
  src/epoll.cpp
  src/list.cpp
  src/md5.cpp
  src/packet.cpp
  src/queue.cpp
  src/udtCommon.cpp
  src/window.cpp
  src/plat/posix/sync.cpp
  src/plat/win/sync.cpp
  src/plat/posix/net.cpp
  src/plat/win/net.cpp
)

# Build options
option(UDT_BUILD_SHARED "Build shared library" ON)
option(UDT_BUILD_STATIC "Build static library" ON)
option(UDT_ENABLE_POLLER "Enable internal Poller abstraction" ON)

if(UDT_ENABLE_POLLER)
  if(WIN32)
    list(APPEND SOURCES src/plat/win/poller_stub.cpp)
  elseif(APPLE)
    list(APPEND SOURCES src/plat/posix/poller_kqueue.cpp)
  else()
    list(APPEND SOURCES src/plat/posix/poller_epoll.cpp)
  endif()
endif()

# Common object library to avoid dual compilation definitions drift
add_library(udt_objs OBJECT ${SOURCES})
set_target_properties(udt_objs PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Public interface
target_include_directories(udt_objs
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(udt_objs PUBLIC cxx_std_17)

# Warnings and portability
if(MSVC)
  target_compile_options(udt_objs PRIVATE /W4)
  # Code uses WINDOWS macro; define it for _WIN32 builds
  target_compile_definitions(udt_objs PUBLIC WINDOWS)
else()
  target_compile_options(udt_objs PRIVATE -Wall -Wextra -Wno-unused-function)
  target_compile_definitions(udt_objs PRIVATE GNU_SOURCE POSIX_C_SOURCE)
  # Preserve default symbol visibility akin to previous attribute usage
  set_target_properties(udt_objs PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
  )
endif()

# Link dependencies
function(udt_apply_linkage tgt)
  if(UNIX AND NOT APPLE)
    target_link_libraries(${tgt} PRIVATE pthread m)
  elseif(APPLE)
    target_link_libraries(${tgt} PRIVATE pthread)
  endif()
  if(DEFINED ARCAN_SHMIF_LIBRARY)
    target_link_libraries(${tgt} PRIVATE ${ARCAN_SHMIF_LIBRARY})
  endif()
endfunction()

# Concrete libraries from objects
if(UDT_BUILD_SHARED)
  add_library(udt SHARED $<TARGET_OBJECTS:udt_objs>)
  target_link_libraries(udt PRIVATE)
  udt_apply_linkage(udt)
  add_library(udt::udt ALIAS udt)
endif()

if(UDT_BUILD_STATIC)
  add_library(udt_static STATIC $<TARGET_OBJECTS:udt_objs>)
  set_target_properties(udt_static PROPERTIES OUTPUT_NAME udt)
  udt_apply_linkage(udt_static)
  add_library(udt::udt_static ALIAS udt_static)
endif()

# Warn when building deprecated Win32 (32-bit) target
if(MSVC AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(WARNING "Windows 32-bit is deprecated for UDT; prefer x64/ARM64.")
endif()

# Install and package config
include(GNUInstallDirs)

set(UDT_INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/udt")

set(_udt_install_targets)
if(TARGET udt)
  list(APPEND _udt_install_targets udt)
endif()
if(TARGET udt_static)
  list(APPEND _udt_install_targets udt_static)
endif()

if(_udt_install_targets)
  install(TARGETS ${_udt_install_targets}
    EXPORT udt-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()

# Public headers: src/ (legacy public) and include/
install(DIRECTORY src/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT udt-targets
  FILE udt-targets.cmake
  NAMESPACE udt::
  DESTINATION ${UDT_INSTALL_CONFIGDIR}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/udtConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/udtConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/udtConfig.cmake
  @ONLY
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/udtConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/udtConfigVersion.cmake"
  DESTINATION ${UDT_INSTALL_CONFIGDIR}
)

# pkg-config file
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/udt.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/udt.pc
  @ONLY
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/udt.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
