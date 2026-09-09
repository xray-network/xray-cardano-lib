# Repository implementation 0003 result

Result-Version: v1
Implementation-ID: repository/0003
Instruction: ./0003-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C01` | `IMPLEMENTED` | Unversioned guide; original rules embedded in 0001 with migration provenance; obsolete version routing removed. | Exact artifact inventory, original source/Created fields and baseline hashes checked. |
| `C02` | `IMPLEMENTED` | Candidate selection is retained in the instruction as advisory preparation; frozen historical selection stays in 0001. | Original selection and inventory verified; candidate requirements reviewed. |
| `C03` | `IMPLEMENTED` | Bounded source/transport and integrity requirements are retained in the instruction and original snapshot specification. | No artifact bytes changed; original mappings and format requirements verified. |
| `C04` | `IMPLEMENTED` | Live authority links, summary requirements and maintained-consumer guidance; each new snapshot owns its exact selection and counts. | Provider/template and consumer mapping review; no new capture or runtime adoption. |

## Outcome

UPLC uses an unversioned informational PROVIDER.md and a self-contained frozen 0001 specification.
The explicitly requested development conversion replaces the previous provider-version scheme.
No new upstream snapshot was captured. The implementation remains REVIEW, awaiting human decision.

## Inputs consumed

Current human authorization on 2026-09-09 supersedes the earlier version-directory model. Local
inputs are the reconciled instruction, repository guidance, provider template, existing guide,
original snapshot and its original applicable capture rules. No new upstream revision was fetched.
Repository-root-relative paths:

- `AGENTS.md`
- `.agents/spectre/templates/TEMPLATE_PROVIDER.md`
- `.agents/spectre/providers/uplc/PROVIDER.md`
- `.agents/spectre/providers/uplc/0001/SNAPSHOT.md`

Original rule/descriptor hashes are recorded in the snapshot’s Migration provenance. The prior
active result before this conversion had SHA-256 `065742613649055551b0c0c77752d71d95cfc56e5281f6646509e312e6f46387`.

## Project changes

- `.agents/spectre/providers/uplc/PROVIDER.md`: unversioned information and tracking/summary guidance.
- `.agents/spectre/providers/uplc/0001/SNAPSHOT.md`: embedded original rules, complete artifact inventory and migration record.
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
[uplc consumer guidance](../../providers/uplc/PROVIDER.md#maintained-consumer-guidance).

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
| `.agents/spectre/providers/uplc/PROVIDER.md` | `6d2ab534b012fe94edd61d104ae8e05e01c428fd5512ac71ca4bca702a34c10a` |
| `.agents/spectre/providers/uplc/0001/SNAPSHOT.md` | `614c88a7ae99d209e8328caa873eefa7db6a9140cea8bd9131ec2f4f34a674e5` |

### Later provider maintenance evidence — 2026-09-09

This note retains verification from the separately requested incremental-capture reconciliation;
it does not change this original LOCAL implementation's scope, inputs or REVIEW decision.

The [uplc 0002 specification](../../providers/uplc/0002/SNAPSHOT.md)
records 140 effective artifacts, 46 local files and 94 reused files, original source identities, prior descriptor
hashes and reconciliation provenance. Its [summary](../../providers/uplc/0002/CAPTURE.md)
records the consumer findings. All inherited bytes resolve through the complete inventory.

The corpus.json bytes remain identical to the original 0002 capture. All 4,822 entry hashes,
1,905 cases, 1,004 shared budgets and eight auxiliary entries verified; no new-corpus runtime
adoption or upstream execution is claimed.

The same maintenance refreshed SPECTRE 1.0.0 to SHA-256
`43985d87bc69d11e5899dc3c7e8f9bb5a53962a0621a491aa61a2ba9667c5bc7`, with 13 exactly extracted
runtime modules and matching router/templates. Both captures verified their 275 standalone
Git source blobs. All five 0001 descriptors and 329 artifacts, 70 archived records and seven REVIEW
pairs retained their bytes during that migration. CML/UPLC storage removed 217 duplicate files
(6,047,572 bytes); later editorial link cleanup does not alter captured evidence or ledger states.

Post-migration `npm --prefix libs/typescript run check` passed 211 tests and ESM/NodeNext/bundler
package smoke checks. These were the maintained 0001 conformance tests. Negative probes rejected
forward references, undeclared copies, altered summaries and tampered inherited evidence.
These are recorded maintenance results, not tests rerun by the subsequent documentation cleanup.
