# TypeScript implementation 0004 instruction

Implementation-Version: v1
Implementation-ID: typescript/0004
Created: 20260723T122735Z
Evidence-Mode: HYBRID
Depends-On: ./0001-IMPL-RESULT.md, ./0003-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-ledger/0001-cardano-ledger/SNAPSHOT.md, ../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`0001-cardano-ledger`](https://github.com/xray-network/xray-cardano-lib/blob/main/.xray/updates/providers/cardano-ledger/0001-cardano-ledger/SNAPSHOT.md) | `PROVIDER` | Yes | Official Byron-through-Conway ledger grammar |
| [`0001-cardano-multiplatform-lib`](https://github.com/xray-network/xray-cardano-lib/blob/main/.xray/updates/providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/SNAPSHOT.md) | `PROVIDER` | Yes | Frozen public compatibility and historical vectors |
| [`typescript/0001`](./0001-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Existing TypeScript CML baseline |
| [`typescript/0003`](./0003-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Existing TypeScript Plutus ownership boundary |

## Objective

Implement supported Cardano Lib ledger validation from the captured official Cardano Ledger CDDL
through direct evidence review and owned TypeScript, while preserving historical compatibility,
stable public ownership, and byte-preserving CBOR behavior.

## Comparison sources

- Previous Cardano Ledger snapshot: `NONE`.
- Frozen comparison snapshot:
  `../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib`.

## Confirmed upstream delta

This initial Cardano Ledger snapshot introduces official CDDL for the supported Byron-through-Conway
eras plus the associated Apache-2.0 license and notice. There is no earlier Ledger snapshot delta.

The official files differ structurally from the frozen CML-derived bundles. The initial comparison
identified material differences for Shelley through Conway; `bigint`, `encoded-cbor`, and `nil`
are standard CDDL prelude names rather than unresolved external rules. Structural differences are
planning evidence and do not by themselves prove runtime incompatibility.

Migration can affect rule-to-source ownership, shared and Conway models, historical-era codecs,
JSON forms, transaction construction, public exports, multi-era dispatch, and byte-exact CBOR
behavior.

## Changes to implement

| Language | Change ID | Requirement | Compatibility | Local owner | Validation | Reason |
| --- | --- | --- | --- | --- | --- | --- |
| `typescript` | `C001` | `REQUIRED` | `compatible` | `libs/typescript/packages/chain/`, `libs/typescript/packages/plutus/src/typed_data/` | `npm --prefix libs/typescript run check` | Initial Ledger baseline |

## Captured artifacts

All selected files are byte-exact copies from commit
`a624de4c8db7286a6c065da149679ea55f7d5629`.

| Artifact | Upstream source | Integrity | License | Intended consumers |
| --- | --- | --- | --- | --- |
| `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/byron.cddl` | `eras/byron/ledger/impl/cddl-spec/byron.cddl` | Exact provider inventory | Apache-2.0 | Byron models, codecs, and tests |
| `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/shelley.cddl` | `eras/shelley/impl/cddl/data/shelley.cddl` | Exact provider inventory | Apache-2.0 | Shelley and shared era ownership |
| `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/allegra.cddl` | `eras/allegra/impl/cddl/data/allegra.cddl` | Exact provider inventory | Apache-2.0 | Allegra and shared era ownership |
| `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/mary.cddl` | `eras/mary/impl/cddl/data/mary.cddl` | Exact provider inventory | Apache-2.0 | Mary and shared era ownership |
| `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/alonzo.cddl` | `eras/alonzo/impl/cddl/data/alonzo.cddl` | Exact provider inventory | Apache-2.0 | Alonzo and shared era ownership |
| `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/babbage.cddl` | `eras/babbage/impl/cddl/data/babbage.cddl` | Exact provider inventory | Apache-2.0 | Babbage and shared era ownership |
| `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/conway.cddl` | `eras/conway/impl/cddl/data/conway.cddl` | Exact provider inventory | Apache-2.0 | Conway governance and shared ownership |
| `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/legal/LICENSE` | `LICENSE` | Byte-exact | Apache-2.0 | Legal review |
| `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/legal/NOTICE` | `NOTICE` | Byte-exact | Apache-2.0 | Legal review |

## Cardano Lib change map

| Upstream change | Evidence | Local owner | Required implementation | Compatibility | Tests |
| --- | --- | --- | --- | --- | --- |
| Official Byron grammar baseline | `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/byron.cddl` | `libs/typescript/packages/chain/src/era/byron/`, `libs/typescript/packages/chain/src/address/` | Align reviewed Byron rules with owned models and codecs; record justified no-op mappings | Historical envelopes and preserved bytes | Byron, chain-model, genesis, and vector tests |
| Official Shelley, Allegra, and Mary grammar baselines | Era CDDL artifacts | `libs/typescript/packages/chain/src/era/{shelley,allegra,mary}/`, `libs/typescript/packages/chain/src/era/shared/` | Apply reviewed rule, bound, optionality, and encoding differences independently by era | Historical-era behavior and JSON shapes | Era, chain-model, multi-era, malformed, and vector tests |
| Official Alonzo and Babbage grammar baselines | Era CDDL artifacts | `libs/typescript/packages/chain/src/era/{alonzo,babbage}/`, `libs/typescript/packages/chain/src/era/shared/`, `libs/typescript/packages/plutus/src/typed_data/` | Apply reviewed ledger-data, script, redeemer, cost-model, and container differences without adding UPLC evaluation | Wire, canonical, preserved-byte, and public-data compatibility | Era, Plutus-data, chain-model, multi-era, and vector tests |
| Official Conway grammar baseline | `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/conway.cddl` | `libs/typescript/packages/chain/src/era/conway/`, `libs/typescript/packages/chain/src/era/shared/` | Align reviewed Conway ledger and governance ownership, models, codecs, and JSON forms | Governance API and Conway wire behavior | Conway foundations, chain-model, multi-era, malformed, and vector tests |
| Cross-era ownership effects | All era CDDL artifacts | `libs/typescript/packages/chain/src/era/multi-era/`, `libs/typescript/packages/chain/src/ledger/`, `libs/typescript/packages/chain/src/builder/`, public barrels | Keep dispatch, operations, builders, exports, and nominal ownership consistent with changed era rules | Public API and downstream consumers | Import, API-contract, builder, ledger, wallet, dApp, and indexer tests |
| Captured legal terms | `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/legal/` | Snapshot evidence and documentation | Preserve notices and surface any changed obligation for human review | Legal and release review | Artifact inventory and repository review |

## Implementation steps

1. Reconcile Byron ownership with the captured official rules and record every applied or justified
   no-op mapping.
2. Reconcile Shelley, Allegra, and Mary independently, preserving each historical era's runtime and
   wire behavior.
3. Reconcile Alonzo and Babbage ledger-data and script-related grammar under the package-owned
   `typed_data` source while preserving the public `@xray-network/cardano-plutus/data` subpath and
   without expanding scope into UPLC language or evaluation.
4. Reconcile Conway governance, shared models, codecs, and JSON forms.
5. Update multi-era dispatch, public exports, builders, and ledger operations only where the
   reviewed era changes require it; retain compatibility aliases only when explicitly justified.
6. Add focused positive, boundary, malformed, canonical, and preserved-wire tests for every source
   change, and run the historical fixture corpus after each era group.
7. Update technical documentation and record every applied or unchanged era mapping in the
   paired TypeScript implementation result.

If a captured definition cannot be resolved unambiguously, stop implementation and require a new
snapshot rather than silently ignoring or approximating it.

## Validation

- Run targeted package-owned tests for every changed era and shared owner.
- Run `node --test libs/typescript/packages/chain/test/chain-models.test.mjs
  libs/typescript/packages/chain/test/multi-era.test.mjs libs/typescript/packages/chain/test/upstream-vectors.test.mjs`.
- Run public entry-point and packed-consumer checks through the TypeScript workspace command.
- Run focused typed Data tests and verify source and emitted modules use the `typed_data` layout.
- Run `npm --prefix libs/typescript run check`.

## Compatibility and human review

This is compatibility-sensitive wire-format work. Every source effect requires review against
existing vectors and APIs because structural grammar differences do not prove behavioral
differences. Review changes to encodings, tags, indexes, fields, bounds, choices, public exports,
and JSON shapes. The internal `typed_data` layout must retain the public
`@xray-network/cardano-plutus/data` export. License material requires human review if its
obligations change.

## Completion criteria

- Every supported official era rule has a recorded official-to-CML-to-owned-source disposition.
- No unsupported or unresolved official CDDL is silently ignored.
- Historical fixtures retain their recorded outcomes and preserved-byte behavior.
- Every source change has focused positive, boundary, malformed, canonical, and compatibility
  coverage.
- Package-owned typed Data source and emitted modules live under `typed_data`, with no stale
  internal `data` directory.
- All intended public API changes are documented.
- `npm --prefix libs/typescript run check` passes.
- The paired TypeScript implementation result records applied and unchanged mappings plus every
  implementation deviation.

## Observed but excluded

- Dijkstra was observed upstream but has no Cardano Lib owner or public entry point.
- General Cardano Ledger source, Haskell implementation, tests, build configuration, and Git
  metadata were not captured.
- Changes unrelated to the selected era CDDL and Cardano Lib ownership are excluded.

## Out of scope

- Dijkstra implementation or public exports
- UPLC language parsing, evaluation, Flat encoding, or builtin semantics
- Message-signing libraries
- Executing any upstream program
- Removing the frozen CML corpus before its remaining comparison value is separately reviewed

## Blockers

None. Any ambiguity discovered during implementation triggers the stop condition above.
