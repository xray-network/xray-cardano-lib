set(install_dir "${CARDANO_BINARY_DIR}/installed")
set(consumer_build "${CARDANO_BINARY_DIR}/installed-consumer")

file(REMOVE_RECURSE "${install_dir}" "${consumer_build}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" --install "${CARDANO_BINARY_DIR}" --prefix "${install_dir}"
  RESULT_VARIABLE install_result
)
if(NOT install_result EQUAL 0)
  message(FATAL_ERROR "CardanoLib installation failed")
endif()

foreach(required_path
    "include/cardano/cardano.hpp"
    "lib/libcardano.a"
    "lib/cmake/CardanoLib/CardanoLibConfig.cmake"
    "share/doc/CardanoLib/LICENSE"
    "share/doc/CardanoLib/README.md"
    "share/doc/CardanoLib/API_PARITY.md"
    "share/doc/CardanoLib/examples/consumer/CMakeLists.txt"
    "share/doc/CardanoLib/examples/consumer/main.cpp")
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
    COMMAND /usr/bin/ar -t "${installed_archive}"
    RESULT_VARIABLE archive_result
    OUTPUT_VARIABLE archive_members
  )
  if(NOT archive_result EQUAL 0 OR archive_members STREQUAL "" OR
     archive_members MATCHES "\\.(ts|js|hs|rs|wasm)(\n|$)")
    message(FATAL_ERROR "Installed archive failed content inspection: ${installed_archive}")
  endif()
endforeach()

execute_process(
  COMMAND
    "${CMAKE_COMMAND}" -E env
    "VCPKG_ROOT=${CARDANO_VCPKG_ROOT}"
    "${CMAKE_COMMAND}"
    -S "${CARDANO_SOURCE_DIR}/examples/consumer"
    -B "${consumer_build}"
    -G "${CARDANO_GENERATOR}"
    "-DCMAKE_PREFIX_PATH=${install_dir}"
    "-DCMAKE_TOOLCHAIN_FILE=${CARDANO_TOOLCHAIN_FILE}"
    "-DCMAKE_MAKE_PROGRAM=${CARDANO_MAKE_PROGRAM}"
    "-DVCPKG_INSTALLED_DIR=${CARDANO_BINARY_DIR}/vcpkg_installed"
  RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
  message(FATAL_ERROR "Installed consumer configuration failed")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${consumer_build}"
  RESULT_VARIABLE build_result
)
if(NOT build_result EQUAL 0)
  message(FATAL_ERROR "Installed consumer build failed")
endif()

execute_process(
  COMMAND "${consumer_build}/cardano_installed_consumer"
  RESULT_VARIABLE run_result
)
if(NOT run_result EQUAL 0)
  message(FATAL_ERROR "Installed consumer execution failed")
endif()
