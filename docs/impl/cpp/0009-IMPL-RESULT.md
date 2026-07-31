# C++ implementation 0009 result

Result-Version: v1
Implementation-ID: cpp/0009
Instruction: ./0009-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C9-CMAKE` | `IMPLEMENTED` | Added local CMake ownership for `core`, `crypto`, `chain`, `plutus`, `cip`, `hardening`, and `runtime`; retained only shared setup, aggregation, and cross-domain gates in the root test file. | Fresh CI preset configure, complete build, 141/141 CTest cases |
| `C9-MIRROR` | `IMPLEMENTED` | Preserved the domain-mirrored test tree and renamed `tests/plutus/data_tests.cpp` to `tests/plutus/typed_data_tests.cpp` without changing its contents. | Test-source byte comparison, complete build, Catch2 discovery, 141/141 CTest cases |
| `C9-DOC` | `IMPLEMENTED` | Documented domain mirroring, local CMake ownership, complete and focused test commands, fresh configuration, sanitizer/hardening workflows, and corrected the stale C++ 0001 lifecycle wording. | Documentation review and repository link checks |

## Outcome

Each C++ test domain now owns its executable definition, source list, dependencies, compile
definitions, and Catch2 discovery next to its tests. The root test CMake file is a small
orchestrator for shared dependencies, domain aggregation, and repository-wide format, API, CDDL,
and provider-integrity gates.

Production sources, public headers, provider evidence, test cases, target names, CTest names, and
installed artifacts were not changed. Test executables now appear in domain-specific build
directories, which are internal build-tree details.

## Inputs consumed

- `libs/cpp/CMakeLists.txt`
- The pre-change `libs/cpp/tests/CMakeLists.txt`
- Existing files below `libs/cpp/src/` and `libs/cpp/tests/`
- `libs/cpp/README.md`
- `libs/cpp/tests/README.md`

No provider evidence or external implementation result was consumed.

## Project changes

- Added `CMakeLists.txt` below `tests/core/`, `tests/crypto/`, `tests/chain/`, `tests/plutus/`,
  `tests/cip/`, `tests/hardening/`, and `tests/runtime/`.
- Replaced central domain target definitions with seven `add_subdirectory` calls.
- Kept format, API inventory, CDDL inventory, and conditional provider-integrity gates at the test
  root.
- Moved installed-consumer ownership into `tests/runtime/CMakeLists.txt`; retained aggregate-smoke
  ownership in `tests/runtime/consumer/CMakeLists.txt`.
- Renamed `tests/plutus/data_tests.cpp` to `tests/plutus/typed_data_tests.cpp` byte-for-byte.
- Updated the C++ workspace and test READMEs with prerequisites, complete and quick workflows,
  discovery, label/name filtering, verbose output, fresh configuration, and sanitizer/hardening
  commands.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C9-CMAKE` | C++ test domains own their CMake targets locally; the test root owns only shared orchestration and cross-domain gates. | CMake target names, dependencies, definitions, labels, tests, and installed output remain compatible. | Put new domain-specific test sources and target settings in that domain's `CMakeLists.txt`; keep repository-wide gates at the root. |
| `C9-MIRROR` | Workspace tests remain outside production trees and mirror production domains; typed Plutus Data tests use the explicit `typed_data_tests.cpp` name. | No source, test, or public behavior change. | Follow the corresponding production-domain path when adding tests. |
| `C9-DOC` | C++ documentation defines the local test-ownership rule. | Documentation-only. | Preserve the documented boundary when extending C++ tests. |

## Validation

| Command or check | Result | Evidence |
| --- | --- | --- |
| `VCPKG_ROOT=/private/tmp/cardano-vcpkg /private/tmp/cardano-build-tools/lib/python3.9/site-packages/cmake/data/bin/cmake --workflow --preset ci` from `libs/cpp/` | PASS | Fresh configure and generation succeeded; 23 affected test objects/targets rebuilt; 141/141 tests passed |
| CTest label summary | PASS | `core`, `crypto`, `chain`, `plutus`, `cip`, `hardening`, `runtime`, `static`, `api`, `provider`, `integrity`, `format`, and `install` coverage remained present |
| Renamed test-source comparison against the pre-change `tests/plutus/data_tests.cpp` | PASS | No byte difference |
| `git diff --check` | PASS | No whitespace errors |

The first workflow invocation omitted the required `VCPKG_ROOT` environment variable and stopped
during configure before building. The final command above used the pinned checkout already
recorded by the workspace and completed successfully.

## Deviations from instruction

None.

## Remaining human review

- Confirm that the local files improve navigation enough to justify the additional small
  `CMakeLists.txt` files.
- Confirm the internal domain-specific executable locations are acceptable for developer tooling.
- Decide whether to move `cpp/0009` from `REVIEW` to `ACCEPTED` or `REJECTED`.

## Reproducibility

From `libs/cpp/`, export `VCPKG_ROOT` to the repository's pinned vcpkg checkout and run:

```sh
cmake --workflow --preset ci
```

The expected result is a successful configure/build/test workflow with 141 passing tests.
