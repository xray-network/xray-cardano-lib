# C++ test authority and coverage

The C++ tests are independent semantic tests. They are authored from the frozen C++ 0001
instruction, accepted language-neutral implementation results, and captured provider evidence.
They do not copy, translate, import, execute, or inspect TypeScript test or implementation source.

## Organization

The workspace-level test tree mirrors the production domains without placing Catch2 or test-only
configuration below `src/` or `include/`:

```text
src/core/       tests/core/
src/crypto/     tests/crypto/
src/chain/      tests/chain/
src/plutus/     tests/plutus/
src/cip/        tests/cip/
```

Each domain owns its test executable, source list, linked dependencies, compile definitions, and
Catch2 discovery in a local `CMakeLists.txt`. The root `tests/CMakeLists.txt` only loads shared
test dependencies, aggregates those domains, and defines cross-domain format, API, provider, and
integrity gates. Runtime consumer checks remain under `tests/runtime/`, and hardening campaigns
remain under `tests/hardening/` because neither belongs to one production component.

## Running tests

All commands in this section run from `libs/cpp/`. Set `VCPKG_ROOT` to the pinned checkout
described in the [C++ workspace README](../README.md#bootstrap-vcpkg):

```sh
cd libs/cpp
export VCPKG_ROOT=/path/to/vcpkg
```

The authoritative completion command configures, builds, and tests the CI preset:

```sh
cmake --workflow --preset ci
```

The CI preset enables tests and provider-integrity checks, treats owned-source warnings as
errors, and uses `build/ci/`. A successful run includes the Catch2 suites, format and API/CDDL
inventory gates, provider integrity, the installed-package consumer, and aggregate linkage.

After the CI preset has been configured once, rebuild and rerun tests without repeating the
configure step:

```sh
cmake --build --preset ci
ctest --preset ci
```

List discovered tests or available labels:

```sh
ctest --preset ci --show-only
ctest --preset ci --print-labels
```

Run one domain by label, or select tests by regular-expression name:

```sh
ctest --preset ci -L chain
ctest --preset ci -L 'provider|integrity'
ctest --preset ci -R 'CIP-8|cardano_installed_consumer'
```

CTest preset runs already enable output on failure. Add `--verbose` when command lines and full
test output are needed:

```sh
ctest --preset ci -L plutus --verbose
```

When `VCPKG_ROOT`, the compiler, or target architecture changes, discard the cached configuration
through CMake and rebuild it:

```sh
cmake --preset ci --fresh
cmake --build --preset ci
ctest --preset ci
```

Additional complete workflows are available for memory/undefined-behavior checks and the larger
deterministic hardening campaign:

```sh
cmake --workflow --preset sanitizers
cmake --workflow --preset hardening
```

Do not invoke test executables from assumed build paths. The domain-local CMake files may place
them in domain-specific build directories; CTest is the stable execution interface.

## Coverage

Current suites cover the public owners delivered by C++ 0001:

| Suite | Coverage |
| --- | --- |
| `core` | Owned bytes, arbitrary integers, collections, Bech32, network constants, lossless and canonical CBOR, malformed inputs, semantic tags, floats, spans, and resource limits |
| `crypto` | Hash vectors, fixed wrappers, Ed25519, BIP32 frozen vectors, move-only secret lifecycle, injected/system randomness, EMIP-3, fail-closed secp256k1 inputs, and BLS12-381 group/pairing operations |
| `chain` | Shelley/Byron addresses, genesis and witnesses; recursive chain-owned Plutus Data; era model validation; explicit-network block dispatch and common views; ledger operations; staged transaction/witness builders, exact CIP-2 selection, change, collateral, reference scripts, governance, drafts, and signing |
| `cip` | CIP-8 COSE Sign/Sign1, CIP-25 V1/V2 metadata, and CIP-36 registration/deregistration wire, JSON, signing, malformed-input, and compatibility contracts |
| `plutus` | Recursive typed Data and schemas plus UPLC 1.0/1.1 Flat/text codecs, builtins 0–100, A–E costs, CEK evaluation, parameter application, phase-two V1/V2/V3 contexts, raw protocol-5–11 valuation, and the applicable official conformance corpus |
| `provider` | Direct checksum, size, regular-file, and exact-inventory validation for captured evidence, plus all 85 accepted historical blocks with preserved round trips/common views and the one declared malformed rejection |
| `hardening` | Seeded malformed-CBOR canonical-convergence cases, exhaustive Ed25519 signature-bit mutations, typed-Data depth/value campaigns, and the deterministic 50,000-case preset |
| `runtime/static` | Checked 975-binding/60-row API inventory, focused/aggregate linkage, clean installed `find_package` consumer, package-content/archive/path scan, Apple clang-format, warnings-as-errors, ASan/UBSan, and conditional clang-tidy |

The frozen binding-by-binding C012 inventory and all public type-only JSON names have checked C++
owners. The official Byron-through-Conway CDDL inventory has an explicit disposition for all 714
definitions/228 unique rule names, with focused validation tests for the implemented boundaries.
C++ 0001 is recorded as `ACCEPTED`. Subsequent implementation work uses its own C++-local
sequence and lifecycle row.
