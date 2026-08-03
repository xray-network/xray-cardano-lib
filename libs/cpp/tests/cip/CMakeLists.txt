add_executable(cardano_cip_tests cip14_tests.cpp cip21_tests.cpp cip25_tests.cpp cip36_tests.cpp cip67_cip68_tests.cpp cip8_tests.cpp cip129_tests.cpp)
target_link_libraries(
  cardano_cip_tests
  PRIVATE cardano::cip Catch2::Catch2WithMain
)
cardano_owned_target(cardano_cip_tests)
catch_discover_tests(
  cardano_cip_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
  PROPERTIES LABELS "cip"
)
