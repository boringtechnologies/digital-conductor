cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

project(DigitalConductorCore LANGUAGES CXX)

include(cmake/common.cmake)


#if(NOT APPLE)
#    set(BUILD_SHARED_LIBS OFF)
#    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
#endif()

# set(ZLIB_USE_STATIC_LIBS "ON")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
find_package(ZLIB REQUIRED)

find_package(LibJuice REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED
        gobject-2.0
        glib-2.0
        gstreamer-1.0
        gstreamer-app-1.0
        gstreamer-net-1.0
)

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

include_directories(${CMAKE_SOURCE_DIR}/src/core/media/include)

add_library(DigitalConductorCore_lib
            client/grpcclient.h
            client/grpcclient.cpp
            client/juiceclient.h
            client/juiceclient.cpp
            media/include/sync.hpp
            media/src/internal.hpp
            media/src/receiver.cpp
            media/src/sender.cpp)

target_include_directories(DigitalConductorCore_lib PRIVATE
            ${GST_INCLUDE_DIRS})

target_link_directories(DigitalConductorCore_lib PRIVATE
            ${GST_LIBRARY_DIRS})

target_link_libraries(DigitalConductorCore_lib PRIVATE
            DigitalConductorCore_proto)

set_target_properties(DigitalConductorCore_lib PROPERTIES
            OUTPUT_NAME "digitalconductorcore")

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

set(OTHER_FILES common/proto/autodiscovery.proto common/initial.txt README.md)

set_source_files_properties(${OTHER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_custom_target(project-related-files SOURCES ${OTHER_FILES})
