# Native cryptography enhancement analysis

Date: 2026-09-25. Audited revision: `e099bc937defd92542c436aece929691c3e19f8e`.
Scope: the maintained TypeScript library, its Cardano consumers, Node.js, browsers and Cloudflare Workers.
Status: analysis and development proposal; no production cryptography changed.

## 1. Recommendation

Use a **hybrid provider design**. Keep the current TypeScript/Noble implementation as the portable reference and fallback. Add native operations individually, with explicit capability and compatibility checks.

The strongest initial candidates are **PBKDF2-HMAC-SHA512, SHA-256/SHA-512, HMAC-SHA512, and signing with normal 32-byte Ed25519 seeds**. Desktop Node also offers SHA3-256, RIPEMD-160, unpersonalized Blake2b-512 and ChaCha20-Poly1305. Workers offers a narrower set; its `node:crypto` ChaCha20-Poly1305 worked in this audit.

**Ed25519 verification is promising but is not currently a compatible direct substitution.** Actual probes found that native Node and Workers accept identity-public-key signatures that XRAY rejects. A compatible adapter must preserve the existing verification policy before the native speed measurements can justify replacement.

Keep these in owned TypeScript/Noble:

- Cardano Blake2b-160/224/256 and personalized Blake2b-512.
- Extended Ed25519 signing, Ed25519 point/scalar operations and Cardano BIP32 derivation logic.
- Plutus secp256k1 ECDSA verification, BIP340 Schnorr, Keccak-256 and BLS12-381 operations.
- Key formats, CBOR, addresses, signing payloads, derivation paths, validation and disposal rules.

**Modern supported Node, browsers and Workers do not need a general crypto polyfill.** Missing algorithms should use the existing Noble implementation. Adding a `crypto` or `Buffer` shim neither supplies native acceleration nor guarantees Cardano-compatible semantics.

## 2. Existing boundaries and repository constraints

The external crypto dependencies are already concentrated in the crypto package and pinned to `2.2.0`: `@noble/curves`, `@noble/hashes`, and `@noble/ciphers`. The library currently uses `@noble/curves` for Ed25519; the standalone `@noble/ed25519` package was a benchmark comparison, not a production dependency.

| Owning source | Existing responsibility | Appropriate enhancement boundary |
|---|---|---|
| [Primitive wrappers](../../packages/crypto/src/primitives/crypto.ts) | Hashes, HMAC/PBKDF2, Ed25519, secp256k1 and ChaCha20-Poly1305 | Select individual compatible kernels here; preserve validation and result types |
| [Randomness](../../packages/crypto/src/primitives/random.ts) | Injectable secure randomness backed by `globalThis.crypto.getRandomValues` | Already native; retain fail-closed behavior |
| [Ed25519 and BIP32 keys](../../packages/crypto/src/keys/ed25519.ts) | Normal/extended keys, derivation, signing and owned secret buffers | Preserve classes and Cardano algorithms; introduce explicit async entry points where needed |
| [Cardano key roles and paths](../../packages/crypto/src/keys/cardano.ts) | Role encodings and CIP-1852 helpers | Keep TypeScript; these are protocol operations |
| [EMIP-3](../../packages/crypto/src/encryption/emip3.ts) | Password KDF, AEAD and encrypted-message layout | Native KDF; optionally native AEAD, with identical bytes and authentication behavior |
| [Byron](../../packages/crypto/src/byron/) | Legacy derivation/signing and proxy-certificate payloads | Partial hash/KDF acceleration; retain legacy/domain-separation rules |
| [UPLC cryptography](../../packages/crypto/src/primitives/uplc.ts) | BLS groups, hashing to groups, compression and pairing operations | Retain Noble and owned wrappers |
| [Plutus machine](../../packages/plutus/src/uplc/machine.ts) | Synchronous execution and builtin costing | No implicit Promise-returning replacement of builtins |

[CONTRIBUTING.md](../../../../CONTRIBUTING.md) prohibits published code from importing `node:*`, using `Buffer`/`require`, or shipping native/WASM artifacts. [ADR 0002](../../../../docs/src/adr/typescript/0002-cryptography-dependency-policy.md) explicitly permits standards-based Web Crypto. [ADR 0004](../../../../docs/src/adr/typescript/0004-cryptography-primitives.md) preserves Cardano semantics in owned wrappers.

Consequently:

1. A portable Web Crypto provider fits the existing architectural direction, subject to normal implementation review and tests.
2. A `node:crypto` provider can live in the consuming `cardano-node-js` application and be injected through a future narrow interface. The current library does not yet expose that interface.
3. Publishing a Node-specific adapter from this library would require a documented policy change. A conditional export alone does not exempt it from the current prohibition.
4. Experimental Web Crypto extensions also need an explicit decision before becoming a supported library backend. Their availability in one Node version is insufficient.

“TypeScript using runtime-provided native crypto” remains binary-free at the package level, but it does not mean every cryptographic calculation executes in JavaScript.

## 3. Platform and operation matrix

**Yes** means the operation is available and is a candidate; it does not waive compatibility tests. **Gated** means a known semantic/API obstacle prevents direct substitution. Node and Workers observations below were tested locally as described in section 8. Browser coverage is documentation-based, not a browser test result.

| Operation used by XRAY | Node `node:crypto` | Node Web Crypto | Browser Web Crypto | Workers Web Crypto | Workers `node:crypto` |
|---|---|---|---|---|---|
| Secure random bytes | Yes; existing global API sufficient | Already used | Already used | Already used | Yes; unnecessary extra route |
| SHA-256 / SHA-512 | Yes, synchronous or host scheduling | Yes, async | Yes, async | Yes, async | Yes |
| HMAC-SHA512 | Yes | Yes | Yes | Yes | Yes |
| PBKDF2-HMAC-SHA512 | Yes, sync or callback API | Yes | Yes | Yes | Yes |
| Normal Ed25519 seed signing | Yes | Yes | Gated: support and deterministic-output policy | Yes | Yes |
| Ed25519 verification, including signatures produced by extended keys | Gated: acceptance differs | Gated: acceptance differs | Gated: acceptance must be tested | Gated: acceptance differs | Gated: acceptance differs |
| Public key from a normal seed | Yes, import private key then derive public key | No uniform baseline extraction API; retain existing path initially | Same portability issue | Retain existing path initially | Candidate via Node-compatible key APIs; separately qualify |
| Extended Ed25519 signing / extended public key | No compatible complete operation | No | No | No | No |
| Cardano BIP32 root/child derivation | KDF/HMAC portions only | KDF/HMAC portions only | KDF/HMAC portions only | KDF/HMAC portions only | KDF/HMAC portions only |
| Blake2b-160 / 224 / 256 | No compatible API in tested build | No | No portable API | No | No in tested runtime |
| Unpersonalized Blake2b-512 | Yes | No portable API | No portable API | No | Unsupported in tested runtime |
| Personalized Blake2b-512 | No compatible parameter API | No | No | No | No |
| SHA3-256 | Yes in tested build | Node 24.7+ extension; tested with experimental warning | No supported cross-browser baseline | Unsupported in test | Unsupported in test |
| Keccak-256 | No suitable algorithm in tested build | No portable API | No portable API | No | No suitable algorithm in test |
| RIPEMD-160 | Yes in tested build | No | No portable API | No | Unsupported in test |
| ChaCha20-Poly1305 for EMIP-3 | Yes | Node 24.7+ extension; special import format | No supported cross-browser baseline | Unsupported in test | Yes; ciphertext/tag and authentication tests passed |
| Plutus secp256k1 ECDSA | Gated: incompatible digest and low-S behavior | No secp256k1 API | No standard secp256k1 API | No secp256k1 API | Tested JWK curve import rejected; no qualified replacement |
| BIP340 Schnorr | No matching public API | No | No | No | No matching public API |
| BLS12-381 groups / hash-to-curve / pairings | No matching public API | No | No | No | No matching public API |
| VRF / KES consensus algorithms | No complete replacement | No | No | No | No complete replacement |

The public API references are [Node crypto](https://nodejs.org/download/release/v24.18.0/docs/api/crypto.html), [Node Web Crypto](https://nodejs.org/download/release/v24.18.0/docs/api/webcrypto.html), [Workers Web Crypto](https://developers.cloudflare.com/workers/runtime-apis/web-crypto/), and [Workers Node compatibility](https://developers.cloudflare.com/workers/runtime-apis/nodejs/crypto/). Availability of `createHash` or `createPublicKey` does not establish support for every OpenSSL algorithm or curve.

The package's declared Node floor is `>=20.19.0`. The Node 24 SHA-3/ChaCha extensions must therefore be optional even when Node is the only deployment target. Node 24.18.0 is the tested version here, not a proposed silent increase of the package minimum.

Browser compatibility data lists Ed25519 from Chrome 137, Firefox 129 and Safari 17. Detect the exact operation at runtime; browser brand alone is insufficient. Web Crypto requires a secure browser context, and is also available in Web Workers. [Browser compatibility dataset](https://raw.githubusercontent.com/mdn/browser-compat-data/main/api/SubtleCrypto.json).

## 4. Compatibility findings that control the design

### 4.1 Ed25519: verification and signing have different replacement boundaries

XRAY's `verifyEd25519` calls Noble with `{ zip215: false }`. It catches invalid encodings and returns `false`; `verifyEd25519Uplc` first enforces public-key/signature lengths and can throw. Both boolean results and errors are part of the contract.

The installed Noble implementation decodes points canonically, checks the scalar range, rejects small-order public keys and uses a cofactored verification equation. A native API carrying the same algorithm name is not evidence of identical acceptance.

Actual results for the same message bytes:

| Public key and signature | Current XRAY | Node native | Node Web Crypto | Workers native | Workers Web Crypto |
|---|---|---|---|---|---|
| Normal XRAY-generated key/signature | Accept | Accept | Accept | Accept | Accept |
| Signature from XRAY extended BIP32 key | Accept | Accept | Accept | Accept | Accept |
| Tampered normal signature | Reject | Reject | Reject | Reject | Reject |
| Identity public key, identity R, S = 0 | Reject | **Accept** | **Accept** | **Accept** | **Accept** |
| Identity public key, base-point R, S = 1 | Reject | **Accept** | **Accept** | **Accept** | **Accept** |
| All-zero encoded public key, identity R, S = 0 | Reject | Reject | Reject | Reject | Reject |

These are verification-policy differences, not a claim that the current XRAY implementation is vulnerable. They also do not establish which policy a particular Cardano ledger era requires; that needs differential testing against the authoritative Cardano implementation.

**Decision: do not replace the verifier by directly forwarding to native crypto.** Qualify an adapter using canonical/noncanonical encodings, all small-order points, mixed-order points, scalar boundaries, malformed lengths and historical ledger/Plutus vectors. Rejecting only the single identity encoding does not solve the general problem. Conversely, imposing a new blanket prime-subgroup restriction could reject inputs the existing verifier accepts.

An adapter may combine validation with native verification and deliberately route a defined input class to the reference implementation. That must be justified and tested in both acceptance directions. Never implement “try every provider until one accepts.” If a compatible and fast adapter cannot be demonstrated, retain Noble verification.

Normal signing is simpler: XRAY's 32-byte normal private key is an Ed25519 **seed**. Node and Workers native signing matched the current deterministic signature bytes in the positive test. Importing a seed through standard PKCS#8 and a public key through raw Web Crypto/SPKI formats worked.

However, Safari's documented Ed25519 signing uses randomized signatures. Those signatures can be valid while differing byte-for-byte from current deterministic output. Preserve deterministic signing with Noble on such a backend, or introduce a separately documented signing mode after protocol review. Do not treat valid-but-different signatures as an interchangeable implementation of the current deterministic API. [Safari behavior in the compatibility dataset](https://raw.githubusercontent.com/mdn/browser-compat-data/main/api/SubtleCrypto.json).

The key distinctions are:

| XRAY material | Meaning | Native standard Ed25519 treatment |
|---|---|---|
| Normal private key: 32 bytes | Seed expanded internally by Ed25519 | Native signing candidate |
| Extended private key: 64 bytes | Expanded `kL || kR` | Cannot import as a normal seed or truncate to one |
| BIP32 private key: 96 bytes | `kL || kR || chainCode` | Keep Cardano derivation/signing |
| Extended public key | Ordinary Ed25519 point, accompanied by chain code in BIP32 form | Its raw 32-byte point can verify ordinary Ed25519 signatures natively, subject to acceptance policy |

XRAY's `extendedSign` computes the nonce from `kR`, hashes `R || A || message`, and performs scalar arithmetic using `kL`. Web Crypto does not expose the required expanded-key signing operation. SHA-512 portions could be accelerated in a future async implementation; the point/scalar work remains TypeScript.

For transaction witnesses, pass the exact Cardano signing payload to ordinary Ed25519. Do not add a second application-level hash or substitute Ed25519ph. The normal/extended signing distinction also applies to Byron and CIP signing paths.

### 4.2 Blake2b is the largest hashing gap that native APIs do not close

`blake2b160`, `blake2b224` and `blake2b256` set the digest length inside Blake2b's parameter block. Truncating a Blake2b-512 output produces a different hash. The digest-size parameter is part of the algorithm. [RFC 7693](https://www.rfc-editor.org/rfc/rfc7693.html).

Observed on Node 24.18.0:

- `createHash('blake2b512')` matched XRAY's unpersonalized 64-byte result.
- `{ outputLength: 32 }` failed with `not XOF or invalid length`.
- An invented `{ digestLength: 32 }` option was silently ignored and returned 64 bytes.
- An invented `personalization` option was silently ignored and produced the unpersonalized result.

Therefore, no native substitution is justified for transaction IDs, datum hashes, script hashes or key/address hashes using Blake2b-224/256. Blake2s-256 is a different algorithm too.

[CIP-4](../../packages/cip/src/cip4/index.ts) uses **personalized** Blake2b-512, so even the available 512-bit native variant cannot replace that call. Dispatch must distinguish personalization, not just the function name or output length.

### 4.3 SHA-2, SHA-3, Keccak and RIPEMD

SHA-256/SHA-512 are straightforward kernel candidates after vector and input-view tests. Their callers may still be synchronous, so Web Crypto requires an async boundary or continued use of the synchronous fallback.

Native desktop Node SHA3-256 and RIPEMD-160 matched this audit's known answers. Workers rejected both through its Web Crypto and Node-compatible APIs. Keccak-256 and SHA3-256 have different domain padding and are not substitutes.

[Byron addresses](../../packages/chain/src/era/byron/address.ts) compose SHA3-256 with Blake2b-224 over encoded data. Accelerating the SHA3 step does not eliminate either the Blake2b or CBOR implementation.

### 4.4 PBKDF2 and HMAC: useful acceleration without replacing BIP32

Preserve the exact existing KDFs:

| Caller | Password/key material | Salt | Iterations | Output |
|---|---|---|---|---|
| Icarus/BIP32 root | Caller password bytes; empty is valid | Entropy bytes | 4,096 | 96 bytes, followed by XRAY normalization |
| EMIP-3 | Password bytes; encryption currently rejects empty | 32-byte salt | 19,162 | 32 bytes |
| Child derivation | Chain code for HMAC-SHA512 | Domain-tagged key/point and little-endian child-index payload | Not PBKDF2 | HMAC output split according to owned derivation rules |

Node and Workers passed both PBKDF2 cases, including the **empty password** root case, through native and Web Crypto APIs. HMAC-SHA512 also matched. Web Crypto `deriveBits` takes a length in **bits**, whereas the current internal helper takes bytes.

Do not replace the root algorithm with generic BIP39 seed derivation, change password normalization, exchange entropy/password arguments, use HKDF, or change iteration counts. Hardened/soft derivation, clamping, scalar additions and chain-code handling remain owned TypeScript.

Native PBKDF2 is a good first async feature because it offloads a substantial complete operation. Making every small HMAC in a dependent derivation path asynchronous can add overhead; measure an entire account/address derivation rather than isolated HMAC throughput.

### 4.5 EMIP-3: compatible primitives, precise byte layout

The current wire layout is:

```text
salt[32] || nonce[12] || authenticationTag[16] || ciphertext
```

Retain PBKDF2-HMAC-SHA512, the 32-byte key, IETF ChaCha20-Poly1305, the 12-byte nonce, 16-byte tag and empty additional authenticated data. AES-GCM is not an EMIP-3 replacement.

Desktop Node and Workers `createCipheriv('chacha20-poly1305', ...)` matched XRAY ciphertext/tag, decrypted successfully, and rejected a modified tag. Buffer all decrypted output until authentication succeeds: an API may produce bytes before its final authentication check, and failure can occur in `update` or `final` depending on the runtime.

Node 24.18 Web Crypto also matched, using **`importKey('raw-secret', ...)`**, not `'raw'`, and returned `ciphertext || tag`. Reorder that result when constructing EMIP-3. Node reported this algorithm as experimental; Workers rejected the algorithm in Web Crypto. [Node extension documentation](https://nodejs.org/download/release/v24.18.0/docs/api/webcrypto.html).

In browsers, an async native PBKDF2 followed by synchronous Noble ChaCha is a useful partial enhancement. No AEAD polyfill is required because the library already owns the fallback dependency.

Preserve wrapper error behavior and secret cleanup. The existing decrypt wrapper also rejects a payload of exactly the metadata length; any change to empty-plaintext round trips should be handled explicitly, not accidentally introduced during backend selection.

### 4.6 Plutus secp256k1, Schnorr and BLS stay portable

XRAY's ECDSA verifier consumes a **prehashed 32-byte message**, a 33-byte compressed secp256k1 key and a 64-byte compact signature, with `lowS: true` and `prehash: false`.

For a signature over SHA-256(message), desktop Node testing showed:

| Native call | Result |
|---|---|
| `verify('sha256', originalMessage, p1363KeyOptions, signature)` | True |
| `verify('sha256', messageDigest, p1363KeyOptions, signature)` | False: the digest is hashed again |
| `verify(null, messageDigest, p1363KeyOptions, signature)` | False: not a compatible raw-digest verifier |
| Verify the high-S counterpart using original message | True, unlike XRAY's low-S policy |

Selecting IEEE-P1363 encoding solves only signature layout. It does not solve digest semantics or low-S acceptance. Keep the existing implementation and its distinct UPLC malformed-input errors.

BIP340 uses its own x-only public-key and signature rules; an ECDSA or Ed25519 API cannot replace it. BLS12-381 needs group operations, subgroup/encoding checks, hash-to-curve domain separation, Miller loops and final verification, beyond merely a generic signature API. Keep those existing kernels and their wrappers.

VRF and KES names/byte wrappers are not complete consensus implementations in this library. Native Ed25519 availability does not supply Cardano VRF or KES; their implementation remains separate work in [NODE-PLAN.md](NODE-PLAN.md).

## 5. Polyfills and deployment configuration

| Environment | Needed for current portable library | Needed for proposed native path | Fallback |
|---|---|---|---|
| Supported modern Node | No crypto/Buffer polyfill; ESM and secure global randomness | Web Crypto for async path; application-owned `node:crypto` adapter for synchronous native operations | Current Noble |
| Modern browser over HTTPS / trusted localhost | No Node shims; supported ESM, BigInt, typed arrays and text encoding | Operation-specific Web Crypto detection; secure context | Current Noble for missing algorithms and deterministic signing |
| Browser Web Worker | Same portable code, using `globalThis`, without a `window` assumption | Same Web Crypto checks | Current Noble in the worker |
| Older browser/WebView | Existing runtime/dependency requirements still apply | Missing Ed25519 can use Noble; lack of required JS features or secure randomness is a separate unsupported-environment issue | Do not promise support through a crypto shim |
| Cloudflare Worker using Web Crypto | No Node compatibility flag or polyfill required | Use `crypto.subtle` and `crypto.getRandomValues` | Current Noble |
| Cloudflare Worker importing `node:crypto` | Not required by the current library | Runtime Node compatibility configuration; separate host adapter | Fall back per operation; desktop Node algorithm coverage does not carry over |
| Test runner / restricted embedding without global crypto | Inject an actual secure random source using the existing boundary | Inject a real `SubtleCrypto` provider for async tests where available | Deterministic test fixtures; fail if production randomness is unavailable |

Cloudflare's current documentation says compatibility dates **2026-08-04 or later enable Node compatibility by default**. For earlier dates, opt in with `nodejs_compat`; the full v2 compatibility baseline uses a date of at least 2024-09-23. Existing flags may remain when moving to a newer date. This is deployment configuration, not a JavaScript crypto polyfill. [Workers Node compatibility](https://developers.cloudflare.com/workers/runtime-apis/nodejs/), [crypto-specific notes](https://developers.cloudflare.com/workers/runtime-apis/nodejs/crypto/).

This audit also ran SHA-256, Ed25519 verification, PBKDF2 and secure randomness in `workerd` with compatibility date `2026-08-03`, **no Node flag, no `node:*` imports and no Buffer usage**. All passed.

Do not add `crypto-browserify`, a global `Buffer`, or a replacement `crypto.subtle` solely to enable this design. Explicitly retaining Noble makes unsupported operations visible and avoids masking a missing native implementation. No fallback may manufacture entropy with `Math.random`.

## 6. Proposed provider architecture

Keep existing synchronous APIs stable. Introduce explicitly asynchronous operations for workflows that benefit from Web Crypto, such as root-key derivation, EMIP-3 and transaction-witness verification. Do not change a return type to `T | Promise<T>` based on the environment.

Suggested boundaries, not existing APIs:

```text
Existing synchronous XRAY API
  -> owned Cardano validation and algorithms
  -> current synchronous Noble primitives

New explicit async workflow
  -> owned Cardano validation and algorithms
  -> qualified operation in Web Crypto provider
  -> explicit Noble fallback for unsupported operation/input class

cardano-node-js / Worker application
  -> optional injected host provider using node:crypto
  -> same owned validation and compatibility contract
```

Provider selection must be **per operation and parameter set**: deterministic seed signing is different from verification; personalized Blake2b differs from unpersonalized; a curve name does not describe a verification policy.

Use these implementation rules:

1. Initialize and freeze a provider for a context. Avoid process-wide mutable switches that could change behavior between concurrent transactions.
2. Detect API presence, then run small known-answer/import/verification checks. Cache the capability result for the runtime. Capability checks do not replace the complete release vector suite.
3. Treat an unsupported algorithm as a reason to select the reference provider. A false signature result or authentication failure is a result, not a reason to search for a more permissive backend. Backend exceptions that indicate an unexpected implementation failure must remain visible.
4. Preserve public nominal classes and byte interfaces. Do not expose Noble points, Node `KeyObject` or Web Crypto `CryptoKey` as replacement public Cardano key types.
5. Preserve byte offsets and lengths of typed-array views. Copy mutable input bytes before an async operation can observe later caller mutation. Avoid transferring/detaching buffers still owned by a key or transaction object.
6. Bound public-key caches and key them by complete key bytes, algorithm, provider and validation policy. Measure fresh-key traffic as well as cache hits. Avoid an unbounded cache populated by peer-supplied keys.
7. Prefer non-extractable native secret keys after import when export is unnecessary. Do not enable export just to simplify public-key extraction. Define disposal and in-flight async behavior; native opaque keys have no portable explicit zeroization API.
8. Wipe owned temporary secret bytes on both success and failure. A native provider does not upgrade JavaScript's best-effort zeroization into a guarantee.
9. Bound async concurrency. A Promise-returning API is not proof of parallel execution or additional Workers CPU entitlement.

The synchronous Plutus interpreter should retain its existing primitive path initially. Suspending every hash/signature builtin requires an interpreter design change, and measured wall-clock acceleration must never alter ledger cost-model accounting, evaluation order or deterministic results.

## 7. Performance: established measurements and missing measurements

Previous [Ed25519 benchmark results](node-bench/results/2026-09-25-ed25519-m4-max.json) used the same 64 normal-key fixtures, 32-byte messages, one active execution thread, and seven measurement windows on an Apple M4 Max. These are primitive benchmarks, not a complete node measurement.

| Backend | Verify operations/s | Verify, Rust = 100% | Normal sign operations/s | Sign, Rust = 100% |
|---|---:|---:|---:|---:|
| Rust Dalek, prepared keys | 49,608 | 100.00% | 124,585 | 100.00% |
| Node `crypto`, prepared keys | 27,544 | 55.52% | 67,507 | 54.19% |
| Standalone `@noble/ed25519` 3.2.0 | 2,187 | 4.41% | 8,417 | 6.76% |
| Current XRAY wrappers / `@noble/curves` 2.2.0 | 1,065 | 2.15% | 5,424 | 4.35% |

Node's prepared-key verification was about 12.59 times the standalone Noble benchmark and 25.86 times the current XRAY wrapper benchmark. With a fresh Node public-key import each operation, verification fell to 15,727/s. Key representation, parsing, caching and wrapper work differ across these paths.

**The native verification number excludes the compatibility adapter now shown to be necessary.** It cannot be quoted as a ready-to-ship XRAY speedup. Rust's strict verifier is also not automatically the same acceptance policy as XRAY's verifier.

Web Crypto throughput, deployed Workers throughput, full witness-pipeline speedup and PBKDF2/EMIP-3 backend speedups were **not measured by this audit**. The tests in section 8 establish capabilities and selected output compatibility only. They do not establish production readiness or sustained performance.

Next benchmarks should include 0/32/128/1,024/65,536-byte messages where valid, fresh and cached keys, realistic witness counts, valid and invalid inputs, sequential awaits and bounded concurrency, cold start, p50/p95 latency, allocations and complete transaction processing. Measure the final validation adapter, not just native calls. Small SHA/HMAC calls can lose their benefit to import, copy and Promise overhead.

## 8. Capability checks actually run for this report

| Item | Value |
|---|---|
| Audit machine | Apple M4 Max, macOS arm64 |
| Node | `v24.18.0`, OpenSSL `3.5.7` |
| Cloudflare local runtime | `@cloudflare/workerd-darwin-arm64@1.20260924.1`; binary reports `workerd 2026-09-24` |
| Main Worker configuration | Compatibility date `2026-09-24`, no explicit compatibility flags |
| Web-only Worker configuration | Compatibility date `2026-08-03`, no compatibility flags or Node imports |
| Fixture source | Existing built XRAY crypto package and its installed Noble dependencies; deterministic public test material |
| Execution | Owned local scripts, Node and `workerd test`; no deployed Worker or real wallet keys |

Fixture recipe: UTF-8 message `XRAY native crypto capability audit`; normal seed = 32 bytes of `0x07`; extended root from 16 entropy bytes of `0x03` and an empty password; EMIP-3 password = UTF-8 `public-test-password`, salt = 32 bytes of `0x09`, nonce = 12 bytes of `0x04`. These fixed values are test data, not a production randomness example.

Known answers were generated through the current library boundary and compared with native results. Therefore, the positive tests establish agreement with this revision; they are not an independent proof of Cardano conformance.

| Check | Node result | Local Workers result |
|---|---|---|
| Native SHA-256, SHA-512, HMAC-SHA512 | Match | Match |
| Native SHA3-256, RIPEMD-160, unpersonalized Blake2b-512 | Match | All rejected as unsupported digests |
| Native PBKDF2, EMIP-3 parameters | Match | Match |
| Native PBKDF2, empty-password 96-byte root parameters | Match | Match |
| Native ChaCha ciphertext, tag, round trip, modified-tag rejection | All pass | All pass |
| Native normal Ed25519 signing and normal/extended-signature verification | Positive cases match | Positive cases match |
| Web Crypto SHA-256/512, HMAC, both PBKDF2 cases | Match | Match |
| Web Crypto normal signing and normal/extended-signature verification | Positive cases match | Positive cases match |
| Web Crypto SHA3-256 | Match; experimental warning | Unsupported |
| Web Crypto ChaCha via `raw-secret` | Match; round trip and tag rejection pass; experimental warning | Unsupported |
| Web Crypto ChaCha via `raw` | Import rejected | Import rejected |
| Web Crypto RIPEMD-160, Blake2b-256, secp256k1 curve import | Unsupported | Unsupported |
| Identity-public-key verification cases | Native differs from XRAY | Native differs from XRAY |

The Node-compatible secp256k1 test on Workers failed at JWK curve import. This establishes failure of that proposed path, not an exhaustive test of every possible key encoding. Likewise, a local `workerd` pass is not a deployed Cloudflare compatibility or performance guarantee.

### Reproduce the most consequential finding

From the repository root after the TypeScript package has been built, run this diagnostic with `node --input-type=module`. Imports from `node:*` here are host diagnostic code, not proposed published library code.

```js
import { createPublicKey, verify } from 'node:crypto';
import { verifyEd25519 } from
  './libs/typescript/packages/crypto/dist/esm/index.js';

const message = new TextEncoder().encode('XRAY native crypto capability audit');
const publicKey = new Uint8Array(32);
publicKey[0] = 1; // Encoded identity point.
const signature = new Uint8Array(64);
signature[0] = 1; // Identity R followed by S = 0.
const spkiPrefix = Uint8Array.from([
  0x30, 0x2a, 0x30, 0x05, 0x06, 0x03, 0x2b, 0x65, 0x70, 0x03, 0x21, 0x00,
]);
const der = new Uint8Array(spkiPrefix.length + publicKey.length);
der.set(spkiPrefix);
der.set(publicKey, spkiPrefix.length);
const nodeKey = createPublicKey({ key: der, format: 'der', type: 'spki' });
const webKey = await crypto.subtle.importKey(
  'raw', publicKey, 'Ed25519', false, ['verify'],
);
console.log({
  xray: verifyEd25519(publicKey, message, signature),
  node: verify(null, message, nodeKey, signature),
  web: await crypto.subtle.verify('Ed25519', webKey, signature, message),
});
// Observed: { xray: false, node: true, web: true }
```

## 9. End-to-end implementation plan

This sequence is proposed future work. Completion of this analysis does not mean the providers or async APIs have been implemented.

| Step | Concrete deliverable | Acceptance gate |
|---|---|---|
| 1. Freeze behavioral contract | Inventory all call sites; record key formats, deterministic outputs, errors and provider policy in an ADR | Separate current XRAY behavior from intended Cardano ledger/Plutus semantics; resolve differences explicitly |
| 2. Preserve and expand differential fixtures | Own the capability harness and deterministic fixtures as test tooling; add independent upstream positive/negative vectors | Identity, small-order, mixed-order, noncanonical, scalar-boundary and malformed-input coverage; no dependence on current mutable upstream branches |
| 3. Add provider boundaries | Narrow byte-based interfaces, immutable context selection and operation-specific capability results | Existing synchronous API, nominal types and browser-safe package graph unchanged; unsupported-operation and failure paths tested |
| 4. Ship the first async workflow | Native PBKDF2 for root derivation and EMIP-3; retain owned normalization and Noble AEAD where necessary | Exact 96/32-byte KDF outputs, empty root password, Unicode byte inputs, round trips and cleanup across success/failure |
| 5. Add deterministic normal signing | Native seed signing where output policy is qualified; explicit reference fallback elsewhere | Independent Ed25519 vectors, exact signature bytes where required, normal/extended key separation and Safari policy tests |
| 6. Qualify native verification | Adapter preserving current acceptance and errors, with defined fallback domains if needed | Zero unexplained differential results in either direction; authoritative Cardano comparison; benchmark includes adapter costs; retain reference path if unresolved |
| 7. Add host accelerators | Application-owned Node adapter for qualified hashes, HMAC and EMIP-3; optional Workers Node adapter | Per-runtime known answers; no Blake truncation/personalization mistakes; no platform-specific imports in published portable graph |
| 8. Integrate realistic flows | Build, sign and verify transaction witnesses; address/root derivation; CIP and Byron payloads; EMIP-3 interoperation | Existing serialized bytes and witness/fee behavior preserved; cross-provider interoperability and owned-key disposal exercised |
| 9. Validate supported environments | Declared Node minimum and deployment Node; Chromium, Firefox, actual Safari/WebKit; Web Worker; pinned local and deployed Workers | Packed-package consumers run without Node polyfills; feature-unavailable fallback passes; actual browser/runtime versions recorded |
| 10. Measure and release | Real workload benchmarks, capability diagnostics and default-provider decision | Material measured improvement without behavior changes; full TypeScript gate, crypto review required by ADRs and documented rollback to reference provider |

Before completing implementation changes, run the required workspace gate:

```sh
npm --prefix libs/typescript run check
```

This report is a documentation-only change. Its Node/Workers probes were run separately; the full gate was not rerun solely for writing this document.

## 10. Evidence to pin for implementation and future upgrades

Store exact URLs, commit/release identifiers, retrieval date, artifact SHA-256, license and selected files alongside each future evidence capture. Record Node/OpenSSL versions and the actual Workers runtime as well as compatibility date. A compatibility date alone does not freeze native cryptographic implementation details.

| Evidence source | What to retain / why |
|---|---|
| [XRAY crypto sources](../../packages/crypto/src/), [tests](../../packages/crypto/test/), [lockfile](../../package-lock.json) | Current wrapper behavior, dependency integrity and known answers at the audited revision |
| [Node 24.18 crypto API](https://nodejs.org/download/release/v24.18.0/docs/api/crypto.html), [Web Crypto API](https://nodejs.org/download/release/v24.18.0/docs/api/webcrypto.html), [Node source tag](https://github.com/nodejs/node/tree/v24.18.0) | Versioned API contract, algorithm normalization, import formats and provider integration |
| [Workers Web Crypto documentation](https://developers.cloudflare.com/workers/runtime-apis/web-crypto/), [Node crypto documentation](https://developers.cloudflare.com/workers/runtime-apis/nodejs/crypto/), [workerd repository](https://github.com/cloudflare/workerd) | Capture docs and exact runtime source/release; retain local/deployed capability outputs because API-level claims do not establish algorithm parity |
| [Web Crypto specification](https://www.w3.org/TR/webcrypto/), [Secure Curves proposal](https://wicg.github.io/webcrypto-secure-curves/), [Modern Algorithms proposal](https://wicg.github.io/webcrypto-modern-algos/) | Distinguish baseline APIs, Ed25519 rules and experimental extensions; pin document revisions |
| [Browser compatibility data](https://github.com/mdn/browser-compat-data/blob/main/api/SubtleCrypto.json) | Pin a commit and relevant Ed25519 signing/import/verify entries; preserve Safari behavior notes |
| [Noble curves 2.2.0](https://github.com/paulmillr/noble-curves/tree/2.2.0), [Noble hashes](https://github.com/paulmillr/noble-hashes), [Noble ciphers](https://github.com/paulmillr/noble-ciphers) | Pin selected 2.2.0 release commits/package integrities and verification/hash/AEAD tests; separate dependency behavior from XRAY policy |
| [Wycheproof](https://github.com/C2SP/wycheproof) | Pin adversarial Ed25519/ECDSA/AEAD vectors; classify policy-sensitive cases rather than blindly copying expected outcomes |
| [RFC 8032](https://www.rfc-editor.org/rfc/rfc8032.html), [RFC 7693](https://www.rfc-editor.org/rfc/rfc7693.html), [RFC 8439](https://www.rfc-editor.org/rfc/rfc8439.html), [RFC 8410](https://www.rfc-editor.org/rfc/rfc8410.html) | Independent signature, hash, AEAD and Ed25519 key-container definitions/vectors |
| [Cardano ledger](https://github.com/IntersectMBO/cardano-ledger), [cardano-base](https://github.com/IntersectMBO/cardano-base), [Plutus](https://github.com/IntersectMBO/plutus), [CIPs](https://github.com/cardano-foundation/CIPs) | Pin revisions matching the supported node/protocol era; establish authoritative acceptance rules and Cardano payload/derivation semantics |

For every runtime or dependency upgrade, rerun the same vector suite and record any newly available algorithms or changed acceptance. Enable a newly discovered native operation only after qualification; preserve an explicit reference-provider option. This document does not create or mutate provider/lifecycle records.
