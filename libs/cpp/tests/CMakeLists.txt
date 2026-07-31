find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_subdirectory(core)
add_subdirectory(crypto)
add_subdirectory(chain)
add_subdirectory(plutus)
add_subdirectory(cip)
add_subdirectory(hardening)
add_subdirectory(runtime)

add_test(
  NAME cardano_format_check
  COMMAND
    "${CMAKE_COMMAND}"
    "-DCARDANO_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
    -P "${CMAKE_SOURCE_DIR}/cmake/format_check.cmake"
)
set_tests_properties(cardano_format_check PROPERTIES LABELS "static;format")

add_test(
  NAME cardano_api_inventory
  COMMAND
    "${CMAKE_COMMAND}"
    "-DCARDANO_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
    "-DCARDANO_REPOSITORY_ROOT=${CMAKE_SOURCE_DIR}/../.."
    -P "${CMAKE_CURRENT_SOURCE_DIR}/api_inventory.cmake"
)
set_tests_properties(cardano_api_inventory PROPERTIES LABELS "static;api")

add_test(
  NAME cardano_cddl_rule_inventory
  COMMAND
    "${CMAKE_COMMAND}"
    "-DCARDANO_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
    "-DCARDANO_REPOSITORY_ROOT=${CMAKE_SOURCE_DIR}/../.."
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cddl_rule_inventory.cmake"
)
set_tests_properties(cardano_cddl_rule_inventory PROPERTIES LABELS "static;api;provider")

if(CARDANO_PROVIDER_TESTS)
  add_test(
    NAME cardano_provider_integrity
    COMMAND
      "${CMAKE_COMMAND}"
      "-DCARDANO_REPOSITORY_ROOT=${CMAKE_SOURCE_DIR}/../.."
      -P "${CMAKE_CURRENT_SOURCE_DIR}/provider_integrity.cmake"
  )
  set_tests_properties(cardano_provider_integrity PROPERTIES LABELS "provider;integrity")
endif()
