# C++ implementation 0010 instruction

Implementation-Version: v1
Implementation-ID: cpp/0010
Created: 20260731T120245Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| `docs/adr/repository/0003-project-identity.md` | `LOCAL` | Yes | Authoritative XRAY Cardano Lib display, manifest, and CMake package identities |
| `libs/cpp/` | `LOCAL` | Yes | Owned C++ manifest, CMake package metadata, documentation, examples, and install validation |

## Objective

Rename the C++ distribution identity from Cardano Lib to XRAY Cardano Lib while preserving the
Cardano protocol namespace, headers, targets, and archive identity.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C10-PKG1` | Rename the vcpkg manifest package from `cardano-lib-cpp` to `xray-cardano-lib-cpp`. | Intentional manifest identity change. | `libs/cpp/vcpkg.json` | Manifest inspection and CMake workflow |
| `C10-CMAKE1` | Rename the installed CMake package and config artifacts from `CardanoLib` to `XRAYCardanoLib`. | Consumers must update `find_package`; existing `cardano::*` link targets remain unchanged. | `libs/cpp/CMakeLists.txt`, config template, examples, and install tests | Configure, install, and clean consumer test |
| `C10-DOC1` | Use XRAY Cardano Lib in maintained C++ descriptions and documentation. | Documentation-only display-name change. | `libs/cpp/` | Repository name scan and CMake workflow |
| `C10-ID1` | Preserve Cardano protocol namespaces, `cardano::*` CMake targets, headers, and `libcardano` archive name. | Avoids an unrelated protocol API break. | C++ source and build metadata | Existing public API and installed-consumer tests |

## Implementation steps

1. Update the vcpkg manifest and CMake project/package identity.
2. Rename the CMake config template and generated/installed config and target filenames.
3. Update the example consumer and install validation to discover `XRAYCardanoLib`.
4. Update maintained C++ branding without renaming Cardano protocol APIs.
5. Run the complete C++ validation workflow.

## Validation

Run from `libs/cpp/` with the pinned vcpkg environment available:

```sh
cmake --workflow --preset ci
```

Also scan maintained C++ source, manifests, documentation, examples, and tests for the retired
distribution identities.

## Compatibility and human review

This is an intentional breaking CMake package rename requested by the project owner. Reviewers
must confirm that the preserved `cardano::*` target namespace correctly remains protocol-owned.

## Completion criteria

- The vcpkg manifest identity is `xray-cardano-lib-cpp`.
- Installed consumers use `find_package(XRAYCardanoLib CONFIG REQUIRED)`.
- Config, target, version, and install-directory names consistently use `XRAYCardanoLib`.
- The complete C++ workflow passes.

## Out of scope

- Renaming Cardano C++ namespaces, headers, targets, or archive files.
- Rewriting terminal implementation records, provider snapshots, captured artifacts, or fixed
  compatibility vectors.
- Renaming or publishing an external vcpkg registry port.

## Blockers

None.
