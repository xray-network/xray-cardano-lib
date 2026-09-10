# TypeScript implementation 0014 result

Result-Version: v1
Implementation-ID: typescript/0014
Instruction: ./0014-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `TS14-SIGN1` | Implemented | Added `CIP8Message.signData`, producing a COSESign1 envelope with protected EdDSA/address headers and an OKP/EdDSA/Ed25519 public COSE key derived from the supplied normal or extended private key. | Focused round-trip tests decoded and checked every generated header and key label, then successfully verified the envelope. |
| `TS14-VERIFY1` | Implemented | Added `CIP8Message.verifyData` with the xray-js extraction order, exact string comparisons, malformed-field exceptions, semantic mismatch `false` results, and final Ed25519 verification. | Address, key-hash, payload, signature, algorithm, curve, key-type, missing field, wrongly typed field, and malformed envelope tests passed. |
| `TS14-TYPE1` | Implemented | Exported `CIP8MessageEnvelope` with readonly hexadecimal `signature` and `key` fields. | Workspace typecheck and packed NodeNext/bundler type consumers passed. |
| `TS14-AGG1` | Implemented | Re-exported the CIP-8-owned facade and envelope type through the runtime CIP facade and `@xray-network/xray-cardano-lib` root by identity. | Aggregate identity/import tests and packed-package runtime tests passed. |

## Outcome

Applications can now remove the local xray-js message-signing wrapper and use `CIP8Message`
directly from the focused CIP-8 or aggregate package. Signing returns the established
`{ signature, key }` hexadecimal envelope. Verification preserves the existing validation
contract: malformed or wrongly typed envelope fields throw, well-typed semantic mismatches return
`false`, and a fully matching envelope returns the Ed25519 verification result.

## Inputs consumed

- The current user instruction
- The adjacent xray-js message-signing facade as a read-only semantic reference
- `docs/adr/typescript/0003-upstream-evidence-and-package-ownership.md`
- Existing CIP-8 COSE models, builders, tests, and focused exports
- Existing crypto-owned private/public key, key-hash, and signature bindings
- Existing aggregate runtime exports and packed consumer harness

No provider evidence or external implementation result was consumed.

## Project changes

- Added `libs/typescript/packages/cip/src/cip8/message.ts` with the facade and envelope type.
- Exported the facade/type from the focused CIP-8 and aggregate runtime entry points.
- Added dedicated sign/verify, tampering, semantic mismatch, invalid-header, malformed-envelope,
  and aggregate identity tests.
- Added packed runtime and type-consumer coverage.
- Documented the high-level facade alongside the existing low-level COSE builders.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `TS14-SIGN1` | `CIP8Message.signData(addressHex, payloadHex, privateKeyBech32)` returns a hexadecimal COSESign1/COSEKey envelope using protected EdDSA and address headers plus an OKP/EdDSA/Ed25519 public key. | Additive; existing COSE builders and key owners are unchanged. | Replace local envelope construction with the facade call. |
| `TS14-VERIFY1` | `CIP8Message.verifyData` validates address, key hash, both EdDSA labels, Ed25519 curve, OKP key type, payload, signature shape, and Ed25519 authenticity with the preserved xray-js throw-versus-false boundary. | Additive and behavior-compatible with the current xray-js wrapper. | Replace local envelope parsing and validation with the facade call. |
| `TS14-TYPE1` | `CIP8MessageEnvelope` contains readonly hexadecimal `signature` and `key` strings. | Additive type export. | Use the exported type instead of a local duplicate. |
| `TS14-AGG1` | The aggregate package exposes the CIP-8 owner's `CIP8Message` binding and envelope type. | Identity-preserving additive export. | Import from `@xray-network/xray-cardano-lib` when using the aggregate package. |

## Validation

The following checks passed from the repository root:

```sh
npm --prefix libs/typescript run typecheck
node --test libs/typescript/packages/cip/test/cip8-message.test.mjs \
  libs/typescript/packages/runtime/test/imports.test.mjs
npm --prefix libs/typescript run check
git diff --check
```

- Focused CIP8Message and aggregate tests: 5 passed.
- Complete built TypeScript suite: 142 passed.
- Packed suite: 530 intended files; ESM runtime plus NodeNext and bundler type consumers passed.
- The packed aggregate facade signed and verified an envelope, and the packed declarations exposed
  `CIP8MessageEnvelope`.

## Deviations from instruction

None.

## Remaining human review

Confirm the protected header/key construction, exact string comparisons, malformed-field error
messages, semantic mismatch ordering, and facade identity, then decide whether this result should
move from `REVIEW` to `ACCEPTED`.

## Reproducibility

From the repository root with Node.js 20.19 or newer and locked dependencies installed, run
`npm --prefix libs/typescript run check`.
