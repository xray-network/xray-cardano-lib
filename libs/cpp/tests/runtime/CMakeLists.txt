add_subdirectory(consumer)

add_test(
  NAME cardano_installed_consumer
  COMMAND
    "${CMAKE_COMMAND}"
    "-DCARDANO_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
    "-DCARDANO_BINARY_DIR=${CMAKE_BINARY_DIR}"
    "-DCARDANO_GENERATOR=${CMAKE_GENERATOR}"
    "-DCARDANO_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
    "-DCARDANO_VCPKG_ROOT=${CARDANO_VCPKG_ROOT}"
    "-DCARDANO_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/installed_consumer.cmake"
)
set_tests_properties(
  cardano_installed_consumer
  PROPERTIES
    LABELS "runtime;install"
    RUN_SERIAL TRUE
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)
