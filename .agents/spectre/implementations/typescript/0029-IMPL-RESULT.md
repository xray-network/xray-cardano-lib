# Typescript implementation 0029 result

Result-Version: v1
Implementation-ID: typescript/0029
Instruction: ./0029-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C01` | `IMPLEMENTED` | Extracted a test-only loader/runner with explicit inventory configuration; retained the original immutable snapshot path and 3013-entry/1003-case checks. | All 1003 pinned cases execute; explicit count mismatches fail in local transport tests. |
| `C02` | `IMPLEMENTED` | Kept Flat bytes binary; dispatch text/Flat through owned codecs and support historical/shared budget filenames with ambiguity rejection. | Original paired fixtures include non-UTF-8 Flat bytes and conflicting budget files. |
| `C03` | `IMPLEMENTED` | Separate codec-stage errors, evaluation failures and successful semantic results with exact budgets. | Wrong-stage, wrong-result/budget and malformed expectation cases fail as required. |
| `C04` | `IMPLEMENTED` | Verify unique safe sorted paths, canonical base64, bounded size and SHA-256 before lookups; require complete file/case accounting. | Missing, duplicate, unsafe, corrupted, orphaned and unknown inventory cases are rejected. |

## Outcome

The existing test harness supports both historical text and modern text/Flat transport layouts. The pinned official baseline remains unchanged; no newer corpus was adopted.

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
- `libs/typescript/packages/plutus/test/conformance.test.mjs`
- `libs/typescript/packages/plutus/test/flat.test.mjs`
- `libs/typescript/tools/run-tests.mjs`

## Project changes

- `libs/typescript/packages/plutus/test/conformance.test.mjs`
- `libs/typescript/packages/plutus/test/conformance-layout.test.mjs`
- `libs/typescript/packages/plutus/test/support/conformance.mjs`

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C01` | Select each immutable corpus explicitly with its own expected inventory. | No latest-snapshot discovery or weakened old counts. | Keep old/new corpus accounting separately reviewable. |
| `C02` | Binary fixtures remain bytes; result and budget pairing is deterministic. | No UTF-8 conversion or script-envelope guessing. | Reject ambiguous associations. |
| `C03` | Codec and evaluation failure expectations are distinct stages. | A runtime error cannot satisfy a decode-failure expectation. | Compare successful values and budgets explicitly. |
| `C04` | Validate transport integrity and account for every captured executable case. | No silent skips or orphaned evidence. | Require explicit inventory/dispositions before any future adoption. |

## Validation

Fresh reconciliation commands from the repository root:

```sh
npm --prefix libs/typescript run check
node --test libs/typescript/packages/plutus/test/conformance.test.mjs libs/typescript/packages/plutus/test/conformance-layout.test.mjs
git diff --check
```

The workspace command rebuilds before testing: 211 tests passed, zero failures/skips, followed by
packed-package ESM, NodeNext and bundler smoke checks (566 intended files). The focused command
passed 9 tests with zero failures/skips. These checks ran on the existing combined source
changes, which were not edited during reconciliation; the full gate is shared validation evidence
for typescript/0025-0029, not five claimed separate executions. Result schema, Change IDs and
ledger links were validated for this record before its REVIEW transition.

## Deviations from instruction

This is test infrastructure only. Existing snapshot counts stay in the pinned baseline invocation
as required by both the original plan and the refreshed protocol. Eight miniature tests are local
transport fixtures; the ninth test runs the existing 1003-case official baseline. No evaluator,
parser, cost model, protocol availability or package export was changed.

## Remaining human review

Review the strict transport/disposition rules. New Plutus release adoption, supported-case exclusions and semantic changes require separately captured evidence and plans.

See the [uplc consumer guidance](../../providers/uplc/PROVIDER.md#maintained-consumer-guidance).
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
| `libs/typescript/packages/plutus/test/conformance.test.mjs` | `ff7d7369aa3c1933a2f40eb160fde4923fa3f3e1e75bfc21e4d4ca7504733512` |
| `libs/typescript/packages/plutus/test/conformance-layout.test.mjs` | `ac4ffdaf9dc7d14ab82a1070f8efab11085166b6a60d99178e5eb71b089540d1` |
| `libs/typescript/packages/plutus/test/support/conformance.mjs` | `636b9df9711a9c897921b47c8acf54de2078a1b23df814669c12d647a103647f` |

## Subsequent provider metadata conversion — 2026-09-09

The human subsequently requested unversioned provider guides and self-contained numbered snapshot
specifications. All five existing descriptors now embed their original rules and exact inventories
with migration provenance. Artifact bytes, library source/tests and archived decisions are unchanged;
this implementation’s behavior, validation history and REVIEW state remain unchanged. The earlier
installation hash above is historical; the synchronized current SPECTRE 1.0.0 source SHA-256 is
`4d9bb25db8489a997b5ebc110ea8f36b0995f50db1c861318c9997ae2993cb5f`. No upstream capture or new compatibility claim follows.
