# Cardano Multiplatform Lib provider snapshot

Provider-Snapshot-Version: v1
Snapshot: 0001-cardano-multiplatform-lib
Provider: cardano-multiplatform-lib
Created: 20260723T122735Z
Previous-Snapshot: ./SNAPSHOT.md
Provider-Version: v1
Source-Type: git
Source-Repository: https://github.com/dcSpark/cardano-multiplatform-lib.git
Source-Commit: 39681e0d435a71f7c47a2601507ab16e691abb9e
Source-Ref: NONE
Source-Tag: NONE

## Evidence objective

Preserve the frozen Cardano Multiplatform Lib CDDL comparison baseline and reusable historical
test vectors selected by the provider contract.

## Captured scope

- 38 selected CDDL files below `artifacts/specs/`.
- Three CML license files below `artifacts/legal/`.
- Six genesis JSON vectors and 86 historical block vectors below
  `artifacts/test-vectors/`.
- Byte-exact upstream provenance plus deterministic manifest, provenance, license, README, and
  Git-attributes control files.

The 92 vectors retain their original bytes and expected outcomes. The manifest records source
paths, repository paths, sizes, SHA-256 values, storage forms, era metadata, and expected results.
Dolos is pinned to `ea7960a1c2e56c523fec7c4bab75f390ee443514`; Pallas is pinned to
`a7b5a86e3922ea46723e7959118293232db7bf3a`.

## Integrity and licensing

The exact inventory and deterministic transformations are defined by
[`PROVIDER.md`](../PROVIDER.md). Test-vector integrity and license mappings are recorded in
`artifacts/test-vectors/manifest.json` and `artifacts/test-vectors/PROVENANCE.json`.

## Semantic evidence

The captured CDDL is a frozen comparison source. The vectors provide historical byte-preservation
and decoding evidence. They are not generated source and do not prescribe a library's public API
or language representation.

## Exclusions

Reference-only CDDL outside the selected baseline, Rust source, Rust tests, unrelated test data,
build configuration, Git metadata, Pallas `u5c*` vectors, message signing, and UPLC evaluation are
excluded.

