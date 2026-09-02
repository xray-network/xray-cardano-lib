# C++ implementation 0009 instruction

Implementation-Version: v1
Implementation-ID: cpp/0009
Created: 20260731T104715Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| `libs/cpp/CMakeLists.txt` and `libs/cpp/tests/CMakeLists.txt` | `LOCAL` | Yes | Existing test configuration, target names, dependencies, labels, and repository-wide gates |
| `libs/cpp/src/` and `libs/cpp/tests/` | `LOCAL` | Yes | Current production/test ownership and directory correspondence |
| `libs/cpp/README.md` and `libs/cpp/tests/README.md` | `LOCAL` | Yes | C++ completion command and test-organization documentation |

## Objective

Make C++ test ownership easier to navigate by retaining the workspace-level `tests/` tree,
mirroring production domains within it, and moving each domain's test-target configuration into a
local `CMakeLists.txt`. Keep repository-wide static, provider, and integrity gates at the test
root, and do not place test code below production `src/` or public `include/` trees.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C9-CMAKE` | Give `core`, `crypto`, `chain`, `plutus`, `cip`, `hardening`, and `runtime` local CMake ownership; reduce the root test CMake file to dependency setup, subdirectory aggregation, and shared gates. | Preserve every target name, linked dependency, compile definition, Catch2 label, installed-consumer test, provider-integrity test, and aggregate smoke test. | `libs/cpp/tests/**/CMakeLists.txt` | Fresh CI configure, build, and all CTest cases |
| `C9-MIRROR` | Keep test paths aligned with their production domains and rename the generic Plutus `data_tests.cpp` to `typed_data_tests.cpp`, matching the `typed_data.cpp` owner. | File organization only; no test case, source, or public API behavior changes. | `libs/cpp/tests/` | Build graph and complete CTest discovery |
| `C9-DOC` | Document the mirrored test layout and local CMake ownership, and correct stale lifecycle wording. | Documentation-only. | `libs/cpp/README.md`, `libs/cpp/tests/README.md` | Documentation and link review |

## Implementation steps

1. Add one `CMakeLists.txt` to each test domain and move its existing target definition, links,
   compile definitions, and Catch discovery unchanged from the root test file.
2. Add `tests/runtime/CMakeLists.txt` to own the aggregate smoke subdirectory and installed-package
   consumer test.
3. Leave format, API inventory, CDDL inventory, and conditional provider-integrity tests in the
   root test CMake file because they span domains or repository evidence.
4. Rename `tests/plutus/data_tests.cpp` to `tests/plutus/typed_data_tests.cpp`; retain every source
   byte and update the local target source list.
5. Update C++ test documentation to describe the ownership rule and preserved test boundaries.

## Validation

- Configure from the C++ workspace using the `ci` preset.
- Build every test target through the `ci` workflow.
- Run the full discovered CTest suite with output on failure.
- Verify target names and test labels remain present.
- Run `git diff --check`.

## Compatibility and human review

This is a build-organization refactor. Public headers, production sources, installed artifacts,
test semantics, target names, and CTest names must remain unchanged. Human review should confirm
that local CMake ownership is clearer without introducing duplicated helper abstractions.

## Completion criteria

- Every test domain owns its target configuration locally.
- The root test CMake file contains only shared setup, aggregation, and cross-domain gates.
- The complete CI workflow passes with the same discovered test coverage.
- C++ test documentation explains the layout.

## Out of scope

- Moving tests into `src/` or `include/`
- Splitting the C++ library into separately installed component workspaces
- Changing test behavior, production behavior, public APIs, dependencies, or provider evidence
- Adding new semantic coverage

## Blockers

None.
