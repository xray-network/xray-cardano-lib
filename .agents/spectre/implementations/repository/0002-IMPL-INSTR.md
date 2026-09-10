# Repository implementation 0002 instruction

Implementation-Version: v1
Implementation-ID: repository/0002
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
| `.agents/spectre/providers/cardano-multiplatform-lib/PROVIDER.md` | `LOCAL` | Yes | Provider information and tracking. |
| `.agents/spectre/providers/cardano-multiplatform-lib/0001/SNAPSHOT.md` | `LOCAL` | Yes | Original capture rules, inventory and migration provenance. |

## Objective

Keep CML live on develop, preserve its original capture specification, and retain the audited
CDDL/behavior selection as preparation for a separately requested capture. Exact new capture rules
belong in that capture’s SNAPSHOT.md; the provider guide contains source and consumer guidance.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C01` | Replace provider-version routing with an unversioned guide; embed the original capture rules and exact artifact inventory in the existing 0001 snapshot through the authorized development metadata migration. | Preserve all artifact bytes/paths, source identities, snapshot ID and Created. | `.agents/spectre/providers/cardano-multiplatform-lib/PROVIDER.md` and `.agents/spectre/providers/cardano-multiplatform-lib/0001/SNAPSHOT.md`; this instruction’s advisory selection | Verify baseline SHA-256 values, migration provenance and absence of obsolete version paths. |
| `C02` | Retain the advisory 30-file CDDL candidate below and its explicit delta from the historical 38-file selection. External placeholders remain references requiring resolved consumer mappings. | The existing snapshot retains all 38 original CDDL files. | `.agents/spectre/providers/cardano-multiplatform-lib/PROVIDER.md` and `.agents/spectre/providers/cardano-multiplatform-lib/0001/SNAPSHOT.md`; this instruction’s advisory selection | Verify candidate paths against the cached complete audit tree; no new captured evidence claim. |
| `C03` | Retain the exact 12 candidate Rust paths below with byte-exact artifacts/upstream/ mapping, source licensing and untrusted-evidence boundaries. | No Rust files are retrofitted into 0001 and no source is executed. | `.agents/spectre/providers/cardano-multiplatform-lib/PROVIDER.md` and `.agents/spectre/providers/cardano-multiplatform-lib/0001/SNAPSHOT.md`; this instruction’s advisory selection | Check each candidate is a regular blob in the cached audit tree. |
| `C04` | Require each new snapshot to specify deterministic SHA256SUMS for every other artifact, vector manifest/provenance/license checks, complete inventory and artifact-backed consumer recommendations. | The audited 144 total is advisory; new captures own independently verified counts. | `.agents/spectre/providers/cardano-multiplatform-lib/PROVIDER.md` and `.agents/spectre/providers/cardano-multiplatform-lib/0001/SNAPSHOT.md`; this instruction’s advisory selection | Review snapshot template requirements and retain original vector integrity checks. |

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
[cardano-multiplatform-lib consumer guidance](../../providers/cardano-multiplatform-lib/PROVIDER.md#maintained-consumer-guidance).

## Authorized development reconciliation

The human’s latest request supersedes the earlier provider-version preparation and blanket
snapshot-descriptor byte-preservation policy. This non-terminal instruction was reconciled on
2026-09-09; its earlier contents had SHA-256 `15127032817dd442a5df596cde28f9d2d7254296e91b45355cf555a7f03e2d0c`.
The original Change IDs are retained; artifact preservation and consumer boundaries still apply.

## Advisory CML capture selection

Use the original 0001 snapshot's explicit 38-file list as the comparison starting point. Remove its ten
`specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/` files:
`address.cddl`, `assets.cddl`, `auxdata.cddl`, `block.cddl`, `byron.cddl`, `certs.cddl`,
`crypto.cddl`, `mod.cddl`, `plutus.cddl`, and `transaction.cddl`.
Add these two files, preserving their source-relative destinations below `artifacts/`:

```text
specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/auxdata.cddl
specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/mod.cddl
```

The resulting scope is 1 CIP-25, 5 CIP-36, 10 Conway, 6 multi-era, and 8 multi-era-Byron CDDL files.
Resolve and publish the full expanded selection in the next requested SNAPSHOT.md, not only this delta. The old ten extern
files are removed, not automatically replaced with arbitrary generated interfaces. The captured
Conway and Byron rules are comparison evidence for the external placeholders; unresolved
consumer mappings must still block a future DIRECT plan.

Select these 12 additional behavior files, preserving source paths below `artifacts/upstream/`:

```text
cip36/rust/src/utils.rs
cip36/rust/src/generated/mod.rs
cip36/rust/src/generated/serialization.rs
cip36/rust/src/generated/cbor_encodings.rs
cip36/rust/tests/cbor_roundtrip.rs
cip36/rust/tests/invariants.rs
cip36/rust/tests/json.rs
cip36/rust/tests/unknown_keys.rs
core/rust/src/ordered_set.rs
chain/rust/src/builders/tx_builder.rs
chain/rust/src/builders/witness_builder.rs
chain/rust/src/builders/redeemer_builder.rs
```

The audit verified these paths at CML commit
`69b8664c84931cd6632e0d951c2795019597527f`. This identity is advisory preparation provenance,
not captured normative library evidence. Read-only tree lookup:
`https://api.github.com/repos/dcSpark/cardano-multiplatform-lib/git/trees/69b8664c84931cd6632e0d951c2795019597527f?recursive=1`.
