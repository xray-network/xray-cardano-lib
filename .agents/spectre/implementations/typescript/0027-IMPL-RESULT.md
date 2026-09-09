# Typescript implementation 0027 result

Result-Version: v1
Implementation-ID: typescript/0027
Instruction: ./0027-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C01` | `IMPLEMENTED` | Reject a complete canonical-equivalent certificate before mutation, while allowing distinct certificates sharing a credential. | Repeated object and differently encoded clone rejection; body, deposit/refund, witness and redeemer state checks. |
| `C02` | `IMPLEMENTED` | Preflight proposal batches for internal and prior duplicates before applying any entry. | Cross-call and late intra-batch duplicates leave aggregate state unchanged; unique additions and totals pass. |
| `C03` | `IMPLEMENTED` | Certificate/proposal redeemer positions count all accepted body entries across calls, including non-Plutus entries; copies retain counters and execution-unit overrides. | Mixed-entry index/copy/override tests and post-rejection positions; existing supported spending evaluation passes. |
| `C04` | `IMPLEMENTED` | Retain keyed witness/script/datum uniqueness, input-role guards and raw decoder preservation. | Repeat add/merge coverage and existing input-role, lossless and workspace checks pass. |

## Outcome

The existing builder construction guards and redeemer-position fixes are reconciled and verified. No blanket decoder set policy or ledger semantic expansion was introduced.

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
- `libs/typescript/packages/chain/README.md`
- `libs/typescript/packages/chain/package.json`
- `libs/typescript/packages/chain/src/builder/transaction.ts`
- `libs/typescript/packages/chain/src/era/shared/validation.ts`
- `libs/typescript/packages/chain/src/era/shared/models.ts`
- `libs/typescript/packages/chain/test/builders.test.mjs`

## Project changes

- `libs/typescript/packages/chain/src/builder/transaction.ts`
- `libs/typescript/packages/chain/test/builders.test.mjs`

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C01` | Reject duplicate complete certificates using canonical value equality before state mutation. | Distinct certificates may share credentials; raw decoders are unchanged. | Apply equality at the construction boundary. |
| `C02` | Validate all proposal batch entries before applying any. | Duplicates raise TypeError instead of silently deduplicating or charging twice. | Test late failure rollback and deposit totals. |
| `C03` | Certificate and proposal redeemers use positions in the complete accepted body sequence. | Non-Plutus entries count; failed additions do not advance positions. | Preserve counters and execution-unit identity through copies. |
| `C04` | Witness construction maintains the existing keyed merge policy. | No new replacement policy or universal wire rejection. | Retain uniqueness and raw-wire regressions. |

## Validation

Fresh reconciliation commands from the repository root:

```sh
npm --prefix libs/typescript run check
node --test libs/typescript/packages/chain/test/builders.test.mjs
git diff --check
```

The workspace command rebuilds before testing: 211 tests passed, zero failures/skips, followed by
packed-package ESM, NodeNext and bundler smoke checks (566 intended files). The focused command
passed 18 tests with zero failures/skips. These checks ran on the existing combined source
changes, which were not edited during reconciliation; the full gate is shared validation evidence
for typescript/0025-0029, not five claimed separate executions. Result schema, Change IDs and
ledger links were validated for this record before its REVIEW transition.

## Deviations from instruction

Existing source work was retained. Script-bearing certificate/proposal tests validate construction
and pointers; they do not establish complete Ledger phase-two parity. Existing supported spending
evaluation is part of the passing suite. The broader Ledger comparison remains in the follow-up
brief. Original negative construction tests failed before the earlier fix; reconciliation reran
only the current checks. Funding in a builder fixture covers its later valid additions.

## Remaining human review

Review canonical duplicate equality, proposal atomicity and full-body redeemer positions. Broader Ledger phase-two comparison remains separate.

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
| `libs/typescript/packages/chain/src/builder/transaction.ts` | `38cdd50e3be9c0381a7d2bd7a9ae57f80a9d56e62cc12b032f5beeeddf8b8a89` |
| `libs/typescript/packages/chain/test/builders.test.mjs` | `837839345a498086f8ee55629a444d07516049f15c5875e020f73b1daacd8b48` |

## Subsequent provider metadata conversion — 2026-09-09

The human subsequently requested unversioned provider guides and self-contained numbered snapshot
specifications. All five existing descriptors now embed their original rules and exact inventories
with migration provenance. Artifact bytes, library source/tests and archived decisions are unchanged;
this implementation’s behavior, validation history and REVIEW state remain unchanged. The earlier
installation hash above is historical; the synchronized current SPECTRE 1.0.0 source SHA-256 is
`4d9bb25db8489a997b5ebc110ea8f36b0995f50db1c861318c9997ae2993cb5f`. No upstream capture or new compatibility claim follows.
