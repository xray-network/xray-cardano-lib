vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO supranational/blst
  REF 54e6e55674722fc2797ebb4bbb71b26d881eb4b8
  SHA512 e11f4f66051de45d812f1be8539bc6fd4f703f4fb39aa5c556051ae3d7b93eb1331af37cb93d4f2e8985200037b921e6bcc848297bdc2050d2a839d5fecc99af
  HEAD_REF NONE
)

set(VCPKG_POLICY_MISMATCHED_NUMBER_OF_BINARIES enabled)

file(COPY
  "${SOURCE_PATH}/bindings/blst.h"
  "${SOURCE_PATH}/bindings/blst.hpp"
  "${SOURCE_PATH}/bindings/blst_aux.h"
  DESTINATION "${CURRENT_PACKAGES_DIR}/include"
)

vcpkg_execute_required_process(
  COMMAND "${SOURCE_PATH}/build.sh"
  WORKING_DIRECTORY "${SOURCE_PATH}"
  LOGNAME build-${TARGET_TRIPLET}
)
file(INSTALL "${SOURCE_PATH}/libblst.a" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/blst-config.cmake" [=[
if(NOT TARGET blst::blst)
  add_library(blst::blst STATIC IMPORTED)
  get_filename_component(_blst_prefix "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
  set(_blst_library "${_blst_prefix}/lib/libblst.a")
  set_target_properties(blst::blst PROPERTIES
    IMPORTED_LOCATION "${_blst_library}"
    INTERFACE_INCLUDE_DIRECTORIES "${_blst_prefix}/include"
  )
endif()
]=])
