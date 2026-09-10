cmake_minimum_required(VERSION 3.28)

project(
  XRAYCardanoLib
  VERSION 0.1.0
  DESCRIPTION "Independent C++23 Cardano wire, crypto, ledger, CIP, and Plutus library"
  LANGUAGES C CXX
)

message(STATUS "XRAY Cardano Lib host: ${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}; compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
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

option(CARDANO_BUILD_TESTS "Build the XRAY Cardano Lib validation suite" OFF)
option(CARDANO_BUILD_BENCHMARKS "Build deterministic XRAY benchmark reporting" OFF)
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

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("#include <expected>\n#include <span>\n#include <bit>\n#include <ranges>\nint main(){std::expected<int,int> x=1; int a[1]{}; std::span<int> s(a); auto b=std::byteswap(1u); auto v=s|std::views::all; return *x+int(b)+int(v.size());}" CARDANO_HAS_REQUIRED_CXX23)
if(NOT CARDANO_HAS_REQUIRED_CXX23)
  message(FATAL_ERROR "XRAY Cardano Lib requires C++23 expected/span/byteswap/ranges capabilities")
endif()

find_package(Boost CONFIG REQUIRED)
find_package(Botan CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)
find_package(libsecp256k1 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(blst CONFIG REQUIRED)

function(cardano_owned_target target)
  target_compile_features(${target} PUBLIC cxx_std_23)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
    if(CARDANO_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    if(CARDANO_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
  if(CARDANO_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
      target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(${target} PUBLIC -fsanitize=address,undefined)
    else()
      message(FATAL_ERROR "CARDANO_ENABLE_SANITIZERS requires Clang, AppleClang, or GCC")
    endif()
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
target_link_libraries(cardano_core PRIVATE $<BUILD_INTERFACE:Boost::headers>)

add_library(cardano_crypto STATIC src/crypto/crypto.cpp src/crypto/identity.cpp src/crypto/derivation.cpp)
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
  src/cip/cip14.cpp
  src/cip/cip21.cpp
  src/cip/cip67.cpp
  src/cip/cip68.cpp
  src/cip/cip129.cpp
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
  src/plutus/blueprint.cpp
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

install(TARGETS cardano_core EXPORT XRAYCardanoLibCoreTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS cardano_crypto EXPORT XRAYCardanoLibCryptoTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS cardano_chain EXPORT XRAYCardanoLibChainTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS cardano_cip EXPORT XRAYCardanoLibCipTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS cardano_plutus EXPORT XRAYCardanoLibPlutusTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS cardano_lib EXPORT XRAYCardanoLibLibTargets
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
  cmake/XRAYCardanoLibConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/XRAYCardanoLibConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/XRAYCardanoLib"
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/XRAYCardanoLibConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
foreach(_component Core Crypto Chain Cip Plutus Lib)
  install(EXPORT XRAYCardanoLib${_component}Targets FILE XRAYCardanoLib${_component}Targets.cmake NAMESPACE cardano:: DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/XRAYCardanoLib")
endforeach()
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/XRAYCardanoLibConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/XRAYCardanoLibConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/XRAYCardanoLib"
)

if(CARDANO_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(CARDANO_BUILD_BENCHMARKS)
  add_executable(cardano_benchmarks benchmarks/cardano_benchmarks.cpp)
  target_link_libraries(cardano_benchmarks PRIVATE cardano::lib)
  cardano_owned_target(cardano_benchmarks)
  execute_process(COMMAND git rev-parse --verify HEAD WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                  OUTPUT_VARIABLE CARDANO_BENCHMARK_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE
                  ERROR_QUIET)
  execute_process(COMMAND git status --porcelain WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                  OUTPUT_VARIABLE CARDANO_BENCHMARK_STATUS OUTPUT_STRIP_TRAILING_WHITESPACE
                  ERROR_QUIET)
  if(CARDANO_BENCHMARK_STATUS STREQUAL "")
    set(CARDANO_BENCHMARK_DIRTY false)
  else()
    set(CARDANO_BENCHMARK_DIRTY true)
  endif()
  target_compile_definitions(cardano_benchmarks PRIVATE
    "CARDANO_BENCHMARK_BUILD_TYPE=\"${CMAKE_BUILD_TYPE}\""
    "CARDANO_BENCHMARK_COMPILER=\"${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}\""
    "CARDANO_BENCHMARK_DIRTY=${CARDANO_BENCHMARK_DIRTY}"
    "CARDANO_BENCHMARK_REVISION=\"${CARDANO_BENCHMARK_REVISION}\"")
endif()
