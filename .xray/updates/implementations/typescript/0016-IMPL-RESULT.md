# TypeScript implementation 0016 result

Result-Version: v1
Implementation-ID: typescript/0016
Instruction: ./0016-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `TS16-HASH1` | Implemented | Added `blake2b512(data, personalization?)` through the crypto primitive owner, including the Blake2b 16-byte personalization bound. | The CIP-4 known vector, crypto dependency policy, browser scan, workspace tests, and packed consumers passed. |
| `TS16-CIP1` | Implemented | Added `CIP4.calculateChecksum(publicKeyHashHex)`, accepting exactly a lowercase 28-byte hexadecimal key hash and producing the personalized Blake2b-512 checksum image plus the FNV-1a-derived text identifier. | The canonical key-hash vector produces `XPPX-4012` and the expected 128-character image; uppercase, wrong-length, non-hex, xpub, and empty inputs are rejected. |
| `TS16-TYPE1` | Implemented | Exported the readonly `CIP4Checksum` result type from the focused CIP-4 entry point and aggregate package. | Workspace typecheck and packed NodeNext/bundler type consumers passed. |
| `TS16-AGG1` | Implemented | Added the focused `./cip4` package export, the `cip4` root namespace, and the aggregate `CIP4` binding by identity. | Focused/root/aggregate identity tests and packed ESM imports passed. |
| `TS16-DOC1` | Implemented | Documented CIP-4 ownership and the canonical key-hash versus SDK xpub-adapter boundary in the CIP README and TypeScript ownership ADR. | Documentation navigation, implementation mirrors, JSON parsing, and diff checks passed. |

## Outcome

XRAY Cardano Lib now owns the canonical CIP-4 calculation as
`CIP4.calculateChecksum(publicKeyHashHex)`. It hashes the UTF-8 lowercase hexadecimal key-hash
string with personalized Blake2b-512, returns the lowercase image seed, and derives the human text
identifier with 32-bit FNV-1a and CIP-4 byte ordering. Application SDKs remain responsible for
turning an xpub into that canonical input.

The adjacent xray-js Cardano SDK integration was updated accordingly: its existing public
`utils.account.checksum(xpubKey)` boundary parses the xpub with `Bip32PublicKey`, derives the raw
public-key hash, and delegates to the aggregate `CIP4` facade. The previous local algorithm was
removed and the deterministic fixture now records the canonical `XPPX-4012` result.

## Inputs consumed

- The current user instruction
- The adjacent xray-js CIP-4 implementation and account checksum tests
- `docs/adr/typescript/0003-upstream-evidence-and-package-ownership.md`
- Existing crypto primitives and reviewed Noble Blake2b dependency
- Existing CIP and aggregate package exports, tests, and packed consumer harness

No provider evidence or external implementation result was consumed.

## Project changes

- Added personalized Blake2b-512 support to the crypto primitive owner.
- Added `libs/typescript/packages/cip/src/cip4/index.ts` with `CIP4` and `CIP4Checksum`.
- Added the focused `@xray-network/xray-cardano-lib-cip/cip4` entry point.
- Added CIP root namespace and aggregate runtime exports by identity.
- Added canonical vector, invalid-input, namespace, aggregate, browser-subpath, and packed consumer
  coverage.
- Documented CIP-4 ownership and the application xpub-adapter boundary.
- Updated the adjacent xray-js CIP-4 adapter and checksum fixture without changing its public
  `utils.account.checksum(xpubKey)` signature.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `TS16-HASH1` | Crypto exposes personalized Blake2b-512 with a required 16-byte personalization value when supplied. | Additive; existing Blake2b-224/256 functions are unchanged. | CIP owners can reuse the crypto primitive rather than importing a second hash implementation. |
| `TS16-CIP1` | `CIP4.calculateChecksum(publicKeyHashHex)` accepts a canonical lowercase 28-byte key-hash string and returns `{ checksumId, checksumImage }`. | Additive canonical API; arbitrary strings and xpubs are intentionally rejected. | Derive or decode the application input before calling the facade. |
| `TS16-TYPE1` | `CIP4Checksum` describes the readonly text identifier and image-seed fields. | Additive type export. | Replace local checksum result interfaces with the owned type. |
| `TS16-AGG1` | The focused CIP package, CIP namespace root, and aggregate package expose the same `CIP4` binding. | Identity-preserving additive export. | Import from the focused path or aggregate facade as appropriate. |
| `TS16-DOC1` | The CIP package owns canonical checksum calculation; SDKs own xpub parsing and public-key-hash derivation. | Clarifies package responsibility without moving key types. | Keep application-specific containers outside the CIP facade. |

## Validation

The following xray-cardano-lib checks passed from the repository root:

```sh
npm --prefix libs/typescript run typecheck
npm --prefix libs/typescript run build
node --test libs/typescript/packages/cip/test/cip4.test.mjs \
  libs/typescript/packages/cip/test/index.test.mjs \
  libs/typescript/packages/runtime/test/imports.test.mjs
npm --prefix libs/typescript run lint
npm --prefix libs/typescript run check
git diff --check
```

- Focused CIP-4, namespace, and aggregate suite: 5 passed.
- Browser/dependency lint suite: 3 passed.
- Complete built TypeScript suite: 149 passed.
- Packed suite: 534 intended files; ESM runtime plus NodeNext and bundler consumers passed.

The adjacent xray-js integration also passed:

```sh
yarn prettier --write packages/cardano-sdk/src/libs/cip4/index.ts \
  packages/cardano-sdk/test/__test.ts
yarn workspace @xray-network/xray-js-cardano typecheck
yarn workspace @xray-network/xray-js-cardano test
yarn workspace @xray-network/xray-js-cardano build
```

- Cardano SDK deterministic suite: 44 passed.
- Cardano SDK ESM, CJS, and declaration builds passed.

## Deviations from instruction

None.

## Remaining human review

Confirm the strict canonical input, Blake2b personalization, FNV-1a byte ordering, focused and
aggregate ownership, and xray-js derivation boundary, then decide whether this result should move
from `REVIEW` to `ACCEPTED`.

## Reproducibility

From the xray-cardano-lib root with Node.js 20.19 or newer and locked dependencies installed, run
`npm --prefix libs/typescript run check`. With the adjacent workspace linked, run the xray-js
Cardano SDK typecheck, test, and build commands shown above.
