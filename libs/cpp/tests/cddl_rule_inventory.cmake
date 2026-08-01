if(NOT DEFINED CARDANO_REPOSITORY_ROOT OR NOT DEFINED CARDANO_SOURCE_DIR)
  message(FATAL_ERROR "CARDANO_REPOSITORY_ROOT and CARDANO_SOURCE_DIR are required")
endif()

set(CDDL_ROOT
    "${CARDANO_REPOSITORY_ROOT}/.xray/updates/providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras"
)
set(CDDL_FILES
    byron.cddl
    shelley.cddl
    allegra.cddl
    mary.cddl
    alonzo.cddl
    babbage.cddl
    conway.cddl
)
set(RULE_PATTERN "^([^ =]+) *=")
set(ALL_RULES)
set(RULE_DEFINITION_COUNT 0)

foreach(CDDL_FILE IN LISTS CDDL_FILES)
  set(CDDL_PATH "${CDDL_ROOT}/${CDDL_FILE}")
  if(NOT EXISTS "${CDDL_PATH}")
    message(FATAL_ERROR "Missing frozen CDDL artifact: ${CDDL_PATH}")
  endif()
  file(READ "${CDDL_PATH}" CDDL_CONTENT)
  string(REPLACE ";" "@SEMICOLON@" CDDL_CONTENT "${CDDL_CONTENT}")
  string(REPLACE "[" "@LBRACKET@" CDDL_CONTENT "${CDDL_CONTENT}")
  string(REPLACE "]" "@RBRACKET@" CDDL_CONTENT "${CDDL_CONTENT}")
  string(REPLACE "\n" ";" CDDL_LINES "${CDDL_CONTENT}")
  set(RULE_LINES)
  foreach(CDDL_LINE IN LISTS CDDL_LINES)
    if(CDDL_LINE MATCHES "${RULE_PATTERN}")
      list(APPEND RULE_LINES "${CDDL_LINE}")
    endif()
  endforeach()
  list(LENGTH RULE_LINES FILE_RULE_COUNT)
  message(STATUS "${CDDL_FILE}: ${FILE_RULE_COUNT} rule definitions")
  math(EXPR RULE_DEFINITION_COUNT "${RULE_DEFINITION_COUNT} + ${FILE_RULE_COUNT}")
  foreach(RULE_LINE IN LISTS RULE_LINES)
    string(REGEX REPLACE "${RULE_PATTERN}.*" "\\1" RULE_NAME "${RULE_LINE}")
    list(APPEND ALL_RULES "${RULE_NAME}")
  endforeach()
endforeach()

if(NOT RULE_DEFINITION_COUNT EQUAL 714)
  message(FATAL_ERROR
          "Frozen CDDL definition count changed: expected 714, got ${RULE_DEFINITION_COUNT}")
endif()

list(REMOVE_DUPLICATES ALL_RULES)
list(SORT ALL_RULES)
list(LENGTH ALL_RULES UNIQUE_RULE_COUNT)
if(NOT UNIQUE_RULE_COUNT EQUAL 228)
  message(FATAL_ERROR "Frozen CDDL rule count changed: expected 228, got ${UNIQUE_RULE_COUNT}")
endif()

file(READ "${CARDANO_SOURCE_DIR}/API_PARITY.md" API_PARITY)
foreach(RULE_NAME IN LISTS ALL_RULES)
  string(FIND "${API_PARITY}" "`${RULE_NAME}`" RULE_POSITION)
  if(RULE_POSITION EQUAL -1)
    message(FATAL_ERROR "API_PARITY.md has no disposition for CDDL rule `${RULE_NAME}`")
  endif()
endforeach()

message(STATUS
        "Verified ${RULE_DEFINITION_COUNT} era definitions and ${UNIQUE_RULE_COUNT} unique rule dispositions"
)
