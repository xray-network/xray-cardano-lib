---
title: XRAY Cardano Lib
description: Architecture decisions for the XRAY Cardano Lib polyglot repository
---

# XRAY Cardano Lib documentation

This directory is the Rspress documentation package. Run `npm install` and `npm run dev` here for local development, or `npm run deploy` to build and deploy the `wiki-xray-cardano-lib-docs` Worker.

Architecture decisions are grouped by their scope. Active implementation records live below
`.agents/spectre/implementations/`, terminal history lives below `.agents/spectre/archive/`, workflow
templates live below `.agents/spectre/templates/`, and aggregate active lifecycle state lives in
`SPECTRE.md`. Shared provider evidence lives below `.agents/spectre/providers/`.

The pages below `docs/src/impl/` preserve the former XRAY Updates documentation snapshot. They are
historical mirrors, not active SPECTRE lifecycle records.

## Repository decisions

| ADR | Decision | Status |
|---|---|---|
| [0001](src/adr/repository/0001-xray-updates-standard.md) | XRAY Updates v1 installation | Accepted |

## TypeScript decisions

| ADR | Decision | Status |
|---|---|---|
| [0001](src/adr/typescript/0001-lossless-cbor-and-encoding-metadata.md) | Lossless CBOR and encoding metadata | Accepted |
| [0002](src/adr/typescript/0002-cryptography-dependency-policy.md) | Security, cryptography dependency, and randomness policy | Accepted |
| [0003](src/adr/typescript/0003-upstream-evidence-and-package-ownership.md) | Upstream evidence and package ownership | Accepted |
| [0004](src/adr/typescript/0004-cryptography-primitives.md) | Cryptography primitive selection | Accepted |
