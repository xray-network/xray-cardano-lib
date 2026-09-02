# TypeScript implementation 0001 result

Result-Version: v1
Implementation-ID: typescript/0001
Instruction: ./0001-IMPL-INSTR.md
Evidence-Mode: DIRECT

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C001` | `IMPLEMENTED` | `libs/typescript/packages/chain/test/` | `npm --prefix libs/typescript run check` |

## Outcome

The frozen CML snapshot now owns both its 38-file CDDL comparison corpus and the implementation
test vectors required by Cardano Lib's compatibility tests. Package TypeScript remains ordinary
reviewed source; package tests consume the snapshot artifacts directly.

## Artifacts consumed

- 38 CDDL files below `../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/specs/`.
- Three CML license files below `../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/legal/`.
- The complete `../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/` corpus.

## Test-vector artifacts consumed

- Six genesis JSON vectors.
- 86 historical block vectors.
- Byte-exact upstream provenance plus snapshot-local manifest, provenance, license, README, and Git
  attributes.

The package-owned integrity test validates exact inventory, paths, byte sizes, SHA-256 values,
storage forms, expected outcomes, source revisions, and license hashes.

## Project changes

- Moved the reusable vector corpus into this snapshot's `../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/`.
- Updated chain and runtime tests to consume the snapshot path directly.
- Updated imported-byte handling, active documentation, provider definitions, and snapshot
  workflow.
- Removed the separate repository-level vector ownership model.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C001` | Establish the frozen CML CDDL and 92-vector corpus as reusable implementation evidence with deterministic provenance, paths, checksums, and expected outcomes. | Internal evidence ownership; vector bytes and expected outcomes are unchanged. | A downstream implementation may consume this result alone or directly consume the linked provider evidence. Preserve the recorded vector outcomes when the implementation supports the corresponding eras. |

## Validation

| Check | Result | Notes |
| --- | --- | --- |
| vector-integrity test | PASS | 92 vectors, provenance, checksums, and license mappings validated |
| `npm --prefix libs/typescript run build` | PASS | All TypeScript project references built |
| `npm --prefix libs/typescript run lint` | PASS | Package, dependency, browser, binary, and credential policies passed |
| package tests | PASS | 137 workspace tests passed |
| packed consumer smoke | PASS | 526 intended files; ESM, NodeNext, and bundler consumers passed |
| `npm --prefix libs/typescript run check` | PASS | Complete TypeScript gate passed |

## Deviations from plan

The vector corpus lives under this snapshot's ownership with flattened provider paths, canonical
manifest paths, and consolidated metadata. Captured upstream bytes and expected outcomes are
unchanged.

## Remaining human review

- Review the captured test-vector inventory and third-party licensing.

## Reproducibility

Implementation used the checksummed corpus from the pinned CML snapshot without fetching or
substituting source material. All consumers use the captured artifact paths.
