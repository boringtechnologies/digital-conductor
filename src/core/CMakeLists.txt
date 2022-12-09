cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

project(DigitalConductorCore LANGUAGES CXX)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
find_package(ZLIB REQUIRED)

find_package(LibJuice REQUIRED)

if(WITH_CONAN)
  find_package(Protobuf REQUIRED)
  find_package(GRPC REQUIRED)

  # workaround: Could not find GRPC_CPP_PLUGIN_EXE using the following names
  get_filename_component(GRPC_ROOT ${GRPC_INCLUDE_DIR} DIRECTORY)
  list(APPEND CMAKE_PREFIX_PATH "${GRPC_ROOT}")

  set(GST_INCLUDE_DIRS)
  set(GST_LIBRARY_DIRS "")
  set(GST_LIBRARIES)
  foreach(
    gst_lib
    gstreamer
    gstpluginsbase
    gstpluginsgood
    gstpluginsbad
    gstlibav)
    find_package(${gst_lib} REQUIRED)
    list(APPEND GST_INCLUDE_DIRS ${${gst_lib}_INCLUDE_DIRS})
    list(APPEND GST_LIBRARIES ${${gst_lib}_LIBRARIES})
  endforeach()

  if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # https://stackoverflow.com/questions/4802624/
    list(APPEND GST_LIBRARIES ${gstreamer_LIBRARIES})
  endif()

  if(APPLE)
    find_library(io_surface_lib IOSurface)
    list(APPEND GST_LIBRARIES ${io_surface_lib})
  endif()

  set(GST_USE_STATIC_PLUGINS 1)

else(WITH_CONAN)
  include(cmake/common.cmake)
  set(GRPC_LIBRARIES ${_REFLECTION} ${_GRPC_GRPCPP})
  set(Protobuf_LIBRARIES ${_PROTOBUF_LIBPROTOBUF})

  find_package(PkgConfig REQUIRED)

  pkg_check_modules(
    GST
    REQUIRED
    gobject-2.0
    glib-2.0
    gstreamer-1.0
    gstreamer-app-1.0
    gstreamer-net-1.0)

  set(GST_USE_STATIC_PLUGINS 0)
endif()

add_subdirectory(common)
add_subdirectory(server)

if(APPLE)
  set(PERMISSION_MGR_IMPL_CPP permission/permissionmgr.mm)
else(APPLE)
  set(PERMISSION_MGR_IMPL_CPP permission/permissionmgr.cpp)
endif(APPLE)

add_library(
  DigitalConductorCore_lib
  include/core/grpcclient.h
  include/core/juiceclient.h
  include/core/media/sync.hpp
  include/core/media/util.h
  client/grpcclient.cpp
  client/juiceclient.cpp
  media/src/internal.hpp
  media/src/receiver.cpp
  media/src/sender.cpp
  media/src/util.cpp
  ${PERMISSION_MGR_IMPL_CPP})

target_include_directories(
  DigitalConductorCore_lib
  PUBLIC ${LibJuice_INCLUDE_DIRS} ${GRPC_INCLUDE_DIRS}
  PRIVATE ${GST_INCLUDE_DIRS})

target_link_directories(DigitalConductorCore_lib PRIVATE ${GST_LIBRARY_DIRS})

target_link_libraries(
  DigitalConductorCore_lib
  PRIVATE DigitalConductorCore_proto
          LibJuice::LibJuice
          fmt::fmt
          ${ZLIB_LIBRARIES}
          ${GST_LIBRARIES})

set_target_properties(DigitalConductorCore_lib PROPERTIES OUTPUT_NAME "digitalconductorcore")

target_compile_definitions(DigitalConductorCore_lib PRIVATE GST_USE_STATIC_PLUGINS=${GST_USE_STATIC_PLUGINS})

add_executable(DigitalConductorCore_client ${DigitalConductorCore_lib} client/main.cpp)

set_target_properties(DigitalConductorCore_client PROPERTIES OUTPUT_NAME "digitalconductorcore-client")

target_link_libraries(
  DigitalConductorCore_client
  PRIVATE DigitalConductorCore_proto
          DigitalConductorCore_lib
          ${GST_LIBRARIES}
          ${GRPC_LIBRARIES}
          ${Protobuf_LIBRARIES})
