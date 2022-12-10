# Do conan dependencies install based on conanfile and profile
function(conan_install conan_file conan_profile)
  # download helper conan.cmake
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(
      DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
      "${CMAKE_BINARY_DIR}/conan.cmake"
      EXPECTED_HASH SHA256=5cdb3042632da3efff558924eecefd580a0e786863a857ca097c3d1d43df5dcd
      TLS_VERIFY ON)
  endif()
  include(${CMAKE_BINARY_DIR}/conan.cmake)

  # install dependencies from conanfile.txt
  set(conan_module_path ${CMAKE_BINARY_DIR}/conan)

  if(NOT conan_profile)
    set(conan_profile "default")
  endif()

  conan_cmake_install(PATH_OR_REFERENCE ${conan_file}
                      BUILD outdated
                      INSTALL_FOLDER ${conan_module_path}
                      PROFILE ${conan_profile}
                      SETTINGS build_type=Release) # ${CMAKE_BUILD_TYPE})

  # fix aliases
  include(${CMAKE_SOURCE_DIR}/conan/cmake/alias_package.cmake)
  alias_package(${conan_module_path} libjuice LibJuice)
  alias_package(${conan_module_path} gRPC GRPC)
  alias_package(${conan_module_path} protobuf Protobuf)
  alias_package(${conan_module_path} gst-plugins-base gstpluginsbase)
  alias_package(${conan_module_path} gst-plugins-good gstpluginsgood)
  alias_package(${conan_module_path} gst-plugins-bad gstpluginsbad)
  alias_package(${conan_module_path} gst-libav gstlibav)

  set(CONAN_MODULE_PATH ${conan_module_path} PARENT_SCOPE)
endfunction()
