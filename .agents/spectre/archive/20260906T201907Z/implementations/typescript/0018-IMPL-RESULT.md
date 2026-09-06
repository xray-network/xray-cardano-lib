# TypeScript implementation 0018 result

Result-Version: v1
Implementation-ID: typescript/0018
Instruction: ./0018-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `TS18-DISC1` | Implemented | Added chain-owned `discover_required_witnesses(transaction, resolvedInputs)` with structural UTxO indexing, spending/collateral resolution, bootstrap handling, and conflicting-duplicate rejection. | Key, bootstrap, primary input, collateral, missing resolution, and conflicting duplicate tests pass. |
| `TS18-FIELD1` | Implemented | Discovery covers typed certificates, withdrawals, explicit signers, native signer requirements, datum hashes, both redeemer wire forms, Plutus language identification, and matching reference scripts. | Native, Plutus, certificate, withdrawal, datum, redeemer, reference-script, and malformed-field tests pass. |
| `TS18-SET1` | Implemented | `RequiredSigners.new()` constructs the canonical empty tag-258 set while existing nonempty validation remains available. | Exact canonical `d9010280` assertion passes. |

## Outcome

Consumers can discover nominal ledger witness requirements from an existing typed transaction and
resolved UTxOs without introducing account or SDK DTOs. Resolution is structural and fail-closed;
reference scripts replace matching standalone script requirements, while unknown script languages
remain conservative script-hash requirements.

## Inputs consumed

- The current user instruction and `typescript/0018` instruction
- `typescript/0004` accepted result
- Existing chain builder, witness, transaction, certificate, address, and resolved-UTxO owners
- Read-only adjacent xray-js signing-key discovery helper

## Project changes

- Added required-witness discovery and strict resolved-input indexing to the chain builder owner.
- Added canonical empty `RequiredSigners` construction.
- Added focused discovery tests and documented the public analysis boundary.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `TS18-DISC1` | A transaction plus resolved inputs yields a deduplicated `RequiredWitnessSet`. | Additive; existing builders and witness assembly are unchanged. | Intersect returned nominal hashes with locally owned keys. |
| `TS18-FIELD1` | Certificate, withdrawal, explicit signer, native, datum, redeemer, and reference-script requirements are included. | Additive protocol analysis; no signing or account ownership. | Supply every spending, collateral, and reference input resolution. |
| `TS18-SET1` | `RequiredSigners.new()` yields the canonical empty tag-258 set. | Additive compatibility helper. | Replace manual empty-set CBOR construction. |

## Validation

```sh
npm run build
node --test packages/chain/test/witness-discovery.test.mjs
npm run check
```

- Focused suite: 5 passed.
- Complete built suite: 167 passed.
- Packed suite: 546 intended files, 2,435,629 unpacked bytes; ESM, NodeNext, and bundler passed.

## Deviations from instruction

None.

## Remaining human review

Confirm conservative unresolved-language behavior, set/map redeemer indexing, duplicate-resolution
semantics, and reference-script replacement, then decide whether this result should move from
`REVIEW` to `ACCEPTED`.

## Reproducibility

Run `npm --prefix libs/typescript run check` from the repository root.
