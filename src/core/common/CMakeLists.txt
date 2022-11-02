cmake_minimum_required(VERSION 3.0)

project(DigitalConductorCoreProto LANGUAGES CXX)

get_filename_component(DigitalConductorCore_proto "proto/autodiscovery.proto" ABSOLUTE)
get_filename_component(DigitalConductorCore_proto_path "${DigitalConductorCore_proto}" PATH)

set(ProtocOutputDir ${CMAKE_CURRENT_BINARY_DIR})

set(DigitalConductorCore_proto_src
    ${ProtocOutputDir}/autodiscovery.pb.h
    ${ProtocOutputDir}/autodiscovery.pb.cc
    ${ProtocOutputDir}/autodiscovery.grpc.pb.h
    ${ProtocOutputDir}/autodiscovery.grpc.pb.cc)

add_custom_command(
    OUTPUT ${DigitalConductorCore_proto_src}
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out "${ProtocOutputDir}"
         --cpp_out "${ProtocOutputDir}"
         -I "${DigitalConductorCore_proto_path}"
         --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${DigitalConductorCore_proto}"
    DEPENDS "${DigitalConductorCore_proto}")

add_custom_target(
	run_protoc
	ALL
	DEPENDS ${DigitalConductorCore_proto})

add_library(DigitalConductorCore_proto ${DigitalConductorCore_proto_src} proto/autodiscovery.proto)

target_link_libraries(DigitalConductorCore_proto
    ${ZLIB_LIBRARIES}
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})

set_target_properties(DigitalConductorCore_proto PROPERTIES
    OUTPUT_NAME "digitalconductorcore_proto")

target_include_directories(DigitalConductorCore_proto PUBLIC ${ProtocOutputDir})
