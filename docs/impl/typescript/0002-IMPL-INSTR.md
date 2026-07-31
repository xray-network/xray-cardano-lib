# TypeScript implementation 0002 instruction

Implementation-Version: v1
Implementation-ID: typescript/0002
Created: 20260727T084625Z
Evidence-Mode: DIRECT
Depends-On: NONE
Provider-Evidence: ../../providers/message-signing/0001-message-signing/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`0001-message-signing`](https://github.com/xray-network/cardano-lib/blob/main/updates/providers/message-signing/0001-message-signing/SNAPSHOT.md) | `PROVIDER` | Yes | CIP-8 wire and signing behavior |

## Objective

Add the signing portion of EMURGO Message Signing 1.1.0 as a native TypeScript `cip8` domain owned
by `@xray-network/cardano-cip`, including COSE Sign/Sign1 models, protected and unprotected
headers, signature structures, signing builders, public COSE keys, detached payloads, external AAD,
Blake2b-224 payload hashing, and `cms_` user-facing encoding. Reuse Cardano Lib's existing
lossless CBOR and Ed25519 owners and keep the published graph free of Rust, WASM, native code, and
new dependencies.

## Comparison sources

- Previous Message Signing snapshot: `NONE`.
- Earlier upstream release `1.0.1` at commit
  `4351607f3339d105d10d783f540d35d1f44fed1c`.
- Current Cardano Lib `@xray-network/cardano-core` CBOR implementation,
  `@xray-network/cardano-crypto` key/hash implementation, `@xray-network/cardano-cip` domain and
  subpath conventions, runtime facade, browser checks, and packed-consumer checks.

## Confirmed upstream delta

This initial provider snapshot captures the complete upstream signing evidence at release `1.1.0`.
The wire model consists of integer-or-text labels; ordered header maps; byte-encoded protected
headers; three-element `COSESignature`; four-element `COSESign1` and `COSESign`; four- or
five-element `SigStructure`; null for detached payloads; a COSE key map; and untagged signed-message
serialization.

The candidate fixes the `1.0.1` builders so `hash_payload()` acts once when the payload is not yet
hashed. `COSESign1Builder` also changes its unprotected text header `"hashed"` from `false` to
`true`; the hashed payload is Blake2b-224. The upstream example confirms that callers sign the
serialized `SigStructure`, not the raw payload, and reconstruct the same structure for verification.

The release also renames the Rust crate and updates binding/build dependencies. Those packaging
changes are evidence only: Cardano Lib will use no upstream Rust, WASM, ASM.js, or upstream runtime
dependency.

## Changes to implement

| Language | Change ID | Requirement | Compatibility | Local owner | Validation | Reason |
| --- | --- | --- | --- | --- | --- | --- |
| `typescript` | `C001` | `REQUIRED` | `compatible` | `libs/typescript/packages/cip/src/cip8/` | `npm --prefix libs/typescript run check` | Initial message-signing baseline |

## Captured artifacts

| Artifact | Upstream source and revision | Preservation and integrity | License | Intended consumers |
| --- | --- | --- | --- | --- |
| `../../providers/message-signing/0001-message-signing/artifacts/upstream/README.md` | `README.md` at `f76a82442594c8435fb577cb85da3ad594cf1063` | Byte-exact; listed in `../../providers/message-signing/0001-message-signing/artifacts/SHA256SUMS` | MIT | Scope and usage evidence |
| `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/Cargo.toml` | `rust/Cargo.toml` at the source commit | Byte-exact; checksum recorded | MIT | Version, dependency, and artifact-policy review |
| `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/src/lib.rs` | `rust/src/lib.rs` at the source commit | Byte-exact; checksum recorded | MIT | Public signing models and behavior |
| `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/src/builders.rs` | `rust/src/builders.rs` at the source commit | Byte-exact; checksum recorded | MIT | Builders, label enums, public COSE key behavior |
| `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/src/serialization.rs` | `rust/src/serialization.rs` at the source commit | Byte-exact; checksum recorded | MIT | Exact COSE map/array/null/tag and malformed-input behavior |
| `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/src/cbor.rs` | `rust/src/cbor.rs` at the source commit | Byte-exact; checksum recorded | MIT | Generic-value comparison with the existing core owner |
| `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/src/utils.rs` | `rust/src/utils.rs` at the source commit | Byte-exact; checksum recorded | MIT | Integer, decoding, and byte-serialization comparison |
| `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/src/crypto.rs` | `rust/src/crypto.rs` at the source commit | Byte-exact; checksum recorded | MIT | Blake2b-224 and FNV-1a behavior |
| `../../providers/message-signing/0001-message-signing/artifacts/upstream/rust/src/error.rs` | `rust/src/error.rs` at the source commit | Byte-exact; checksum recorded | MIT | Duplicate, missing, malformed, and tag error behavior |
| `../../providers/message-signing/0001-message-signing/artifacts/upstream/examples/rust/src/main.rs` | `examples/rust/src/main.rs` at the source commit | Byte-exact; checksum recorded | MIT | End-to-end Sign1/AAD/Ed25519 compatibility test |
| `../../providers/message-signing/0001-message-signing/artifacts/legal/LICENSE` | `LICENSE` at the source commit | Byte-exact; checksum recorded | MIT | Legal review |
| `../../providers/message-signing/0001-message-signing/artifacts/SHA256SUMS` | Snapshot-local control artifact | Deterministic inventory of all eleven byte-exact artifacts | Repository metadata | Preparation and implementation integrity checks |

## Cardano Lib change map

| Upstream change | Evidence | Local owner | Required implementation | Compatibility | Tests |
| --- | --- | --- | --- | --- | --- |
| Integer/text COSE labels and lists | `lib.rs`, `serialization.rs`, `utils.rs` | New `libs/typescript/packages/cip/src/cip8/model.ts`; public `LabelKind`, `Label`, and `Labels` | Store labels as `bigint` or string, accept the existing core `Int` compatibility value for `Label.new_int`, clone values at boundaries, preserve list order, and encode/decode through core CBOR | Reject non-integer/non-text labels and out-of-range CBOR integers; no duplicate nominal integer owner | Label round trips, bigint boundaries, wrong kinds, collection cloning and bounds |
| Standard and extension header parameters | `lib.rs` `HeaderMap`; `serialization.rs` header map codec | `libs/typescript/packages/cip/src/cip8/model.ts`; public `HeaderMap` | Support standard labels 1 through 7 with their upstream types and arbitrary integer/text extension values backed by core `CborValue`; preserve decoded pair order and encoding metadata, reject duplicate semantic labels, and expose `header`, `set_header`, `keys`, and typed accessors | Header bytes are signature-sensitive; decoded noncanonical maps must round-trip in preserve mode while canonical output sorts canonical key bytes | Every standard header, text/negative extensions, insertion order, duplicate keys, wrong value kinds, definite/indefinite and canonical/preserved cases |
| Protected header bytes and combined headers | `ProtectedHeaderMap` and `Headers` in `lib.rs`/`serialization.rs` | `libs/typescript/packages/cip/src/cip8/model.ts`; public `ProtectedHeaderMap`, `Headers` | Retain the exact protected byte string after decode; empty headers use an empty byte string; nonempty protected bytes must decode as exactly one valid `HeaderMap`; mutations create a new deterministic encoding rather than changing signed bytes in place | Re-encoding protected headers changes signatures, so untouched bytes must be byte-exact and trailing embedded CBOR must fail | Empty/nonempty protected headers, nonminimal and reordered protected map preservation, invalid/trailing embedded CBOR, defensive copies |
| COSE signature and signed-message models | `COSESignature`, `COSESign1`, `COSESign`, `COSESignatures`, `CounterSignature`, `SignedMessage` in `lib.rs` and their codecs | `libs/typescript/packages/cip/src/cip8/model.ts`; corresponding public classes and `SignedMessageKind` | Implement the exact three- and four-element arrays, null detached payloads, ordered signature lists, single-vs-multiple counter-signature representation, and fourth-element discrimination between Sign1 bytes and Sign signature arrays; provide preserved and canonical CBOR byte/hex entry points following package conventions | Reject extra/truncated fields and trailing bytes; preserve valid noncanonical container and string encodings; avoid ambiguous COSESign/Sign1 decoding | Attached/detached Sign and Sign1, zero/one/multiple signatures, counter signatures, malformed lengths/types/nulls/trailing bytes, canonical/preserved mutation tests |
| Signature structure construction | `SigContext` and `SigStructure` in `lib.rs`/`serialization.rs` | `libs/typescript/packages/cip/src/cip8/model.ts`; public `SigContext`, `SigStructure` | Encode exact context strings `Signature`, `Signature1`, and `CounterSignature`; use four fields without signer-protected bytes and five with them; expose copied body/sign protected bytes, external AAD, and payload | Serialized bytes are the Ed25519 signing input; context spelling, field count, and protected bytes are consensus for verification | Exact four-/five-field CBOR, all contexts, external AAD, invalid context and lengths, deterministic sign/verify using existing crypto |
| Sign and Sign1 builders | `builders.rs`, including the post-1.0.1 hashing fix | `libs/typescript/packages/cip/src/cip8/builders.ts`; public `COSESignBuilder`, `COSESign1Builder` | Clone inputs; default external AAD to empty; support attached/detached payload selection; make the correct `SigStructure`; make `hash_payload()` idempotently replace the payload with `blake2b224`; initialize Sign1 unprotected `"hashed"` to `false` and set it to `true` on hashing exactly as 1.1.0 | Double hashing or signing the raw payload breaks interoperability; Sign and Sign1 differ in context and final signature shape | Upstream example flow, attached/detached payloads, empty/nonempty AAD, one- and two-call hashing, exact 28-byte digest and `"hashed"` transitions |
| COSE key model and Ed25519 public-key builder | `COSEKey`, `EdDSA25519Key`, and label enums in `lib.rs`, `builders.rs`, `serialization.rs` | `libs/typescript/packages/cip/src/cip8/model.ts` and `builders.ts`; public `COSEKey`, `AlgorithmId`, `KeyType`, `ECKey`, `CurveType`, `KeyOperation`, `EdDSA25519Key` | Implement required key type label 1, optional labels 2-5, extension headers, EdDSA `-8`, OKP `1`, Ed25519 curve `6`, x-coordinate label `-2`, and sign/verify operations; enforce a 32-byte public key and never accept or serialize private label `-4` through the builder | Public COSE key compatibility is retained; upstream `set_private_key` is intentionally excluded to keep secret ownership in crypto and prevent accidental disclosure | Exact Ed25519 COSE key map, optional operations, extension headers, missing key type, duplicate labels, invalid public-key length, assertion that builder output has no `-4` |
| Signed-message `cms_` encoding | `SignedMessage` in `lib.rs`, FNV in `crypto.rs`, embedded test vector in `lib.rs` | `libs/typescript/packages/cip/src/cip8/user-facing.ts` used by `SignedMessage` | Encode `cms_` plus unpadded base64url body and four-byte big-endian FNV-1a-32 checksum; decode the upstream accepted padded/unpadded forms; verify prefix, minimum size, base64url, checksum, and strict signed-message body | Text form is checksum-protected, not cryptographically authenticated; malformed padding or body must not be silently accepted | Exact upstream `cms_hE...` vector and its three padding forms, round trip, wrong prefix/checksum/base64/body, short input |
| Public package surface | Upstream public signing types plus current CIP/runtime conventions | `libs/typescript/packages/cip/src/cip8/index.ts`, `libs/typescript/packages/cip/src/index.ts`, `libs/typescript/packages/cip/package.json`, `libs/typescript/packages/runtime/src/cip/index.ts`, `libs/typescript/packages/runtime/src/index.ts` | Add `@xray-network/cardano-cip/cip8`, the `cip.cip8` namespace, and named runtime facade exports; re-export the existing core `Int` and crypto `Ed25519Signature` and `PublicKey` from the CIP-8 subpath by identity, but keep `PrivateKey` at its existing crypto/runtime entry points | Existing exports remain unchanged; all facades must share class identity and the browser graph gains no dependency | `imports.test.mjs`, `api-contract.test.mjs`, browser-package dependency rules, ESM/NodeNext/bundler packed-consumer smoke tests |

## Implementation steps

1. Create `libs/typescript/packages/cip/src/cip8/model.ts` with the label/header/key/signature/signing-structure
   state models and lossless core-CBOR codecs. Give every decoded public model preserved and
   canonical CBOR byte/hex methods consistent with existing CIP domains, enforce complete-input
   decoding, clone byte arrays and returned nominal values, and route recursive limits through
   `decodeCbor`.
2. Create `libs/typescript/packages/cip/src/cip8/user-facing.ts` with browser-safe base64url and FNV-1a-32 helpers,
   then connect `SignedMessage.from_user_facing_encoding` and `to_user_facing_encoding` to strict
   signed-message decoding.
3. Create `libs/typescript/packages/cip/src/cip8/builders.ts` with Sign/Sign1 builders and the public-only
   Ed25519 COSE key builder. Use the existing `blake2b224`, key, and signature owners; do not add a
   dependency or expose private-key bytes through COSE keys.
4. Create `libs/typescript/packages/cip/src/cip8/index.ts`, add the `./cip8` package export and `cip.cip8`
   namespace, and add explicit runtime facade re-exports while preserving nominal class identity.
5. Add `libs/typescript/packages/cip/test/cip8.test.mjs` covering the captured upstream example and user-facing
   vector, exact wire shapes, signing/verification, hashing, detached payloads, header/key
   behavior, preserved/canonical CBOR, boundaries, malformed input, defensive copies, and private
   key exclusion.
6. Extend `libs/typescript/packages/runtime/test/imports.test.mjs`, `api-contract.test.mjs`,
   `browser-package.test.mjs`, and `pack-smoke.mjs` for the new subpath, facade identities,
   universal ESM graph, browser safety, type consumption, and an installed-package Sign1
   sign/verify flow.

## Validation

- Run `npm --prefix libs/typescript run build`.
- Run `node --test libs/typescript/packages/cip/test/cip8.test.mjs`.
- Run `node --test libs/typescript/packages/runtime/test/imports.test.mjs libs/typescript/packages/runtime/test/api-contract.test.mjs libs/typescript/packages/runtime/test/browser-package.test.mjs`.
- Run `node libs/typescript/packages/runtime/test/pack-smoke.mjs`.
- Run the TypeScript completion command `npm --prefix libs/typescript run check`.
- Expected evidence: exact captured vectors pass; preserved/canonical and malformed cases pass;
  facade identities match; no new dependency, Node built-in/global, Rust, WASM, native addon, or
  unintended packed artifact appears.

## Compatibility and human review

- Wire compatibility depends on the exact protected-header byte string, context text, detached
  payload choice, external AAD, and Sign-vs-Sign1 builder context. Review these boundaries as
  security-sensitive signing input construction.
- Cardano Lib intentionally adds lossless preserved decoding and strict complete-input/resource
  limits beyond the upstream implementation. Constructed canonical output remains deterministic.
- The upstream generic CBOR wrapper classes, WASM error/lifecycle API, and `BigNum` are not copied;
  existing core owners remain authoritative.
- `EdDSA25519Key.set_private_key` is intentionally not implemented. Human review must confirm the
  public-only COSE key boundary before release.
- The upstream MIT license is compatible with the repository license and is retained in the
  snapshot. Published Cardano Lib packages contain owned TypeScript, not the captured Rust.
- A stable security-sensitive release still requires the independent review described by ADR 0002.

## Completion criteria

- `@xray-network/cardano-cip/cip8` and the runtime facade expose the planned signing API with
  shared nominal identities and no regression to existing exports.
- Sign1 and Sign builders produce and reconstruct the exact `SigStructure` bytes needed for
  existing Cardano Lib Ed25519 signing and verification, including external AAD, detached payloads,
  and idempotent Blake2b-224 hashing.
- Header, key, signature, signed-message, and signature-structure codecs pass focused positive,
  boundary, malformed, preserved, and canonical tests.
- The captured `cms_` vector and example key/message/AAD flow pass.
- No private key is serialized by the COSE key builder, no new dependency is added, and
  `npm --prefix libs/typescript run check` passes.

## Observed but excluded

- COSEEncrypt0, COSEEncrypt, COSERecipient(s), PasswordEncryption tag 16, PubKeyEncryption tag 96,
  and upstream comments about future encryption
- Generic `CBORValue`, `CBORArray`, `CBORObject`, `CBORSpecial`, `TaggedCBOR`, `BigNum`, and
  upstream deserializer/runtime wrapper APIs already covered by stronger Cardano Lib owners
- The `binaryen` gitlink, generated bindings, package/build/release scripts, CI, lockfiles, and
  standalone empty `rust/src/tests.rs`
- Release-only crate rename, wasm-bindgen, serde-wasm-bindgen, and mobile/package publication work

## Out of scope

- COSE encryption or decryption
- Importing upstream Rust, WASM, ASM.js, native code, generated JavaScript, or dependencies
- A high-level wallet/provider API or changes to the CIP-30 mock interface
- Changing existing Ed25519 signing, key derivation, generic CBOR, or lossless ledger behavior
- Serializing private key material in COSE keys

## Blockers

None.
