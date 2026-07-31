# Cardano Ledger provider

Provider: cardano-ledger
Provider-Version: v1

## Purpose

Capture official Cardano Ledger CDDL needed to implement and validate Cardano Lib's owned
TypeScript against the frozen CML comparison baseline. The files are evidence for an
implementation plan; they never generate or overwrite runtime source.

## Source

| Field | Value |
| --- | --- |
| Repository | `https://github.com/IntersectMBO/cardano-ledger.git` |
| Followed ref | `refs/heads/master` |
| Revision policy | Full commit reachable from the followed ref |
| Source mode | Live; resolve independently for every snapshot |
| Submodules | Not part of the source |
| License | Apache-2.0 |

A tag is optional descriptive evidence. The resolved full commit is authoritative.

## Artifact selection

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

The filenames and destinations are part of this provider version. A missing, moved, renamed,
symlinked, or ambiguous candidate requires a reviewed provider-version change.

## Evidence-only paths

Read these at the candidate commit when present, but do not copy them as snapshot artifacts:

- `CHANGELOG.md`
- `eras/{shelley,allegra,mary,alonzo,babbage,conway,dijkstra}/impl/CHANGELOG.md`
- `eras/dijkstra/impl/cddl/data/dijkstra.cddl`
- relevant changes below `docs/`, `eras/*/formal-spec/`, `semantics/`,
  `eras/*/impl/cddl/lib/`, and `eras/*/impl/cddl/exe/`

Use the selected-file diff and this evidence to establish upstream facts. Summarize and cite paths
at the pinned commit; do not copy an upstream changelog into the snapshot document.

## Comparison and planning requirements

- Compare with the latest accepted Cardano Ledger snapshot.
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

Dijkstra is evidence-only because Cardano Lib has no Dijkstra owner or public entry point. Adding
it requires an explicit scope decision, a provider-version update, package ownership, multi-era
handling, fixtures, and tests. Until then, do not copy Dijkstra artifacts or include Dijkstra
implementation work.

## Excluded source material

- General Cardano Ledger source and generated Haskell
- Upstream tests, build output, package configuration, and Git metadata
- Message-signing functionality
- UPLC language parsing, evaluation, Flat encoding, or builtin semantics

Ledger CDDL fields for Plutus data, scripts, redeemers, execution units, and cost models remain
ordinary ledger wire grammar and are not excluded.
