# C++ implementation 0003 instruction

Implementation-Version: v1
Implementation-ID: cpp/0003
Created: 20260731T075521Z
Evidence-Mode: HYBRID
Depends-On: ./0001-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/SNAPSHOT.md, ../../providers/uplc/0001-uplc/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`cpp/0001`](./0001-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted workspace, component graph, package, historical-block, UPLC, and completion baseline |
| [`0001-cardano-multiplatform-lib`](https://github.com/xray-network/cardano-lib/blob/main/updates/providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/SNAPSHOT.md) | `PROVIDER` | Yes | Immutable historical block and genesis benchmark fixtures |
| [`0001-uplc`](https://github.com/xray-network/cardano-lib/blob/main/updates/providers/uplc/0001-uplc/SNAPSHOT.md) | `PROVIDER` | Yes | Immutable Flat/text/CEK and phase-two benchmark fixtures |
| `libs/cpp/CMakeLists.txt`, `libs/cpp/CMakePresets.json`, and `libs/cpp/cmake/` | `LOCAL` | Yes | Current host restrictions, target exports, dependency discovery, and validation workflows |

## Objective

Replace host-identity checks with tested C++23 capability checks, make installed CMake components
load only their transitive dependency closure, and add reproducible benchmark reporting over the
already captured block and UPLC corpora. Portability here is a source/package commitment for the
specified matrix, not a stable ABI or performance guarantee.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C3-PLAT1` | Support the exact compiler/platform matrix below and reject missing language/library capabilities rather than unrelated OS version strings. | Retain the accepted macOS ARM64 configuration while adding source-compatible hosts. | Root CMake, presets, CI documentation | Configure/build/test jobs for every matrix row |
| `C3-CMK1` | Remove Apple-only command paths and flags; express warnings, sanitizers, archives, and runtime differences through compiler capabilities. | No public C++ semantic change. | `CMakeLists.txt`, `cmake/`, test scripts | Configure tests plus compile/link/install on each host |
| `C3-PKG1` | Install component-specific target/config files and discover only the requested component's transitive external dependencies. | Existing `find_package(CardanoLib CONFIG REQUIRED)` and aggregate `cardano::lib` continue to work. | Package config and install/export rules | Clean consumers for each component and aggregate |
| `C3-BEN1` | Add an opt-in deterministic benchmark executable and JSON report for block decode/preserve/canonical encode and UPLC decode/evaluate workloads. | Benchmarks are non-production, do not change test pass/fail semantics, and ship no provider artifact. | `benchmarks/`, benchmark CMake option/preset | Fixture-integrity, repeatability, schema, and smoke tests |
| `C3-DOC1` | Document supported versus merely buildable hosts, dependency closures, benchmark protocol, and non-ABI status. | Preserve the `0.1.x` no-stable-ABI promise. | `libs/cpp/README.md`, `API_PARITY.md` | Documentation/inventory and installed-content gates |

## Required portability matrix

| Host | Architecture | Compiler | Required validation |
| --- | --- | --- | --- |
| macOS 26 | ARM64 | Apple Clang 21 | `ci`, sanitizers, hardening, install consumers |
| Ubuntu 24.04 LTS | x86-64 | GCC 14 | `ci`, hardening, install consumers |
| Ubuntu 24.04 LTS | x86-64 | Clang 18 | `ci`, ASan/UBSan, install consumers |
| Windows Server 2025 | x86-64 | MSVC 19.4x | `ci`, install consumers |

All rows require CMake 3.28 or newer, a C++23 standard library with `std::expected`,
`std::span`, `std::byteswap`, and the formatting/ranges operations actually used, and Ninja 1.11
or newer. Exact patch versions are recorded in the result. A compiler version alone must not
bypass compile-feature probes.

## Implementation steps

1. Delete the `sw_vers` execution, exact macOS patch check, Apple-only compiler fatal check, and
   hard-coded `/usr/bin/ar` or Unix shell assumptions. Retain architecture/compiler diagnostics as
   informational status and fail only a declared matrix job or a required capability probe.
2. Centralize warning and sanitizer flags by compiler family: Apple Clang/Clang and GCC use their
   supported `-W`/sanitizer forms; MSVC uses `/W4`, `/WX` when requested, and its supported runtime
   hardening without pretending ASan/UBSan parity. Do not weaken warnings-as-errors for owned code.
3. Keep the target graph `core`; `crypto -> core`; `chain -> core, crypto`;
   `cip -> core, crypto, chain`; `plutus -> core, crypto, chain`; `lib -> all`. Install one target
   file per component and have `CardanoLibConfig.cmake` load the requested component plus its
   closure. With no components, load `lib`. A `core` consumer must not call `find_dependency` for
   Botan, sodium, secp256k1, blst, or JSON; any component whose closure includes `crypto` discovers
   precisely the exported static-link dependencies.
4. Add clean configure/build/link/run consumers for `core`, `crypto`, `chain`, `cip`, `plutus`,
   and `lib`. Each consumer asserts that focused headers work, unrelated targets are absent, and
   requesting an unknown component fails with a useful message.
5. Add `CARDANO_BUILD_BENCHMARKS=OFF` by default and a `benchmarks` preset. The executable reads
   fixtures only from explicitly supplied captured-snapshot paths in the build tree; provider
   bytes are never generated source, copied into install output, or embedded in the library.
6. Freeze two workload manifests: every accepted historical block used by the 0001 provider tests
   for decode plus preserved and canonical encode, and a documented deterministic UPLC subset
   spanning Flat decode, text parse, CEK builtins, cost models, and phase-two contexts. Validate
   every provider checksum before measurement and record exclusions with a reason.
7. Use one warm-up and at least ten measured iterations per workload in a single process. Emit
   schema-versioned JSON containing git revision, dirty flag, host, architecture, compiler,
   standard library, build type, dependency versions, workload identity/hash, iteration count,
   wall-clock nanoseconds, peak resident bytes when the host exposes it, and owned allocation
   counts where instrumented. Report median and p95; never use elapsed values as a correctness gate
   in this sequence.
8. Keep benchmark output untracked and outside install/package contents. Add a deterministic smoke
   mode that fixes iteration count and checks workload/result hashes and JSON shape without
   asserting timing.

## Validation

- Run the required matrix commands in clean build directories and record exact host, compiler,
  standard-library, CMake, Ninja, and dependency versions.
- Run all six installed-component consumers. Inspect the CMake trace for the `core` consumer to
  prove no crypto dependency is discovered, and prove each other component loads exactly its
  transitive closure.
- Run `cmake --workflow --preset ci` on all rows, the declared sanitizer jobs on macOS/Clang, and
  hardening on macOS/GCC. Platform-specific skipped capabilities must be explicit and must not skip
  semantic tests.
- Run benchmark smoke validation against both snapshots, then one full report on macOS ARM64 and
  Linux x86-64. Check fixture hashes before and after and scan install archives for provider paths
  and bytes.
- Run `git diff --check`.

## Compatibility and human review

Public types, wire behavior, package name, component names, and aggregate semantics are unchanged.
Static library files and C++ ABI may differ by compiler and remain unsupported across toolchains.
Provider fixtures are immutable evidence and benchmark input only.

Human review must verify the package dependency closure, compiler-conditional flags, absence of
silent test reduction, benchmark corpus identity, metric caveats, and provider-artifact exclusion.

## Completion criteria

- Every required matrix row configures, builds, tests, installs, and runs its component consumers.
- `core` installation is usable without crypto packages; every other component resolves exactly
  its declared closure.
- Benchmark smoke and full reports are reproducible by identity and never enter release contents.
- Existing semantic, provider-integrity, inventory, sanitizer, and hardening gates remain green.
- The result records exact versions, matrix dispositions, and benchmark manifests.

## Out of scope

- Stable C or C++ ABI, shared-library ABI policy, package-manager recipes beyond installed CMake
  config, Android/iOS/WebAssembly, big-endian hosts, or architectures outside the matrix.
- Performance optimizations, throughput thresholds, competitive claims, continuous trend storage,
  or execution of captured upstream programs as repository tooling.
- Updating, fetching, regenerating, or shipping provider evidence.

## Blockers

None.
