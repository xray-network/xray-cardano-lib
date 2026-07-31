add_executable(cardano_crypto_tests crypto_tests.cpp)
target_link_libraries(cardano_crypto_tests PRIVATE cardano::crypto Catch2::Catch2WithMain)
cardano_owned_target(cardano_crypto_tests)
catch_discover_tests(
  cardano_crypto_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
  PROPERTIES LABELS "crypto"
)
