# Official Plutus UPLC provider snapshot

Provider-Snapshot-Version: v1
Snapshot: 0001-uplc
Provider: uplc
Created: 20260727T100703Z
Previous-Snapshot: ./SNAPSHOT.md
Provider-Version: v1
Source-Type: git
Source-Repository: https://github.com/IntersectMBO/plutus.git
Source-Commit: 91e8c2af9c7bec705b035c6cf8e679c35c4c2ad5
Source-Ref: refs/tags/1.66.0.0
Source-Tag: 1.66.0.0

## Evidence objective

Preserve official Plutus UPLC and Cardano Ledger phase-two evidence for protocol majors 5 through
11.

## Comparison source

Cardano Ledger commit `a624de4c8db7286a6c065da149679ea55f7d5629` from
`refs/heads/master` is captured separately within this snapshot for ledger integration behavior.

## Captured scope

- 113 byte-exact Plutus metadata, conformance, Plutus Core, and Plutus Ledger API files below
  `artifacts/plutus/`.
- 24 byte-exact Cardano Ledger phase-two and context files below
  `artifacts/cardano-ledger/`.
- A deterministic 3,013-entry conformance corpus in `artifacts/conformance/corpus.json`.
- Snapshot-local corpus documentation and `artifacts/SHA256SUMS`.

The final inventory is 140 regular files. The checksum inventory covers every artifact other than
itself.

## Integrity and licensing

The exact selections and deterministic corpus transformation are defined by
[`PROVIDER.md`](../PROVIDER.md). The captured Plutus and Cardano Ledger materials are Apache-2.0.

## Semantic evidence

The evidence covers UPLC 1.0.0 and 1.1.0, Flat encoding, CEK evaluation, builtin tags 0 through
100, cost models, protocol/language availability, Plutus V1/V2/V3 contexts, and raw transaction
valuation. It is evidence rather than runtime code.

## Exclusions

PlutusV4, protocol 12, Dijkstra, compiler and optimizer functionality, full phase-one validation,
third-party normative sources, generated bindings, native code, WASM, and upstream execution are
excluded.

