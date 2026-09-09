# Typescript implementation 0026 result

Result-Version: v1
Implementation-ID: typescript/0026
Instruction: ./0026-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C01` | `IMPLEMENTED` | Registration and deregistration views defensively retain the original complete valid metadata bytes, preserving label order, widths and nested/outer container hints. | Byte-exact round-trips, input/output buffer isolation and malformed reserved/duplicate-label checks. |
| `C02` | `IMPLEMENTED` | Metadata conversions preserve extra labels with the chain-owned type; add_to_metadata merges only CIP36 fields into the caller. Fresh outputs keep their established ordering. | Both view types: conversion, caller mutation and unrelated-label collision cases. |
| `C03` | `IMPLEMENTED` | Unrelated metadata does not enter signing preimages or JSON views; canonical encoding remains explicitly selected through Metadata. | Signing-hash equality, existing synthetic signature tests and canonical equivalence checks. |

## Outcome

The existing full-metadata preservation fix is reconciled and verified for both CIP36 view types. No source rewrite was needed.

This result reconciles work already present before the 2026-09-09 request to refresh SPECTRE and
mark the completed batch REVIEW. It records actual work and checks; it does not imply human acceptance.

## Inputs consumed

The original instruction's LOCAL inputs remain the basis of the work. The current human request
on 2026-09-09 explicitly authorizes refreshing SPECTRE, fitting the existing work to its new rules,
and moving the seven completed implementations to REVIEW. No new upstream revision is consumed.
Paths in code formatting below are repository-root-relative unless explicitly described otherwise.

- `AGENTS.md`
- `docs/src/adr/typescript/0001-lossless-cbor-and-encoding-metadata.md`
- `docs/src/adr/typescript/0003-upstream-evidence-and-package-ownership.md`
- `libs/typescript/README.md`
- `libs/typescript/package.json`
- `libs/typescript/packages/cip/README.md`
- `libs/typescript/packages/cip/package.json`
- `libs/typescript/packages/cip/src/cip36/metadata.ts`
- `libs/typescript/packages/cip/test/cip36.test.mjs`
- `libs/typescript/packages/core/src/cbor/types.ts`

## Project changes

- `libs/typescript/packages/cip/src/cip36/metadata.ts`
- `libs/typescript/packages/cip/test/cip36.test.mjs`

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C01` | Unchanged valid metadata views retain the complete original CBOR representation defensively. | Extra labels and encoding choices survive byte round-trips. | Use an owned lossless representation and test copy isolation. |
| `C02` | Full conversions retain extra metadata; merges write only the view-owned CIP36 labels. | Caller-owned unrelated labels and fresh construction order are preserved. | Keep full conversion and merge contracts distinct. |
| `C03` | Signing uses only the registration/deregistration payload; canonical output remains explicit. | No signing preimage or JSON shape expansion. | Retain independent hash/signature and canonical tests. |

## Validation

Fresh reconciliation commands from the repository root:

```sh
npm --prefix libs/typescript run check
node --test libs/typescript/packages/cip/test/cip36.test.mjs
git diff --check
```

The workspace command rebuilds before testing: 211 tests passed, zero failures/skips, followed by
packed-package ESM, NodeNext and bundler smoke checks (566 intended files). The focused command
passed 11 tests with zero failures/skips. These checks ran on the existing combined source
changes, which were not edited during reconciliation; the full gate is shared validation evidence
for typescript/0025-0029, not five claimed separate executions. Result schema, Change IDs and
ledger links were validated for this record before its REVIEW transition.

## Deviations from instruction

Existing implementation was retained and validated. Complete metadata is validated with the
chain-owned Metadata validator before preservation; malformed unrelated values/labels can no
longer be silently ignored. Fresh deregistration conversion keeps its existing ordering. The
weight fix shares this source file but remains separately attributed to typescript/0025. Earlier
lossless tests failed before this fix; no pre-fix run was fabricated during reconciliation.

## Remaining human review

Review full-view preservation and merge behavior, including validation of unrelated metadata. New upstream evidence adoption remains separate.

See the [cardano-multiplatform-lib consumer guidance](../../providers/cardano-multiplatform-lib/PROVIDER.md#maintained-consumer-guidance).
These are separate work, not missing changes in this completed bounded implementation.

## Reproducibility

Recorded against base Git commit `25a28e31b7377cd678e0cc1c5ca8425b1d403425` plus the existing working-tree changes.
Reconciliation environment: Node.js v24.18.0 and npm 11.16.0. Commands above run from the repository
root. Provider audit metadata is advisory; future captures independently resolve and verify upstream.
At that earlier reconciliation, all historical snapshots, archived decisions and original
instructions retained their prior bytes. See the subsequent metadata conversion note below.

The earlier separately authorized setup refresh installed the then-current development source from
`../projects/spectre/protocol/v1.0.0/SPECTRE-PROTOCOL.md` (relative to the workspace root), still
Standard-Version 1.0.0, SHA-256
`90354b0a414e73b223ff01875edafb2c29eff7a5c7f4d3399536470b1a755066`.
Its router, 13 runtime modules, three templates and repository pointers are synchronized. The
accepted bootstrap and other archived records are preserved; this is not a new installation record.

Final implementation file hashes at reconciliation:

| Repository-relative file | SHA-256 |
| --- | --- |
| `libs/typescript/packages/cip/src/cip36/metadata.ts` | `6d7bf2ba750dff3f7c507e90eae6ec5dcdee389d8d8a84270ad1220c08172538` |
| `libs/typescript/packages/cip/test/cip36.test.mjs` | `b09977d56eb0d274469047e54165319652e26df26b53f9812cc3bb50af755ed6` |

## Subsequent provider metadata conversion — 2026-09-09

The human subsequently requested unversioned provider guides and self-contained numbered snapshot
specifications. All five existing descriptors now embed their original rules and exact inventories
with migration provenance. Artifact bytes, library source/tests and archived decisions are unchanged;
this implementation’s behavior, validation history and REVIEW state remain unchanged. The earlier
installation hash above is historical; the synchronized current SPECTRE 1.0.0 source SHA-256 is
`4d9bb25db8489a997b5ebc110ea8f36b0995f50db1c861318c9997ae2993cb5f`. No upstream capture or new compatibility claim follows.
