# C++ implementation 0001 result

Result-Version: v1
Implementation-ID: cpp/0001
Instruction: ./0001-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C001` | `IMPLEMENTED` | Independent C++23 CMake workspace, component graph, pinned vcpkg manifest/overlay, presets, install/export metadata, and macOS host documentation under `libs/cpp/` | CI configure/build/install and clean installed-consumer gate |
| `C002` | `IMPLEMENTED` | `cardano::core` bytes, arbitrary integers, results/errors, collections, network values, Bech32, and injectable secure randomness | Core vectors, ownership tests, malformed cases, and hardening campaigns |
| `C003` | `IMPLEMENTED` | Lossless complete-input CBOR tree, source spans, duplicate maps, indefinite/chunked forms, tags, preservation, canonical encoding, and independent resource limits | Core CBOR tests, deterministic malformed corpus, format/static gates |
| `C004` | `IMPLEMENTED` | Owned crypto wrappers for fixed hashes, Ed25519/BIP32, hashes, secp256k1, BLS12-381, EMIP-3, Byron signing/ABOR/proxy certificates, randomness, and secret lifecycle | Published/frozen vectors, negative acceptance tests, mutation campaign, sanitizers |
| `C005` | `IMPLEMENTED` | Shelley/Byron addresses, credentials, pointer naturals, Base58/Bech32/CRC32, Byron attributes, witnesses, genesis parsing, and network constants | Address, Byron, genesis, checksum, witness, and captured-vector tests |
| `C006` | `IMPLEMENTED` | Distinct Byron-through-Conway wire models organized below public and private `chain/era/` subfolders, lossless/canonical CBOR, generic and specialized JSON, transactions, blocks, headers, witnesses, certificates, scripts, Data, metadata, values, updates, protocol parameters, governance, and official-era validation; `era_models.hpp` remains the compatibility umbrella | 714 CDDL definitions/228 unique-rule disposition gate, direct focused-header and umbrella identity compilation, focused boundary/malformed tests, 85 accepted historical block round trips and one expected rejection |
| `C007` | `IMPLEMENTED` | Explicit multi-era dispatch/common views, preserved hashes, fees, minimum ADA, deposits/refunds, ExUnits, script-data operations, metadata/Plutus JSON, genesis IDs, and witness helpers | Ledger, multi-era, JSON, hashing, accounting, and historical-corpus tests |
| `C008` | `IMPLEMENTED` | Staged transaction/witness builders, CIP-2 largest-first/RandomImprove, change, collateral, governance, reference scripts, metadata, deterministic redeemers, drafts, valuation, and checked/unchecked signing | Builder invariants, deterministic randomness, balance/fee/minimum-ADA/size/signing tests |
| `C009` | `IMPLEMENTED` | Focused CIP-8, CIP-25, and CIP-36 owners with preserved/canonical encoding and frozen historical behavior | CIP vectors, signing verification, JSON/metadata, malformed-input, and mutation tests |
| `C010` | `IMPLEMENTED` | Chain-owned ledger-wire `PlutusData` in `src/chain/ledger/plutus_data.cpp`; recursive typed `Data`, schemas, casts, CBOR hex, JSON-like conversion, large integers, and limits in `src/plutus/typed_data.cpp` | Data/schema variant, recursive, ambiguity, bound, nominal-ownership, and round-trip tests |
| `C011` | `IMPLEMENTED` | UPLC 1.0/1.1 terms, text/Flat/script codecs, builtins 0–100, semantics A–E, CEK budgets, cost mappings, parameter application, V1/V2/V3 contexts, and protocol 5–11 phase-two valuation | Official applicable conformance corpus, goldens, cost/budget/context, trace, depth, and valuation tests |
| `C012` | `IMPLEMENTED` | Focused headers, aggregate facade, examples, install metadata, ownership checks, and exhaustive frozen requirement/API crosswalk | Checked 60-row/975-binding inventory, component/aggregate identity, and clean installed consumer |
| `C013` | `IMPLEMENTED` | Apple Clang warning/error policy, deterministic hardening, ASan/UBSan, provider integrity, formatting, conditional static analysis, installed-content/path/archive scans, and macOS workflows | `ci`, `sanitizers`, and `hardening` workflows all pass |

## Outcome

Implemented the complete frozen C++ 0001 semantic scope as an independently owned, installable
C++23 library. The result covers core encoding and collections, cryptography, Shelley and Byron
addresses, official Byron-through-Conway ledger models, multi-era operations, transaction
construction, CIP-8/CIP-25/CIP-36, typed Data, UPLC/CEK, and raw phase-two valuation.

The public API does not depend on TypeScript, JavaScript, npm, browser APIs, or third-party JSON
types. Component consumers and the aggregate facade use the same nominal C++ owners. Preserved
CBOR and canonical CBOR remain explicit operations, and every public decoder is complete-input and
resource-bounded.

Era-model implementation is split into shared, Byron, Shelley, Alonzo, and Conway validation
subfolders plus shared dispatch and JSON modules. Focused public era headers and the retained
`cardano/chain/era_models.hpp` umbrella expose the same nominal `cardano::chain` types.

`libs/cpp/API_PARITY.md` is the reviewed crosswalk. Its automated gates cover all 60 frozen public
inventory rows and 975 bindings, plus all 714 production definitions and 228 unique rule names
from the seven declared official era artifacts.

## Inputs consumed

- `updates/implementations/typescript/0001-IMPL-RESULT.md` — evidence ownership, public capability baseline,
  captured genesis/block outcomes, and portability contract.
- `updates/implementations/typescript/0002-IMPL-RESULT.md` — CIP-8 COSE, signing, hashing, and `cms_`
  compatibility contract.
- `updates/implementations/typescript/0003-IMPL-RESULT.md` — UPLC, CEK, costs, contexts, phase-two valuation,
  and additional cryptography contract.
- `updates/implementations/typescript/0004-IMPL-RESULT.md` — official era validation and narrow historical
  compatibility contract.
- `updates/providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/SNAPSHOT.md`
  — six genesis fixtures and 86 historical block outcomes.
- `updates/providers/message-signing/0001-message-signing/SNAPSHOT.md` — CIP-8 wire,
  checksum, signing, and compatibility evidence.
- `updates/providers/uplc/0001-uplc/SNAPSHOT.md` — 139 integrity artifacts, 3,013
  embedded corpus entries, 1,003 applicable programs, cost models, contexts, and crypto vectors.
- `updates/providers/cardano-ledger/0001-cardano-ledger/SNAPSHOT.md` — seven official
  Byron-through-Conway CDDL artifacts and Ledger legal material.
- `docs/adr/repository/0001-repository-architecture.md` — independent-workspace, package
  ownership, evidence, lifecycle, and documentation rules.

No TypeScript source, emitted JavaScript, declarations, or TypeScript tests were consumed as
implementation input or invoked by the C++ build.

## Project changes

- Built the independent `libs/cpp/` CMake/vcpkg workspace, focused targets, aggregate target,
  install/export package, examples, presets, macOS host instructions, and release-content scans.
- Implemented owned core, CBOR, crypto, address, Byron, era, multi-era, ledger, builder, CIP,
  typed-Data, UPLC, context, and phase-two source.
- Named the two Data implementation layers explicitly: `chain/ledger/plutus_data.cpp` owns the
  ledger-wire type, while `plutus/typed_data.cpp` owns the higher-level typed/schema API.
- Replaced the 3,490-line `src/chain/era_models.cpp` with ordered, review-sized implementation
  modules below `src/chain/era/`; added focused public era entry points below
  `include/cardano/chain/era/` while preserving `era_models.hpp` as a thin compatibility umbrella.
- Added exact specialized JSON contracts for transaction inputs, addresses, credentials,
  rational/unit-interval values, ExUnits/prices, anchors, protocol versions, VRF certificates,
  Values, IP values, scripts, and bootstrap witnesses. Other era models retain the frozen bounded
  CBOR-shaped generic conversion and distinct owned DTOs.
- Added direct/nested validation for every supported official era rule. Exact era differences
  include protocol-major limits, transaction-body keys, output formats, witness alternatives,
  script versions, certificate choices, cost-model lengths, auxiliary formats, and Conway
  governance.
- Retained only the declared read-only compatibility paths: historical Byron SSC certificate and
  shares forms, wider Conway protocol-parameter fields at frozen historical block ingestion,
  address suffix compatibility, the internal V1/V2 phase-two trailing-object behavior, and the
  specified Data/cost-model version boundaries.
- Added focused tests matching the frozen TypeScript semantic contracts without executing or
  translating the TypeScript implementation.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C001`–`C003` | Consumers can build an independent C++23 package and use lossless/preserved or canonical bounded CBOR explicitly. | Semantic and wire compatibility; no C++ ABI promise. | Use focused CMake targets or `cardano::lib`; choose preserved versus canonical output deliberately. |
| `C004`–`C005` | Consumers receive owned cryptography, Cardano key/address, Byron, witness, and genesis behavior. | Frozen acceptance rules and byte outputs are preserved; secret cleanup remains best effort. | Perform independent cryptography/security review before production distribution. |
| `C006`–`C007` | Consumers receive official Byron-through-Conway models, validation, JSON contracts, multi-era views, hashes, fees, and ledger accounting. | Official CDDL is strict except at the explicitly declared historical ingestion boundaries. | Do not route new permissive behavior through compatibility paths; add a new implementation sequence for later eras. |
| `C008` | Consumers receive deterministic, checked transaction construction with injectable randomness and explicit unchecked finalization. | Frozen CIP-2, fee, change, collateral, redeemer, and signing behavior is retained. | Supply secure randomness for RandomImprove and prefer checked final assembly. |
| `C009`–`C011` | Consumers receive CIP-8/25/36, typed Data, UPLC/CEK, contexts, and raw phase-two valuation. | Proposal/version-specific wire, JSON, signing, budget, and historical behaviors are retained. | Verify CIP-36 signatures explicitly and keep CPU/memory versus steps/memory ordering explicit. |
| `C012`–`C013` | Consumers receive stable nominal ownership, installable CMake metadata, and macOS validation/hardening gates. | JavaScript package identity and browser/npm mechanics are language-specific and intentionally absent. | Validate later platforms in separate records; retain the inventory, provider, sanitizer, and installed-consumer gates. |

## Validation

Final validation on macOS 26.5.2 ARM64 used Apple Clang 21.0.0, CMake 4.1.0, Ninja from the
isolated build-tools environment, and vcpkg baseline
`cd61e1e26a038e82d6550a3ebbe0fbbfe7da78e3`.

- `cmake --workflow --preset ci`: passed 141/141 tests with warnings as errors, formatting,
  provider integrity, 975-binding API inventory, 714-definition/228-rule CDDL inventory, package
  installation/content scan, and a clean `find_package` consumer.
- The era-module refactor reconstructed the prior implementation byte-for-byte within its ordered
  private translation unit; direct focused-header compilation and the monolith-prevention
  inventory check passed.
- `cmake --workflow --preset sanitizers`: passed 140/140 tests under AddressSanitizer and
  UndefinedBehaviorSanitizer.
- `cmake --workflow --preset hardening`: passed 141/141 tests, including the deterministic
  seed-`0xc0b012f0` 50,000-case malformed-input campaign.
- Captured CML evidence: all six genesis fixtures passed; 85 historical blocks decoded and
  preserved byte-for-byte; the single declared rejection remained rejected.
- UPLC evidence: all 139 artifact checksums, 3,013 embedded entry sizes/hashes, and 1,003
  applicable conformance programs/goldens/budgets passed.
- Installed/public scans found no provider artifacts, TypeScript/JavaScript, source from other
  languages, test-only dependencies, secrets, build paths, or unplanned binaries.
- `git diff --check` passed.

Pinned runtime dependencies were `boost-multiprecision 1.91.0`, `botan 3.12.0`,
`libsodium 1.0.22`, `secp256k1 0.7.1`, `nlohmann-json 3.12.0#2`, and `blst 0.3.17`.
`Catch2 3.15.1` remained test-only. All third-party implementation types remain private.

`clang-tidy` was not installed in the frozen host environment, so the instruction's conditional
clang-tidy pass did not run. Warning-as-error compilation, Apple clang-format, API/CDDL static
inventory checks, sanitizers, hardening, and installed-content scans all ran.

## Deviations from instruction

No required semantic feature was deferred.

The implementation uses idiomatic snake-case C++ methods, RAII, owned byte containers,
`std::expected`, CMake targets, and explicit randomness in place of the instruction's identified
JavaScript/browser/npm mechanics. The exhaustive adaptation table in
`libs/cpp/tests/api_inventory.cmake` records every non-identical frozen binding name.

Only macOS ARM64 is documented and validated for 0001, as required by the amended instruction.
Linux, Windows, other architectures, and stable C/C++ ABI commitments remain outside this result.

## Remaining human review

- Confirm the complete requirement/API/CDDL crosswalk and the narrow placement of every historical
  compatibility exception.
- Perform independent cryptography review of derivation, signing, randomness, secret handling,
  EMIP-3, secp256k1, BLS subgroup/encoding rules, and domain separation.
- Review consensus-sensitive CBOR canonicalization, script-data hashes, fee/minimum-ADA arithmetic,
  builder balance, UPLC Flat/CEK/cost accounting, phase-two contexts, and CPU/memory conversion.
- Review MIT/Apache-2.0 and dependency license/notice obligations before distribution.
- Measure performance and memory on representative historical blocks and UPLC workloads.

These are human acceptance and production-readiness reviews; they do not leave an implementation
requirement deferred.

## Reproducibility

On macOS ARM64, install the Xcode command-line tools and the Homebrew packages listed in
`libs/cpp/README.md`. Bootstrap vcpkg at the exact configured baseline, export `VCPKG_ROOT`, then
run from `libs/cpp/`:

```sh
cmake --workflow --preset ci
cmake --workflow --preset sanitizers
cmake --workflow --preset hardening
```

The presets resolve only `libs/cpp/vcpkg.json`, `libs/cpp/vcpkg-configuration.json`, and the
declared local overlays. They do not build or invoke the TypeScript workspace.
