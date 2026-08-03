# TypeScript implementation 0021 result

Result-Version: v1
Implementation-ID: typescript/0021
Instruction: ./0021-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `TS21-EXPORT1` | Implemented | Audited the existing Conway, chain-root, runtime-chain, and aggregate barrels; no re-export was missing, so the existing `CostModels` owner was retained unchanged. | Strict binding identity across all four paths passes. |
| `TS21-CONSUME1` | Implemented | Added numeric-language `from_json`/`to_json` symmetry and malformed-key consumption through every intended path. | Numeric IDs `0`, `1`, `2`, signed values, symmetry, and malformed rejection pass. |
| `TS21-PACK1` | Implemented | Added packed ESM runtime identity/JSON execution and NodeNext/bundler type consumption. | Browser package and packed-consumer gates pass. |

## Outcome

The accepted chain-owned `CostModels` JSON API is directly consumable through every intended path
as one binding. No duplicate nominal type, protocol-parameter DTO, or adapter was introduced.

## Inputs consumed

- The current user instruction and `typescript/0021` instruction
- `typescript/0004` accepted result
- Existing `CostModels.from_json`/`to_json`, barrels, browser checks, and packed consumers

## Project changes

- Added cross-path identity and JSON-consumption tests.
- Added real packed ESM, NodeNext, and bundler CostModels consumers.
- Documented numeric-language JSON consumption in chain and aggregate READMEs.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `TS21-EXPORT1` | Conway, chain, runtime-chain, and aggregate paths share `CostModels`. | Validation-only; existing exports retained. | Import the existing owner from the appropriate public path. |
| `TS21-CONSUME1` | Numeric-key JSON is symmetric and rejects malformed language keys everywhere. | Existing accepted behavior proven. | Replace manual CostModels CBOR map construction with `from_json`. |
| `TS21-PACK1` | Published package consumers compile and execute the JSON API. | Packaging-only. | No additional adapter is needed. |

## Validation

```sh
npm run build
node --test packages/chain/test/cost-models-json.test.mjs packages/runtime/test/imports.test.mjs
node packages/runtime/test/pack-smoke.mjs
npm run check
```

- Focused suite: 8 passed.
- Complete built suite: 176 passed.
- Packed suite: 550 intended files, 2,449,568 unpacked bytes; ESM, NodeNext, and bundler passed.

## Deviations from instruction

None.

## Remaining human review

Confirm that all exports resolve to the existing chain owner and numeric language IDs retain the
accepted JSON contract, then decide whether this result should move from `REVIEW` to `ACCEPTED`.

## Reproducibility

Run `npm --prefix libs/typescript run check` from the repository root.
