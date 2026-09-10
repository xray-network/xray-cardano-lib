# TypeScript implementation 0007 result

Result-Version: v1
Implementation-ID: typescript/0007
Instruction: ./0007-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C001` | Implemented | Added browser-native `blake2b160(data)` through the existing reviewed Noble primitive owner with `dkLen: 20`, defensive input reading, and a fresh 20-byte result. | Independent `abc` digest, result length/ownership, and non-truncation checks pass with dependency and browser scans. |
| `C002` | Implemented | Added the immutable `AssetFingerprint` owner with `from_parts`, strict `from_bech32`, canonical `to_bech32`, defensive `to_raw_bytes`, and digest equality. | All eight captured CIP-0014 vectors and malformed HRP, length, checksum, Bech32m, mixed-case, owner-bound, equality, and mutation checks pass. |
| `C003` | Implemented | Added the `./cip14` package export, CIP root `cip14` namespace, runtime CIP facade export, and aggregate binding by identity. | Focused/package-root/runtime-facade/aggregate identity, universal ESM, browser, and packed-consumer checks pass. |

## Outcome

XRAY Cardano Lib now owns the active CIP-0014 display fingerprint as
`AssetFingerprint`. Construction reuses the existing 28-byte `ScriptHash` policy owner and
0-to-32-byte `AssetName` owner, hashes their exact raw concatenation with Blake2b-160, and emits
canonical lowercase Bech32 with the `asset` HRP.

Strict parsing accepts valid single-case Bech32 only and rejects wrong HRPs, wrong digest lengths,
bad checksums, Bech32m, and mixed case. The fingerprint is documented as a user comparison aid,
not a reversible ledger identifier or authorization primitive.

## Inputs consumed

- The current user instruction
- `typescript/0001` accepted implementation result
- `cardano-cips/0001-cardano-cips` provider snapshot
- Captured `artifacts/upstream/CIP-0014/README.md`, including all eight official vectors
- Existing crypto primitive, `ScriptHash`, `AssetName`, CIP package, runtime facade, and aggregate
  owners
- Existing dependency-policy, browser, public-import, and packed-consumer validation

## Project changes

- Added and exported `blake2b160(data)` beside existing Blake2b primitives.
- Added `libs/typescript/packages/cip/src/cip14/index.ts` as the sole `AssetFingerprint` owner.
- Added the CIP `./cip14` focused export and root `cip14` namespace.
- Re-exported the same binding through the runtime CIP facade and aggregate package.
- Added official-vector, malformed-input, defensive-ownership, export-identity, browser, and
  packaging coverage.
- Documented component ownership and the display-oriented 160-bit collision/security boundary.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C001` | `blake2b160(message)` computes Blake2b configured for a 20-byte digest and returns owned bytes. | Additive; existing Blake2b variants are unchanged. | Reuse this primitive for Cardano protocols requiring Blake2b-160; do not truncate a longer digest. |
| `C002` | `AssetFingerprint.from_parts(policyId, assetName)` hashes `policy.raw || name.raw`; strict parsing and canonical formatting use HRP `asset`. | Additive; component owners remain `ScriptHash` and `AssetName`. | Replace local CIP-14 codecs while retaining policy/name as the actual ledger identity. |
| `C003` | Focused CIP-14, CIP namespace, runtime CIP facade, and aggregate imports resolve to one nominal binding. | Additive and identity-preserving. | Import from the narrowest package path appropriate to the consumer. |

## Validation

The following checks passed from the TypeScript workspace:

```sh
npm run build
node --test packages/crypto/test/*.test.mjs \
  packages/cip/test/*.test.mjs \
  packages/runtime/test/imports.test.mjs \
  packages/runtime/test/browser-package.test.mjs
npm run check
```

- Required focused crypto/CIP/runtime suite: 41 passed.
- Complete built TypeScript suite: 153 passed.
- Packed package smoke suite: 538 intended files, 2,379,336 unpacked bytes; ESM, NodeNext, and
  bundler consumers passed.

## Deviations from instruction

None.

## Remaining human review

Confirm the Blake2b digest configuration, exact concatenation order, strict Bech32 behavior,
component ownership, and display-only security wording, then decide whether this result should
move from `REVIEW` to `ACCEPTED`.

## Reproducibility

From the repository root with Node.js 20.19 or newer and locked dependencies installed, run
`npm --prefix libs/typescript run check`.
