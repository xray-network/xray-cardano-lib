# Cardano Ledger provider snapshot

Provider-Snapshot-Version: v1
Snapshot: 0001-cardano-ledger
Provider: cardano-ledger
Created: 20260723T122735Z
Previous-Snapshot: ./SNAPSHOT.md
Provider-Version: v1
Source-Type: git
Source-Repository: https://github.com/IntersectMBO/cardano-ledger.git
Source-Commit: a624de4c8db7286a6c065da149679ea55f7d5629
Source-Ref: refs/heads/master
Source-Tag: NONE

## Evidence objective

Preserve official Byron-through-Conway Cardano Ledger CDDL as immutable protocol evidence.

## Comparison source

The frozen CML comparison evidence is
[`0001-cardano-multiplatform-lib`](../../cardano-multiplatform-lib/0001-cardano-multiplatform-lib/SNAPSHOT.md).

## Captured scope

The snapshot contains byte-exact Byron, Shelley, Allegra, Mary, Alonzo, Babbage, and Conway CDDL
plus the upstream Apache-2.0 license and notice. All nine files came from commit
`a624de4c8db7286a6c065da149679ea55f7d5629`.

## Integrity and licensing

The exact inventory is defined by [`PROVIDER.md`](../PROVIDER.md). The seven CDDL files and two
legal files are regular files preserved byte-for-byte under `artifacts/`.

## Semantic evidence

The artifacts define supported ledger wire structure, including era-specific choices, bounds,
indexes, tags, optionality, containers, and embedded CBOR. CDDL is not generated source and does
not by itself prescribe public API or language representation.

## Exclusions

Dijkstra, general Cardano Ledger source, generated Haskell, upstream tests, build configuration,
Git metadata, message signing, and UPLC language/evaluator behavior are excluded.

