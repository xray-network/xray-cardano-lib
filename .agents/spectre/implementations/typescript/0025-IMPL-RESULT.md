# Typescript implementation 0025 result

Result-Version: v1
Implementation-ID: typescript/0025
Instruction: ./0025-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C01` | `IMPLEMENTED` | Weighted delegation verification accepts any nonempty uint32 distribution with a positive weight and rejects all-zero distributions; legacy single-key behavior is unchanged. | Cases [0], [0,0], [1], [0,1], [1,0], uint32 maximum, empty/bounds and legacy coverage. |
| `C02` | `IMPLEMENTED` | Corrected inverted fixtures and covered verify and all metadata conversion/merge entry points with synthetic signatures. Invalid weights fail before caller mutation. | CIP36 focused suite and full workspace gate passed; signing/hash assertions retained. |

## Outcome

The existing delegation-weight fix is reconciled and verified. No runtime source was changed again during this operation.

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

## Project changes

- `libs/typescript/packages/cip/src/cip36/metadata.ts`
- `libs/typescript/packages/cip/test/cip36.test.mjs`

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C01` | A nonempty weighted delegation is valid exactly when some uint32 weight is positive. | All-zero acceptance and positive-weight rejection are intentionally corrected; legacy registration remains valid. | Update equivalent validators and boundary cases if maintained. |
| `C02` | Invalid delegation conversion must fail before mutating caller metadata. | verify remains structural validation, not signature verification. | Keep signing preimages unchanged and test mutation atomicity. |

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

Runtime work already existed from the earlier implementation turn. This operation records that
work and reruns validation under the updated lifecycle rules; it does not claim the source changes
were newly authored here. The original inverted tests were observed failing before the fix in the
earlier turn; this reconciliation records fresh passing checks rather than reconstructing that run.

## Remaining human review

Review the intentional change in accepted weight distributions. No new CML snapshot or broader upstream-parity claim is included.

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
