# TypeScript implementation 0023 instruction

Implementation-Version: v1
Implementation-ID: typescript/0023
Created: 20260824T092633Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| Human-provided Eternl transaction diagnostic | `LOCAL` | Yes | Identifies the rejected ADA-only output form `[coin, {}]` and the requirement to omit an empty multi-asset map. |
| `libs/typescript/packages/chain/src/era/conway/model.ts` and transaction builders | `LOCAL` | Yes | Define `Value` construction, arithmetic, output construction, and preserved CBOR behavior. |
| Chain tests, README, and accepted lossless-CBOR ADR | `LOCAL` | Yes | Define interoperability coverage, documentation, and the boundary between constructed and decoded values. |

## Objective

Canonicalize constructed ADA-only values without changing lossless transaction decoding.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C01` | Make `Value.new(coin, assets)` treat an absent or empty `MultiAsset` as coin-only CBOR and retain the tuple form only for non-empty assets. | Behavioral correction with no public signature or export change; constructed empty assets become observationally equivalent to `Value.from_coin(coin)`. | Conway `Value` model | Exact-CBOR construction tests for empty, absent, and non-empty assets. |
| `C02` | Ensure value arithmetic and transaction-output builder paths inherit the construction invariant so generated ADA-only outputs never contain an optional empty multi-asset map. | Existing valid token-bearing values remain unchanged. | Conway value arithmetic and chain builders | Arithmetic, output-builder, and transaction-body regression tests. |
| `C03` | Preserve byte-for-byte `from_cbor_*` round trips for already received `[coin, {}]` values; do not silently rewrite parsed transaction bytes or hashes while signing or inspecting them. | Maintains the accepted lossless-CBOR contract; callers must rebuild malformed transactions to obtain canonical output. | Conway codec and preservation tests | Preserved-versus-constructed fixtures and hash-sensitive round-trip assertions. |
| `C04` | Document the ADA-only construction rule and add completion coverage without adding dependencies or changing package boundaries. | Documentation and validation only. | Chain README and TypeScript workspace tests | Documentation scan and full TypeScript completion gate. |

## Implementation steps

1. Normalize empty multi-assets at the `Value.new` construction boundary.
2. Add focused value, arithmetic, transaction-output, and transaction-body regressions.
3. Prove decoded input remains lossless while newly constructed ADA-only values use coin-only CBOR.
4. Document the construction/preservation distinction and run the complete TypeScript gate.

## Validation

- `npm --prefix libs/typescript run check`
- Confirm `Value.new(4_000_000n, MultiAsset.new()).to_cbor_hex()` equals the coin-only form `1a003d0900` and not `821a003d0900a0`.
- Confirm arithmetic and output builders cannot reintroduce an empty multi-asset tuple.
- Confirm non-empty multi-asset values retain their existing tuple encoding and semantics.
- Confirm decoding and preserved serialization of an existing `[coin, {}]` fixture remain byte-identical.
- `git diff --check`

## Compatibility and human review

Review the normalization boundary: newly constructed values become ledger-compatible, while parsed transaction CBOR remains lossless and is never repaired during signing.

## Completion criteria

All newly constructed ADA-only values and outputs use the coin-only form, token-bearing values are unchanged, decoded CBOR remains byte-preserving, documentation is explicit, and the complete TypeScript gate passes.

## Out of scope

- Rewriting arbitrary incoming transaction CBOR, changing transaction hashes during signing, dependency upgrades, package publication, or changes outside the TypeScript implementation

## Blockers

None.
