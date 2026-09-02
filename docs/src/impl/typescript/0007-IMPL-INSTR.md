# TypeScript implementation 0007 instruction

Implementation-Version: v1
Implementation-ID: typescript/0007
Created: 20260731T075521Z
Evidence-Mode: HYBRID
Depends-On: ./0001-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`typescript/0001`](./0001-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted package identities, CML compatibility baseline, and browser boundary |
| [`0001-cardano-cips`](https://github.com/xray-network/xray-cardano-lib/blob/main/.xray/updates/providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md) | `PROVIDER` | Yes | Immutable CIP-0014 definition and eight official vectors |
| `artifacts/upstream/CIP-0014/README.md` in the declared snapshot | `PROVIDER` | Yes | Exact digest, concatenation, HRP, and vector semantics |
| `libs/typescript/packages/crypto/`, `chain/`, `cip/`, and `runtime/` | `LOCAL` | Yes | Existing hash, `ScriptHash`, `AssetName`, CIP, and aggregate owners |

## Objective

Add the active CIP-0014 user-facing asset fingerprint as a small, strict API that reuses existing
policy and asset-name owners and adds only the missing Cardano-owned Blake2b-160 primitive.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C001` | Add browser-native `blake2b160(bytes)` returning exactly 20 bytes | Additive cryptographic primitive | `libs/typescript/packages/crypto/src/primitives/`, crypto exports/tests | Primitive vectors, length, copy, browser, dependency policy |
| `C002` | Add strict `AssetFingerprint` construction and parsing for CIP-0014 | Additive CIP API | `libs/typescript/packages/cip/src/cip14/`, CIP tests | All eight official vectors and malformed encodings |
| `C003` | Publish focused, package-root, and aggregate identity-preserving exports | Additive exports | CIP manifest/barrels, runtime CIP facade/tests, READMEs | Import, identity, browser, and packed-consumer checks |

## Required semantics

- `blake2b160(message)` is Blake2b with a 20-byte digest, not truncation of Blake2b-256. It must
  defensively read input and return a fresh `Uint8Array`.
- `AssetFingerprint.from_parts(policyId, assetName)` must accept the existing 28-byte
  `ScriptHash` policy owner and 0-to-32-byte `AssetName` owner, hash the exact concatenation
  `policyId.raw || assetName.raw`, and own the 20-byte result.
- `AssetFingerprint.from_bech32(text)` must require Bech32 (not Bech32m), HRP exactly `asset`,
  decoded length exactly 20, a valid checksum, and no mixed case. Canonical `to_bech32()` output
  is lowercase with HRP `asset`.
- Expose `to_raw_bytes()` as a defensive copy and equality by digest bytes. Do not claim that a
  fingerprint is collision-free or reversible.
- Do not introduce new policy-ID or asset-name nominal types. `AssetFingerprint` has one owner:
  `@xray-network/xray-cardano-lib-cip/cip14`.
- Add `./cip14` to `@xray-network/xray-cardano-lib-cip`; export namespace `cip14` from its root; re-export
  the exact same binding through the runtime CIP facade and aggregate runtime.

## Implementation steps

1. Add `blake2b160` beside the existing Cardano-owned Blake2b primitives using the already pinned
   browser-compatible cryptography dependency.
2. Implement `AssetFingerprint` in a focused `cip14` owner with immutable private state.
3. Wire focused/root/runtime exports without recreating or subclassing the nominal binding.
4. Document the display-oriented collision model and input ownership.
5. Add official, boundary, malformed, identity, browser, and packaging tests.

## Validation

- Assert all eight captured CIP-0014 vectors, including empty, non-UTF-8-sized, and 32-byte asset
  names.
- Independently assert `blake2b160` against captured vector inputs and verify it differs from
  naive truncation of Blake2b-256.
- Reject wrong HRPs, 19/21-byte payloads, bad checksums, Bech32m, mixed case, invalid policy
  lengths, and asset names above 32 bytes.
- Prove input/output mutation cannot alter a fingerprint.
- Assert strict equality of the focused, package-root, runtime-CIP, and aggregate bindings.
- Run `node --test libs/typescript/packages/crypto/test/*.test.mjs
  libs/typescript/packages/cip/test/*.test.mjs
  libs/typescript/packages/runtime/test/imports.test.mjs
  libs/typescript/packages/runtime/test/browser-package.test.mjs`.
- Run `npm --prefix libs/typescript run check`.

## Compatibility and human review

This is additive. Review the digest configuration, exact policy/name concatenation, strict HRP
validation, dependency graph, and binding identity. The security documentation must present the
fingerprint as a user-facing comparison aid with a 160-bit digest, not an authorization primitive.

## Completion criteria

- `blake2b160` is browser-native, deterministic, and returns exactly 20 owned bytes.
- All captured CIP-0014 vectors pass through construction and strict parsing.
- Existing `ScriptHash` and `AssetName` bindings remain the only component owners.
- Focused, root, runtime, and packed-consumer exports resolve to one `AssetFingerprint` identity.
- No runtime provider artifact or new dependency is shipped.
- `npm --prefix libs/typescript run check` passes.
- The paired result names the exact CIP artifact consumed and exports a portable semantic change
  contract.

## Out of scope

- Asset registries, reverse lookup, token discovery, or provider APIs
- Visual checksums, icons, or UI components
- Network-specific fingerprints or configurable HRPs
- Using fingerprints as ledger identities or security authorization

## Blockers

None.
