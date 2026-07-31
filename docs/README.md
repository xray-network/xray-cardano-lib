---
title: XRAY Cardano Lib
description: Architecture decisions for the XRAY Cardano Lib polyglot repository
---

# XRAY Cardano Lib documentation

Architecture decisions are grouped by their scope. Canonical implementation records live below
`updates/implementations/`, while their workflow templates remain at the root of `updates/`.
Shared provider evidence lives below `updates/providers/`, and any implementation may consume it
through declared inputs.

Mintlify-readable copies of numbered implementation instructions and results live below
`docs/impl/<language>/`. Canonical records remain below
`updates/implementations/<language>/`; statuses, providers, artifacts, and templates are not
copied into the documentation tree.

## Repository decisions

| ADR | Decision | Status |
|---|---|---|
| [0001](adr/repository/0001-repository-architecture.md) | Repository architecture and implementation records | Accepted; layout superseded |
| [0002](adr/repository/0002-shared-update-ledger.md) | Shared update ledger and provider evidence | Accepted |
| [0003](adr/repository/0003-project-identity.md) | XRAY Cardano Lib project identity | Accepted |

## TypeScript decisions

| ADR | Decision | Status |
|---|---|---|
| [0001](adr/typescript/0001-lossless-cbor-and-encoding-metadata.md) | Lossless CBOR and encoding metadata | Accepted |
| [0002](adr/typescript/0002-cryptography-dependency-policy.md) | Security, cryptography dependency, and randomness policy | Accepted |
| [0003](adr/typescript/0003-upstream-evidence-and-package-ownership.md) | Upstream evidence and package ownership | Accepted |
| [0004](adr/typescript/0004-cryptography-primitives.md) | Cryptography primitive selection | Accepted |
