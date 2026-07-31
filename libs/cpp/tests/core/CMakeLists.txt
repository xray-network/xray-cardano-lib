add_executable(cardano_core_tests core_tests.cpp cbor_tests.cpp)
target_link_libraries(cardano_core_tests PRIVATE cardano::core Catch2::Catch2WithMain)
cardano_owned_target(cardano_core_tests)
catch_discover_tests(
  cardano_core_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
  PROPERTIES LABELS "core"
)
