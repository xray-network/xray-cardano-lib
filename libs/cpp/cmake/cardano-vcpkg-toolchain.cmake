if(NOT DEFINED ENV{VCPKG_ROOT} OR "$ENV{VCPKG_ROOT}" STREQUAL "")
  message(FATAL_ERROR "VCPKG_ROOT must name the pinned vcpkg checkout")
endif()

set(
  CARDANO_VCPKG_ROOT
  "$ENV{VCPKG_ROOT}"
  CACHE PATH
  "Pinned vcpkg checkout used by XRAY Cardano Lib"
)

set(
  VCPKG_OVERLAY_TRIPLETS
  "${CMAKE_CURRENT_LIST_DIR}/vcpkg-triplets"
  CACHE STRING
  "XRAY Cardano Lib triplets with frozen secp256k1 module policy"
)

include("$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
