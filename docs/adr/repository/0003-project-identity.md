# ADR 0003: XRAY Cardano Lib project identity

- Status: Accepted
- Date: 2026-07-31

## Context

The repository and its aggregate language packages were originally named Cardano Lib with the
repository slug `cardano-lib`. The project owner has renamed the project to XRAY Cardano Lib with
the repository slug `xray-cardano-lib`.

The repository also contains Cardano protocol namespaces and domain packages. Those identifiers
describe the protocol surface rather than the repository brand and should not change as part of
the project rename.

## Decision

- Use **XRAY Cardano Lib** for the current project, repository, documentation, and aggregate
  package display name.
- Use `xray-cardano-lib` as the current repository slug and in repository URLs.
- Use `@xray-network/xray-cardano-lib` for the TypeScript aggregate runtime and
  `@xray-network/xray-cardano-lib-*` for its independently published domain packages:
  `core`, `crypto`, `chain`, `cip`, and `plutus`.
- Rename the C++ vcpkg manifest package from `cardano-lib-cpp` to `xray-cardano-lib-cpp` and its
  installed CMake package from `CardanoLib` to `XRAYCardanoLib`.
- Keep Cardano protocol identifiers unchanged, including public TypeScript protocol symbols,
  `cardano::*` C++ targets, `cardano` headers and namespaces, and the `libcardano` archive name.
- Do not rewrite terminal implementation records, immutable provider snapshots, captured
  artifacts, or compatibility fixtures merely to replace the historical project name. They
  continue to describe the identity that existed when the evidence or decision was recorded.

## Consequences

Consumers of the TypeScript packages must update their dependencies and imports to the
`@xray-network/xray-cardano-lib` package family. C++ consumers must use
`find_package(XRAYCardanoLib CONFIG REQUIRED)` while continuing to link the same `cardano::*`
targets.

Current documentation, manifests, tests, and planned work use the new identity. Historical and
immutable records may still contain Cardano Lib or `cardano-lib`, and those occurrences do not
represent the current project name.
