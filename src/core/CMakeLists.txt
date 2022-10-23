cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

project(DigitalConductorCore LANGUAGES CXX)

include(cmake/common.cmake)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
find_package(ZLIB REQUIRED)

find_package(LibJuice REQUIRED)
find_package(Threads REQUIRED)

if(WITH_CONAN)
    find_package(gstreamer REQUIRED)
    find_package(gst-plugins-base REQUIRED)
    find_package(gst-plugins-good REQUIRED)
    find_package(gst-plugins-bad REQUIRED)
    find_package(gst-libav REQUIRED)

    set(GST_INCLUDE_DIRS "${gstreamer_INCLUDE_DIRS};${gst-plugins-base_INCLUDE_DIRS};${gst-plugins-good_INCLUDE_DIRS};${gst-plugins-bad_INCLUDE_DIRS};${gst-libav_INCLUDE_DIRS}")
    set(GST_LIBRARY_DIRS "")
    set(GST_LIBRARIES "${gstreamer_LIBRARIES};${gst-plugins-base_LIBRARIES};${gst-plugins-good_LIBRARIES};${gst-plugins-bad_LIBRARIES};${gst-libav_LIBRARIES}")

    if(APPLE)
        find_library(io_surface_lib IOSurface)
        list(APPEND GST_LIBRARIES ${io_surface_lib})
    endif()

    set(GST_USE_STATIC_PLUGINS 1)

else(WITH_CONAN)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(GST REQUIRED
        gobject-2.0
        glib-2.0
        gstreamer-1.0
        gstreamer-app-1.0
        gstreamer-net-1.0
    )

    set(GST_USE_STATIC_PLUGINS 0)
endif()

# Proto files
get_filename_component(DigitalConductorCore_proto "common/proto/autodiscovery.proto" ABSOLUTE)
get_filename_component(DigitalConductorCore_proto_path "${DigitalConductorCore_proto}" PATH)

set(DigitalConductorCore_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/autodiscovery.pb.cc")
set(DigitalConductorCore_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/autodiscovery.pb.h")
set(DigitalConductorCore_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/autodiscovery.grpc.pb.cc")
set(DigitalConductorCore_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/autodiscovery.grpc.pb.h")

add_custom_command(
      OUTPUT "${DigitalConductorCore_proto_srcs}" "${DigitalConductorCore_proto_hdrs}" "${DigitalConductorCore_grpc_srcs}" "${DigitalConductorCore_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${DigitalConductorCore_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${DigitalConductorCore_proto}"
      DEPENDS "${DigitalConductorCore_proto}")

add_library(DigitalConductorCore_proto
          ${DigitalConductorCore_grpc_srcs}
          ${DigitalConductorCore_grpc_hdrs}
          ${DigitalConductorCore_proto_srcs}
          ${DigitalConductorCore_proto_hdrs})

target_link_libraries(DigitalConductorCore_proto
          ${ZLIB_LIBRARIES}
          ${_REFLECTION}
          ${_GRPC_GRPCPP}
          ${_PROTOBUF_LIBPROTOBUF})

set_target_properties(DigitalConductorCore_proto PROPERTIES
          OUTPUT_NAME "digitalconductorcore_proto")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories(${CMAKE_SOURCE_DIR}/src/core/include)

if(APPLE)
    set(PERMISSION_MGR_IMPL_CPP permission/permissionmgr.mm)
else(APPLE)
    set(PERMISSION_MGR_IMPL_CPP permission/permissionmgr.cpp)
endif(APPLE)

add_library(DigitalConductorCore_lib
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

target_include_directories(DigitalConductorCore_lib PRIVATE
            ${GST_INCLUDE_DIRS})

target_link_directories(DigitalConductorCore_lib PRIVATE
            ${GST_LIBRARY_DIRS})

target_link_libraries(DigitalConductorCore_lib PRIVATE
            DigitalConductorCore_proto
            LibJuice::LibJuice
            ${ZLIB_LIBRARIES}
            ${GST_LIBRARIES})

set_target_properties(DigitalConductorCore_lib PROPERTIES
            OUTPUT_NAME "digitalconductorcore")

target_compile_definitions(DigitalConductorCore_lib PRIVATE GST_USE_STATIC_PLUGINS=${GST_USE_STATIC_PLUGINS})

add_executable(DigitalConductorCore_server
            server/main.cpp)

set_target_properties(DigitalConductorCore_server PROPERTIES
            OUTPUT_NAME "digitalconductorcore-server")

target_link_libraries(DigitalConductorCore_server
            DigitalConductorCore_proto
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF})

add_executable(DigitalConductorCore_client
            ${DigitalConductorCore_lib}
            client/main.cpp)

set_target_properties(DigitalConductorCore_client PROPERTIES
            OUTPUT_NAME "digitalconductorcore-client")

target_include_directories(DigitalConductorCore_client PRIVATE
            ${GST_INCLUDE_DIRS})

target_link_directories(DigitalConductorCore_client PRIVATE
            ${GST_LIBRARY_DIRS})

target_link_libraries(DigitalConductorCore_client PRIVATE
            DigitalConductorCore_proto
            DigitalConductorCore_lib
            ${GST_LIBRARIES}
            LibJuice::LibJuice
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF})

set(OTHER_FILES common/proto/autodiscovery.proto README.md)

set_source_files_properties(${OTHER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_custom_target(project-related-files SOURCES ${OTHER_FILES})
