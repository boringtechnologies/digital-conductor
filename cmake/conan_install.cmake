function(conan_install conan_file host_profile)
  # download helper conan.cmake
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(
      DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
      "${CMAKE_BINARY_DIR}/conan.cmake"
      EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
      TLS_VERIFY ON)
  endif()
  include(${CMAKE_BINARY_DIR}/conan.cmake)

  # install dependencies from conanfile.txt
  set(CONAN_MODULE_PATH ${CMAKE_BINARY_DIR}/conan)

  if(NOT host_profile)
    set(host_profile "default")
  endif()

  conan_cmake_install(PATH_OR_REFERENCE ${conan_file}
                      BUILD missing
                      INSTALL_FOLDER ${CONAN_MODULE_PATH}
                      PROFILE_HOST ${host_profile}
                      PROFILE_BUILD default
                      SETTINGS build_type=Release) # ${CMAKE_BUILD_TYPE})

  # fix aliases
  include(cmake/alias_package.cmake)
  # alias_package(${CONAN_MODULE_PATH} libjpeg-turbo JpegTurbo)

  set(CONAN_MODULE_PATH ${CONAN_MODULE_PATH} PARENT_SCOPE)
endfunction()
