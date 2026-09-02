# TypeScript implementation 0001 instruction

Implementation-Version: v1
Implementation-ID: typescript/0001
Created: 20260723T122735Z
Evidence-Mode: DIRECT
Depends-On: NONE
Provider-Evidence: ../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`0001-cardano-multiplatform-lib`](https://github.com/xray-network/xray-cardano-lib/blob/main/.xray/updates/providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/SNAPSHOT.md) | `PROVIDER` | Yes | Frozen CDDL, historical vectors, provenance, and licensing |

## Objective

Make this snapshot the physical owner of the frozen CML specifications and test vectors required
by Cardano Lib's compatibility tests, while keeping all TypeScript as ordinary package-owned
source.

## Comparison sources

- Previous Cardano Multiplatform Lib snapshot: `NONE`.
- The pre-snapshot CML-derived Cardano Lib implementation and repository-level vector corpus.

## Confirmed upstream delta

This initial frozen snapshot captures 38 selected CDDL files, three CML license files, six genesis
JSON files, 86 golden block vectors, and the upstream golden-vector provenance document. It also
adds deterministic inventory, checksum, license, path-mapping, and byte-preservation metadata.

The snapshot changes evidence and test-vector ownership rather than vector bytes, expected
outcomes, wire behavior, or public API. Its provenance pins Dolos commit
`ea7960a1c2e56c523fec7c4bab75f390ee443514` and Pallas revision
`a7b5a86e3922ea46723e7959118293232db7bf3a`.

## Changes to implement

| Language | Change ID | Requirement | Compatibility | Local owner | Validation | Reason |
| --- | --- | --- | --- | --- | --- | --- |
| `typescript` | `C001` | `REQUIRED` | `internal` | `libs/typescript/packages/chain/test/` | `npm --prefix libs/typescript run check` | Initial comparison and vector baseline |

## Captured artifacts

| Artifact scope | Upstream source | Integrity | License | Intended consumers |
| --- | --- | --- | --- | --- |
| `../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/specs/` | Selected CML `specs/` files at the source commit | Exact 38-file provider inventory | MIT | CDDL comparison evidence |
| `../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/legal/` | CML repository license files | Three byte-exact files | Recorded per file | Legal review |
| `../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/genesis/` | CML Byron and Shelley genesis data | Six byte-exact files with manifest checksums | MIT | Chain genesis tests |
| `../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/blocks/mainnet/` | CML-carried Dolos blocks | 34 byte-exact files with manifest checksums | Apache-2.0 | Multi-era and runtime tests |
| `../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/blocks/pallas/` | CML-carried Pallas blocks | 52 byte-exact files with manifest checksums | Apache-2.0 | Multi-era and runtime tests |
| `../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/PROVENANCE.md` | CML golden-vector provenance | Byte-exact with supplemental checksum | Upstream document | Provenance review |
| Test-vector control metadata | Snapshot-authored manifest, provenance, README, Git attributes, and license text | Deterministic five-file control set | Recorded in provenance | Integrity tests and consumers |

All 92 vectors remain byte-exact. The manifest records their checksums, storage forms, expected
results, and stable repository paths.

## Cardano Lib change map

| Upstream change | Evidence | Local owner | Required implementation | Compatibility | Tests |
| --- | --- | --- | --- | --- | --- |
| Frozen CDDL comparison corpus | `../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/specs/` | Snapshot evidence; package TypeScript remains package-owned | Preserve exact scope and nested specification paths without generating source | No runtime or public API change | Provider inventory review |
| Genesis and historical block corpus | `../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/` | `libs/typescript/packages/chain/test/`, `libs/typescript/packages/runtime/test/` | Consume the single snapshot-owned corpus directly | Preserve 85 byte-exact round trips and one recorded rejection | Genesis, multi-era, vector-integrity, and indexer tests |
| Deterministic provenance and checksums | Manifest and provenance control files | `libs/typescript/packages/chain/test/upstream-vectors.test.mjs` | Validate inventory, paths, bytes, expected outcomes, revisions, and licenses | Artifact integrity and legal traceability | Upstream-vector integrity test |
| Snapshot ownership | Complete snapshot | Root documentation and all vector consumers | Remove the separate root vector corpus and use stable snapshot paths | No duplicated evidence or fixture ownership | Root build, tests, and package smoke checks |

## Implementation steps

1. Preserve the exact 38-file CDDL selection and three CML legal files.
2. Preserve the exact genesis, block, and provenance selections defined by the provider.
3. Store vectors deterministically below `../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/`.
4. Generate and validate the snapshot-local manifest and provenance mapping.
5. Point package tests directly at the snapshot artifacts without copying data into packages.
6. Remove the separate repository-level vector corpus and update active documentation.
7. Run the complete repository validation.

## Validation

- Run the package-owned vector-integrity, genesis, multi-era, and downstream tests.
- Run `npm --prefix libs/typescript run build`.
- Run `npm --prefix libs/typescript run lint`.
- Run the packed-consumer smoke tests through the TypeScript workspace command.
- Run `npm --prefix libs/typescript run check`.

## Compatibility and human review

No vector byte, expected outcome, wire behavior, or public API change is intended. The known
malformed Conway vector must remain a rejection case. Human review must confirm the selected
test-vector inventory, third-party provenance, and Apache-2.0 license mapping.

## Completion criteria

- The snapshot owns the exact CDDL evidence and all 92 required vectors.
- Package tests consume the vector artifacts directly from this snapshot.
- The manifest covers the complete corpus and every checksum passes.
- No separate repository-level vector corpus remains.
- `npm --prefix libs/typescript run check` passes.

## Observed but excluded

- Reference-only CDDL outside the selected 38-file baseline
- Rust source, Rust test code, unrelated test data, build configuration, and Git metadata
- Pallas `u5c*` vectors

## Out of scope

- Changing CDDL semantics or runtime behavior
- Advancing the frozen source commit
- Adding unrelated CML test data
- UPLC language or evaluator functionality
- Message-signing functionality

## Blockers

None.
