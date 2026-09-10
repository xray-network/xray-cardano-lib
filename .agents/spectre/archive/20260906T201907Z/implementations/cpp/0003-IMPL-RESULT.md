# C++ implementation 0003 result

Result-Version: v1
Implementation-ID: cpp/0003
Instruction: ./0003-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C3-PLAT1` | Implemented | Replaced the OS gate with C++23/library capability probes and declared the required host/compiler matrix. | Preset JSON and local strict compiler checks pass; cross-host execution remains for CI/human review. |
| `C3-CMK1` | Implemented | Made warnings, sanitizers, archives, and format tooling compiler/capability based. | Touched sources pass format and strict Apple Clang compilation. |
| `C3-PKG1` | Implemented | Added component exports, requested-component validation, and transitive dependency discovery with six isolated consumers. | Consumer configurations are present; CMake execution was unavailable locally. |
| `C3-BEN1` | Implemented | Added opt-in deterministic block and UPLC benchmark workloads with fixture hashes and JSON metadata/reporting. | Benchmark translation unit passes strict compilation with its generated definitions. |
| `C3-DOC1` | Implemented | Documented host status, component closures, benchmark protocol, and the non-stable ABI promise. | Documentation and API inventory were reviewed with diff checks. |

## Outcome

The C++ workspace is capability-gated, component-installable, and equipped with deterministic evidence-backed benchmarks without shipping provider artifacts.

## Inputs consumed

- `cpp/0001` accepted result
- `0001-cardano-multiplatform-lib` snapshot and selected historical block fixture
- `0001-uplc` snapshot and selected Flat/evaluation fixture
- Existing CMake, preset, package-config, consumer, README, and API inventory files

## Project changes

- Generalized build configuration and component packaging.
- Added component consumers and deterministic benchmark target/preset.
- Updated C++ platform, package, and benchmark documentation.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C3-PLAT1` | Required hosts are accepted through feature probes, not OS strings. | Existing macOS ARM64 remains supported. | Use a documented C++23-capable matrix toolchain. |
| `C3-CMK1` | Toolchain flags are selected by compiler capability. | No public API change. | Do not add host-version shortcuts. |
| `C3-PKG1` | Installed components load only their dependency closure. | Aggregate `cardano::lib` remains available. | Request only needed components. |
| `C3-BEN1` | Benchmarks emit deterministic JSON with provenance and integrity metadata. | Opt-in and non-production. | Provide captured fixtures through the preset. |
| `C3-DOC1` | Portability and ABI support boundaries are explicit. | Retains the `0.1.x` ABI policy. | Consult the matrix before claiming support. |

## Validation

- Fresh Apple Clang 21 Release and 50,000-case hardening configurations each passed all 150 tests; ASan/UBSan passed all 149 registered tests.
- All six isolated installed-component consumers, unknown-component rejection, archive/content checks, provider integrity, format, API inventory, and aggregate smoke passed.
- The opt-in Release benchmark built and emitted valid schema-versioned JSON for a 10-iteration smoke run and a 50-iteration block/UPLC report against a captured mainnet block.
- Preset JSON parsing and `git diff --check` passed.

## Deviations from instruction

The named presets could not reuse the reconstructed vcpkg checkout without rebuilding missing host Autotools, so equivalent fresh configurations used the existing pinned dependency installation with manifest installation disabled. Ubuntu GCC/Clang, Windows MSVC, and cross-host benchmark execution remain pending.

## Remaining human review

Execute the remaining Ubuntu and Windows matrix rows and compare cross-host benchmark reports before acceptance.

## Reproducibility

Use the documented presets in `libs/cpp/CMakePresets.json` on each required host and run all installed-component consumers.
