# TypeScript implementation 0014 instruction

Implementation-Version: v1
Implementation-ID: typescript/0014
Created: 20260801T082633Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| Current user instruction | `LOCAL` | Yes | Authoritative CIP8Message API, compatibility, export, type, and test requirements |
| Current xray-js message-signing facade supplied by the adjacent local workspace | `LOCAL` | Yes | Read-only semantic reference for the validation and error/false boundaries requested by the user |
| `docs/adr/typescript/0003-upstream-evidence-and-package-ownership.md` | `LOCAL` | Yes | CIP package ownership and identity-preserving aggregate export rules |
| `libs/typescript/packages/cip/src/cip8/` | `LOCAL` | Yes | Owned COSE models, builders, signing structures, focused exports, and tests |
| `libs/typescript/packages/runtime/` | `LOCAL` | Yes | Owned aggregate exports and package-consumer tests |

## Objective

Add a browser-safe CIP8Message facade that replaces xray-js's local sign/verify wrapper while
preserving its validation semantics and exporting the envelope type.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `TS14-SIGN1` | Add `CIP8Message.signData(addressHex, payloadHex, privateKeyBech32)` returning a hexadecimal COSESign1 signature and public COSE key envelope. The protected headers contain EdDSA and address bytes; the key is OKP with EdDSA, Ed25519, and the derived public key. | Additive facade over existing owners; no duplicate crypto or COSE types. | CIP-8 facade | Sign/verify round trip and decoded-header assertions |
| `TS14-VERIFY1` | Add `CIP8Message.verifyData(addressHex, keyHash, payloadHex, signedMessage)` preserving xray-js behavior: malformed envelopes or missing/wrongly typed address, algorithm, curve, key type, public key, payload, or signature throw; well-typed address, key-hash, EdDSA algorithm, Ed25519 curve, OKP key type, or payload mismatches return `false`; otherwise return Ed25519 verification. String comparisons remain exact. | Additive; existing COSE decoding and signature behavior remains unchanged. | CIP-8 facade | Success, tampering, invalid-header, wrong-key, and mismatch tests |
| `TS14-TYPE1` | Export the signed-message envelope type containing `signature: string` and `key: string`. | Additive type export. | CIP-8 focused entry point and aggregate facade | Typecheck and packed type consumer |
| `TS14-AGG1` | Export `CIP8Message` and its envelope type through `@xray-network/xray-cardano-lib` by identity. | Additive aggregate API with CIP-8 as sole owner. | Runtime CIP and root facades | Aggregate identity/import and packed-package tests |

## Implementation steps

1. Implement the facade in the CIP-8 owner using core hex utilities, existing COSE models/builders,
   and crypto-owned keys and signatures.
2. Preserve the exact malformed-input versus semantic-mismatch boundary from xray-js.
3. Export the facade and envelope type from the focused CIP-8 entry point and aggregate runtime.
4. Add round-trip, tampering, invalid-header, aggregate identity, and packed consumer coverage.
5. Update package documentation and run the complete TypeScript validation gate.

## Validation

Run from the repository root:

```sh
npm --prefix libs/typescript run typecheck
node --test libs/typescript/packages/cip/test/cip8-message.test.mjs \
  libs/typescript/packages/runtime/test/imports.test.mjs
npm --prefix libs/typescript run check
git diff --check
```

## Compatibility and human review

Reviewers must confirm the generated headers and key labels, exact string-comparison behavior,
throw-versus-false boundary, use of the crypto owners by identity, and aggregate export identity.

## Completion criteria

- `signData` produces an envelope accepted by `verifyData` for the same address, key hash, and
  payload.
- Payload/signature/key/address tampering fails, and malformed typed headers follow the preserved
  xray-js error boundary.
- `CIP8Message` and its envelope type are available from the focused CIP-8 and aggregate packages.
- Focused, aggregate, typecheck, packed-package, and complete workspace checks pass.

## Out of scope

- Changing low-level COSE model or Ed25519 behavior.
- Accepting detached payloads, non-EdDSA algorithms, non-Ed25519 curves, or non-OKP keys.
- Changing xray-js itself.
- Adding a C++ facade for this TypeScript application integration.

## Blockers

None.
