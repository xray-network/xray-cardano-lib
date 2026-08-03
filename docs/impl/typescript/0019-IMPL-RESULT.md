# TypeScript implementation 0019 result

Result-Version: v1
Implementation-ID: typescript/0019
Instruction: ./0019-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `TS19-SCRIPT1` | Implemented | Added `Script.new_native`, `new_plutus_v1`, `new_plutus_v2`, `new_plutus_v3`, typed `kind()`, and matching accessors to the existing Conway owner. | All four variants, malformed discriminants, preservation, canonicalization, and mutation tests pass. |
| `TS19-REF1` | Implemented | Added `ScriptRef.new(script)`, `new_script(script)`, and defensive `script()` directly to the tag-24 owner. | Exact tag/payload, malformed tag, malformed embedded CBOR, and defensive access tests pass. |
| `TS19-ENV1` | Implemented | Added Plutus-owned `SerializedPlutusScript` and `SerializedPlutusScriptKind` with explicit raw Flat, single-CBOR, and double-CBOR constructors and conversions. | Exact, noncanonical-preserved, malformed, trailing, extra-wrapping, ambiguity, and mutation tests pass. |
| `TS19-EXPORT1` | Implemented | Exported serialized envelopes through Plutus focused/root and aggregate runtime paths by binding identity; chain owners use existing exports. | Strict identity, browser, and packed-consumer gates pass. |

## Outcome

Consumers can construct and inspect ledger scripts and tag-24 references without numeric
discriminants or manual CBOR. Serialized UPLC form is always selected explicitly, validated as an
exact Flat program at the declared nesting level, and never inferred heuristically.

## Inputs consumed

- The current user instruction and `typescript/0019` instruction
- `typescript/0003` and `typescript/0004` accepted results
- Existing chain script/ScriptRef validation and Plutus Flat codecs
- Read-only adjacent xray-js script helpers

## Project changes

- Added typed ledger script and reference construction/access.
- Added immutable explicit serialized-script envelopes in the Plutus package.
- Migrated chain builder internals to the typed script/reference APIs.
- Added focused chain/Plutus tests and package documentation.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `TS19-SCRIPT1` | Every ledger script variant has a named factory and typed accessor. | Additive; generic raw construction remains. | Replace numeric `Script.new(...)` calls. |
| `TS19-REF1` | `ScriptRef.new(script)` owns exact tag-24 construction and typed recovery. | Additive. | Remove manual tag-24 encoding. |
| `TS19-ENV1` | Raw, single-CBOR, and double-CBOR UPLC forms are explicit strict identities. | Additive; existing Flat/envelope functions remain. | Replace heuristic double-CBOR normalization with a declared constructor. |
| `TS19-EXPORT1` | Focused, root, and aggregate paths share nominal owners. | Identity-preserving. | Import from the narrowest appropriate path. |

## Validation

```sh
npm run build
node --test packages/chain/test/script-factories.test.mjs packages/plutus/test/serialized-script.test.mjs
npm run check
```

- Focused suite: 7 passed.
- Complete built suite: 174 passed.
- Packed suite: 550 intended files, 2,445,053 unpacked bytes; ESM, NodeNext, and bundler passed.

## Deviations from instruction

None.

## Remaining human review

Confirm ledger Plutus owners contain the single-CBOR script bytes expected by ledger hashing,
strict nesting behavior, and preserved noncanonical same-form bytes, then decide whether this
result should move from `REVIEW` to `ACCEPTED`.

## Reproducibility

Run `npm --prefix libs/typescript run check` from the repository root.
