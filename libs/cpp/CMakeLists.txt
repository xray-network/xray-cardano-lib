cmake_minimum_required(VERSION 3.28)

project(
  CardanoLib
  VERSION 0.1.0
  DESCRIPTION "Independent C++23 Cardano wire, crypto, ledger, CIP, and Plutus library"
  LANGUAGES C CXX
)

if(NOT APPLE OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
  message(FATAL_ERROR "C++ implementation 0001 supports only macOS on ARM64")
endif()
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  message(FATAL_ERROR "C++ implementation 0001 requires Apple Clang")
endif()
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 21 OR
   NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 22)
  message(FATAL_ERROR "C++ implementation 0001 requires Apple Clang 21.x")
endif()
execute_process(
  COMMAND /usr/bin/sw_vers -productVersion
  OUTPUT_VARIABLE CARDANO_MACOS_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE CARDANO_SW_VERS_RESULT
)
if(NOT CARDANO_SW_VERS_RESULT EQUAL 0 OR
   CARDANO_MACOS_VERSION VERSION_LESS 26.5 OR
   NOT CARDANO_MACOS_VERSION VERSION_LESS 26.6)
  message(FATAL_ERROR "C++ implementation 0001 requires macOS 26.5.x")
endif()
execute_process(
  COMMAND "${CMAKE_MAKE_PROGRAM}" --version
  OUTPUT_VARIABLE CARDANO_NINJA_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE CARDANO_NINJA_RESULT
)
if(NOT CMAKE_GENERATOR STREQUAL "Ninja" OR
   NOT CARDANO_NINJA_RESULT EQUAL 0 OR
   CARDANO_NINJA_VERSION VERSION_LESS 1.11)
  message(FATAL_ERROR "C++ implementation 0001 requires Ninja 1.11 or newer")
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(CARDANO_BUILD_TESTS "Build the Cardano Lib validation suite" OFF)
option(CARDANO_PROVIDER_TESTS "Run tests directly against captured provider evidence" OFF)
option(CARDANO_WARNINGS_AS_ERRORS "Treat owned-source compiler warnings as errors" OFF)
option(CARDANO_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
set(CARDANO_HARDENING_CASES "0" CACHE STRING "Number of deterministic adversarial cases")
find_program(CARDANO_CLANG_TIDY NAMES clang-tidy)

if(NOT CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD LESS 23)
  set(CMAKE_CXX_STANDARD 23)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost CONFIG REQUIRED)
find_package(Botan CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)
find_package(libsecp256k1 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(blst CONFIG REQUIRED)

function(cardano_owned_target target)
  target_compile_features(${target} PUBLIC cxx_std_23)
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  if(CARDANO_WARNINGS_AS_ERRORS)
    target_compile_options(${target} PRIVATE -Werror)
  endif()
  if(CARDANO_ENABLE_SANITIZERS)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PUBLIC -fsanitize=address,undefined)
  endif()
  if(CARDANO_CLANG_TIDY)
    set_property(
      TARGET ${target}
      PROPERTY CXX_CLANG_TIDY
        "${CARDANO_CLANG_TIDY};--warnings-as-errors=*"
    )
  endif()
endfunction()

function(cardano_public_headers target)
  target_include_directories(
    ${target}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
endfunction()

add_library(cardano_core STATIC
  src/core/bech32.cpp
  src/core/big_integer.cpp
  src/core/bytes.cpp
  src/core/cbor.cpp
  src/core/error.cpp
)
add_library(cardano::core ALIAS cardano_core)
set_target_properties(cardano_core PROPERTIES EXPORT_NAME core)
cardano_owned_target(cardano_core)
cardano_public_headers(cardano_core)
target_link_libraries(cardano_core PRIVATE Boost::headers)

add_library(cardano_crypto STATIC src/crypto/crypto.cpp)
add_library(cardano::crypto ALIAS cardano_crypto)
set_target_properties(cardano_crypto PROPERTIES EXPORT_NAME crypto)
cardano_owned_target(cardano_crypto)
cardano_public_headers(cardano_crypto)
target_link_libraries(
  cardano_crypto
  PUBLIC cardano_core
  PRIVATE
    Botan::Botan-static
    unofficial-sodium::sodium
    libsecp256k1::secp256k1
    nlohmann_json::nlohmann_json
    blst::blst
)

add_library(cardano_chain STATIC
  src/chain/builder.cpp
  src/chain/byron.cpp
  src/chain/byron_witness.cpp
  src/chain/chain.cpp
  src/chain/era/models.cpp
  src/chain/genesis.cpp
  src/chain/ledger.cpp
  src/chain/multi_era.cpp
  src/chain/ledger/plutus_data.cpp
)
add_library(cardano::chain ALIAS cardano_chain)
set_target_properties(cardano_chain PROPERTIES EXPORT_NAME chain)
cardano_owned_target(cardano_chain)
cardano_public_headers(cardano_chain)
target_link_libraries(cardano_chain PUBLIC cardano_core cardano_crypto PRIVATE nlohmann_json::nlohmann_json)

add_library(cardano_cip STATIC
  src/cip/cip.cpp
  src/cip/cip25.cpp
  src/cip/cip36.cpp
  src/cip/cip8.cpp
)
add_library(cardano::cip ALIAS cardano_cip)
set_target_properties(cardano_cip PROPERTIES EXPORT_NAME cip)
cardano_owned_target(cardano_cip)
cardano_public_headers(cardano_cip)
target_link_libraries(cardano_cip PUBLIC cardano_core cardano_crypto cardano_chain PRIVATE nlohmann_json::nlohmann_json)

add_library(cardano_plutus STATIC
  src/plutus/typed_data.cpp
  src/plutus/phase_two.cpp
  src/plutus/phase_two_context.cpp
  src/plutus/plutus.cpp
  src/plutus/uplc.cpp
)
add_library(cardano::plutus ALIAS cardano_plutus)
set_target_properties(cardano_plutus PROPERTIES EXPORT_NAME plutus)
cardano_owned_target(cardano_plutus)
cardano_public_headers(cardano_plutus)
target_link_libraries(
  cardano_plutus
  PUBLIC cardano_core cardano_crypto cardano_chain
  PRIVATE nlohmann_json::nlohmann_json
)

add_library(cardano_lib STATIC src/cardano.cpp)
add_library(cardano::lib ALIAS cardano_lib)
set_target_properties(cardano_lib PROPERTIES EXPORT_NAME lib OUTPUT_NAME cardano)
cardano_owned_target(cardano_lib)
cardano_public_headers(cardano_lib)
target_link_libraries(cardano_lib PUBLIC cardano_core cardano_crypto cardano_chain cardano_cip cardano_plutus)

install(
  TARGETS cardano_core cardano_crypto cardano_chain cardano_cip cardano_plutus cardano_lib
  EXPORT CardanoLibTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES README.md API_PARITY.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(DIRECTORY examples/ DESTINATION "${CMAKE_INSTALL_DOCDIR}/examples")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE" DESTINATION ${CMAKE_INSTALL_DOCDIR})

configure_package_config_file(
  cmake/CardanoLibConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/CardanoLibConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CardanoLib"
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/CardanoLibConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(
  EXPORT CardanoLibTargets
  FILE CardanoLibTargets.cmake
  NAMESPACE cardano::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CardanoLib"
)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CardanoLibConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/CardanoLibConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CardanoLib"
)

if(CARDANO_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
