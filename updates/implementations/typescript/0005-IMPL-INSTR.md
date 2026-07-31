# TypeScript implementation 0005 instruction

Implementation-Version: v1
Implementation-ID: typescript/0005
Created: 20260731T075521Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| `libs/typescript/README.md` and package manifests | `LOCAL` | Yes | Workspace ownership, public package, browser, and completion-command requirements |
| `libs/typescript/packages/chain/src/era/conway/model.ts` | `LOCAL` | Yes | Existing `Value`, `MultiAsset`, and metadata ownership |
| `libs/typescript/packages/chain/src/builder/transaction.ts` | `LOCAL` | Yes | Existing transaction-builder state, selection, fee, and collateral behavior |
| `libs/typescript/packages/chain/test/` and `libs/typescript/packages/runtime/test/` | `LOCAL` | Yes | Compatibility, package-identity, and regression baseline |

This `LOCAL` instruction defines XRAY Cardano Lib's checked-builder safety policy from the existing
builder configuration and behavior. In particular, its collateral formula, return requirements,
and rejection rules are repository-local construction guarantees; they are not presented as a
provider-backed or complete phase-one ledger-validation contract.

## Objective

Make value handling and transaction construction exact, alias-safe, order-independent, and
failure-atomic without changing public package identities or expanding the library into wallet or
provider orchestration.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C001` | Snapshot mutable value inputs and preserve every ledger integer exactly in JSON | Compatible bug fix; unsafe JSON numbers become canonical decimal strings | `libs/typescript/packages/chain/src/era/conway/model.ts`, chain-model tests | Mutation, boundary, JSON, and CBOR round trips |
| `C002` | Enforce disjoint input roles and complete collateral accounting | Compatible safety correction; previously accepted ambiguous builders reject deterministically | `libs/typescript/packages/chain/src/builder/transaction.ts`, builder tests | Input-order permutations, collateral percentage, and native-asset cases |
| `C003` | Make fee/change convergence explicit, bounded, and failure-atomic | Compatible bug fix; non-convergence now throws instead of emitting a stale fee | `libs/typescript/packages/chain/src/builder/transaction.ts`, builder/runtime tests | Convergence, cycle, rollback, size-boundary, and packed-consumer tests |

## Required semantics

### C001: value ownership and exact JSON

- `Value.new(coin, multiAsset)` must deep-snapshot `multiAsset`; later mutation of the caller's
  `MultiAsset`, policy map, or asset map must not change the value.
- `Value.multi_asset()` and every nested collection returned through it must be a defensive copy.
  Preserve the existing nominal `Value`, `MultiAsset`, `MapAssetNameToCoin`, `AssetName`, and
  `ScriptHash` owners.
- JSON writers for `Value` quantities and `TransactionMetadatum` integers must emit a JSON number
  only when the value is a JavaScript safe integer. Emit every other integer as a base-10 string
  with no leading plus sign or redundant leading zero.
- Matching JSON readers must accept safe integral numbers and canonical decimal strings, reject
  unsafe numeric literals, fractions, exponents in strings, negative zero, and out-of-range
  values, and satisfy exact `from_json(to_json(value))` round trips through `UINT64_MAX` and the
  existing metadata integer bounds.
- Do not change CBOR preservation or canonical-encoding behavior.

### C002: input and checked-builder collateral correctness

- Identify a UTxO by the canonical transaction-input bytes. Spending inputs, collateral inputs,
  reference inputs, and selection candidates must be pairwise disjoint regardless of insertion
  order. Reject duplicates within a role and conflicts across roles; do not silently deduplicate.
- `select_utxos` must exclude UTxOs already used as collateral or reference inputs before either
  largest-first or random-improve selection.
- Under the checked-builder policy, when Plutus witnesses or redeemers require collateral, require
  at least one collateral input and require collateral coin of
  `ceil(fee * collateralPercentage / 100)`, using checked integer arithmetic. Treat
  `collateralPercentage` and `maxCollateralInputs` as caller-supplied builder configuration and
  enforce both without claiming full ledger validation.
- If collateral return is present, total collateral must equal collateral input coin minus return
  coin. The return must contain exactly all native assets carried by collateral inputs. If
  asset-bearing collateral has no return, reject instead of silently exposing those assets to
  forfeiture. Reject a return without collateral, an excessive return, and every mismatched asset
  bundle.
- Emit body fields 16 and 17 together and only after these checks. Keep non-Plutus transactions
  free of an invented collateral requirement.

### C003: convergence and atomicity

- Replace the silent eight-pass fee loop with exact fixed-point convergence over the fee and
  canonical change-output bytes.
- Stop successfully only when both values repeat unchanged. Detect a repeated non-fixed state as a
  cycle, cap work at 64 iterations, and throw a deterministic convergence error on a cycle or cap.
- `add_change_if_needed`, `build_for_evaluation`, and `build` must restore the prior fee and output
  state after any error. Temporary `UINT64_MAX` sizing in `min_fee` must use `try/finally`.
- A failed selection, collateral check, size check, or convergence attempt must leave all
  previously observable builder state unchanged.

## Implementation steps

1. Add one internal deep-copy path for value/multiasset state and route constructors and getters
   through it.
2. Add shared exact-integer JSON helpers and use them for value quantities and transaction
   metadata without changing unrelated JSON shapes.
3. Centralize canonical input-role membership checks and invoke them from every add/select path.
4. Centralize collateral requirement and conservation checks, then use them before body emission.
5. Run fee/change calculation against temporary state, commit only a fixed point, and roll back on
   every failure.
6. Add focused regressions before running the complete workspace gate.

## Validation

- Extend `chain-models.test.mjs` with caller/returned-value mutation tests, safe-number boundaries,
  `UINT64_MAX`, canonical decimal-string rejection cases, and JSON/CBOR round trips.
- Extend `builders.test.mjs` with every insertion-order permutation across spending, collateral,
  reference, and selectable inputs; duplicate collateral; 149/150/151-percent rounding examples;
  insufficient collateral; collateral-return asset conservation; and maximum-input boundaries.
- Add stable, multi-pass, cyclic, over-64-pass, size-width-transition, and injected-failure tests
  proving both convergence and rollback.
- Run `node --test libs/typescript/packages/chain/test/chain-models.test.mjs
  libs/typescript/packages/chain/test/builders.test.mjs`.
- Run `npm --prefix libs/typescript run check`.

## Compatibility and human review

Review all newly rejected builder states, especially asset-bearing collateral and input-role
conflicts. Exact JSON remains round-trippable but intentionally changes unsafe integer output from
lossy numbers to decimal strings. Public nominal bindings, subpaths, CBOR wire preservation, and
browser-native production code must remain unchanged.

## Completion criteria

- Value instances cannot be changed through caller-owned or returned collections.
- All supported ledger integers round-trip through JSON without precision loss.
- Input roles are pairwise disjoint for every call order and selection strategy.
- The configured checked-builder collateral percentage, total collateral, return coin, and return
  assets are conserved exactly.
- Fee/change construction either reaches an exact fixed point or throws with no state mutation.
- Targeted tests and `npm --prefix libs/typescript run check` pass.
- The paired result records all behavior changes and any intentionally rejected legacy cases.

## Out of scope

- Wallet UTxO discovery, chain queries, submission, or provider APIs
- New protocol-parameter ownership or automatic protocol-parameter refresh
- New coin-selection strategies
- Full phase-one ledger validation, or ledger validation unrelated to value, input roles,
  collateral, fee, or change

## Blockers

None.
