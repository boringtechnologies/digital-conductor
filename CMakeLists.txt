cmake_minimum_required(VERSION 3.0.0)

project(DigitalConductor)

if(WIN32)
  find_program(PYTHON "python")
else()
  find_program(PYTHON "python3")
endif()
if(${PYTHON} MATCHES NOTFOUND)
  message(FATAL_ERROR "Can't find ${PYTHON} executable")
else()
  message(STATUS "Found Python3: ${PYTHON}")
endif()

if(WIN32)
  set(QT_DIR_GLOB "C:/Qt*/6.*/msvc*_64")
elseif(APPLE)
  set(QT_DIR_GLOB "$ENV{HOME}/Qt*/6.*/macos")
else()
  set(QT_DIR_GLOB "$ENV{HOME}/Qt*/6.*/gcc_64")
endif()
file(GLOB QT_DIR ${QT_DIR_GLOB})
if(QT_DIR STREQUAL "")
  message(STATUS "Can't find Qt at ${QT_DIR_GLOB}, not building demo app")
else()
  list(GET QT_DIR 0 QT_DIR)
  message(STATUS "Found Qt: ${QT_DIR}")
  list(APPEND CMAKE_PREFIX_PATH "${QT_DIR}")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release")
endif()

include_directories(
  "${PROJECT_SOURCE_DIR}/src/lib"
  )

include_directories(SYSTEM
  "${PROJECT_SOURCE_DIR}/3rdparty/cxxopts/include"
  )

link_directories(
  "${PROJECT_SOURCE_DIR}/bin"
  )

if(NOT WIN32)
  link_libraries(
    pthread
    )
endif()

add_subdirectory(3rdparty 3rdparty)

add_subdirectory(src/core core)
add_subdirectory(src/lib  lib)
add_subdirectory(src/cli  cli)
set(CUSTOM_BUILD_DEPENDS DigitalConductorLib DigitalConductorCli)

if(NOT QT_DIR STREQUAL "")
  add_subdirectory(src/demo demo)
  set(CUSTOM_BUILD_DEPENDS DigitalConductorLib DigitalConductorCli DigitalConductorDemo)
endif()

add_custom_command(
  COMMENT "Copying compile_commands.json to project root"
  DEPENDS ${CUSTOM_BUILD_DEPENDS}
  OUTPUT
    "${PROJECT_SOURCE_DIR}/compile_commands.json"
  COMMAND "${CMAKE_COMMAND}" -E copy
    "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json"
    "${PROJECT_SOURCE_DIR}"
  )

add_custom_target(DigitalConductor ALL
  DEPENDS "${PROJECT_SOURCE_DIR}/compile_commands.json"
  )
