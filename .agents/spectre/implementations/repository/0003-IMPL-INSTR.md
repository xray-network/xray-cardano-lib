# Repository implementation 0003 instruction

Implementation-Version: v1
Implementation-ID: repository/0003
Created: 20260909T114243Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

All code-formatted repository paths are repository-root-relative. This is LOCAL governance work.
Snapshot files are local metadata migration inputs, not new upstream behavioral inputs.

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| Current human request on 2026-09-09: convert all providers to unversioned guides and numbered frozen specifications | `LOCAL` | Yes | Explicit development metadata migration and record reconciliation. |
| `AGENTS.md` | `LOCAL` | Yes | Repository routing and authority. |
| `README.md` | `LOCAL` | Yes | Maintained targets. |
| `.agents/spectre/templates/TEMPLATE_PROVIDER.md` | `LOCAL` | Yes | Unversioned guide and frozen snapshot rules. |
| `.agents/spectre/providers/uplc/PROVIDER.md` | `LOCAL` | Yes | Provider information and tracking. |
| `.agents/spectre/providers/uplc/0001/SNAPSHOT.md` | `LOCAL` | Yes | Original capture rules, inventory and migration provenance. |

## Objective

Keep UPLC and Ledger tracking live, preserve the original frozen UPLC specification, and retain
complete text/binary corpus preparation for a separate capture. Corpus counts and exact selection
are facts of each SNAPSHOT.md; changing the informational guide does not version the provider.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C01` | Replace provider-version routing with an unversioned guide; embed the original selection, corpus format, counts and integrity/consumer rules in 0001 through the authorized metadata migration. | All original artifact bytes/paths, source identities, snapshot ID and Created remain fixed. | `.agents/spectre/providers/uplc/PROVIDER.md` and `.agents/spectre/providers/uplc/0001/SNAPSHOT.md`; this instruction’s advisory selection | Verify baseline hashes, migration provenance, original 3013 corpus entries and no obsolete version paths. |
| `C02` | Retain complete regular-file conformance selection with independent source-tree enumeration and exact per-snapshot membership/counts, as described below. | Do not filter evidence by extension or runtime support. | `.agents/spectre/providers/uplc/PROVIDER.md` and `.agents/spectre/providers/uplc/0001/SNAPSHOT.md`; this instruction’s advisory selection | Review truncated-tree rejection and one-to-one membership rules. |
| `C03` | Retain deterministic schemaVersion 1 packaging, binary-safe base64, length/SHA-256 checks, path ordering and 16 MiB entry bounds as advisory preparation below. | No old corpus or transport bytes change; future specifications state their resolved format. | `.agents/spectre/providers/uplc/PROVIDER.md` and `.agents/spectre/providers/uplc/0001/SNAPSHOT.md`; this instruction’s advisory selection | Verify the original frozen inventory and the complete packaging requirements. |
| `C04` | Keep latest stable Plutus release policy, independent Ledger resolution, source-domain boundaries and artifact-backed summaries mapped to maintained consumers. | Protocols 5–11 remain the maintained boundary; C++ is opt-in. | `.agents/spectre/providers/uplc/PROVIDER.md` and `.agents/spectre/providers/uplc/0001/SNAPSHOT.md`; this instruction’s advisory selection | Review authority links, exclusions and consumer mappings; no capture or runtime adoption occurs. |

## Implementation steps

1. Hash all existing snapshots/artifacts, legacy rules, library files and archived records.
2. Resolve each snapshot’s original capture rules before removing legacy files. Embed them with
   the exact existing artifact inventory and an honest metadata migration record.
3. Rewrite the provider guide as information, source links, live tracking, summary expectations
   and maintained-consumer mappings. Retain candidate selection here as advisory preparation.
4. Remove obsolete provider-version files only once required rules and live references are
   accounted for; preserve terminal records. Reconcile this result and leave its row in REVIEW.

## Validation

Check all local links, frozen selection/count rules, inventory sizes/SHA-256 and original artifact
identity. Verify archived records and library source are byte-identical to the migration baseline.
Check exact runtime/template extraction from the refreshed SPECTRE source and run git diff --check.
Validate each Change ID in the paired result. No upstream capture is performed by this objective.

## Compatibility and human review

This is the explicitly requested development conversion, not a general permission to edit evidence.
Snapshot descriptors change only to become self-contained; artifact bytes and source identities do
not change. Future snapshots are frozen after publication and get independent specifications.
No scheduling, GitHub Actions, automatic library upgrades or C++ parity obligation.

## Completion criteria

The guide is unversioned, the existing snapshot is self-contained, obsolete version files have no
required live references, and the result accounts for C01–C04 with validation and deviations.

## Out of scope

New snapshot capture, library implementation, runtime generation, acceptance or archive mutation.

## Blockers

None for this LOCAL conversion. New evidence adoption remains separate; see the
[uplc consumer guidance](../../providers/uplc/PROVIDER.md#maintained-consumer-guidance).

## Authorized development reconciliation

The human’s latest request supersedes the earlier provider-version preparation and blanket
snapshot-descriptor byte-preservation policy. This non-terminal instruction was reconciled on
2026-09-09; its earlier contents had SHA-256 `3bbec737995e1b2cd1a3baee2cae706b9d5f6cca3ddd625a618ee550655692e4`.
The original Change IDs are retained; artifact preservation and consumer boundaries still apply.

## Advisory UPLC capture selection

For the next separately requested capture, start from 0001’s exact 113 Plutus and 24 Ledger source
allowlists and destination mapping. If a selected path moved, resolve the new bounded specification
explicitly; do not silently substitute files. Independently pin both upstream identities.

Select every regular file under `plutus-conformance/test-cases/` at the resolved Plutus commit,
including text, binary Flat, result/budget and support files. Independently enumerate the complete
source tree, reject truncated inventories, then require one-to-one source membership in corpus.json.
Reject missing, additional, unsafe, duplicate, symlink, gitlink, special and over-16-MiB entries.
Preserve the original schemaVersion 1 object/key order, RFC 4648 padded base64, lengths, SHA-256,
UTF-8 byte path ordering and trailing newline described in 0001. Binary bytes are never decoded
as UTF-8 to transport them. Preserve the corpus README and deterministic all-artifact SHA256SUMS.
Freeze the resolved rules and exact counts in the new snapshot before publication.

The prior audit observed 4,822 corpus files at Plutus 1.68.0.0; this is advisory, not a permanent
limit or a newly captured corpus. The unchanged outer source mapping previously yielded 140
artifacts. Recompute source, artifact and executable-case totals separately at capture time.
Record case discovery, result/budget associations and completeness. Unsupported cases remain
evidence with explicit dispositions; test failures are not grounds for silent exclusion.

Maintain Plutus V1–V3, UPLC 1.0/1.1, protocol majors 5–11 and builtin tags 0–100 until an explicit
consumer scope decision changes them. New release code alone does not authorize V4, protocol 12,
Dijkstra or future-gated builtins. The independently resolved Ledger source supplies only bounded
integration evidence. Artifact-backed comparisons map codecs, CEK, costs and ledger integration to
maintained consumers and focused tests before any DIRECT adoption plan.
