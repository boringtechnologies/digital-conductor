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

set(CMAKE_FIND_DEBUG_MODE TRUE)
find_program(PROTOC_EXE protoc)
message(STATUS "PROTOC_EXE=${PROTOC_EXE}")
if(NOT PROTOC_EXE)
    set(PROTOC_EXE $<TARGET_FILE:protobuf::protoc>)
endif()
find_program(GRPC_CPP_PLUGIN_EXE grpc_cpp_plugin)
message(STATUS "GRPC_CPP_PLUGIN_EXE=${GRPC_CPP_PLUGIN_EXE}")
if(NOT GRPC_CPP_PLUGIN_EXE)
    set(PROTOC_EXE $<TARGET_FILE:grpc_cpp_plugin>)
endif()
set(CMAKE_FIND_DEBUG_MODE FALSE)

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
