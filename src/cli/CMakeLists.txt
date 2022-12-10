cmake_minimum_required(VERSION 3.0.0)

project(DigitalConductorCli)

add_executable(DigitalConductorCli "Main.cpp")

add_dependencies(DigitalConductorCli DigitalConductorLib)

target_link_libraries(DigitalConductorCli PRIVATE DigitalConductorLib)

set_target_properties(DigitalConductorCli PROPERTIES OUTPUT_NAME "digitalconductor")

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
  add_custom_command(TARGET DigitalConductorCli
    COMMAND strip "$<TARGET_FILE:DigitalConductorCli>"
    COMMENT "Stipping DigitalConductorCli"
    POST_BUILD)
endif()
