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

find_program(PROTOC_EXE protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN_EXE grpc_cpp_plugin REQUIRED)

add_custom_command(
    OUTPUT ${DigitalConductorCore_proto_src}
    COMMAND ${PROTOC_EXE}
    ARGS --grpc_out "${ProtocOutputDir}"
         --cpp_out "${ProtocOutputDir}"
         -I "${DigitalConductorCore_proto_path}"
         --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_EXE}"
        "${DigitalConductorCore_proto}"
    DEPENDS "${DigitalConductorCore_proto}")

add_custom_target(
	run_protoc
	ALL
	DEPENDS ${DigitalConductorCore_proto})

add_library(DigitalConductorCore_proto ${DigitalConductorCore_proto_src} proto/autodiscovery.proto)

target_link_libraries(DigitalConductorCore_proto
    ${ZLIB_LIBRARIES}
    ${gRPC_LIBRARIES}
    ${Protobuf_LIBRARIES})

target_include_directories(DigitalConductorCore_proto PUBLIC ${gRPC_INCLUDE_DIRS})

set_target_properties(DigitalConductorCore_proto PROPERTIES
    OUTPUT_NAME "digitalconductorcore_proto")

target_include_directories(DigitalConductorCore_proto PUBLIC ${ProtocOutputDir})
