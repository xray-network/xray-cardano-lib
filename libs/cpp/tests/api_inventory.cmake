if(NOT DEFINED CARDANO_SOURCE_DIR OR NOT DEFINED CARDANO_REPOSITORY_ROOT)
  message(FATAL_ERROR "CARDANO_SOURCE_DIR and CARDANO_REPOSITORY_ROOT are required")
endif()

file(
  READ
  "${CARDANO_REPOSITORY_ROOT}/.xray/updates/implementations/cpp/0001-IMPL-INSTR.md"
  CARDANO_INSTRUCTION
)
file(
  GLOB_RECURSE
  CARDANO_PUBLIC_HEADERS
  LIST_DIRECTORIES FALSE
  "${CARDANO_SOURCE_DIR}/include/cardano/*.hpp"
)

set(
  CARDANO_ERA_HEADERS
  shared
  byron
  shelley
  allegra
  mary
  alonzo
  babbage
  conway
)
foreach(CARDANO_ERA_HEADER IN LISTS CARDANO_ERA_HEADERS)
  if(NOT EXISTS
     "${CARDANO_SOURCE_DIR}/include/cardano/chain/era/${CARDANO_ERA_HEADER}.hpp")
    message(FATAL_ERROR "Missing focused era header: ${CARDANO_ERA_HEADER}.hpp")
  endif()
endforeach()
if(EXISTS "${CARDANO_SOURCE_DIR}/src/chain/era_models.cpp")
  message(FATAL_ERROR "The monolithic era_models.cpp implementation must not return")
endif()
foreach(CARDANO_ERA_FRAGMENT IN ITEMS shared shelley alonzo conway byron)
  if(NOT EXISTS
     "${CARDANO_SOURCE_DIR}/src/chain/era/${CARDANO_ERA_FRAGMENT}/validation.ipp")
    message(FATAL_ERROR
            "Missing era implementation module: ${CARDANO_ERA_FRAGMENT}/validation.ipp")
  endif()
endforeach()
foreach(CARDANO_SHARED_FRAGMENT IN ITEMS dispatch json)
  if(NOT EXISTS
     "${CARDANO_SOURCE_DIR}/src/chain/era/shared/${CARDANO_SHARED_FRAGMENT}.ipp")
    message(FATAL_ERROR
            "Missing shared era module: shared/${CARDANO_SHARED_FRAGMENT}.ipp")
  endif()
endforeach()

set(CARDANO_PUBLIC_API "")
foreach(CARDANO_HEADER IN LISTS CARDANO_PUBLIC_HEADERS)
  file(READ "${CARDANO_HEADER}" CARDANO_HEADER_TEXT)
  string(APPEND CARDANO_PUBLIC_API "\n${CARDANO_HEADER_TEXT}")
endforeach()

string(REGEX MATCHALL "C0(02|03|04|05|06|07|08|09|10|11)\\|(runtime|type-only)\\|[^\n]+"
                      CARDANO_C006_ROWS
                      "${CARDANO_INSTRUCTION}")
list(LENGTH CARDANO_C006_ROWS CARDANO_C006_ROW_COUNT)
if(NOT CARDANO_C006_ROW_COUNT EQUAL 60)
  message(FATAL_ERROR "Expected 60 frozen API inventory rows, found ${CARDANO_C006_ROW_COUNT}")
endif()

set(CARDANO_MISSING_BINDINGS "")
set(CARDANO_BINDING_COUNT 0)
set(
  CARDANO_API_ADAPTATIONS
  "CBOR_INT_MAX=>BigInteger"
  "CBOR_INT_MIN=>BigInteger"
  "asInt64=>as_int64"
  "asUint64=>as_uint64"
  "assertBigIntInRange=>BigInteger"
  "assertByteLength=>assert_byte_length"
  "bytesEqual=>bytes_equal"
  "bytesToHex=>bytes_to_hex"
  "cloneValue=>CborValue"
  "copyBytes=>copy_bytes"
  "decodeProtocolMagic=>ProtocolMagic"
  "encodeProtocolMagic=>ProtocolMagic"
  "hexToBytes=>hex_to_bytes"
  "resultOrThrow=>CardanoResult"
  "unknownToError=>CardanoError"
  "decodeBech32=>decode_bech32"
  "encodeBech32=>encode_bech32"
  "decodeCbor=>decode_cbor"
  "decodeEmbeddedCbor=>decode_embedded_cbor"
  "encodeCbor=>encode_cbor"
  "ScriptHash{cip/cip25=PolicyId}=>ScriptHash"
  "legacyPublicKey=>legacy_public_key"
  "legacySign=>legacy_sign"
  "secureRandomBytes=>secure_random_bytes"
  "systemSecureRandomSource=>SecureRandomSource"
  "verifyEd25519=>verify_ed25519"
  "verifyEd25519Uplc=>verify_ed25519_strict"
  "verifySecp256k1Ecdsa=>verify_secp256k1_ecdsa"
  "verifySecp256k1EcdsaUplc=>verify_secp256k1_ecdsa_strict"
  "verifySecp256k1Schnorr=>verify_secp256k1_schnorr"
  "verifySecp256k1SchnorrUplc=>verify_secp256k1_schnorr_strict"
  "Address{cip/cip36=PaymentAddress}=>Address"
  "decodeBase58=>decode_base58"
  "encodeBase58=>encode_base58"
  "parseByronGenesis=>parse_byron_genesis"
  "parseShelleyGenesis=>parse_shelley_genesis"
  "builtinCost=>builtin_cost"
  "builtinTag=>builtin_tag"
  "dataConstant=>UplcConstant"
  "decodeFlatProgram=>decode_flat_program"
  "decodeProgramEnvelope=>decode_program_envelope"
  "decodeProgramEnvelopeCompatible=>decode_program_envelope_compatible"
  "defaultBuiltinCostModel=>default_builtin_cost_model"
  "defaultMachineCosts=>default_machine_costs"
  "encodeFlatProgram=>encode_flat_program"
  "encodePlutusData=>encode_plutus_data"
  "encodeProgramEnvelope=>encode_program_envelope"
  "evaluateProgram=>evaluate_program"
  "makeBuiltinCostModel=>make_builtin_cost_model"
  "parseUplcText=>parse_uplc_text"
  "validatePlutusDataNode=>validate_plutus_data_node"
)
foreach(CARDANO_ROW IN LISTS CARDANO_C006_ROWS)
  string(REPLACE "|" ";" CARDANO_COLUMNS "${CARDANO_ROW}")
  list(GET CARDANO_COLUMNS 1 CARDANO_KIND)
  list(GET CARDANO_COLUMNS 3 CARDANO_BINDING_LIST)
  string(REPLACE "," ";" CARDANO_BINDINGS "${CARDANO_BINDING_LIST}")
  foreach(CARDANO_BINDING IN LISTS CARDANO_BINDINGS)
    math(EXPR CARDANO_BINDING_COUNT "${CARDANO_BINDING_COUNT} + 1")
    string(FIND "${CARDANO_PUBLIC_API}" "${CARDANO_BINDING}" CARDANO_EXACT_POSITION)
    set(CARDANO_PRESENT FALSE)
    if(NOT CARDANO_EXACT_POSITION EQUAL -1)
      set(CARDANO_PRESENT TRUE)
    elseif(CARDANO_KIND STREQUAL "type-only" AND CARDANO_BINDING MATCHES "^(.+)JSON$")
      set(CARDANO_BASE "${CMAKE_MATCH_1}")
      string(
        FIND
        "${CARDANO_PUBLIC_API}"
        "CARDANO_MODEL_JSON(${CARDANO_BASE})"
        CARDANO_MACRO_POSITION
      )
      if(NOT CARDANO_MACRO_POSITION EQUAL -1)
        set(CARDANO_PRESENT TRUE)
      endif()
    endif()
    if(NOT CARDANO_PRESENT)
      foreach(CARDANO_ADAPTATION IN LISTS CARDANO_API_ADAPTATIONS)
        string(REPLACE "=>" ";" CARDANO_ADAPTATION_PARTS "${CARDANO_ADAPTATION}")
        list(GET CARDANO_ADAPTATION_PARTS 0 CARDANO_ORIGINAL)
        list(GET CARDANO_ADAPTATION_PARTS 1 CARDANO_CPP)
        if(CARDANO_BINDING STREQUAL CARDANO_ORIGINAL)
          string(FIND "${CARDANO_PUBLIC_API}" "${CARDANO_CPP}" CARDANO_CPP_POSITION)
          if(CARDANO_CPP_POSITION EQUAL -1)
            message(
              FATAL_ERROR
              "Adaptation ${CARDANO_ORIGINAL} names missing C++ owner ${CARDANO_CPP}"
            )
          endif()
          set(CARDANO_PRESENT TRUE)
          break()
        endif()
      endforeach()
    endif()
    if(NOT CARDANO_PRESENT)
      list(APPEND CARDANO_MISSING_BINDINGS "${CARDANO_KIND}:${CARDANO_BINDING}")
    endif()
  endforeach()
endforeach()

if(CARDANO_MISSING_BINDINGS)
  list(JOIN CARDANO_MISSING_BINDINGS ", " CARDANO_MISSING_TEXT)
  message(FATAL_ERROR "C006 bindings without a public C++ owner: ${CARDANO_MISSING_TEXT}")
endif()
if(NOT CARDANO_BINDING_COUNT EQUAL 975)
  message(FATAL_ERROR "Expected 975 API inventory bindings, found ${CARDANO_BINDING_COUNT}")
endif()

message(STATUS "Verified ${CARDANO_BINDING_COUNT} bindings across 60 frozen API rows")
