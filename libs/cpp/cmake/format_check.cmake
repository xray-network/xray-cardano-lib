if(NOT DEFINED CARDANO_SOURCE_DIR OR NOT IS_DIRECTORY "${CARDANO_SOURCE_DIR}")
  message(FATAL_ERROR "CARDANO_SOURCE_DIR must name the C++ workspace")
endif()

find_program(CARDANO_XCRUN xcrun REQUIRED)
execute_process(
  COMMAND "${CARDANO_XCRUN}" --find clang-format
  RESULT_VARIABLE find_result
  OUTPUT_VARIABLE clang_format
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT find_result EQUAL 0 OR clang_format STREQUAL "")
  message(FATAL_ERROR "Apple clang-format is required for the format gate")
endif()

file(
  GLOB_RECURSE source_files
  LIST_DIRECTORIES FALSE
  "${CARDANO_SOURCE_DIR}/include/*.hpp"
  "${CARDANO_SOURCE_DIR}/src/*.cpp"
  "${CARDANO_SOURCE_DIR}/src/*.hpp"
  "${CARDANO_SOURCE_DIR}/tests/*.cpp"
  "${CARDANO_SOURCE_DIR}/tests/*.hpp"
  "${CARDANO_SOURCE_DIR}/examples/*.cpp"
  "${CARDANO_SOURCE_DIR}/examples/*.hpp"
)
list(SORT source_files)
foreach(source_file IN LISTS source_files)
  execute_process(
    COMMAND
      "${clang_format}" --style=file --dry-run --Werror "${source_file}"
    RESULT_VARIABLE format_result
  )
  if(NOT format_result EQUAL 0)
    message(FATAL_ERROR "Formatting check failed: ${source_file}")
  endif()
endforeach()
