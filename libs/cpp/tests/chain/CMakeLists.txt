add_executable(
  cardano_chain_tests
  address_tests.cpp
  builder_tests.cpp
  byron_tests.cpp
  conway_construction_tests.cpp
  era_header_tests.cpp
  ledger_tests.cpp
  multi_era_tests.cpp
  plutus_data_tests.cpp
)
target_link_libraries(
  cardano_chain_tests
  PRIVATE
    cardano::chain
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)
cardano_owned_target(cardano_chain_tests)
target_compile_definitions(
  cardano_chain_tests
  PRIVATE CARDANO_REPOSITORY_ROOT="${CMAKE_SOURCE_DIR}/../.."
)
catch_discover_tests(
  cardano_chain_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
  PROPERTIES LABELS "chain"
)
