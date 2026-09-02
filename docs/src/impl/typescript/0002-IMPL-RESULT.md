# TypeScript implementation 0002 result

Result-Version: v1
Implementation-ID: typescript/0002
Instruction: ./0002-IMPL-INSTR.md
Evidence-Mode: DIRECT

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C001` | `IMPLEMENTED` | `libs/typescript/packages/cip/src/cip8/` | `npm --prefix libs/typescript run check` |

## Outcome

Implemented the EMURGO Message Signing 1.1.0 signing surface as native TypeScript owned by
`@xray-network/cardano-cip/cip8`. The implementation provides COSE Sign/Sign1 models, headers,
protected-header bytes, signature structures, signature collections, public COSE keys, Sign and
Sign1 builders, detached payloads, external AAD, idempotent Blake2b-224 payload hashing, and the
checksum-protected `cms_` user-facing encoding.

The CIP package owns every new nominal class. The runtime facade and CIP namespace re-export
those bindings without wrappers. Existing core CBOR, integer, Ed25519, public-key, signature, and
Blake2b owners are reused, and no production dependency was added.

## Artifacts consumed

- `../../providers/message-signing/0001-message-signing/artifacts/upstream/README.md`
- `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/Cargo.toml`
- `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/src/lib.rs`
- `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/src/builders.rs`
- `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/src/serialization.rs`
- `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/src/cbor.rs`
- `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/src/utils.rs`
- `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/src/crypto.rs`
- `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/src/error.rs`
- `../../providers/message-signing/0001-message-signing/artifacts/upstream/examples/rust/src/main.rs`
- `../../providers/message-signing/0001-message-signing/artifacts/legal/LICENSE`
- `../../providers/message-signing/0001-message-signing/artifacts/SHA256SUMS`

All eleven byte-exact upstream artifacts passed the authoritative SHA-256 inventory before and
after implementation. The captured source identity remains EMURGO Message Signing tag `1.1.0`,
commit `f76a82442594c8435fb577cb85da3ad594cf1063`, under the captured MIT license.

## Project changes

- Added `libs/typescript/packages/cip/src/cip8/model.ts` with package-owned label, header, key, signature,
  signed-message, and signature-structure models backed by lossless core CBOR.
- Added `libs/typescript/packages/cip/src/cip8/builders.ts` with Sign, Sign1, and public-only Ed25519 COSE key
  builders. `EdDSA25519Key.set_private_key` and builder-created private label `-4` remain excluded.
- Added `libs/typescript/packages/cip/src/cip8/user-facing.ts` with browser-safe base64url and FNV-1a-32
  user-facing encoding.
- Added the `@xray-network/cardano-cip/cip8` public subpath, `cip.cip8` namespace, TypeScript
  path mapping, and explicit runtime facade exports.
- Added focused owner tests for captured vectors, signing, verification, hashing, detached
  payloads, protected headers, exact wire shapes, public keys, malformed inputs, defensive copies,
  preserved CBOR, and canonical CBOR.
- Extended cross-package identity, lifecycle-method, browser graph, package-subpath, ESM,
  NodeNext, bundler, and packed-consumer tests.

Decoded objects preserve valid original CBOR, including protected-header bytes, non-minimal heads,
indefinite containers, and map order. Canonical serialization is explicit and independent.
Complete-input decoding, duplicate-header rejection, core resource limits, and defensive byte
ownership apply throughout.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C001` | Add CIP-8 COSE Sign/Sign1 models, protected and unprotected headers, signature structures, public COSE keys, detached payloads, external AAD, idempotent Blake2b-224 payload hashing, and `cms_` encoding. | Compatible additive API with signature-sensitive wire behavior. | Reproduce the semantic and wire behavior in the downstream library, declare it not applicable, or consume the provider evidence directly for an independent implementation. |

## Validation

| Check | Result | Evidence |
| --- | --- | --- |
| `npm --prefix libs/typescript run build` | PASS | Universal ESM TypeScript project build completed |
| `node --test libs/typescript/packages/cip/test/cip8.test.mjs` | PASS | 8 focused CIP-8 tests passed |
| `node --test libs/typescript/packages/runtime/test/imports.test.mjs libs/typescript/packages/runtime/test/api-contract.test.mjs libs/typescript/packages/runtime/test/browser-package.test.mjs` | PASS | 7 facade, API, and browser-package tests passed |
| `node libs/typescript/packages/runtime/test/pack-smoke.mjs` | PASS | 526 intended files; ESM, NodeNext, and bundler consumers passed |
| `npm --prefix libs/typescript run check` | PASS | All 137 workspace tests and packed-consumer checks passed |
| `shasum -a 256 -c SHA256SUMS` | PASS | All eleven captured upstream artifacts matched |
| `git diff --check` | PASS | No whitespace errors |

## Deviations from plan

None.

## Remaining human review

- Confirm the signature-sensitive treatment of protected-header bytes, contexts, external AAD,
  and detached payloads.
- Confirm the intentional public-only `EdDSA25519Key` builder boundary.
- This implementation has automated security evidence but no independent security audit; the
  ADR 0002 stable-release review requirement remains.

## Reproducibility

Implementation used only the captured snapshot artifacts. It did not fetch, refresh, execute, or
substitute upstream material, and it introduced no upstream Rust, WASM, ASM.js, native artifact,
generated binding, or new dependency.
