# Create 'alias' for FindXXX.cmake script
# Used to not change naming in existing scripts
function(alias_package source_package_dir source_package_name target_package_name)
  set (source_find "${source_package_dir}/Find${source_package_name}.cmake")
  if (NOT EXISTS ${source_find})
    message(WARNING "Skip aliasing because ${source_find} not exists")
    return()
  endif()

  file(READ ${source_find} find_source_package_cmake_script)

  string(TOLOWER "${source_package_name}" source_package_name_lower)

  # replace IMPORTED targets
  string(REPLACE "${source_package_name}::${source_package_name}"
    "${target_package_name}::${target_package_name}"
    find_target_package_cmake_script
    "${find_source_package_cmake_script}")

  # replace FindXXX.cmake reference
  string(REPLACE "Find${source_package_name}.cmake"
    "Find${target_package_name}.cmake"
    find_target_package_cmake_script
    "${find_target_package_cmake_script}")

  # replace variables
  foreach(suffix
      _FOUND
      _VERSION
      _INCLUDE_DIRS
      _INCLUDE_DIR
      _INCLUDES
      _DEFINITIONS
      _LIBRARIES
      _LIBRARIES_TARGETS
      _LIBS
      _LIBRARY_LIST
      _LIB_DIRS
      _RES_DIRS
      _COMPILE_OPTIONS_LIST
      _COMPILE_DEFINITIONS
      _LINKER_FLAGS_LIST
      _SYSTEM_LIBS
      _FRAMEWORK_DIRS
      _FRAMEWORKS
      _FRAMEWORKS_FOUND
      _BUILD_MODULES_PATHS
      _COMPILE_OPTIONS_C
      _COMPILE_OPTIONS_CXX)
    string(REPLACE "${source_package_name}${suffix}"
      "${target_package_name}${suffix}"
      find_target_package_cmake_script
      "${find_target_package_cmake_script}")
    string(REPLACE "${source_package_name_lower}${suffix}"
      "${target_package_name}${suffix}"
      find_target_package_cmake_script
      "${find_target_package_cmake_script}")
  endforeach(suffix)

  # this logic need for canse-insensiteve FS
  file(REMOVE "${source_package_dir}/Find${target_package_name}.cmake")
  file(WRITE "${source_package_dir}/Find${target_package_name}.cmake" "${find_target_package_cmake_script}")

  # replace reference in other scripts
  file(GLOB find_scripts "${source_package_dir}/Find*.cmake")
  list(REMOVE_ITEM find_scripts "${source_package_dir}/Find${source_package_name}.cmake")
  foreach(find_script ${find_scripts})
    file(READ "${find_script}" find_source_script)

    string(REPLACE "${source_package_name}::${source_package_name}"
      "${target_package_name}::${target_package_name}"
      find_target_script
      "${find_source_script}")

    foreach(suffix _FOUND " REQUIRED" " already found")
      string(REPLACE "${source_package_name}${suffix}"
        "${target_package_name}${suffix}"
        find_target_script "${find_target_script}" )
    endforeach(suffix)

    file(WRITE ${find_script} "${find_target_script}")
  endforeach()
endfunction()
