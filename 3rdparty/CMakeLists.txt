cmake_minimum_required(VERSION 3.0.0)

project(DigitalConductorDeps)

include(ExternalProject)

function(Submodule_Add DIRECTORY)
  if(NOT EXISTS ${PROJECT_SOURCE_DIR}/${DIRECTORY}/.git)
    find_package(Git QUIET REQUIRED)
    message(STATUS "Checking out ${PROJECT_SOURCE_DIR}/${DIRECTORY}...")
    execute_process(
      COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive ${DIRECTORY}
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      RESULT_VARIABLE GIT_EXIT_CODE
      )
    if(NOT GIT_EXIT_CODE EQUAL "0")
      message(FATAL_ERROR "Failed to check out ${PROJECT_SOURCE_DIR}/${DIRECTORY}")
    endif()
  endif()
endfunction(Submodule_Add)

set(ALL_DEPS)

# cxxopts
list(APPEND ALL_DEPS CxxoptsTarget)
Submodule_Add(cxxopts)
ExternalProject_Add(CxxoptsTarget
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/cxxopts"
  PREFIX "${PROJECT_BINARY_DIR}/cxxopts"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  )

# traget for all deps
add_custom_target(
  DigitalConductorDeps
  )
add_dependencies(DigitalConductorDeps
  ${ALL_DEPS}
  )

# serialize deps
list(REVERSE ALL_DEPS)
set(OTHER_DEPS ${ALL_DEPS})
foreach(DEP IN LISTS ALL_DEPS)
  list(REMOVE_ITEM OTHER_DEPS ${DEP})
  if(OTHER_DEPS)
    add_dependencies(${DEP}
      ${OTHER_DEPS}
      )
  endif()
endforeach()
