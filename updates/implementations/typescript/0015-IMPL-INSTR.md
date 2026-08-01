# TypeScript implementation 0015 instruction

Implementation-Version: v1
Implementation-ID: typescript/0015
Created: 20260801T085101Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| Current user instruction | `LOCAL` | Yes | Authoritative CostModels JSON parsing, validation, compatibility, scope, and test requirements |
| `libs/typescript/packages/chain/src/era/conway/model.ts` | `LOCAL` | Yes | CostModels owner and existing generic Conway JSON behavior that must remain unchanged |
| `libs/typescript/packages/chain/src/era/conway/validation.ts` | `LOCAL` | Yes | Existing CostModels language-id and signed-64-bit validation contract |
| `libs/typescript/packages/chain/src/era/shared/json-types.ts` | `LOCAL` | Yes | Public numeric-string-key CostModels JSON contract |
| `libs/typescript/packages/chain/test/` | `LOCAL` | Yes | Focused Conway model and JSON validation coverage |

## Objective

Make `CostModels.from_json()` honor its public numeric-string-key JSON contract, make JSON
serialization symmetric for representable cost models, and ensure `CostModels.new()` applies the
CostModels-specific language-id validation.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `TS15-JSON1` | Add a specialized `CostModels.from_json()` accepting only a non-null object whose keys are decimal language IDs in `0..255` and whose values are arrays of safe integers converted to signed 64-bit parameters. | Scoped override; generic Conway JSON conversion remains unchanged. | Conway `CostModels` | Successful parsing, bigint-key lookup, unsigned-CBOR-key inspection, empty input, and malformed-input rejection tests |
| `TS15-JSON2` | Serialize CostModels as a JSON object with numeric-string keys and parameter arrays so `from_json(to_json())` preserves canonical CBOR for JSON-representable models. | Restores the declared `CostModelsJSON` shape without changing other map models. | Conway `CostModels` | Exact JSON shape and canonical-CBOR round-trip tests |
| `TS15-NEW1` | Make `CostModels.new(MapU64ToArrI64)` construct its CBOR node and run `CostModels.validateNode(node)` before returning. | Preserves the constructor, getter, language views, exports, and valid behavior; newly rejects language IDs above 255. | Conway `CostModels` | Valid construction and explicit key `256n` rejection tests |

## Implementation steps

1. Add CostModels-local JSON root, language-id, and parameter validation in `model.ts`.
2. Build the parsed representation through `MapU64ToArrI64` and `CostModels.new()`.
3. Override CostModels JS-value serialization with the declared object shape.
4. Validate the node in `CostModels.new()` through the existing CostModels validator.
5. Add focused tests for valid, empty, round-trip, CBOR-node, and rejected inputs.
6. Run formatting, linting, typechecking, package tests, and the full TypeScript workspace gate.

## Validation

Run from the repository root:

```sh
npm --prefix libs/typescript run build
node --test libs/typescript/packages/chain/test/cost-models-json.test.mjs
npm --prefix libs/typescript run lint
npm --prefix libs/typescript run typecheck
npm --prefix libs/typescript run test
npm --prefix libs/typescript run check
git diff --check
```

## Compatibility and human review

Reviewers must confirm numeric JSON keys become unsigned CBOR keys, invalid JSON never falls
through to generic text-key conversion, valid constructor/getter/language-view behavior is
unchanged, and no generic Conway JSON behavior or xray-js source changed.

## Completion criteria

- Numeric-string CostModels JSON parses to the same canonical representation as
  `CostModels.new(MapU64ToArrI64)`.
- JSON serialization emits the declared object shape and round-trips canonical CBOR.
- Empty objects are supported and all specified malformed roots, keys, and parameters are
  rejected.
- `CostModels.new()` rejects a `MapU64ToArrI64` containing language ID `256n`.
- Focused and complete TypeScript validation passes.

## Out of scope

- Changing generic `ConwayData.from_json()` or `ConwayData.to_js_value()` behavior.
- Changing `MapU64ToArrI64`'s full-uint64 key contract.
- Changing CostModels exports, `get()`, `language_views_encoding()`, or xray-js.

## Blockers

None.
