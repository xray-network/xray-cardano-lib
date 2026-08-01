if(NOT DEFINED CARDANO_REPOSITORY_ROOT)
  message(FATAL_ERROR "CARDANO_REPOSITORY_ROOT is required")
endif()

function(require_regular_nonempty path)
  if(NOT EXISTS "${path}" OR IS_DIRECTORY "${path}" OR IS_SYMLINK "${path}")
    message(FATAL_ERROR "Provider artifact is missing or not a regular file: ${path}")
  endif()
  file(SIZE "${path}" size)
  if(size EQUAL 0)
    message(FATAL_ERROR "Provider artifact is empty: ${path}")
  endif()
endfunction()

function(check_sha256_inventory sums_file expected_count)
  require_regular_nonempty("${sums_file}")
  get_filename_component(artifact_root "${sums_file}" DIRECTORY)
  file(STRINGS "${sums_file}" lines)
  list(LENGTH lines count)
  if(NOT count EQUAL expected_count)
    message(FATAL_ERROR
      "Unexpected checksum inventory size in ${sums_file}: ${count}, expected ${expected_count}")
  endif()
  foreach(line IN LISTS lines)
    if(NOT line MATCHES "^([0-9a-f]+)  (.+)$")
      message(FATAL_ERROR "Malformed SHA256SUMS line: ${line}")
    endif()
    set(expected "${CMAKE_MATCH_1}")
    set(relative "${CMAKE_MATCH_2}")
    string(LENGTH "${expected}" digest_length)
    if(NOT digest_length EQUAL 64)
      message(FATAL_ERROR "Malformed SHA-256 digest in: ${line}")
    endif()
    set(path "${artifact_root}/${relative}")
    require_regular_nonempty("${path}")
    file(SHA256 "${path}" actual)
    if(NOT actual STREQUAL expected)
      message(FATAL_ERROR "Provider checksum mismatch: ${path}")
    endif()
  endforeach()
endfunction()

set(provider_root "${CARDANO_REPOSITORY_ROOT}/.xray/updates/providers")

check_sha256_inventory(
  "${provider_root}/message-signing/0001-message-signing/artifacts/SHA256SUMS"
  11
)
check_sha256_inventory(
  "${provider_root}/uplc/0001-uplc/artifacts/SHA256SUMS"
  139
)

set(cml_root
  "${provider_root}/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors")
set(cml_manifest_path "${cml_root}/manifest.json")
require_regular_nonempty("${cml_manifest_path}")
file(READ "${cml_manifest_path}" cml_manifest)

string(JSON schema_version GET "${cml_manifest}" schemaVersion)
if(NOT schema_version EQUAL 1)
  message(FATAL_ERROR "Unsupported CML vector manifest schema: ${schema_version}")
endif()

foreach(group IN ITEMS genesisJson goldenBlocks)
  string(JSON count LENGTH "${cml_manifest}" fixtures "${group}")
  if(group STREQUAL "genesisJson" AND NOT count EQUAL 6)
    message(FATAL_ERROR "Expected six CML genesis fixtures, got ${count}")
  elseif(group STREQUAL "goldenBlocks" AND NOT count EQUAL 86)
    message(FATAL_ERROR "Expected 86 CML block fixtures, got ${count}")
  endif()
  math(EXPR last "${count} - 1")
  foreach(index RANGE 0 "${last}")
    string(JSON fixture_path GET "${cml_manifest}" fixtures "${group}" "${index}" fixturePath)
    string(JSON source_path GET "${cml_manifest}" fixtures "${group}" "${index}" path)
    string(JSON expected_sha GET "${cml_manifest}" fixtures "${group}" "${index}" sha256)
    set(path "${cml_root}/${source_path}")
    require_regular_nonempty("${path}")
    file(SHA256 "${path}" actual_sha)
    if(NOT actual_sha STREQUAL expected_sha)
      message(FATAL_ERROR "CML fixture checksum mismatch: ${fixture_path}")
    endif()
    if(group STREQUAL "genesisJson")
      string(JSON expected_size GET "${cml_manifest}" fixtures "${group}" "${index}" bytes)
    else()
      string(JSON expected_size GET "${cml_manifest}" fixtures "${group}" "${index}" storedBytes)
    endif()
    file(SIZE "${path}" actual_size)
    if(NOT actual_size EQUAL expected_size)
      message(FATAL_ERROR "CML fixture size mismatch: ${fixture_path}")
    endif()
  endforeach()
endforeach()

set(ledger_root
  "${provider_root}/cardano-ledger/0001-cardano-ledger/artifacts")
set(ledger_files
  "${ledger_root}/cddl/eras/byron.cddl"
  "${ledger_root}/cddl/eras/shelley.cddl"
  "${ledger_root}/cddl/eras/allegra.cddl"
  "${ledger_root}/cddl/eras/mary.cddl"
  "${ledger_root}/cddl/eras/alonzo.cddl"
  "${ledger_root}/cddl/eras/babbage.cddl"
  "${ledger_root}/cddl/eras/conway.cddl"
  "${ledger_root}/legal/LICENSE"
  "${ledger_root}/legal/NOTICE"
)
set(ledger_sha256
  "bc6f7fc1c6295046a2944ad784ce4b5ea544a185400add4aa7b122cd8e46a107"
  "3a4723732bcd9dafbbb5d2e6c29d9ae3347575212adbf6bbd5e4de16a6790791"
  "e71cbf08f4fe62bb654f8371f4934eba750c17546b7d44c0c4f38b470eaabcd6"
  "aa13e8687343658c5195b115d54fa1f4dfd7beeb70020f3e6c57f63f9be7aef1"
  "7460f60206160f3b459ee58befb1b912acf1812402113faa3963bf4bda0cf98d"
  "fcca168539a91a16c45b55c724b52e34bd85ff6499148a976ae5e01b66cff272"
  "316ed8ee090ea172983083329e849f24f4360a236d26be0a6f2094c6078f1e1f"
  "0d542e0c8804e39aa7f37eb00da5a762149dc682d7829451287e11b938e94594"
  "58721f8b6ca67f0fcbe1cd739b384fec3126a35f7d79951aedaa2bc3863a3162"
)
set(index 0)
foreach(path IN LISTS ledger_files)
  require_regular_nonempty("${path}")
  list(GET ledger_sha256 "${index}" expected_sha)
  file(SHA256 "${path}" actual_sha)
  if(NOT actual_sha STREQUAL expected_sha)
    message(FATAL_ERROR "Cardano Ledger provider checksum mismatch: ${path}")
  endif()
  math(EXPR index "${index} + 1")
endforeach()
file(GLOB_RECURSE actual_ledger_files LIST_DIRECTORIES FALSE "${ledger_root}/*")
list(LENGTH actual_ledger_files ledger_count)
if(NOT ledger_count EQUAL 9)
  message(FATAL_ERROR "Cardano Ledger provider inventory must contain exactly nine files")
endif()
