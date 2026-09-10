# Repository implementation 0002 result

Result-Version: v1
Implementation-ID: repository/0002
Instruction: ./0002-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C01` | `IMPLEMENTED` | Unversioned guide; original rules embedded in 0001 with migration provenance; obsolete version routing removed. | Exact artifact inventory, original source/Created fields and baseline hashes checked. |
| `C02` | `IMPLEMENTED` | Candidate selection is retained in the instruction as advisory preparation; frozen historical selection stays in 0001. | Original selection and inventory verified; candidate requirements reviewed. |
| `C03` | `IMPLEMENTED` | Bounded source/transport and integrity requirements are retained in the instruction and original snapshot specification. | No artifact bytes changed; original mappings and format requirements verified. |
| `C04` | `IMPLEMENTED` | Live authority links, summary requirements and maintained-consumer guidance; each new snapshot owns its exact selection and counts. | Provider/template and consumer mapping review; no new capture or runtime adoption. |

## Outcome

CML uses an unversioned informational PROVIDER.md and a self-contained frozen 0001 specification.
The explicitly requested development conversion replaces the previous provider-version scheme.
No new upstream snapshot was captured. The implementation remains REVIEW, awaiting human decision.

## Inputs consumed

Current human authorization on 2026-09-09 supersedes the earlier version-directory model. Local
inputs are the reconciled instruction, repository guidance, provider template, existing guide,
original snapshot and its original applicable capture rules. No new upstream revision was fetched.
Repository-root-relative paths:

- `AGENTS.md`
- `.agents/spectre/templates/TEMPLATE_PROVIDER.md`
- `.agents/spectre/providers/cardano-multiplatform-lib/PROVIDER.md`
- `.agents/spectre/providers/cardano-multiplatform-lib/0001/SNAPSHOT.md`

Original rule/descriptor hashes are recorded in the snapshot’s Migration provenance. The prior
active result before this conversion had SHA-256 `b029450bba3c2daf0c9610477ac8b9e2550905ea477cdbd060068b6b55298551`.

## Project changes

- `.agents/spectre/providers/cardano-multiplatform-lib/PROVIDER.md`: unversioned information and tracking/summary guidance.
- `.agents/spectre/providers/cardano-multiplatform-lib/0001/SNAPSHOT.md`: embedded original rules, complete artifact inventory and migration record.
- This active instruction/result and the follow-up brief: reconciled authority and advisory selection.
- Obsolete provider version directories removed after accounting for original rules and references.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C01` | Read each numbered snapshot as its own frozen capture specification. | Artifact bytes and original source identities are preserved. | Verify its local inventory/rules without mutable provider or historical-version lookup. |
| `C02` | Define each future selected scope explicitly and verify complete source membership. | Advisory candidate selection does not rewrite older evidence. | Resolve a bounded new specification during a separately requested capture. |
| `C03` | Preserve deterministic mappings, transport bytes and integrity checks. | Captured source is untrusted evidence, never executable tooling. | Verify evidence before consuming it. |
| `C04` | Summarize relevant upstream behavior and map maintained modules, APIs and tests. | Counts belong to each snapshot; language-neutral semantics and opt-in C++ remain. | Request bounded consumer plans only after captured comparison resolves behavior. |

## Validation

- Original artifact membership, byte lengths and SHA-256 verified against the migration baseline.
- Frozen source identities, original Created and snapshot ID preserved; previous self-link corrected
  to NONE because this is the initial snapshot. Migration is metadata only and explicitly recorded.
- All original applicable selection, transformation, integrity and consumption rules accounted for.
- Rechecked the advisory candidate against the cached complete CML audit tree: 30 unique CDDL and
  12 unique Rust paths are regular blobs. The existing snapshot retains its original 38 CDDL files.
- Active links and Change IDs checked; all seven implementation rows remain REVIEW.
- Shared migration gate passed: five self-contained snapshots, 329 artifact inventories, all 3,013
  UPLC corpus entries and 1,003 program/result/budget associations, 70 archived record hashes,
  seven REVIEW instruction/result pairs and every live reference verified by local Python checks.
- All 13 runtime modules, three templates and the router match deterministic canonical extraction.
- `npm --prefix docs run build` in the SPECTRE source repository: passed, including protocol checks
  and publication-mirror generation. Standard-Version remains 1.0.0.
- `npm --prefix libs/typescript run check`: passed, 211 tests with zero failures and package smoke
  checks for ESM, NodeNext and bundler consumers (566 files, 2,631,292 unpacked bytes).
- `git diff --check`: passed in both repositories.

## Deviations from instruction

The original instruction requested versioned provider contracts. The latest explicit human request
replaces that model with snapshot-local specifications. Exact historical snapshot descriptor bytes
therefore change under the bounded development conversion; every artifact byte is preserved.
Release-specific future counts are advisory preparation, not mutable provider requirements.
The earlier prepared provider revisions were never separate upstream captures and get no new
snapshot IDs. The active instruction is reconciled honestly; terminal records remain untouched.

## Remaining human review

Review the metadata conversion. The later 0002 capture is available; evidence-based consumer
adoption remains separate requested work, described in the
[cardano-multiplatform-lib consumer guidance](../../providers/cardano-multiplatform-lib/PROVIDER.md#maintained-consumer-guidance).

## Reproducibility

Conversion date: 2026-09-09. Base Git commit remains `25a28e31b7377cd678e0cc1c5ca8425b1d403425`
plus the pre-existing working-tree changes. Snapshot inventories contain exact preserved artifact
hashes; archived records and library sources retain their pre-migration bytes. This result records
local conversion checks, not a fresh upstream completeness audit or human acceptance.

Installed development source: SPECTRE 1.0.0, SHA-256 `4d9bb25db8489a997b5ebc110ea8f36b0995f50db1c861318c9997ae2993cb5f`.
The canonical source, 13 extracted runtime modules, templates and router are synchronized.

Final implementation file hashes after metadata conversion:

| Repository-relative file | SHA-256 |
| --- | --- |
| `.agents/spectre/providers/cardano-multiplatform-lib/PROVIDER.md` | `1ecbc4543ea36cf45d9ae8153b7d0f4d8bc686022dfd261c5d1d263d2128f02e` |
| `.agents/spectre/providers/cardano-multiplatform-lib/0001/SNAPSHOT.md` | `08cd6c6fd13e0ccab4b5726eadf51d21be0794d2778e4f7ef8315571b523cd80` |

### Later provider maintenance evidence — 2026-09-09

This note retains verification from the separately requested incremental-capture reconciliation;
it does not change this original LOCAL implementation's scope, inputs or REVIEW decision.

The [cardano-multiplatform-lib 0002 specification](../../providers/cardano-multiplatform-lib/0002/SNAPSHOT.md)
records 144 effective artifacts, 21 local files and 123 reused files, original source identities, prior descriptor
hashes and reconciliation provenance. Its [summary](../../providers/cardano-multiplatform-lib/0002/CAPTURE.md)
records the consumer findings. All inherited bytes resolve through the complete inventory.

All 92 vector bytes and expected outcomes remain intact; the new manifest resolves fixturePath
to the existing .agents 0001 files without changing the historical manifest.
