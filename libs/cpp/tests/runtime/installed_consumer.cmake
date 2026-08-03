set(install_dir "${CARDANO_BINARY_DIR}/installed")
set(consumer_root "${CARDANO_BINARY_DIR}/installed-consumers")

file(REMOVE_RECURSE "${install_dir}" "${consumer_root}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" --install "${CARDANO_BINARY_DIR}" --prefix "${install_dir}"
  RESULT_VARIABLE install_result
)
if(NOT install_result EQUAL 0)
  message(FATAL_ERROR "XRAYCardanoLib installation failed")
endif()

foreach(required_path
    "include/cardano/cardano.hpp"
    "lib/libcardano.a"
    "lib/cmake/XRAYCardanoLib/XRAYCardanoLibConfig.cmake"
    "share/doc/XRAYCardanoLib/LICENSE"
    "share/doc/XRAYCardanoLib/README.md"
    "share/doc/XRAYCardanoLib/API_PARITY.md"
    "share/doc/XRAYCardanoLib/examples/consumer/CMakeLists.txt"
    "share/doc/XRAYCardanoLib/examples/consumer/main.cpp")
  if(NOT EXISTS "${install_dir}/${required_path}")
    message(FATAL_ERROR "Installed package is missing ${required_path}")
  endif()
endforeach()

file(
  GLOB_RECURSE installed_files
  LIST_DIRECTORIES FALSE
  RELATIVE "${install_dir}"
  "${install_dir}/*"
)
foreach(installed_file IN LISTS installed_files)
  if(installed_file MATCHES
      "(^|/)(providers?|tests?|vcpkg|node_modules)(/|$)|\\.(ts|tsx|js|mjs|cjs|hs|lhs|rs|wasm)$")
    message(FATAL_ERROR "Forbidden installed content: ${installed_file}")
  endif()
  if(installed_file MATCHES "\\.(hpp|cmake|md|txt)$" OR
     installed_file MATCHES "(^|/)CMakeLists\\.txt$")
    file(READ "${install_dir}/${installed_file}" installed_text)
    string(FIND installed_text "${CARDANO_SOURCE_DIR}" source_path_offset)
    string(FIND installed_text "${CARDANO_BINARY_DIR}" build_path_offset)
    if(NOT source_path_offset EQUAL -1 OR NOT build_path_offset EQUAL -1)
      message(FATAL_ERROR "Installed text leaks a build path: ${installed_file}")
    endif()
  endif()
endforeach()

file(GLOB installed_archives "${install_dir}/lib/*.a")
foreach(installed_archive IN LISTS installed_archives)
  execute_process(
    COMMAND "${CARDANO_AR}" -t "${installed_archive}"
    RESULT_VARIABLE archive_result
    OUTPUT_VARIABLE archive_members
  )
  if(NOT archive_result EQUAL 0 OR archive_members STREQUAL "" OR
     archive_members MATCHES "\\.(ts|js|hs|rs|wasm)(\n|$)")
    message(FATAL_ERROR "Installed archive failed content inspection: ${installed_archive}")
  endif()
endforeach()

foreach(component core crypto chain cip plutus lib)
  set(consumer_build "${consumer_root}/${component}")
  execute_process(
    COMMAND
      "${CMAKE_COMMAND}" -E env
      "VCPKG_ROOT=${CARDANO_VCPKG_ROOT}"
      "${CMAKE_COMMAND}"
      -S "${CARDANO_SOURCE_DIR}/examples/consumer"
      -B "${consumer_build}"
      -G "${CARDANO_GENERATOR}"
      "-DCARDANO_COMPONENT=${component}"
      "-DCMAKE_PREFIX_PATH=${install_dir}"
      "-DCMAKE_TOOLCHAIN_FILE=${CARDANO_TOOLCHAIN_FILE}"
      "-DCMAKE_MAKE_PROGRAM=${CARDANO_MAKE_PROGRAM}"
      "-DVCPKG_INSTALLED_DIR=${CARDANO_BINARY_DIR}/vcpkg_installed"
    RESULT_VARIABLE configure_result
  )
  if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "Installed ${component} consumer configuration failed")
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" --build "${consumer_build}"
                  RESULT_VARIABLE build_result)
  if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "Installed ${component} consumer build failed")
  endif()
  set(consumer_executable "${consumer_build}/cardano_installed_consumer")
  if(WIN32)
    string(APPEND consumer_executable ".exe")
  endif()
  execute_process(COMMAND "${consumer_executable}" RESULT_VARIABLE run_result)
  if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "Installed ${component} consumer execution failed")
  endif()
endforeach()

execute_process(
  COMMAND "${CMAKE_COMMAND}" -S "${CARDANO_SOURCE_DIR}/examples/consumer"
          -B "${consumer_root}/unknown" -G "${CARDANO_GENERATOR}"
          "-DCARDANO_COMPONENT=unknown" "-DCMAKE_PREFIX_PATH=${install_dir}"
  RESULT_VARIABLE unknown_result
  OUTPUT_QUIET ERROR_QUIET
)
if(unknown_result EQUAL 0)
  message(FATAL_ERROR "Unknown installed component unexpectedly configured")
endif()
