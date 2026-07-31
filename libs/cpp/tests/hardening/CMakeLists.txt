add_executable(cardano_hardening_tests hardening_tests.cpp)
target_link_libraries(
  cardano_hardening_tests
  PRIVATE cardano::lib Catch2::Catch2WithMain
)
target_compile_definitions(
  cardano_hardening_tests
  PRIVATE CARDANO_HARDENING_CASES=${CARDANO_HARDENING_CASES}
)
cardano_owned_target(cardano_hardening_tests)
catch_discover_tests(
  cardano_hardening_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
  PROPERTIES LABELS "hardening"
)
