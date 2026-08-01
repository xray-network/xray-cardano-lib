---
title: XRAY Cardano Lib
description: Architecture decisions for the XRAY Cardano Lib polyglot repository
---

# XRAY Cardano Lib documentation

Architecture decisions are grouped by their scope. Canonical implementation records live below
`.xray/updates/implementations/`, while their workflow templates live below
`.xray/updates/templates/` and aggregate lifecycle state lives in
`.xray/updates/XRAY-UPDATES-STATUS.md`. Shared provider evidence lives below
`.xray/updates/providers/`, and any implementation may consume it through declared inputs.

The Mintlify-readable aggregate status copy lives at `docs/impl/XRAY-UPDATES-STATUS.md`, and copies
of numbered implementation instructions and results live below `docs/impl/<target>/`. Canonical
records remain below `.xray/updates/`; providers, artifacts, and templates are not copied into the
documentation tree.

## Repository decisions

| ADR | Decision | Status |
|---|---|---|
| [0001](adr/repository/0001-xray-updates-standard.md) | XRAY Updates v1 installation | Accepted |

## TypeScript decisions

| ADR | Decision | Status |
|---|---|---|
| [0001](adr/typescript/0001-lossless-cbor-and-encoding-metadata.md) | Lossless CBOR and encoding metadata | Accepted |
| [0002](adr/typescript/0002-cryptography-dependency-policy.md) | Security, cryptography dependency, and randomness policy | Accepted |
| [0003](adr/typescript/0003-upstream-evidence-and-package-ownership.md) | Upstream evidence and package ownership | Accepted |
| [0004](adr/typescript/0004-cryptography-primitives.md) | Cryptography primitive selection | Accepted |
