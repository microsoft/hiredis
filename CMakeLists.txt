cmake_minimum_required(VERSION 3.0)
project(hiredis)

option(HIREDIS_BUILD_SHARED "Build a shared library." OFF)
option(HIREDIS_BUILD_EXAMPLE "Build the example program." OFF)
option(HIREDIS_BUILD_TEST "Build the test program." OFF)

set(SOURCES async.c hiredis.c net.c sds.c)

include_directories(.)

if(WIN32)
  set (WIN32_INTEROP_SOURCES
       msvs/win32_interop/win32_common.cpp
       msvs/win32_interop/win32_fixes.c
       msvs/win32_interop/win32_ANSI.c
       msvs/win32_interop/win32_APIs.c
       msvs/win32_interop/win32_error.c
       msvs/win32_interop/win32_fdapi.cpp
       msvs/win32_interop/win32_fdapi_crt.cpp
       msvs/win32_interop/win32_rfdmap.cpp
       msvs/win32_interop/win32_variadic_functor.cpp
       msvs/win32_interop/win32_wsiocp.c)

  set (SOURCES ${SOURCES}
       ${WIN32_INTEROP_SOURCES}
       msvs/deps/adlist.c
       msvs/deps/ae.c)

  include_directories(msvs/deps msvs/win32_interop)

  # no "unsafe" warnings for _snprintf
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)

  # some flags only for win32_interop sources
  set_source_files_properties(${WIN32_INTEROP_SOURCES} PROPERTIES COMPILE_FLAGS "-DUNICODE -DPSAPI_VERSION=1")
endif()

if(HIREDIS_BUILD_SHARED)
  set(BUILD_SHARED_LIBS ON)
else()
  set(BUILD_SHARED_LIBS OFF)
endif()

add_library(hiredis ${SOURCES})

if(HIREDIS_BUILD_SHARED)
  target_compile_definitions(hiredis PRIVATE -DHIREDIS_BUILD_SHARED)
endif()

if(HIREDIS_BUILD_EXAMPLE)
  add_executable(hiredis_example examples/example.c)
  target_link_libraries(hiredis_example hiredis)
endif()

if(HIREDIS_BUILD_TEST)
  add_executable(hiredis_test test.c)
  target_link_libraries(hiredis_test hiredis)
endif()
