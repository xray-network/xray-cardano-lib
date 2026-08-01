# TypeScript implementation 0016 instruction

Implementation-Version: v1
Implementation-ID: typescript/0016
Created: 20260801T090457Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| Current user instruction | `LOCAL` | Yes | Authoritative CIP-4 ownership boundary: the library consumes canonical specification input and the SDK adapts xpub inputs |
| Adjacent xray-js `packages/cardano-sdk/src/libs/cip4/` working-tree implementation | `LOCAL` | Yes | Read-only semantic reference for existing checksum image, text identifier, and return-field compatibility |
| `docs/adr/typescript/0003-upstream-evidence-and-package-ownership.md` | `LOCAL` | Yes | CIP package ownership, universal ESM, dependency direction, and aggregate identity rules |
| `libs/typescript/packages/crypto/` | `LOCAL` | Yes | Existing reviewed Blake2b dependency and hash primitive owner |
| `libs/typescript/packages/cip/` | `LOCAL` | Yes | CIP-4 focused owner, exports, documentation, and tests |
| `libs/typescript/packages/runtime/` | `LOCAL` | Yes | Aggregate export and packed-consumer owner |

## Objective

Add a browser-safe CIP-4 wallet-checksum facade that calculates the checksum from the canonical
lowercase hexadecimal public-key-hash input, while leaving xpub decoding and derivation to SDK
consumers.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `TS16-HASH1` | Add the personalized Blake2b-512 primitive required by CIP-4 through the existing crypto owner and reviewed Noble dependency. | Additive crypto primitive; existing hash functions remain unchanged. | Crypto primitives | CIP-4 known-vector test and crypto/browser gates |
| `TS16-CIP1` | Add `CIP4.calculateChecksum(publicKeyHashHex)` for a canonical 28-byte lowercase hexadecimal key hash, returning `{ checksumId, checksumImage }` using personalized Blake2b-512 and 32-bit FNV-1a text reduction. | Additive facade; rejects noncanonical inputs instead of accepting arbitrary strings or xpub containers. | Focused CIP-4 package | Known vector, output-shape, determinism, and invalid-input tests |
| `TS16-TYPE1` | Export the checksum result type from the focused and aggregate packages. | Additive structural type. | CIP-4 and aggregate entry points | Typecheck and packed declaration consumer |
| `TS16-AGG1` | Export `CIP4` through the CIP root namespace and `@xray-network/xray-cardano-lib` by identity, without duplicate implementations. | Additive identity-preserving exports. | CIP and runtime entry points | Focused/root/aggregate identity and packed import tests |
| `TS16-DOC1` | Document CIP-4 ownership and the canonical-input versus SDK-xpub adapter boundary. | Documentation-only clarification. | CIP README and TypeScript ownership ADR | Documentation review and mirror/navigation checks |

## Implementation steps

1. Extend the crypto primitive owner with personalized Blake2b-512 support.
2. Implement the CIP-4 checksum image and text identifier from a strict lowercase 28-byte key-hash
   hexadecimal input.
3. Export the facade and result type from `./cip4`, the CIP namespace root, and the aggregate
   runtime by identity.
4. Add canonical vector, malformed-input, aggregate identity, and packed consumer coverage.
5. Document that SDKs decode/derive xpubs and call the canonical library API with the resulting key
   hash.
6. Run focused, typecheck, browser/lint, full workspace, and packed-package validation.

## Validation

Run from the repository root:

```sh
npm --prefix libs/typescript run typecheck
npm --prefix libs/typescript run build
node --test libs/typescript/packages/cip/test/cip4.test.mjs \
  libs/typescript/packages/runtime/test/imports.test.mjs
npm --prefix libs/typescript run lint
npm --prefix libs/typescript run test
npm --prefix libs/typescript run check
git diff --check
```

## Compatibility and human review

Reviewers must confirm the personalized Blake2b input is the UTF-8 encoding of the canonical
lowercase key-hash hexadecimal string, the image is lowercase Blake2b-512 hexadecimal, FNV-1a and
byte ordering match CIP-4, and xpub parsing is absent from the canonical facade.

## Completion criteria

- `CIP4.calculateChecksum()` matches the committed canonical key-hash vector.
- Non-lowercase, non-hex, and wrong-length inputs are rejected.
- The focused CIP-4, CIP namespace, and aggregate exports share one binding.
- The checksum type is available to TypeScript consumers.
- Browser, focused, full-workspace, and packed-package checks pass.

## Out of scope

- Accepting Bech32 xpubs directly in the xray-cardano-lib CIP-4 facade.
- Owning SDK account derivation or xpub validation in the CIP package.
- Generating blockies images from the checksum image seed.
- Changing unrelated CIP, key-derivation, or checksum behavior.

## Blockers

None.
