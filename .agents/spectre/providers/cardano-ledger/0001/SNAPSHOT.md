# Cardano Ledger provider snapshot

Provider-Snapshot-Version: v1
Snapshot: 0001
Provider: cardano-ledger
Created: 20260723T122735Z
Previous-Snapshot: NONE
Source-Type: git
Source-Repository: https://github.com/IntersectMBO/cardano-ledger.git
Source-Commit: a624de4c8db7286a6c065da149679ea55f7d5629
Source-Ref: refs/heads/master
Source-Tag: NONE
Source-URL: NONE
Source-SHA256: NONE

## Evidence objective

Preserve official Byron-through-Conway Cardano Ledger CDDL as immutable protocol evidence.

## Comparison source

The frozen CML comparison evidence is
[`0001`](../../cardano-multiplatform-lib/0001/SNAPSHOT.md).

## Frozen capture specification

The rules below preserve this capture’s original source selection, mappings, transformations,
completeness, licensing and consumer boundaries. They apply only to the immutable identities
recorded above. Discovery/ref and future-planning statements describe the original capture context;
they cannot retarget this snapshot. Repository-root paths remain repository-root-relative;
`artifacts/` paths resolve from this directory. No mutable provider guide supplies normative rules.

### Purpose

Capture official Cardano Ledger CDDL needed to implement and validate XRAY Cardano Lib's owned
TypeScript against the frozen CML comparison baseline. The files are evidence for an
implementation plan; they never generate or overwrite runtime source.

### Source

| Field | Value |
| --- | --- |
| Repository | `https://github.com/IntersectMBO/cardano-ledger.git` |
| Followed ref | `refs/heads/master` |
| Revision policy | Full commit reachable from the followed ref |
| Source mode | Immutable captured evidence; discovery policy below does not change the recorded commit |
| Submodules | Not part of the source |
| License | Apache-2.0 |

A tag is optional descriptive evidence. The resolved full commit is authoritative.

### Artifact selection

Copy these regular files byte-for-byte:

| Upstream path | Snapshot artifact |
| --- | --- |
| `eras/byron/ledger/impl/cddl-spec/byron.cddl` | `artifacts/cddl/eras/byron.cddl` |
| `eras/shelley/impl/cddl/data/shelley.cddl` | `artifacts/cddl/eras/shelley.cddl` |
| `eras/allegra/impl/cddl/data/allegra.cddl` | `artifacts/cddl/eras/allegra.cddl` |
| `eras/mary/impl/cddl/data/mary.cddl` | `artifacts/cddl/eras/mary.cddl` |
| `eras/alonzo/impl/cddl/data/alonzo.cddl` | `artifacts/cddl/eras/alonzo.cddl` |
| `eras/babbage/impl/cddl/data/babbage.cddl` | `artifacts/cddl/eras/babbage.cddl` |
| `eras/conway/impl/cddl/data/conway.cddl` | `artifacts/cddl/eras/conway.cddl` |
| `LICENSE` | `artifacts/legal/LICENSE` |
| `NOTICE` | `artifacts/legal/NOTICE` |

These exact filenames and destinations are the frozen selection for this snapshot. Missing,
renamed, symlinked or ambiguous files fail verification; later selections belong to later snapshots.

### Evidence-only paths

Read these at the candidate commit when present, but do not copy them as snapshot artifacts:

- `CHANGELOG.md`
- `eras/{shelley,allegra,mary,alonzo,babbage,conway,dijkstra}/impl/CHANGELOG.md`
- `eras/dijkstra/impl/cddl/data/dijkstra.cddl`
- relevant changes below `docs/`, `eras/*/formal-spec/`, `semantics/`,
  `eras/*/impl/cddl/lib/`, and `eras/*/impl/cddl/exe/`

Use the selected-file diff and this evidence to establish upstream facts. Summarize and cite paths
at the pinned commit; do not copy an upstream changelog into the snapshot document.

### Comparison and planning requirements

- Compare with the immediately previous same-provider snapshot, or NONE for this initial capture.
- Use the frozen CML snapshot as a separate comparison source, not as the previous Ledger snapshot.
- Classify selected CDDL changes independently by era, including changed choices, indexes, tags,
  bounds, optionality, container encoding, and unresolved references.
- Map every included Ledger change to its captured evidence, CML comparison rule when one exists,
  exact owned TypeScript modules and public entry points, runtime behavior, compatibility risk, and
  focused tests.
- Preserve historical-era behavior and byte-preserving CBOR semantics.
- CDDL is evidence rather than runtime conformance proof or a TypeScript source generator.
- Unexpected syntax, unsupported CDDL, unresolved references, and uncertain grammar mappings are
  preparation blockers.

Dijkstra is evidence-only because XRAY Cardano Lib has no Dijkstra owner or public entry point. Adding
it requires an explicit scope decision, a new snapshot specification, package ownership, multi-era
handling, fixtures, and tests. Until then, do not copy Dijkstra artifacts or include Dijkstra
implementation work.

### Excluded source material

- General Cardano Ledger source and generated Haskell
- Upstream tests, build output, package configuration, and Git metadata
- Message-signing functionality
- UPLC language parsing, evaluation, Flat encoding, or builtin semantics

Ledger CDDL fields for Plutus data, scripts, redeemers, execution units, and cost models remain
ordinary ledger wire grammar and are not excluded.

### Verified artifact inventory

This table freezes all 9 regular artifacts, including existing control files. Paths are
relative to this snapshot. Hashes and sizes were verified during the metadata conversion on
2026-09-09; this local verification does not claim a new source-tree capture or upstream audit.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `artifacts/cddl/eras/allegra.cddl` | 8855 | `e71cbf08f4fe62bb654f8371f4934eba750c17546b7d44c0c4f38b470eaabcd6` |
| `artifacts/cddl/eras/alonzo.cddl` | 17275 | `7460f60206160f3b459ee58befb1b912acf1812402113faa3963bf4bda0cf98d` |
| `artifacts/cddl/eras/babbage.cddl` | 19203 | `fcca168539a91a16c45b55c724b52e34bd85ff6499148a976ae5e01b66cff272` |
| `artifacts/cddl/eras/byron.cddl` | 5282 | `bc6f7fc1c6295046a2944ad784ce4b5ea544a185400add4aa7b122cd8e46a107` |
| `artifacts/cddl/eras/conway.cddl` | 24581 | `316ed8ee090ea172983083329e849f24f4360a236d26be0a6f2094c6078f1e1f` |
| `artifacts/cddl/eras/mary.cddl` | 8983 | `aa13e8687343658c5195b115d54fa1f4dfd7beeb70020f3e6c57f63f9be7aef1` |
| `artifacts/cddl/eras/shelley.cddl` | 8031 | `3a4723732bcd9dafbbb5d2e6c29d9ae3347575212adbf6bbd5e4de16a6790791` |
| `artifacts/legal/LICENSE` | 10174 | `0d542e0c8804e39aa7f37eb00da5a762149dc682d7829451287e11b938e94594` |
| `artifacts/legal/NOTICE` | 575 | `58721f8b6ca67f0fcbe1cd739b384fec3126a35f7d79951aedaa2bc3863a3162` |

## Captured scope

The snapshot contains byte-exact Byron, Shelley, Allegra, Mary, Alonzo, Babbage, and Conway CDDL
plus the upstream Apache-2.0 license and notice. All nine files came from commit
`a624de4c8db7286a6c065da149679ea55f7d5629`.

## Integrity and licensing

The exact inventory is defined by [this frozen capture specification](#frozen-capture-specification). The seven CDDL files and two
legal files are regular files preserved byte-for-byte under `artifacts/`.

## Semantic evidence

The artifacts define supported ledger wire structure, including era-specific choices, bounds,
indexes, tags, optionality, containers, and embedded CBOR. CDDL is not generated source and does
not by itself prescribe public API or language representation.

## Exclusions

Dijkstra, general Cardano Ledger source, generated Haskell, upstream tests, build configuration,
Git metadata, message signing, and UPLC language/evaluator behavior are excluded.

## Change summary

| Change | Evidence | Observed significance |
| --- | --- | --- |
| Baseline | [Captured scope](#captured-scope) and [artifact inventory](#verified-artifact-inventory) | Initial same-provider snapshot; no previous snapshot exists. Original comparison-source observations remain above. |
| Metadata conversion | [Migration provenance](#migration-provenance) | Capture rules are now self-contained; artifact bytes and source identities did not change. No new upstream comparison was performed. |

## Consumer impact and recommended work

Migration-time guidance, not a newly performed upstream audit. Existing captured semantics and
consumer boundaries remain authoritative; this metadata conversion requires no runtime change.

| Evidence | Maintained target and owners | Validation | Recommendation |
| --- | --- | --- | --- |
| Era-specific wire formats and historical compatibility; [semantic evidence](#semantic-evidence) | TypeScript: `libs/typescript/packages/chain/src/` | `libs/typescript/packages/chain/test/` | No implementation change from this migration. Compare a separately captured update before planning behavior changes. |

Behavioral evidence is language-neutral. C++ is an unmaintained, opt-in consumer.

## Unresolved questions

No fresh upstream completeness or delta audit was performed for this metadata migration.
The inventory verifies stored bytes; original capture claims are retained as historical claims.
Any future update must independently enumerate its pinned sources and resolve semantic mappings.

## Migration provenance

- Metadata conversion date: 2026-09-09, explicitly requested by the human while SPECTRE 1.0.0 is in development.
- Previous snapshot descriptor SHA-256: `2fecfab931165c51ff2f4e2fad4989b2c9c5cc8d736aa1b335153d06d193f806`.
- Original applicable capture-rules SHA-256: `5dfbb0785597d05ed96bbfa886bf2f0c49d8741eb0470dc37879ee4f5234103f` (historical label v1).
- Embedded the original applicable rules, with snapshot-relative scope and obsolete provider-version
  routing removed. Added this exact artifact inventory and clearly labeled migration-time guidance.
- Corrected the initial snapshot’s self-referential Previous-Snapshot to NONE; filled inapplicable
  URL source fields with NONE. Preserved Created, snapshot ID, source identities, all artifact paths
  and every artifact byte. No new snapshot or upstream capture was created.
- The converted descriptor and its artifacts are frozen after this migration. Future changes
  require a new numbered snapshot with its own complete specification.
