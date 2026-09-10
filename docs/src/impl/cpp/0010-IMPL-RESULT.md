# C++ implementation 0010 result

Result-Version: v1
Implementation-ID: cpp/0010
Instruction: ./0010-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C10-PKG1` | Implemented | Renamed the vcpkg manifest package to `xray-cardano-lib-cpp`. | vcpkg manifest configuration and the complete CMake workflow passed. |
| `C10-CMAKE1` | Implemented | Renamed the CMake project, config template, generated and installed config/version/targets files, install directory, consumer discovery, and component variables to `XRAYCardanoLib`. | Configure, install, clean installed-consumer configuration/build/run, and all CTest checks passed. |
| `C10-DOC1` | Implemented | Updated maintained C++ headings, descriptions, package discovery examples, and toolchain messages to XRAY Cardano Lib. | Maintained-name scan and complete C++ workflow passed. |
| `C10-ID1` | Implemented | Preserved every `cardano::*` target, Cardano header/namespace, and the `libcardano` archive identity. | Aggregate build-tree and installed-package consumers linked and ran successfully. |

## Outcome

The C++ distribution now identifies as XRAY Cardano Lib. vcpkg uses
`xray-cardano-lib-cpp`, installed CMake consumers discover `XRAYCardanoLib`, and all existing
Cardano protocol targets and binary/header identities remain unchanged.

## Inputs consumed

- `docs/adr/repository/0003-project-identity.md`
- The owned C++ manifest, CMake package metadata, README, example consumer, and install tests under
  `libs/cpp/`

No provider evidence or external implementation result was consumed.

## Project changes

- Renamed the vcpkg manifest package to `xray-cardano-lib-cpp`.
- Renamed the CMake project and config template to `XRAYCardanoLib`.
- Renamed installed config, targets, version, and documentation paths consistently.
- Updated the installed example and validation harness to use
  `find_package(XRAYCardanoLib CONFIG REQUIRED)`.
- Updated maintained C++ display-name descriptions while preserving Cardano protocol APIs.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C10-PKG1` | The C++ vcpkg manifest identity is `xray-cardano-lib-cpp`. | Manifest consumers using the old project package name must migrate. | Use `xray-cardano-lib-cpp` for current distribution metadata. |
| `C10-CMAKE1` | The installed CMake package identity is `XRAYCardanoLib`. | `find_package(CardanoLib ...)` no longer discovers the installed config. | Use `find_package(XRAYCardanoLib CONFIG REQUIRED)`; requested component names are unchanged. |
| `C10-DOC1` | The maintained C++ distribution display name is XRAY Cardano Lib. | No wire or runtime behavior change. | Use the new display name in current integrations and documentation. |
| `C10-ID1` | C++ protocol targets remain `cardano::core`, `cardano::crypto`, `cardano::chain`, `cardano::cip`, `cardano::plutus`, and `cardano::lib`. | Existing link-target, header, namespace, and archive use remains compatible after package discovery. | Change only the `find_package` name. |

## Validation

The required C++ workflow passed with the repository's pinned cached toolchain:

```sh
env PATH=/private/tmp/cardano-build-tools/bin:/usr/bin:/bin:/usr/sbin:/sbin \
  VCPKG_ROOT=/private/tmp/cardano-vcpkg \
  /private/tmp/cardano-build-tools/bin/cmake --workflow --preset ci
```

Configuration and vcpkg manifest resolution passed, the build completed, and all 141 CTest tests
passed. The tests included formatting, provider integrity, aggregate build-tree linking, package
installation, `XRAYCardanoLib` discovery, installed consumer compilation, and consumer execution.

## Deviations from instruction

The default shell did not expose `cmake` on `PATH`. Validation used the exact cached CMake, Ninja,
and pinned vcpkg locations recorded by the existing C++ build instead of installing new host
tools. The prescribed `ci` workflow and all validation stages were unchanged.

## Remaining human review

Confirm the intentional breaking CMake package rename and decide whether the result should move
from `REVIEW` to `ACCEPTED`. Publishing or migration aliases for external package registries remain
outside this implementation.

## Reproducibility

From `libs/cpp/`, provide CMake 3.28 or newer, Ninja 1.11 or newer, and the pinned vcpkg checkout,
then run `cmake --workflow --preset ci`.
