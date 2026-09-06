# TypeScript implementation 0017 result

Result-Version: v1
Implementation-ID: typescript/0017
Instruction: ./0017-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `TS17-PARTS1` | Implemented | Added typed `Transaction.new(...)`, `body()`, `witness_set()`, `is_valid()`, and `auxiliary_data()` directly to the existing owner. | Present/absent auxiliary data, both validity values, malformed arrays, and defensive byte ownership pass. |
| `TS17-WIRE1` | Implemented | Field accessors retain accepted CBOR preservation metadata while each child canonicalizes independently. | An indefinite transaction/body/witness fixture retains exact bytes and converges canonically. |
| `TS17-EXPORT1` | Implemented | Reused existing Conway, chain-root, and aggregate exports without a new wrapper. | Strict runtime binding identity and packed consumers pass. |

## Outcome

Consumers can now decompose and reconstruct a Conway `Transaction` exclusively through existing
typed owners. The generic validated constructor remains available, while typed construction fills
the absent auxiliary field with ledger `null`.

## Inputs consumed

- The current user instruction and `typescript/0017` instruction
- `typescript/0004` accepted result
- Existing Conway transaction models/validation and read-only xray-js transaction-parts helper

## Project changes

- Added typed transaction construction and four defensive field accessors.
- Added true/false, optional, malformed, mutation, lossless, canonical, and export-identity tests.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `TS17-PARTS1` | `Transaction` exposes typed body, witness set, validity, and optional auxiliary data. | Additive; generic raw construction remains. | Remove manual transaction-array decoding and reconstruction. |
| `TS17-WIRE1` | Decomposed children retain accepted wire bytes and expose explicit canonical encoding. | Preserves the accepted lossless contract. | Keep using explicit canonical methods when canonical bytes are required. |
| `TS17-EXPORT1` | All intended paths expose the same `Transaction` owner. | Identity-preserving. | Import from focused, chain, or aggregate paths as appropriate. |

## Validation

```sh
npm run build
node --test packages/chain/test/conway-foundations.test.mjs packages/runtime/test/imports.test.mjs
npm run check
```

- Focused suite: 11 passed.
- Complete built suite: 162 passed.
- Packed suite: 546 intended files, 2,417,693 unpacked bytes; ESM, NodeNext, and bundler passed.

## Deviations from instruction

None.

## Remaining human review

Confirm field order, defensive ownership, optional auxiliary handling, and noncanonical child-byte
preservation, then decide whether this result should move from `REVIEW` to `ACCEPTED`.

## Reproducibility

Run `npm --prefix libs/typescript run check` from the repository root.
