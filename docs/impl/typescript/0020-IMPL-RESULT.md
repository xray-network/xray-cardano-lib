# TypeScript implementation 0020 result

Result-Version: v1
Implementation-ID: typescript/0020
Instruction: ./0020-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `TS20-EVAL1` | Implemented | Added `evaluatePhaseTwo(transaction, resolvedInputs, costModels, ...)`, delegating to the unchanged raw evaluator through existing chain byte owners. | Typed/raw cost and log parity, resolution conversion, freezing, and malformed tests pass. |
| `TS20-ID1` | Implemented | Added `TypedPhaseTwoEvaluation` containing the existing chain `RedeemerWitnessKey`; returned legacy redeemers are strictly validated before key extraction. Tightened direct key construction to ledger tag `0..5` and uint32 index bounds. | All six tags, exact index bounds, malformed tag, nominal identity, and typed accessor tests pass. |
| `TS20-EXPORT1` | Implemented | Exported the typed function and contract through Plutus root and aggregate runtime paths. | Binding identity, browser, and packed-consumer gates pass. |

## Outcome

Normal phase-two consumers receive a chain-owned typed redeemer identity and immutable evaluation
without decoding CBOR arrays. The byte-oriented compatibility API and its result shape remain
unchanged.

## Inputs consumed

- The current user instruction and `typescript/0020` instruction
- `typescript/0003` and `typescript/0004` accepted results
- Existing Plutus raw evaluator and chain transaction, UTxO, CostModels, legacy redeemer, and key owners
- Read-only adjacent xray-js evaluator loop

## Project changes

- Added typed phase-two input/result conversion in the Plutus API.
- Enforced the ledger uint32 pointer bound in `RedeemerWitnessKey` direct construction.
- Added parity, bounds, malformed, immutability, and identity tests and documentation.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `TS20-EVAL1` | `evaluatePhaseTwo` consumes typed ledger owners and returns typed immutable results. | Additive; raw evaluation remains unchanged. | Use the typed API for normal ledger workflows. |
| `TS20-ID1` | Each result exposes `redeemer.tag()` and `redeemer.index()` through the existing owner. | Identity-preserving; direct invalid key construction now fails at ledger bounds. | Remove manual legacy-redeemer array decoding. |
| `TS20-EXPORT1` | Plutus and runtime expose the same function and chain key owner. | Additive by identity. | Import from Plutus or aggregate runtime as appropriate. |

## Validation

```sh
npm run build
node --test packages/plutus/test/api.test.mjs
npm run check
```

- Focused suite: 11 passed.
- Complete built suite: 175 passed.
- Packed suite: 550 intended files, 2,449,110 unpacked bytes; ESM, NodeNext, and bundler passed.

## Deviations from instruction

None.

## Remaining human review

Confirm CPU/memory tuple order, UTxO byte conversion, strict legacy-redeemer validation, and the
uint32 pointer bound, then decide whether this result should move from `REVIEW` to `ACCEPTED`.

## Reproducibility

Run `npm --prefix libs/typescript run check` from the repository root.
