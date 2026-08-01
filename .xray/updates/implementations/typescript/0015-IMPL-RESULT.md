# TypeScript implementation 0015 result

Result-Version: v1
Implementation-ID: typescript/0015
Instruction: ./0015-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `TS15-JSON1` | Implemented | Added a CostModels-only `from_json()` override that accepts non-null object roots, converts decimal keys to bigint language IDs in `0..255`, requires parameter arrays of safe integers, converts them through signed 64-bit bigint arrays, and constructs unsigned CBOR map keys. | Focused tests cover IDs `0`, `1`, and `2`, bigint-key lookup, unsigned CBOR keys, exact equivalence with `CostModels.new()`, empty objects, and every requested malformed root, key, and value. |
| `TS15-JSON2` | Implemented | Added a CostModels-only `to_js_value()` override that emits the declared numeric-string-key object shape and refuses values that cannot be represented as safe JSON integers without loss. | Exact JS/JSON shape and `from_json(to_json())` canonical-CBOR equality tests passed. |
| `TS15-NEW1` | Implemented | Changed `CostModels.new()` to construct the node, run `CostModels.validateNode(node)`, and return only after CostModels-specific validation succeeds. | Existing valid language-view coverage remains green and a focused `256n` constructor rejection test passed. |

## Outcome

`CostModels.from_json()` now honors the public `CostModelsJSON` contract: numeric JSON property
names become unsigned CBOR language IDs instead of text keys, while malformed roots, IDs, and
parameters are rejected locally. CostModels JSON serialization uses the matching object form and
round-trips canonical CBOR for safely representable JSON values. `CostModels.new()` now applies
the existing `0..255` CostModels language-id bound in addition to the map owner's uint64 bound.

## Inputs consumed

- The current user instruction
- `libs/typescript/packages/chain/src/era/conway/model.ts`
- `libs/typescript/packages/chain/src/era/conway/validation.ts`
- `libs/typescript/packages/chain/src/era/shared/json-types.ts`
- Existing Conway foundation and TypeScript workspace tests

No provider evidence or external implementation result was consumed.

## Project changes

- Specialized CostModels JSON parsing and serialization in
  `libs/typescript/packages/chain/src/era/conway/model.ts`.
- Added CostModels node validation to `CostModels.new()`.
- Added `libs/typescript/packages/chain/test/cost-models-json.test.mjs` with valid parsing,
  representation, serialization, empty-map, invalid-input, and constructor-bound coverage.
- Left generic Conway JSON conversion, `MapU64ToArrI64`, exports, getters, language views, and
  xray-js unchanged.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `TS15-JSON1` | CostModels JSON accepts only object roots with decimal language IDs in `0..255` and safe-integer parameter arrays; parsed language IDs are unsigned CBOR keys and parameters are signed 64-bit values. | Corrects the existing public JSON contract without changing generic Conway map JSON behavior. | Call `CostModels.from_json()` directly with the declared numeric-string-key JSON form. |
| `TS15-JSON2` | CostModels JSON serialization emits an object keyed by decimal language IDs and round-trips canonical CBOR for safely representable values. | Corrective, CostModels-local serialization; other model serializers are unchanged. | Consumers can persist `models.to_json()` and restore it with `CostModels.from_json()`. |
| `TS15-NEW1` | `CostModels.new()` rejects language IDs above `255` even though `MapU64ToArrI64` continues to accept the full uint64 range. | Valid existing inputs and public APIs are unchanged; invalid CostModels fail earlier. | Do not place non-language uint64 keys in a map passed to `CostModels.new()`. |

## Validation

The following checks passed from the repository root:

```sh
npm --prefix libs/typescript run build
node --test libs/typescript/packages/chain/test/cost-models-json.test.mjs \
  libs/typescript/packages/chain/test/conway-foundations.test.mjs
npm --prefix libs/typescript run lint
npm --prefix libs/typescript run typecheck
npm --prefix libs/typescript run test
npm --prefix libs/typescript run check
git diff --check
```

- Focused CostModels suite: 5 passed.
- Focused CostModels plus existing Conway foundations: 11 passed.
- Lint dependency/browser suite: 3 passed.
- Complete built TypeScript suite: 147 passed.
- Packed suite: 530 intended files; ESM runtime plus NodeNext and bundler consumers passed.
- The workspace defines no separate formatter command; changed source, tests, and records were
  formatted to the repository's existing style and passed `git diff --check`.

## Deviations from instruction

None.

## Remaining human review

Confirm the specialized parser's error boundaries, unsigned-key representation, safe-integer JSON
serialization boundary, constructor validation, and generic-Conway scope, then decide whether this
result should move from `REVIEW` to `ACCEPTED`.

## Reproducibility

From the repository root with Node.js 20.19 or newer and locked dependencies installed, run
`npm --prefix libs/typescript run check`.
