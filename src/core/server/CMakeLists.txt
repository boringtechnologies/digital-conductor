cmake_minimum_required(VERSION 3.0)

project(DigitalConductorCoreServer LANGUAGES CXX)

add_executable(DigitalConductorCore_server main.cpp)

set_target_properties(DigitalConductorCore_server PROPERTIES
	OUTPUT_NAME "digitalconductorcore-server")

target_link_libraries(DigitalConductorCore_server
    DigitalConductorCore_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
