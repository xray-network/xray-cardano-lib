# TypeScript implementation 0013 instruction

Implementation-Version: v1
Implementation-ID: typescript/0013
Created: 20260801T074533Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| Current user instruction | `LOCAL` | Yes | Authoritative typed Void schema, aggregate export, compatibility, test, and validation requirements |
| `docs/adr/typescript/0001-lossless-cbor-and-encoding-metadata.md` | `LOCAL` | Yes | Lossless decoding and explicit canonical encoding requirements |
| `docs/adr/typescript/0003-upstream-evidence-and-package-ownership.md` | `LOCAL` | Yes | Plutus typed-data ownership and aggregate identity re-export requirements |
| `libs/typescript/packages/plutus/` | `LOCAL` | Yes | Owned typed Plutus Data implementation, public exports, tests, and documentation |
| `libs/typescript/packages/runtime/` | `LOCAL` | Yes | Owned aggregate exports, package tests, packed-consumer checks, and documentation |

## Objective

Extend the typed Plutus Data API with an exact void schema and expose the typed-data API through
the aggregate package so xray-js can remove its local Plutus Data implementation.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `TS13-VOID1` | Add `VoidSchema`, include it in `DataSchema`, and add `Data.Void()` with static TypeScript type `undefined`; encode only `undefined` as constructor alternative zero with no fields and decode only that exact constructor shape to `undefined`. | Additive typed-data API; existing schema and wire behavior remains unchanged. | Plutus typed-data types and codec | Typecheck plus valid and invalid void tests |
| `TS13-RAW1` | Preserve `Data.void()` as the raw-CBOR helper returning `d87980`. | Fully compatible. | Plutus typed-data facade | Existing and focused typed-data tests |
| `TS13-AGG1` | Re-export `Data` and `Constr` by identity from `@xray-network/xray-cardano-lib`, together with the typed-data schema types, without duplicate nominal implementations or conflicting star exports. | Additive aggregate API. | Runtime Plutus and root facades | Aggregate identity, named-import, type-consumer, and packed-package tests |
| `TS13-TEST1` | Cover the void round trip, invalid values and CBOR, aggregate imports, static typing, and packed-package consumption. | Test-only coverage. | Plutus and runtime tests | Targeted tests and complete workspace gate |

## Implementation steps

1. Add the void schema type and constructor to the Plutus typed-data owner.
2. Handle the exact zero-alternative, zero-field constructor in both schema cast directions while
   leaving the raw helper and every existing schema unchanged.
3. Add explicit aggregate value and type re-exports from the Plutus owner.
4. Add runtime, type-consumer, invalid-input, and packed-package coverage and update package docs.
5. Run formatting, typechecking, built tests, packed-package tests, and the complete TypeScript
   validation gate.

## Validation

Run from the repository root:

```sh
npx --prefix libs/typescript tsc -b libs/typescript/tsconfig.json --pretty false
npm --prefix libs/typescript run typecheck
npm --prefix libs/typescript run test:built
npm --prefix libs/typescript run pack:smoke
npm --prefix libs/typescript run check
```

Also run the repository formatter check through the complete gate and inspect the generated
declarations to confirm `Data.Static<ReturnType<typeof Data.Void>>` resolves to `undefined`.

## Compatibility and human review

Reviewers must confirm that `Data.Void()` accepts exactly the Plutus constructor alternative zero
with no fields, that lowercase `Data.void()` remains the raw `d87980` helper, and that the
aggregate bindings are identity-preserving re-exports from the Plutus package.

## Completion criteria

- `Data.to(undefined, Data.Void())` returns `d87980` and the inverse returns `undefined`.
- Wrong JavaScript values, wrong Plutus variants, constructor alternatives, field counts, and
  malformed CBOR are rejected.
- The aggregate package supports named `Data` and `Constr` imports and exposes the typed-data
  schema types without introducing another implementation.
- Existing typed-data tests, typechecking, built tests, and packed-package tests pass.
- The complete TypeScript validation gate passes.

## Out of scope

- Changing existing Data or Constr behavior.
- Removing or changing `Data.void()`.
- Changing chain-owned `PlutusData` wire models.
- Adding TypeScript-specific `undefined` semantics to the independent C++ API.
- Changing xray-js itself.

## Blockers

None.
