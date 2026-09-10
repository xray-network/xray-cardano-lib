add_executable(
  cardano_plutus_tests
  blueprint_tests.cpp
  typed_data_tests.cpp
  uplc_conformance_tests.cpp
  uplc_tests.cpp
)
target_link_libraries(
  cardano_plutus_tests
  PRIVATE
    cardano::plutus
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)
cardano_owned_target(cardano_plutus_tests)
target_compile_definitions(
  cardano_plutus_tests
  PRIVATE CARDANO_REPOSITORY_ROOT="${CMAKE_SOURCE_DIR}/../.."
)
catch_discover_tests(
  cardano_plutus_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
  PROPERTIES LABELS "plutus"
)
