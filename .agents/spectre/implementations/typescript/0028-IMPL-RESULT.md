# Typescript implementation 0028 result

Result-Version: v1
Implementation-ID: typescript/0028
Instruction: ./0028-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C01` | `IMPLEMENTED` | Added original generic/nested/tuple/qualified definition fixtures through parsed redeemer, datum and parameter value validation. | Valid values resolve and invalid values still fail after successful reference resolution. |
| `C02` | `IMPLEMENTED` | Covered pointer escape order, literal ~1 identity, titles, duplicate JSON keys, unresolved/remote/unsafe references. | Five focused blueprint tests and the full workspace gate pass without blueprint runtime changes. |

## Outcome

Existing test-only blueprint coverage is reconciled. The current parser already satisfies the tested local requirements; production blueprint code is unchanged.

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
- `libs/typescript/packages/plutus/README.md`
- `libs/typescript/packages/plutus/package.json`
- `libs/typescript/packages/plutus/src/blueprint/index.ts`
- `libs/typescript/packages/plutus/test/blueprint.test.mjs`

## Project changes

- `libs/typescript/packages/plutus/test/blueprint.test.mjs`

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C01` | Definition names are opaque keys, including generic, qualified and tuple notation. | No type-expression parser or renaming. | Test lookup and value validation through all declared argument positions. |
| `C02` | Decode JSON Pointer escapes once; titles do not determine identity. | Keep local-only reference resolution and input safety. | Retain positive and malformed-reference regressions. |

## Validation

Fresh reconciliation commands from the repository root:

```sh
npm --prefix libs/typescript run check
node --test libs/typescript/packages/plutus/test/blueprint.test.mjs
git diff --check
```

The workspace command rebuilds before testing: 211 tests passed, zero failures/skips, followed by
packed-package ESM, NodeNext and bundler smoke checks (566 intended files). The focused command
passed 5 tests with zero failures/skips. These checks ran on the existing combined source
changes, which were not edited during reconciliation; the full gate is shared validation evidence
for typescript/0025-0029, not five claimed separate executions. Result schema, Change IDs and
ledger links were validated for this record before its REVIEW transition.

## Deviations from instruction

No source change was needed. This records earlier original local tests and fresh validation, without claiming adoption of a newer uncaptured CIP-57 revision.

## Remaining human review

Review regression completeness. A fresh CIPs snapshot and evidence confirmation remain separate.

See the [cardano-cips consumer guidance](../../providers/cardano-cips/PROVIDER.md#maintained-consumer-guidance).
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
| `libs/typescript/packages/plutus/test/blueprint.test.mjs` | `f3312f66173e09d73aab9e9e159326a8be4cf3669a5717d9cb3e86226e0792be` |

## Subsequent provider metadata conversion — 2026-09-09

The human subsequently requested unversioned provider guides and self-contained numbered snapshot
specifications. All five existing descriptors now embed their original rules and exact inventories
with migration provenance. Artifact bytes, library source/tests and archived decisions are unchanged;
this implementation’s behavior, validation history and REVIEW state remain unchanged. The earlier
installation hash above is historical; the synchronized current SPECTRE 1.0.0 source SHA-256 is
`4d9bb25db8489a997b5ebc110ea8f36b0995f50db1c861318c9997ae2993cb5f`. No upstream capture or new compatibility claim follows.
