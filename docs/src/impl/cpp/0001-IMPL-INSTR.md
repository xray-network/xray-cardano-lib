# C++ implementation 0001 instruction

Implementation-Version: v1
Implementation-ID: cpp/0001
Created: 20260730T073858Z
Evidence-Mode: HYBRID
Depends-On: ../typescript/0001-IMPL-RESULT.md, ../typescript/0002-IMPL-RESULT.md, ../typescript/0003-IMPL-RESULT.md, ../typescript/0004-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/SNAPSHOT.md, ../../providers/message-signing/0001-message-signing/SNAPSHOT.md, ../../providers/uplc/0001-uplc/SNAPSHOT.md, ../../providers/cardano-ledger/0001-cardano-ledger/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`typescript/0001`](https://github.com/xray-network/xray-cardano-lib/blob/main/.xray/updates/implementations/typescript/0001-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Frozen CML evidence ownership, historical-vector outcomes, and portable compatibility contract |
| [`typescript/0002`](https://github.com/xray-network/xray-cardano-lib/blob/main/.xray/updates/implementations/typescript/0002-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | CIP-8 COSE, signing, hashing, and user-facing encoding contract |
| [`typescript/0003`](https://github.com/xray-network/xray-cardano-lib/blob/main/.xray/updates/implementations/typescript/0003-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | UPLC, phase-two valuation, cost-model, context, and additional cryptography contract |
| [`typescript/0004`](https://github.com/xray-network/xray-cardano-lib/blob/main/.xray/updates/implementations/typescript/0004-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Official era validation and documented historical compatibility contract |
| [`0001-cardano-multiplatform-lib`](https://github.com/xray-network/xray-cardano-lib/blob/main/.xray/updates/providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/SNAPSHOT.md) | `PROVIDER` | Yes | Frozen CDDL comparison corpus, genesis vectors, and historical block vectors |
| [`0001-message-signing`](https://github.com/xray-network/xray-cardano-lib/blob/main/.xray/updates/providers/message-signing/0001-message-signing/SNAPSHOT.md) | `PROVIDER` | Yes | CIP-8 wire and signing evidence |
| [`0001-uplc`](https://github.com/xray-network/xray-cardano-lib/blob/main/.xray/updates/providers/uplc/0001-uplc/SNAPSHOT.md) | `PROVIDER` | Yes | Official UPLC, conformance, cost-model, cryptography, and phase-two evidence |
| [`0001-cardano-ledger`](https://github.com/xray-network/xray-cardano-lib/blob/main/.xray/updates/providers/cardano-ledger/0001-cardano-ledger/SNAPSHOT.md) | `PROVIDER` | Yes | Official Byron-through-Conway ledger grammar |
| `docs/adr/repository/0001-repository-architecture.md` | `LOCAL` | Yes | Independent workspace, package ownership, evidence, lifecycle, and documentation rules |

## Objective

Create a complete, independently implemented C++ Cardano library under `libs/cpp/` with semantic
feature parity to the exhaustive baseline recorded below, which was prepared from the maintained
TypeScript library and declared evidence. The implementation must cover core codecs and
collections, cryptography, Byron-through-Conway ledger models and validation, addresses, ledger
operations, transaction construction, CIP-8/CIP-25/CIP-36, typed Plutus Data, UPLC, phase-two
valuation, and an aggregate facade.

Parity means equivalent supported capabilities, accepted wire forms, canonical output, validation,
security boundaries, and observable results. It does not mean copying TypeScript source, preserving
JavaScript object design, or reproducing npm/ESM packaging. Public C++ APIs must be idiomatic,
memory-safe, independently owned, and auditable against the declared results and provider evidence.

This instruction is the complete planning record. Preparation does not create C++ implementation
source, a result record, or a C++ release.

## Authority and exhaustive requirements baseline

The four accepted TypeScript results are the portable cross-library semantic contracts. The four
provider snapshots are direct authority for the wire, vector, consensus, and compatibility
behavior within their captured scopes. A documented compatibility disposition in an accepted
result, including a deliberately retained historical exception, takes precedence over a naive
reading of a provider artifact.

The maintained TypeScript workspace at repository commit
`f4fd43348429f249be6348992de9c47be36a6202` was inspected only while this instruction was prepared,
to identify feature families and language-specific mechanics. It is not a declared implementation
input and must not be consulted during implementation. Do not copy, translate, generate from,
execute as C++ build tooling, or create a runtime dependency on TypeScript source, emitted
JavaScript, declarations, or tests.

`C001` through `C013`, the complete required feature map, every frozen semantic contract and
public-inventory row, and the required compatibility dispositions below are the exhaustive
normative baseline for this sequence. The C++ implementation result must contain or link a
reviewed crosswalk mapping every explicit requirement, inventory row and owned binding, feature-map
item, compatibility disposition, and type/JSON contract stated here to one C++ owner and a
validation reference. Only JavaScript-, TypeScript-, browser-, or npm-specific mechanics explicitly
listed in this instruction may be marked `LANGUAGE_SPECIFIC`. Later TypeScript changes, and any
capability discoverable only by inspecting TypeScript source, are outside this instruction and
require a new C++ sequence.

If the accepted results, captured provider evidence, and explicit requirements below cannot be
reconciled, stop and prepare a new instruction rather than silently selecting one behavior.

## C++ workspace and ownership contract

Create one independent CMake workspace with this minimum shape:

```text
libs/cpp/
  CMakeLists.txt
  CMakePresets.json
  vcpkg.json
  vcpkg-configuration.json
  README.md
  API_PARITY.md
  cmake/
  include/cardano/
    core/
    crypto/
    chain/
    cip/
    plutus/
    cardano.hpp
  src/
    core/
    crypto/
    chain/
    cip/
    plutus/
  tests/
    core/
    crypto/
    chain/
    cip/
    plutus/
    runtime/
```

- Require CMake 3.28 or newer and C++23, including `std::expected`, `std::span`, and `std::byte`.
- Export installable targets `cardano::core`, `cardano::crypto`, `cardano::chain`,
  `cardano::cip`, `cardano::plutus`, and aggregate `cardano::lib`.
- Preserve dependency direction: `core`; `crypto -> core`; `chain -> core, crypto`;
  `cip -> core, crypto, chain`; `plutus -> core, crypto, chain`; aggregate facade to all owners.
- Use namespace `cardano` with domain namespaces. A public nominal type has one owner. Focused
  targets and the aggregate header expose that same C++ binding by inclusion or alias, never by a
  competing wrapper, subclass, copy, or generated declaration.
- Public byte inputs use non-owning constant spans where safe; retained and returned bytes use
  owned containers with defensive-copy or explicit move semantics. Secrets are move-only RAII
  values with best-effort zeroization.
- Fallible untrusted-input operations return an owned error/result contract based on
  `std::expected`. Errors retain a stable category, structural path, byte offset when available,
  and nested cause without exposing dependency-specific error objects.
- Support macOS on ARM64 with Apple Clang. Linux, Windows, x86-64, and other compiler/platform
  combinations are deferred to later C++ implementation records. Do not promise a stable binary
  ABI in the initial `0.1.0` release.
- Provide `find_package(CardanoLib CONFIG REQUIRED)` component support, install/export metadata,
  an installed-consumer smoke project, and one documented completion command:
  `cmake --workflow --preset ci`.

All third-party dependencies are private implementation details. Pin the builtin vcpkg registry to
the immutable `2026.06.24` baseline commit
[`cd61e1e26a038e82d6550a3ebbe0fbbfe7da78e3`](https://github.com/microsoft/vcpkg/commit/cd61e1e26a038e82d6550a3ebbe0fbbfe7da78e3)
and declare exact manifest overrides. The dependency selection is:

| Source | Package | Exact selection | Enabled feature policy | License | Private use |
| --- | --- | --- | --- | --- | --- |
| Builtin baseline | `boost-multiprecision` | `1.91.0#0` | No optional features | BSL-1.0 | Arbitrary Cardano integers |
| Builtin baseline | `botan` | `3.12.0#0` | No vcpkg optional features; adapters use only required hash, PBKDF2, and ChaCha20-Poly1305 primitives | BSD-2-Clause | Hash, KDF, and AEAD |
| Builtin baseline | `libsodium` | `1.0.22#0` | No optional features | ISC | Ed25519, OS randomness, and secure wipe |
| Builtin baseline | `secp256k1` | `0.7.1#0` | Core verification plus `extrakeys` and `schnorrsig`; disable `recovery`, `ecdh`, `musig`, `ellswift`, and all other optional modules | MIT | Compact ECDSA and BIP-340 verification |
| Builtin baseline | `nlohmann-json` | `3.12.0#2` | No optional features | MIT | Private JSON parsing and emission |
| Builtin baseline | `catch2` | `3.15.1#0` | Test-only; no optional features | BSL-1.0 | Unit and integration tests |
| Checked-in overlay `libs/cpp/cmake/vcpkg-ports/blst/` | `blst` | `v0.3.17`, commit [`54e6e55674722fc2797ebb4bbb71b26d881eb4b8`](https://github.com/supranational/blst/releases/tag/v0.3.17) | Native C/assembly library only; no language bindings | Apache-2.0 | BLS12-381 |

The blst overlay port must fetch only
`https://github.com/supranational/blst/archive/54e6e55674722fc2797ebb4bbb71b26d881eb4b8.tar.gz`
and require source-archive SHA-512
`e11f4f66051de45d812f1be8539bc6fd4f703f4fb39aa5c556051ae3d7b93eb1331af37cb93d4f2e8985200037b921e6bcc848297bdc2050d2a839d5fecc99af`.
It must install the source archive's license and contain no mutable branch or tag lookup. Every
`vcpkg.json` dependency object must set
`default-features` to `false`; the manifest feature `tests` is the sole owner of Catch2 and is
enabled only by build-and-test presets.
`vcpkg-configuration.json` must name only the pinned builtin registry and checked-in blst overlay.
No dependency type may cross a public header boundary.

The required validation environment is macOS 26.5 ARM64 with Apple Clang 21.0. Require CMake 3.28
and Ninja 1.11 or newer. Apple Clang owns sanitizer validation. The result records the exact patch
versions actually used, confirms the frozen features and licenses, and identifies the primitive
supplied by each dependency; it does not select or upgrade them.

A dependency may not be floated, fetched outside the manifest, substituted, or upgraded during
this implementation. If a selected dependency or toolchain cannot satisfy the required macOS
ARM64 platform,
encoding, security, or license constraint, stop and create a new instruction.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C001` | Create the independent C++23 CMake workspace, exact dependency manifest, install/export package, documentation, and completion presets. | New C++ library; no root build proxy or TypeScript dependency. | `libs/cpp/` | Configure, build, install, and clean consumer tests on the required macOS ARM64 toolchain. |
| `C002` | Implement owned byte, number, error/result, collection, Bech32, and network primitives. | Semantic parity with the explicit core requirements using language-native C++ contracts. | `libs/cpp/include/cardano/core/`, `libs/cpp/src/core/` | Core unit, boundary, ownership, and property tests. |
| `C003` | Implement strict lossless CBOR decoding plus preserved and canonical encoding, embedded CBOR, resource limits, and metadata invalidation. | Byte-exact preservation and deterministic canonical output. | `libs/cpp/include/cardano/core/cbor/`, `libs/cpp/src/core/cbor/` | Golden, malformed, limit, mutation, duplicate-map, and canonical-convergence tests. |
| `C004` | Implement the complete cryptography, key, hash, encryption, UPLC primitive, secure-random, and Byron compatibility surface behind owned adapters. | Match all accepted formats and security-sensitive acceptance rules. | `libs/cpp/include/cardano/crypto/`, `libs/cpp/src/crypto/` | Published, provider, positive, negative, malformed, disposal, and randomness vectors. |
| `C005` | Implement Shelley-style and Byron addresses, genesis parsing, network handling, witnesses, and address compatibility behavior. | Match valid encodings, historical forms, and strict malformed-input behavior. | `libs/cpp/include/cardano/chain/address/`, `libs/cpp/src/chain/address/` | Address, genesis, base58, Bech32, CRC32, witness, and long-address vectors. |
| `C006` | Implement lossless and canonical Byron-through-Conway ledger models, JSON contracts, era validators, scripts, Plutus Data wire types, metadata, multi-assets, protocol parameters, and governance. Keep shared infrastructure and Byron-through-Conway implementation in focused modules below the public and private `chain/era/` directories; retain `era_models.hpp` only as the compatibility umbrella. | Enforce official CDDL while retaining only the accepted historical exceptions. Preserve all public names and nominal identities through focused headers and the compatibility umbrella. | `libs/cpp/include/cardano/chain/era/`, `libs/cpp/include/cardano/chain/era_models.hpp`, `libs/cpp/src/chain/era/` | Per-era positive, boundary, malformed, JSON, canonical, preserved-wire, direct-header, umbrella-identity, and official-rule tests. |
| `C007` | Implement explicit-network multi-era dispatch, common views, hashing, fees, minimum ADA, deposits/refunds, ExUnits, script-data operations, metadata/Plutus JSON conversion, and witness helpers. | Match the accepted TypeScript observable results without reproducing its object model. | `libs/cpp/src/chain/multi_era/`, `libs/cpp/src/chain/ledger/` | Historical block corpus, ledger-operation vectors, JSON, and cross-era tests. |
| `C008` | Implement complete transaction and witness construction, CIP-2 coin selection, change, mint/burn, collateral, certificates, withdrawals, governance, metadata, redeemer indexing, valuation handoff, and signing assembly. | Preserve value balance, deterministic behavior, exact signing payloads, and explicit randomness. | `libs/cpp/include/cardano/chain/builder/`, `libs/cpp/src/chain/builder/` | Builder invariants, deterministic seeds, fee/size limits, witnesses, and end-to-end fixtures. |
| `C009` | Implement CIP-8, CIP-25, and CIP-36 under one C++ CIP owner with focused headers and aggregate aliases. | Match proposal-specific wire, signing, JSON/metadata, and historical compatibility behavior. | `libs/cpp/include/cardano/cip/`, `libs/cpp/src/cip/` | Proposal vectors, signatures, malformed inputs, preserved/canonical encoding, and facade identity. |
| `C010` | Implement recursive typed Plutus Data values, schemas, schema casts, CBOR-hex conversion, JSON-like conversion, limits, and large integers. | Match the frozen Data capability and chain-owned ledger Data types without duplicate nominal owners. | `libs/cpp/include/cardano/plutus/data.hpp`, `libs/cpp/src/plutus/typed_data.cpp`, `libs/cpp/include/cardano/chain/plutus_data.hpp`, `libs/cpp/src/chain/ledger/plutus_data.cpp` | Variant, schema, bound, recursive, JSON, and round-trip tests. |
| `C011` | Implement UPLC 1.0.0/1.1.0, Flat/text/script codecs, cost models, CEK, builtins 0-100, parameter application, and raw phase-two valuation for protocol majors 5-11. | Consensus-sensitive parity with accepted UPLC and ledger contracts. | `libs/cpp/include/cardano/plutus/uplc/`, `libs/cpp/src/plutus/` | Full applicable conformance corpus, exact budgets, contexts, limits, and valuation fixtures. |
| `C012` | Implement component headers, aggregate facade, exhaustive instruction-requirement crosswalk, examples, install metadata, and ownership checks. | One nominal C++ owner per public concept; no TypeScript package or runtime dependency. | `libs/cpp/include/cardano/`, `libs/cpp/README.md`, `libs/cpp/API_PARITY.md` | Compile-only API, requirement-coverage, component, aggregate identity, and installed-consumer tests. |
| `C013` | Add integrity, hardening, fuzz/property, sanitizer, static-analysis, macOS, and release-content gates for the complete workspace. | Fail closed on malformed or adversarial inputs and prevent unreviewed artifacts or dependencies. | `libs/cpp/tests/`, `libs/cpp/CMakePresets.json`, `libs/cpp/cmake/` | Completion, sanitizer, deterministic hardening, package inventory, and Apple Clang gates. |

## Required feature map

### Core and lossless encoding

- Implement byte copying, equality, length validation, hexadecimal conversion, Bech32, an
  injectable secure-random interface, protocol-magic constants, checked signed/unsigned bounds,
  arbitrary Cardano integers, structural equality, ordered maps, duplicate-preserving pair maps,
  and nonempty vectors/maps.
- Represent all CBOR major types, every unsigned tag number, booleans, null, undefined, simple
  values `0..255`, and half/single/double floats. Give tags 2, 3, 24, 30, and 258 their required
  Cardano semantic validation without preventing lossless representation of other tags. Retain
  integer/tag head width, length width, definite/indefinite containers, byte/text chunks, source
  spans, optional tags, map order, duplicate keys, float width, and embedded-CBOR bounds.
- Freeze `DEFAULT_CBOR_LIMITS` to depth 512, collection length 1,000,000, string bytes 67,108,864,
  and tokens 2,000,000; caller overrides may only replace named limits with validated nonnegative
  values. Count every decoded node as a token and enforce the configured depth before descending.
- Preserved encoding reuses compatible decoded metadata. Canonical encoding ignores it, uses
  canonical-key byte length followed by unsigned bytewise order, canonical integer/tag/length
  heads, and the shortest exact float representation; canonical NaN is half-precision `f97e00`
  while negative zero remains distinct. Mutation invalidates only incompatible hints; unchanged
  nested values retain valid hints.
- NaNs are the explicit exception to byte-exact preserved CBOR. Preserve their decoded width, but
  payload and sign bits are not an interoperability promise; the half-float path re-emits every
  NaN as positive quiet `f97e00`, so `f97e01` and `f9fe01` both normalize to `f97e00`. Canonical
  encoding always emits `f97e00`.
- Reject trailing bytes, truncation, invalid breaks, invalid UTF-8 where text is required, depth
  overflow, token/collection/string allocation overflow, invalid bignums, and numeric range
  failures with stable error paths and offsets.

### Cryptography

- Own the 16 fixed-size Cardano hash/key wrappers, normal and extended Ed25519 keys and signatures,
  Cardano Ed25519-BIP32 root derivation, hard and soft child derivation, extended public keys,
  chain codes, supported byte forms, hex/Bech32 conversion, equality, defensive ownership, and
  move-only secret lifecycle.
- Implement Blake2b-224/256, SHA2-256, SHA3-256, Keccak-256, RIPEMD-160, Ed25519 verification,
  compact secp256k1 ECDSA and BIP-340 Schnorr verification, and all required BLS12-381 G1/G2
  arithmetic, compression, hash-to-group, Miller-loop, ML-result multiplication, and final
  verification operations.
- Preserve UPLC-specific malformed-encoding, low-S, subgroup, infinity, message-length, domain-
  separation, and failure behavior. Dependency-specific points, fields, keys, and cipher values
  remain private.
- Use injectable OS CSPRNG-backed generation and fail closed when unavailable. Never use time,
  a deterministic engine, or an undocumented fallback for production randomness. Best-effort
  zeroization occurs on destruction and explicit clearing without claiming guaranteed erasure.
- Implement EMIP-3 PBKDF2-SHA512 and ChaCha20-Poly1305 layout plus legacy Daedalus signing, ABOR,
  and Byron proxy-certificate behavior required by the chain owner.

### Chain, eras, and builders

- Implement Shelley base, pointer, enterprise, and reward addresses; Byron addresses and
  attributes; variable natural numbers; Bech32/base58; CRC32; HD payloads; stake distribution;
  bootstrap/vkey witnesses; Byron and Shelley genesis parsing; and known network constants.
- Implement complete validated wire and JSON models for Byron, Shelley, Allegra, Mary, Alonzo,
  Babbage, and Conway. Cover blocks, headers, transactions, witnesses, certificates, pools,
  relays, native/timelock scripts, metadata, values and multi-assets, Plutus scripts/Data/redeemers,
  inline data, reference inputs/scripts, cost models, protocol parameters, and Conway governance,
  DReps, committees, constitutions, votes, proposals, treasury actions, and Plutus V3.
- Dispatch explicit-network blocks across Byron epoch-boundary/main blocks and Shelley through
  Conway. Expose language-native common block/header/body/input/output/certificate/update views
  without weakening owning-era validation.
- Implement transaction, auxiliary-data, datum, script, script-data, and native-script hashes;
  script signer discovery; minimum transaction/script/reference-script fees; minimum ADA; total
  ExUnits; deposits, refunds, implicit inputs; metadata/Plutus JSON schemas; arbitrary-byte
  metadata chunking; genesis transaction IDs; and Icarus/Daedalus/vkey witness helpers.
- Implement transaction inputs/UTxOs, outputs, datums/reference scripts, mint/burn, certificates,
  withdrawals, proposals, votes, collateral, reference inputs, metadata, network/validity/treasury
  fields, witness requirements, datum/script deduplication, deterministic redeemer pointers,
  ExUnits replacement, draft transactions, checked/unchecked signing, and final witness assembly.
- Implement CIP-2 largest-first and random-improve for coin and multi-asset selection. Randomized
  selection requires an explicit injectable source in tests. Enforce balance, fee, size, value,
  minimum-ADA, collateral, and pure/native-asset change invariants.

### CIPs and Plutus

- CIP-25 covers V1 text and V2 byte asset keys, label 721 conversion, UTF-8 64-byte strings,
  chunking, file/detail/list models, metadata merge/extraction, and the accepted loose/noisy
  historical parser behavior.
- CIP-36 covers legacy and weighted delegation, nonempty bounds, registration/deregistration,
  optional voting purpose and explicit default presence, exact hashes-to-sign, witnesses, metadata
  labels, and preserved/canonical encoding. The registration compatibility check is deliberately
  narrow: it rejects a weighted-delegation registration when any delegation weight is nonzero and
  does not verify its Ed25519 witness. Deregistration exposes no corresponding compatibility-check
  method. Callers verify either signature explicitly through the crypto owner's public-key
  verification API; no CIP-36 decode, construction, or compatibility-check path performs implicit
  signature verification.
- CIP-8 covers integer/text labels, ordered headers, protected bytes, COSE Sign/Sign1/signatures/
  countersignatures, exact signature structures, detached payloads, external AAD, idempotent
  Blake2b-224 payload hashing, public-only Ed25519 COSE keys, and checksum-protected `cms_`
  encoding. COSE encryption, recipients, and private label `-4` are not added.
- Typed Data covers recursive integer, bytes, list, map, and constructor values; integer, bytes,
  boolean, any, list, tuple, object, map, literal, nullable, and enum schemas; bounds and uniqueness;
  CBOR-hex encode/decode; schema casts; JSON-like conversion; void values; large integers; and
  recursive limits. Ledger `PlutusData`, scripts, redeemers, and `ExUnits` remain chain-owned.
- UPLC covers immutable 1.0.0/1.1.0 programs, all term forms, the complete constant universe,
  De Bruijn scope, text parsing, iterative Flat and serialized-script codecs, builtin tags 0-100,
  semantics A-E, exact V1/V2/V3 current cost mapping, default machine costs, restricting CEK,
  memory and CPU accounting, Trace ordering, adversarial depth, and exact error order.
- `apply_params_to_script` is strict, bounded, left-to-right, scope-checked, complete-input, and
  emits one canonical CBOR byte-string envelope.
- Raw phase-two valuation supports protocol majors 5-11, Alonzo/Babbage/Conway contexts, Plutus
  V1/V2/V3 availability, all six redeemer purposes, witness/reference scripts, datum/UTxO
  resolution, optional collection checks, sorted pointers, independent per-redeemer maximum
  budgets, Trace logs, and returned redeemer CBOR with recalculated ExUnits.

## Frozen cryptography and address contract

### Cardano keys, text encodings, and hashes

- A Cardano Ed25519-BIP32 private key is exactly `kL[32] || kR[32] || chain_code[32]` (96
  bytes). Construction rejects any length other than 96 and requires `(kL[0] & 0x07)==0` and
  `(kL[31] & 0xe0)==0x40`. Root derivation is
  `PBKDF2-HMAC-SHA512(password_bytes, entropy_bytes, 4096, 96)`, then `kL[0]&=0xf8` and
  `kL[31]=(kL[31]&0x1f)|0x40`; mnemonic parsing is not part of this API. The random constructor
  applies the same normalization to 96 CSPRNG bytes.
- Derivation indexes are uint32 encoded little-endian. For private derivation, hardened means
  `i>=2^31`; let `M=kL||kR` when hardened and `M=A` (the 32-byte extended public key) otherwise.
  Compute `Z=HMAC-SHA512(c,(0x00 hard/0x02 soft)||M||LE32(i))` and
  `I=HMAC-SHA512(c,(0x01 hard/0x03 soft)||M||LE32(i))`; child
  `kL=(LE(kL)+8*LE(Z[0:28])) mod 2^256`, child
  `kR=(LE(kR)+LE(Z[32:64])) mod 2^256`, child chain code `I[32:64]`. An xpub is
  `A[32]||c[32]`; public derivation permits only `i<2^31` and returns
  `(A + (8*LE(Z[0:28]))B) || I[32:64]` using the 0x02/0x03 inputs. Reject negative,
  nonintegral, >uint32, hardened-xpub, and invalid-point derivations.
- The extended public key is compressed `[LE(kL) mod L]B`. Extended signing uses
  `r=LE(SHA512(kR||m)) mod L`, `R=rB`, `h=LE(SHA512(R||A||m)) mod L`,
  `S=(r+h*LE(kL)) mod L`, signature `R||LE32(S)`. Normal 32-byte keys use RFC 8032 Ed25519;
  verification is strict/non-ZIP-215 and returns false for malformed signatures/points.
  `to_128_xprv` is `kL||kR||A||c`; `from_128_xprv` accepts exactly 128 bytes, keeps bytes 0:64
  and 96:128, and deliberately does not validate bytes 64:96 against the derived public key.
- Fixed Bech32 HRPs and lengths are: normal secret `ed25519_sk`/32, extended secret
  `ed25519e_sk`/64, public `ed25519_pk`/32, signature `ed25519_sig`/64, xprv `xprv`/96, xpub
  `xpub`/64. Reject a mismatched HRP, checksum, mixed case, padding, or length; output lowercase.
  Hex input is even-length case-insensitive and output is lowercase. Returned bytes and chain codes
  are copies; secret values are move-only and best-effort wiped.
- The 16 fixed wrappers are 28 bytes for `Ed25519KeyHash`, `ScriptHash`, `GenesisDelegateHash`,
  `GenesisHash`; 32 bytes for `TransactionHash`, `AuxiliaryDataHash`, `PoolMetadataHash`,
  `VRFKeyHash`, `BlockBodyHash`, `BlockHeaderHash`, `DatumHash`, `ScriptDataHash`, `VRFVkey`,
  `KESVkey`, `NonceHash`, `AnchorDocHash`. They compare bytewise and defensively copy. Their
  Bech32 form has caller-selected HRP: decode validates Bech32 but does not require or retain a
  particular HRP. `PublicKey::hash` is Blake2b-224 of its 32 compressed bytes.
- The required BIP32 regression uses entropy
  `000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f` and UTF-8 password
  `cardano`; it must produce xprv
  `28bcf7f6439e62f304f589619f6b1612f9a984978b445e4ec6f59e595c051150fa47864365423db7ed7a117cd33c89aef1295322d385cad08eff39ede419e913af586f2ce665c8bdddc4f470cdcea8b7a3a33e35730235f2c50fa08abbef2e48`,
  and soft child 17 must produce xpub
  `e184c106be8885cfcaa5834fe6ff9f9e34531cd31729cef0517a0996cec2eae683bb05dd2f53b203ecab6bb857c0979e0d4c810cf166412ab0564e91520ba22e`.

### EMIP-3

- APIs consume/return strict hexadecimal bytes. Encryption requires password nonempty, salt exactly
  32 bytes, nonce exactly 12 bytes. Derive
  `K=PBKDF2-HMAC-SHA512(password,salt,19162,32)` and run IETF ChaCha20-Poly1305 with the 12-byte
  nonce and empty AAD. The envelope is exactly
  `salt[32] || nonce[12] || tag[16] || ciphertext`; output is lowercase hex. Decryption requires
  more than 60 bytes (therefore rejects a metadata-only/empty-ciphertext envelope), parses the same
  layout, authenticates before returning plaintext, and collapses authentication/decryption errors
  to one failure category. Best-effort wipe password/key temporaries.
- Regression vector: password `70617373776f7264`, salt
  `50515253c0c1c2c3c4c5c6c750515253c0c1c2c3c4c5c6c750515253c0c1c2c3`, nonce
  `50515253c0c1c2c3c4c5c6c7`, plaintext
  `736f6d65206461746120746f20656e6372797074` ->
  `50515253c0c1c2c3c4c5c6c750515253c0c1c2c3c4c5c6c750515253c0c1c2c350515253c0c1c2c3c4c5c6c7c266630887d216bf88cc4990f73bad7f35bc7c0225b38fe24a7c28b5f9bda6283e3c5768`.

### Legacy Byron, ABOR, and proxy certificates

- A legacy Daedalus private key is exactly 96 opaque bytes `kL||kR||c`; only length is validated
  (no scalar-bit validation). Its public value is `extended_public(kL||kR)||c`, signing is the
  extended-sign algorithm above over `kL||kR`, and its bootstrap witness carries public key 32,
  signature 64, chain code 32, and the address attributes.
- ABOR tags are `1=u8`, `2=u16`, `3=u32`, `4=u64`, `5=u128`, `6=bytes`, `7=array`. Integers are
  unsigned fixed-width little-endian after the tag. Byte strings are tag 6, one-byte length, payload
  and must be <256 bytes. Arrays are tag 7 plus a one-byte count and must be balanced/<256; the
  frozen encoder’s nested-array count is the total ABOR value tokens opened after that marker
  (nested marker plus descendants), not merely direct children. Reject wrong tags, range overflow,
  truncation, unmatched/unclosed arrays, and pending bytes at final `end()`. Regression encoding:
  `u16(10),u32(0x12345),u64(0xffeeddcc00112233),u128(0xffeeddcc0011223321490219480912),bytes(010203040506070809)`
  ->
  `020a0003452301000433221100ccddeeff051209481902492133221100ccddeeff000609010203040506070809`.
- Byron signing tags are 0x01 Tx, 0x02 RedeemTx, 0x03 VssCert, 0x04 UpdateProposal,
  0x05 Commitment, 0x06 UpdateVote, 0x07 MainBlock, 0x08 MainBlockLight, 0x09 MainBlockHeavy,
  0x0a ProxySecretKey. Proxy-certificate signing bytes are
  `0x0a || canonical_CBOR_uint(protocol_magic_uint32) || canonical_CBOR_bytes(0x30 0x30 || delegate_xpub_64 || canonical_CBOR_uint(omega_uint64))`;
  sign with issuer xprv’s raw extended key and verify with issuer xpub’s raw key. Reject omega
  outside uint64 and protocol magic outside uint32. Frozen proxy-data vector is
  `0a1a13936ea358433030695b380fc72ae7d830d46f902a7c9d4057a4b9a7a0be235b87fdf51e698619e033aac8d93fd4cb82785973bb943f2047ddd1e664d4e185e7be634722e108389a00`;
  expected signature
  `a72bf0119afd1ba5bed56b6521544105b6077c884609666296dbc59275477149a1b8230ce5b6c0fa81e1ec61c717164be57422e86a8f2f5773cdc66da99fcc0e`.

### Shelley-style and Byron address compatibility

- Shelley-style raw headers use high-nibble variants 0–3 base, 4–5 pointer, 6–7 enterprise,
  14–15 reward; variant 8 is a Byron raw envelope; reject 9–13. The low nibble is the network id.
  Payment/stake credential bits are header bits 4/5. Base is 57 bytes before compatibility suffix,
  enterprise/reward 29, and pointer is header+28-byte credential+three variable naturals. Variable
  naturals are unsigned big-endian base-128 groups with continuation bit; decode to arbitrary
  precision (must accept `(2^1400)-1` and three uint64-max components), accept nonminimal forms, and
  emit the minimal form. Unterminated values fail.
- After the semantic payload, accept no bytes or exactly one suffix from this frozen whitelist
  (hex): `cb57afb0b35fc89c63061c9914e055001a518c7516`,
  `13d5f4a3fe0478b2241e0168e3cba5001a22c15a11`, `00`,
  `6a33306635616d6b776877716134777666796a64657a7961656c6d6e6e676436643465`,
  `35616379327230656b7270717a716a6c71646b386c7a716e357234356e`,
  `061d070c0d041b07020f0b0d0b0f020912051d1c100911040e1f0713110301000b101600`,
  `126e7735333567367673703778376668787071327074736839676b72`, or `2c`. Reject every other suffix.
  Generic `Address` preserves an accepted suffix byte-for-byte on raw/hex/Bech32 re-encoding;
  converting through a typed Base/Pointer/Enterprise/Reward view emits the canonical suffix-free
  address. Regression long address:
  `addr1q9d66zzs27kppmx8qc8h43q7m4hkxp5d39377lvxefvxd8j7eukjsdqc5c97t2zg5guqadepqqx6rc9m7wtnxy6tajjvk4a0kze4ljyuvvrpexg5up2sqxj33363v35gtew`
  must round-trip; the suffix `00040206030086cc` must be rejected.
- Bech32 address decoding validates checksum/case/padding but deliberately does not constrain the
  HRP. Default output is `stake` for reward and `addr` otherwise, with `_test` only when network id
  is zero; an explicit caller HRP overrides it. Generic variant-8 parsing stores any nonempty
  high-nibble-8 payload without validating the Byron envelope; `ByronAddress::from_address`
  performs envelope validation. Generic Byron `network_id`/default Bech32/JSON are errors.
  `Address::is_valid` accepts either a parseable Bech32 payload or a CRC-valid Byron Base58 address.
- Generic Bech32 and address decoding deliberately impose no 90-character limit. Typed
  Shelley-style constructors accept a uint8 network value; raw serialization uses
  `network & 0x0f`, so conversion to generic `Address` exposes the truncated nibble. The
  uint64-maximum pointer fixture
  `addr_test1grqe6lg9ay8wkcu5k5e38lne63c80h3nq6xxhqfmhewf645pllllllllllll7lupllllllllllll7lupllllllllllll7lc9wayvj`
  must decode and round-trip.
- Generic Bech32 encoding imposes no 90-character limit, requires a nonempty lowercase HRP
  containing only ASCII `0x21..0x7e`, and permits an empty payload. Decoding accepts uniformly
  lowercase or uppercase input, returns a lowercase HRP, uses the last `1` separator, permits an
  empty payload, validates the six-character Bech32 checksum and zero padding, and deliberately
  does not validate decoded HRP characters. Mixed case, invalid alphabet, checksum, or padding
  rejects; Bech32m is unsupported.
- Byron spending data is canonical CBOR `[kind,bytes]`: kind 0/xpub64, 1/script32, 2/redeem public
  key32. `AddressId = Blake2b-224(SHA3-256(CBOR([addr_type, spending_data, attributes])))`;
  `StakeholderId = Blake2b-224(SHA3-256(CBOR_bytes(xpub64)))`. Address content is
  `[address_id_bytes28, attributes_map, addr_type_0_to_2]`. Attribute map keys/outer values are
  uint/bytes and duplicates/unknowns fail: key 0 embeds stake distribution (`[0,stakeholder28]` or
  `[1]`), key 1 embeds a CBOR byte string containing unrestricted HD payload bytes, key 2 embeds a
  CBOR uint32 protocol magic. Canonical emission orders keys 0,1,2 and omits BootstrapEra key 0;
  mainnet bootstrap construction also omits magic 764824073. Byron envelope is
  `[tag24(bytes(content_cbor)), crc32(content_cbor)]`, CRC-32/IEEE, and Base58 encodes the full
  envelope. Validate the tag, complete content, uint32 CRC, and checksum.
- Byron protocol magic defaults to 764824073 when absent. Map 764824073 to network 1; map
  1097911063, 1, 2, and 4 to network 0; other magic values have no known network-id mapping.
  Icarus construction omits mainnet magic and includes non-mainnet magic. A bootstrap witness wire
  value is `[pubkey32,sig64,chain_code32,bytes(attributes_cbor)]`; Icarus signs the transaction-body
  hash with the xprv raw extended key, while Daedalus uses the legacy extended-sign key.

### Public JSON/text contracts in this scope

- Runtime `Address` and `RewardAddress` JSON is a compact JSON string containing default Bech32.
  Parsing accepts only a JSON string and then the Bech32 path (not Byron Base58); reward parsing
  additionally requires reward kind. Base/Pointer/Enterprise wrappers and `ByronAddress` have no
  standalone runtime JSON codec; Byron's textual form is Base58.
- In ledger JSON, fixed bytes, hashes, signatures, normal/xpub keys, scripts, address ids, and
  stakeholder ids are lowercase hex strings without `0x`; Shelley/reward addresses are Bech32 and
  Byron addresses are Base58. Crypto key/hash classes themselves expose raw/hex/Bech32, not
  `from_json`/`to_json` methods.
- Preserve these public type-only Byron shapes even though most do not own runtime JSON methods:
  `AddrAttributesJSON={derivation_path?:number[]|null,protocol_magic?:number|null,stake_distribution?:"BootstrapEra"|{SingleKey:string}|null}`;
  `AddressContentJSON={addr_attributes:AddrAttributesJSON,addr_type:"PublicKeyJSON"|"ScriptJSON"|"Redeem",address_id:string}`
  (the two `...JSON` discriminator spellings are frozen);
  `SpendingDataJSON={SpendingDataPubKey:string}|{SpendingDataScript:string}|{SpendingDataRedeem:string}`;
  `ByronTxOutJSON={address:string,amount:number}`; `HDAddressPayloadJSON=number[]`,
  `Crc32JSON/ProtocolMagicJSON=number`, and address/key/hash JSON aliases are strings.
- There is a frozen declaration/runtime mismatch that the instruction should disposition explicitly
  rather than conceal: public `BootstrapWitnessJSON` declares
  `{attributes:AddrAttributesJSON,chain_code:number[],public_key:string,signature:string}`, but the
  actual `BootstrapWitness.from_json/to_json` codec requires/emits a compact object whose four
  fields are strings: `public_key`, `signature`, and `chain_code` are lowercase hex; `attributes`
  is lowercase CBOR hex. Recommended normative split: retain the declared DTO as a type/schema
  compatibility mapping, but make the C++ runtime JSON codec follow the observable
  four-hex-string form; document this disposition in `API_PARITY.md`.

## Frozen CIP-25, CIP-36, and typed Data contract

These are local normative requirements transcribed from the frozen comparison revision. They create no TypeScript build, runtime, or implementation dependency.

### CIP-25

- Metadata label is unsigned integer `721`.
- A string component is CBOR text containing at most 64 UTF-8 bytes. Empty strings are allowed.
- A chunkable string is either one CBOR text value or a CBOR array of text chunks. The array may be empty. `from_string` UTF-8-encodes first, slices at raw 64-byte offsets, then strictly decodes each slice; it rejects a multibyte code point crossing an offset, including `"x" * 63 + "é"`.
- File details use required text keys `name`, `mediaType`, and `src`.
- Asset details require `name` and `image`; optional keys are `files`, `mediaType`, and `description`. Newly encoded key order is `name`, optional `files`, `image`, optional `mediaType`, optional `description`.
- V1 label data is `{ policy-id-text => { asset-name-text => details } }`. Policy text is 28-byte lowercase hexadecimal on output. Asset names must decode as valid UTF-8 and satisfy the chain-owned 32-byte bound.
- V2 label data is `{ "data" => { policy-id-bytes => { asset-name-bytes => details } }, "version" => 2 }`.
- Encoding sorts policies, then assets, by lowercase hexadecimal bytes. It emits newly constructed definite containers and minimal heads; CIP-25 does not preserve original CBOR. Accepted indefinite/noisy input is normalized and need not round-trip byte-for-byte.
- Parsing first attempts V1. It skips non-text policy and asset keys; any failure while processing a text-keyed V1 entry abandons the entire V1 attempt and retries V2. V2 requires text fields `data` and `version`, requires unsigned `version == 2`, and skips non-byte policy and asset keys.
- Named maps ignore non-text keys, reject duplicate text keys, ignore unknown text keys unless required by the enclosing model, and reject missing required keys or wrong value kinds.
- A top-level byte parser requires a map containing an unsigned `721` entry and uses the first matching entry. Other labels are discarded when producing a new CIP-25 object. `add_to_metadata` replaces label `721` in the supplied chain-owned metadata while retaining unrelated labels.
- JSON output shapes are:

```text
Chunkable := {"Single": string} | {"Chunked": [string...]}
File      := {"name": string, "media_type": string, "src": Chunkable}
Details   := {
  "name": string,
  "image": Chunkable,
  "media_type"?: string,
  "description"?: Chunkable,
  "files"?: [File...]
}
Label721  := {
  "nfts": {policyHex: {assetHex: Details}},
  "version": "V1" | "V2"
}
Metadata  := {"key_721": Label721}
Mini      := {"name": string | null, "image": Chunkable | null}
```

- Chunkable JSON input additionally accepts a bare JSON string. File input accepts `media_type` or `mediaType`; output always uses `media_type`. Label input selects V2 only for `"V2"` or numeric `1`; every other `version` value selects V1, and absent `nfts` means an empty object.
- Historical JSON coercion is retained: file `name` and media type and asset-detail `name` are passed through string conversion instead of first requiring JSON strings. Optional asset fields are applied only in their accepted kinds.
- Loose mini parsing requires a CBOR map. It searches text-valued name candidates in order `name`, `Name`, `title`, `id`. A candidate exceeding 64 UTF-8 bytes is ignored, but still ends the search. It accepts only lowercase `image`; malformed name/image values are swallowed. Duplicate text keys still fail.

### CIP-36

- Metadata labels are:

```text
61284 = key registration
61285 = registration or deregistration witness
61286 = key deregistration
```

- A delegation is `[voting_public_key_bytes32, weight_uint32]`. A weighted distribution is a nonempty CBOR array of delegations. A legacy distribution is one 32-byte voting public key.
- A witness is `{1: signature_bytes64}`.
- Key registration is `{1: delegation, 2: stake_key_bytes32, 3: payment_address_bytes, 4: nonce_uint64, ?5: voting_purpose_uint64}`.
- Key deregistration is `{1: stake_key_bytes32, 2: nonce_uint64, ?3: voting_purpose_uint64}`.
- Proposal maps reject non-unsigned, unknown, and duplicate keys and require every mandatory key. Metadata-view parsing ignores non-unsigned and unrelated unsigned labels, rejects duplicate unsigned labels, and requires the applicable proposal and witness labels.
- Hashes-to-sign are exactly:

```text
registration   = Blake2b-256(CBOR({61284: key-registration}))
deregistration = Blake2b-256(CBOR({61286: key-deregistration}))
```

  The caller signs the resulting 32-byte digest. `forceCanonical == false` preserves the decoded inner proposal node while using the new one-entry outer map; `true` canonically encodes the entire preimage. Witness metadata is never part of either preimage.

- Parsed proposal, delegation, distribution, and witness nodes retain their original CBOR. Normal encoding preserves nonminimal heads, map order, and indefinite forms; canonical encoding ignores those hints.
- Purpose presence is exact:

  - `new(weighted registration)` emits key `5` with `0`.
  - `new(legacy registration)` omits key `5`.
  - Parsed registrations preserve key `5` exactly, including its absence or explicit zero.
  - After `set_voting_purpose`, a weighted registration emits key `5` when the new value is nonzero or it was previously explicit. A parsed weighted registration lacking key `5`, then set to zero, omits it.
  - A legacy registration never emits key `5` after mutation, even when set to a nonzero value.
  - New deregistrations omit key `3`. Parsed deregistrations preserve explicit zero. After mutation, key `3` is emitted when nonzero or previously explicit.
  - JSON field presence controls explicit-purpose presence; JSON output nevertheless always includes `voting_purpose`, using zero when absent on wire.

- Registration `verify()` is only the frozen compatibility check: for weighted delegation it rejects every nonzero weight. It does not check a sum and does not verify Ed25519. Registration metadata emission invokes this check. Deregistration has no corresponding method. Construction and decoding never verify signatures; callers explicitly verify the appropriate hash through the crypto public-key API.
- Registration metadata bytes are emitted in label order `61284`, `61285`. Deregistration metadata bytes are emitted in order `61285`, `61286`; adding deregistration to an existing metadata object inserts `61286` then `61285`. Adding to existing metadata retains unrelated labels.
- Unsigned JSON integers accept a nonnegative safe integer or an ASCII digit-only decimal string. Output uses a number through `Number.MAX_SAFE_INTEGER`, otherwise a decimal string.
- JSON shapes are:

```text
Delegation   := {"voting_pub_key": hex32, "weight": uint32}
Distribution := {"Legacy": hex32}
              | {"Weighted": {"weighted": [Delegation...]}}
Witness      := {"stake_witness": hex64}
Registration := {
  "delegation": Distribution,
  "stake_credential": hex32,
  "payment_address": bech32,
  "nonce": number | decimal-string,
  "voting_purpose": number | decimal-string
}
Deregistration := {
  "stake_credential": hex32,
  "nonce": number | decimal-string,
  "voting_purpose": number | decimal-string
}
RegistrationView   := {
  "key_registration": Registration,
  "registration_witness": Witness
}
DeregistrationView := {
  "key_deregistration": Deregistration,
  "deregistration_witness": Witness
}
```

  Weighted-distribution input also accepts `{"Weighted": [Delegation...]}`; output always uses the nested `weighted` object.

### Typed Data

- Raw values are arbitrary integers, hexadecimal byte strings, lists, insertion-ordered maps, or `Constr(index, fields)`. Constructor indices must be nonnegative safe integers; decoding rejects larger constructor alternatives.
- `Data.to` and `Data.from` use the chain-owned `PlutusData` CBOR contract. `Data.void()` is exactly `d87980`, constructor alternative zero with no fields.
- Serialization, deserialization, and schema casts start at depth zero, allow depth 128, and reject depth greater than 128.
- Schema behavior is:

| Schema | Exact representation |
| --- | --- |
| Integer | Requires arbitrary integer; inclusive and exclusive minimum/maximum apply. |
| Bytes | Requires valid hex; `minLength`/`maxLength` count decoded bytes; `enum` compares the supplied hex string exactly before normalization. |
| Boolean | `false = Constr(0, [])`; `true = Constr(1, [])`. |
| Any | Passes the raw value through; later serialization still rejects unsupported values. |
| Array | Plutus list; applies `minItems`, `maxItems`, and optional structural uniqueness. |
| Tuple | Fixed-length list by default; with `hasConstr`, `Constr(0, fields)`. |
| Object | Exact field set in schema property order; `Constr(0, fields)` by default, list when `hasConstr == false`. |
| Map | Plutus map with typed keys/values and `minItems`/`maxItems`. |
| Literal | Title must satisfy the frozen constructor-title predicate below; standalone encoding is `Constr(0, [])`. |
| Nullable | non-null is `Constr(0, [value])`; null is `Constr(1, [])`. |
| Enum | Nonempty ordered alternatives; the array index is the constructor alternative. Literal alternatives have no fields. Object alternatives must have exactly one constructor-name property. Tuple/object payload fields are flattened into the outer constructor. |

- Enum and literal constructor names must satisfy the frozen predicate below. Unknown alternatives,
  wrong field counts, extra/missing object properties, wrong raw variants, or wrong constructor
  alternatives are structural errors.
- Retain the historical constructor-title predicate exactly:
  `title.length > 0 && title[0] === title[0].toUpperCase()`. It is not a Unicode uppercase-category
  test: `"1x"`, `"-x"`, and `"😀x"` pass, while `"a"` fails.
- Enum encoding flattens every payload whose encoded raw value is a list, including Array schemas
  and Any values holding a list. Decoding treats only tuple/object payloads as flattened and
  otherwise requires exactly one field. Consequently the empty, one-integer, and two-integer
  `Items` values under `Enum([Object({Items:Array(Integer())})])` encode respectively as
  `d87980`, `d8798101`, and `d879820102`, and all three fail schema decoding. Preserve this
  historical asymmetry rather than repairing it within this sequence.
- `uniqueItems` compares a deterministic structural fingerprint before byte normalization: object keys are sorted; list, constructor, and map order are significant; byte strings compare by the supplied hex spelling.
- Schema-free `fromJson` converts:

  - strings beginning `0x` to the following hex bytes;
  - other strings to UTF-8 bytes represented as lowercase hex;
  - safe integer numbers and arbitrary integer values to Plutus integers;
  - arrays recursively to lists;
  - objects recursively to maps whose property names are converted as strings.

  Boolean and null values require an explicit schema and otherwise fail.

- Schema-free `toJson` converts safe integers to numbers and larger integers to arbitrary-integer values; byte strings become UTF-8 text when strict decoding succeeds and otherwise `0x`-prefixed lowercase hex; lists recurse; maps become null-prototype objects. Map keys must convert to string, number, or integer, with ordinary string-key collision replacement. Constructors require an explicit schema.
- Hex, bounds, collection, depth, constructor, and schema failures are fail-closed. C++ maps structural/type failures and bounds/range failures to owned stable error categories rather than dependency exceptions.

## Frozen static public inventory

Format is `change-owner|runtime-or-type-only|public-entry-point-set|owned-bindings`. A binding is listed once even when re-exported. `{entry=alias}` records the only differing public name. The aggregate facade is `lib`.

The SHA-256 is over the exact UTF-8 contents of this code block, with LF line endings and a final LF: `5870ff3d4fcb093cf6c4398a5846a7393e95d0daf01c0f5274bd72b3fdbfc82e`.

```text
cardano-lib-ts-static-api-v1
comparison-commit=f4fd43348429f249be6348992de9c47be36a6202
entrypoint-counts-runtime/type-only=chain:466/272,chain/allegra:17/10,chain/alonzo:16/10,chain/babbage:20/12,chain/byron:114/65,chain/conway:180/147,chain/mary:6/4,chain/multi-era:18/2,chain/shelley:43/31,cip:3/0,cip/cip25:15/0,cip/cip36:17/8,cip/cip8:27/0,core:37/22,core/bech32:2/0,crypto:50/0,lib:556/288,plutus:19/37,plutus/data:2/21,plutus/uplc:15/13
distinct-bindings-runtime/type-only=632/343
export-exposures=2565
C002|runtime|cip/cip8,core,lib|Int
C002|runtime|core|BYRON_MAINNET_NETWORK_MAGIC,BYRON_TESTNET_NETWORK_MAGIC,CBOR_INT_MAX,CBOR_INT_MIN,DeserializeError,INT64_MAX,INT64_MIN,NonEmptyMap,NonEmptyVec,OrderedMap,PREPROD_NETWORK_MAGIC,PREVIEW_NETWORK_MAGIC,PairMap,SANCHO_TESTNET_NETWORK_MAGIC,UINT64_MAX,asInt64,asUint64,assertBigIntInRange,assertByteLength,bytesEqual,bytesToHex,cloneValue,copyBytes,decodeProtocolMagic,encodeProtocolMagic,hexToBytes,resultOrThrow,unknownToError
C002|runtime|core,lib|BigInteger,CardanoBoundsError,CardanoError,ProtocolMagic
C002|runtime|core/bech32|decodeBech32,encodeBech32
C002|type-only|core|ByteArray,CardanoErrorCode,CardanoErrorOptions,CardanoResult,Cloneable,Comparator,DeserializeFailure,Equatable,ErrorPathComponent,SecureRandomSource
C003|runtime|core|DEFAULT_CBOR_LIMITS,decodeCbor,decodeEmbeddedCbor,encodeCbor
C003|type-only|core|CborByteChunk,CborDecoderLimits,CborHeadEncoding,CborHeadWidth,CborLengthEncoding,CborMode,CborSpan,CborStringEncoding,CborTextChunk,CborValue,DecodeCborOptions,EncodeCborOptions
C004|runtime|chain,crypto,lib|Ed25519KeyHash,TransactionHash
C004|runtime|cip/cip25,crypto,lib|ScriptHash{cip/cip25=PolicyId}
C004|runtime|cip/cip36,cip/cip8,crypto,lib|Ed25519Signature,PublicKey
C004|runtime|crypto|blake2b224,blake2b256,bls12_381_add,bls12_381_compress,bls12_381_equal,bls12_381_final_verify,bls12_381_hash_to_group,bls12_381_miller_loop,bls12_381_mul_ml_result,bls12_381_neg,bls12_381_scalar_mul,bls12_381_uncompress,keccak_256,legacyPublicKey,legacySign,ripemd_160,secureRandomBytes,sha2_256,sha3_256,systemSecureRandomSource,verifyEd25519,verifyEd25519Uplc,verifySecp256k1Ecdsa,verifySecp256k1EcdsaUplc,verifySecp256k1Schnorr,verifySecp256k1SchnorrUplc
C004|runtime|crypto,lib|AnchorDocHash,AuxiliaryDataHash,Bip32PrivateKey,Bip32PublicKey,BlockBodyHash,BlockHeaderHash,DatumHash,GenesisDelegateHash,GenesisHash,KESVkey,LegacyDaedalusPrivateKey,NonceHash,PoolMetadataHash,PrivateKey,ScriptDataHash,VRFKeyHash,VRFVkey,emip3_decrypt_with_password,emip3_encrypt_with_password
C005|runtime|chain,chain/byron,lib|AddrAttributes,AddressContent,AddressId,BootstrapWitness,ByronAddrType,ByronAddress,ByronScript,Crc32,HDAddressPayload,SpendingData,SpendingDataKind,StakeDistribution,StakeDistributionKind,StakeholderId
C005|runtime|chain,chain/conway,cip/cip36,lib|NetworkId
C005|runtime|chain,chain/conway,lib|BootstrapWitnessList,NonEmptyBootstrapWitnessList,NonEmptyVkeywitnessList,Vkeywitness,VkeywitnessList
C005|runtime|chain,cip/cip36,lib|Address{cip/cip36=PaymentAddress}
C005|runtime|chain,lib|AddressHeaderKind,AddressKind,BaseAddress,EnterpriseAddress,Pointer,PointerAddress,RewardAddress
C005|runtime|chain/byron|crc32,decodeBase58,encodeBase58,parseByronGenesis
C005|runtime|chain/shelley|parseShelleyGenesis
C005|type-only|chain,chain/conway,lib|AddrAttributesJSON,AddressContentJSON,AddressJSON,BootstrapWitnessJSON,ByronAddrTypeJSON,ByronAddressJSON,Crc32JSON,HDAddressPayloadJSON,NonEmptyVecBootstrapWitnessJSON,NonEmptyVecVkeywitnessJSON,RewardAddressJSON,SpendingDataJSON,StakeDistributionJSON,StakeholderIdJSON,VkeywitnessJSON
C005|type-only|chain/byron|ParsedByronGenesis
C005|type-only|chain/shelley|ParsedShelleyGenesis
C006|runtime|chain,chain/allegra,chain/shelley,lib|MIRAction,MIRActionKind,MIRPot,MoveInstantaneousReward,MoveInstantaneousRewardsCert
C006|runtime|chain,chain/allegra,lib|AllegraAuxiliaryData,AllegraAuxiliaryDataKind,AllegraBlock,AllegraCertificate,AllegraCertificateKind,AllegraCertificateList,AllegraTransaction,AllegraTransactionBody,AllegraTransactionBodyList,AllegraTransactionWitnessSet,AllegraTransactionWitnessSetList,MapTransactionIndexToAllegraAuxiliaryData
C006|runtime|chain,chain/alonzo,lib|AlonzoAuxiliaryData,AlonzoAuxiliaryDataKind,AlonzoBlock,AlonzoFormatAuxData,AlonzoProposedProtocolParameterUpdates,AlonzoProtocolParamUpdate,AlonzoRedeemer,AlonzoRedeemerList,AlonzoRedeemerTag,AlonzoTransaction,AlonzoTransactionBody,AlonzoTransactionBodyList,AlonzoTransactionWitnessSet,AlonzoTransactionWitnessSetList,AlonzoUpdate,MapTransactionIndexToAlonzoAuxiliaryData
C006|runtime|chain,chain/babbage,lib|BabbageAuxiliaryData,BabbageAuxiliaryDataKind,BabbageBlock,BabbageFormatAuxData,BabbageFormatTxOut,BabbageProposedProtocolParameterUpdates,BabbageProtocolParamUpdate,BabbageScript,BabbageScriptKind,BabbageScriptRef,BabbageTransaction,BabbageTransactionBody,BabbageTransactionBodyList,BabbageTransactionOutput,BabbageTransactionOutputKind,BabbageTransactionOutputList,BabbageTransactionWitnessSet,BabbageTransactionWitnessSetList,BabbageUpdate,MapTransactionIndexToBabbageAuxiliaryData
C006|runtime|chain,chain/byron,chain/multi-era,lib|Blake2b224,Blake2b256
C006|runtime|chain,chain/byron,chain/shelley,lib|SoftForkRule,SoftForkRuleList
C006|runtime|chain,chain/byron,lib|AddressIdList,BigIntegerList,BlockHeaderExtraData,Bvermod,ByronAny,ByronAnyList,ByronAttributes,ByronAttributesList,ByronBlock,ByronBlockBody,ByronBlockConsensusData,ByronBlockHeader,ByronBlockKind,ByronBlockSignature,ByronBlockSignatureKind,ByronBlockSignatureNormal,ByronBlockSignatureProxyHeavy,ByronBlockSignatureProxyLight,ByronBlockVersion,ByronBodyProof,ByronDelegation,ByronDelegationList,ByronDelegationSignature,ByronDifficulty,ByronEbBlock,ByronMainBlock,ByronPkWitness,ByronPkWitnessEntry,ByronRedeemWitness,ByronRedeemerScript,ByronRedeemerWitnessEntry,ByronScriptWitness,ByronScriptWitnessEntry,ByronSlotId,ByronSoftwareVersion,ByronTx,ByronTxFeePolicy,ByronTxFeePolicyList,ByronTxIn,ByronTxInGenesis,ByronTxInKind,ByronTxInList,ByronTxInRegular,ByronTxOut,ByronTxOutList,ByronTxOutPtr,ByronTxProof,ByronTxWitness,ByronTxWitnessKind,ByronTxWitnessList,ByronUpdate,ByronUpdateData,ByronUpdateProposal,ByronUpdateProposalList,ByronUpdateVote,ByronUpdateVoteList,ByronValidatorScript,BytesList,EbbConsensusData,EbbHead,EpochRange,LightWeightDelegationSignature,LightWeightDlg,MapSystemTagToByronUpdateData,Ssc,SscCert,SscCertificatesPayload,SscCertificatesProof,SscCerts,SscCommitment,SscCommitmentsPayload,SscCommitmentsProof,SscKind,SscOpeningsPayload,SscOpeningsProof,SscOpens,SscProof,SscProofKind,SscShares,SscSharesPayload,SscSharesProof,SscSharesSubmap,SscSignedCommitment,SscSignedCommitments,StakeholderIdList,StdFeePolicy,SystemTagList,TxAux,TxPayload,VssEncryptedShare,VssProof,VssShares
C006|runtime|chain,chain/conway,cip/cip25,cip/cip36,lib|Metadata,TransactionMetadatum
C006|runtime|chain,chain/conway,cip/cip25,lib|AssetName
C006|runtime|chain,chain/conway,lib|AlonzoFormatTxOut,AlonzoFormatTxOutList,Anchor,AssetNameList,AuthCommitteeHotCert,AuxiliaryData,AuxiliaryDataKind,Block,Certificate,CertificateKind,CertificateList,CommitteeColdCredentialList,Constitution,ConstrPlutusData,ConwayFormatAuxData,ConwayFormatTxOut,CostModels,Credential,CredentialKind,DNSName,DRep,DRepKind,DRepVotingThresholds,DatumOption,DatumOptionKind,Ed25519KeyHashList,ExUnitPrices,ExUnits,GenesisHashList,GovAction,GovActionId,GovActionIdList,GovActionKind,HardForkInitiationAction,Header,HeaderBody,Ipv4,Ipv6,KESSignature,Language,LanguageList,LegacyRedeemer,LegacyRedeemerList,MapAssetNameToCoin,MapAssetNameToNonZeroInt64,MapAssetNameToU64,MapCommitteeColdCredentialToEpoch,MapGovActionIdToVotingProcedure,MapRedeemerKeyToRedeemerVal,MapStakeCredentialToCoin,MapStakeCredentialToDeltaCoin,MapTransactionIndexToAuxiliaryData,MapTransactionIndexToMetadata,MapU64ToArrI64,MapVoterToMapGovActionIdToVotingProcedure,MetadatumList,MetadatumMap,Mint,MultiAsset,MultiHostName,NativeScript,NativeScriptKind,NativeScriptList,NewConstitution,NoConfidence,NonEmptyCertificateList,NonEmptyLegacyRedeemerList,NonEmptyMapGovActionIdToVotingProcedure,NonEmptyMapRedeemerKeyToRedeemerVal,NonEmptyNativeScriptList,NonEmptyPlutusDataList,NonEmptyPlutusV1ScriptList,NonEmptyPlutusV2ScriptList,NonEmptyPlutusV3ScriptList,NonEmptyProposalProcedureList,NonEmptyTransactionInputList,Nonce,NonceKind,OperationalCert,ParameterChangeAction,PlutusData,PlutusDataKind,PlutusDataList,PlutusMap,PlutusV1Script,PlutusV1ScriptList,PlutusV2Script,PlutusV2ScriptList,PlutusV3Script,PlutusV3ScriptList,PolicyIdList,PoolMetadata,PoolParams,PoolRegistration,PoolRetirement,PoolVotingThresholds,ProposalProcedure,ProposalProcedureList,ProtocolParamUpdate,ProtocolVersion,Rational,RedeemerKey,RedeemerKeyList,RedeemerTag,RedeemerVal,Redeemers,RedeemersKind,RegCert,RegDrepCert,Relay,RelayKind,RelayList,RequiredSigners,ResignCommitteeColdCert,RewardAccountList,Script,ScriptAll,ScriptAny,ScriptInvalidBefore,ScriptInvalidHereafter,ScriptKind,ScriptNOfK,ScriptPubkey,ScriptRef,ShelleyMAFormatAuxData,SingleHostAddr,SingleHostName,StakeCredentialList,StakeDelegation,StakeDeregistration,StakeRegDelegCert,StakeRegistration,StakeVoteDelegCert,StakeVoteRegDelegCert,Transaction,TransactionBody,TransactionBodyList,TransactionInput,TransactionInputList,TransactionMetadatumKind,TransactionMetadatumLabels,TransactionMetadatumList,TransactionOutput,TransactionOutputKind,TransactionOutputList,TransactionWitnessSet,TransactionWitnessSetList,TreasuryWithdrawalsAction,UnitInterval,UnregCert,UnregDrepCert,UpdateCommittee,UpdateDrepCert,Url,VRFCert,Value,Vote,VoteDelegCert,VoteRegDelegCert,Voter,VoterKind,VoterList,VotingProcedure,VotingProcedures,Withdrawals
C006|runtime|chain,chain/mary,lib|MaryBlock,MaryTransaction,MaryTransactionBody,MaryTransactionBodyList,MaryTransactionOutput,MaryTransactionOutputList
C006|runtime|chain,chain/shelley,lib|GenesisKeyDelegation,MultisigAll,MultisigAny,MultisigNOfK,MultisigPubkey,MultisigScript,MultisigScriptKind,MultisigScriptList,ProtocolVersionStruct,ShelleyBlock,ShelleyCertificate,ShelleyCertificateKind,ShelleyCertificateList,ShelleyDNSName,ShelleyHeader,ShelleyHeaderBody,ShelleyMoveInstantaneousReward,ShelleyMoveInstantaneousRewardsCert,ShelleyMultiHostName,ShelleyPoolParams,ShelleyPoolRegistration,ShelleyProposedProtocolParameterUpdates,ShelleyProtocolParamUpdate,ShelleyRelay,ShelleyRelayKind,ShelleyRelayList,ShelleySingleHostName,ShelleyTransaction,ShelleyTransactionBody,ShelleyTransactionBodyList,ShelleyTransactionOutput,ShelleyTransactionOutputList,ShelleyTransactionWitnessSet,ShelleyTransactionWitnessSetList,ShelleyUpdate
C006|runtime|chain/conway|ConwayData,ConwayList,ConwayMap,NATIVE_SCRIPT_SCHEMA,PLUTUS_DATA_SCHEMA,TRANSACTION_INPUT_SCHEMA
C006|type-only|chain,chain/allegra,chain/shelley,lib|MIRActionJSON,MIRPotJSON,MoveInstantaneousRewardJSON,MoveInstantaneousRewardsCertJSON
C006|type-only|chain,chain/allegra,lib|AllegraAuxiliaryDataJSON,AllegraBlockJSON,AllegraCertificateJSON,AllegraTransactionBodyJSON,AllegraTransactionJSON,AllegraTransactionWitnessSetJSON
C006|type-only|chain,chain/alonzo,lib|AlonzoAuxiliaryDataJSON,AlonzoBlockJSON,AlonzoFormatAuxDataJSON,AlonzoProtocolParamUpdateJSON,AlonzoRedeemerJSON,AlonzoRedeemerTagJSON,AlonzoTransactionBodyJSON,AlonzoTransactionJSON,AlonzoTransactionWitnessSetJSON,AlonzoUpdateJSON
C006|type-only|chain,chain/babbage,lib|BabbageAuxiliaryDataJSON,BabbageBlockJSON,BabbageFormatAuxDataJSON,BabbageFormatTxOutJSON,BabbageMintJSON,BabbageProtocolParamUpdateJSON,BabbageScriptJSON,BabbageTransactionBodyJSON,BabbageTransactionJSON,BabbageTransactionOutputJSON,BabbageTransactionWitnessSetJSON,BabbageUpdateJSON
C006|type-only|chain,chain/byron,chain/shelley,lib|SoftForkRuleJSON
C006|type-only|chain,chain/byron,lib|AnyJSON,Blake2B256JSON,BlockHeaderExtraDataJSON,BvermodJSON,ByronBlockBodyJSON,ByronBlockConsensusDataJSON,ByronBlockHeaderJSON,ByronBlockJSON,ByronBlockSignatureJSON,ByronBlockSignatureNormalJSON,ByronBlockSignatureProxyHeavyJSON,ByronBlockSignatureProxyLightJSON,ByronBlockVersionJSON,ByronBodyProofJSON,ByronDelegationJSON,ByronDelegationSignatureJSON,ByronDifficultyJSON,ByronEbBlockJSON,ByronMainBlockJSON,ByronPkWitnessEntryJSON,ByronPkWitnessJSON,ByronRedeemWitnessJSON,ByronRedeemerScriptJSON,ByronRedeemerWitnessEntryJSON,ByronScriptWitnessEntryJSON,ByronScriptWitnessJSON,ByronSlotIdJSON,ByronSoftwareVersionJSON,ByronTxFeePolicyJSON,ByronTxInGenesisJSON,ByronTxInJSON,ByronTxInRegularJSON,ByronTxJSON,ByronTxOutPtrJSON,ByronTxProofJSON,ByronTxWitnessJSON,ByronUpdateDataJSON,ByronUpdateJSON,ByronUpdateProposalJSON,ByronUpdateVoteJSON,ByronValidatorScriptJSON,EbbConsensusDataJSON,EbbHeadJSON,EpochRangeJSON,LightWeightDelegationSignatureJSON,LightWeightDlgJSON,SscCertJSON,SscCertificatesPayloadJSON,SscCertificatesProofJSON,SscCommitmentJSON,SscCommitmentsPayloadJSON,SscCommitmentsProofJSON,SscJSON,SscOpeningsPayloadJSON,SscOpeningsProofJSON,SscProofJSON,SscSharesPayloadJSON,SscSharesProofJSON,SscSignedCommitmentJSON,StdFeePolicyJSON,TxAuxJSON,VssEncryptedShareJSON,VssProofJSON
C006|type-only|chain,chain/conway,lib|AlonzoFormatTxOutJSON,AnchorDocHashJSON,AnchorJSON,ArrayOf_CredentialJSON,ArrayOf_Ed25519KeyHashJSON,ArrayOf_TransactionInputJSON,AssetNameJSON,AuthCommitteeHotCertJSON,AuxiliaryDataHashJSON,AuxiliaryDataJSON,BigIntegerJSON,Bip32PublicKeyJSON,BlockBodyHashJSON,BlockHeaderHashJSON,BlockJSON,ByronTxOutJSON,CertificateJSON,ConstitutionJSON,ConwayFormatAuxDataJSON,ConwayFormatTxOutJSON,CostModelsJSON,CredentialJSON,DNSNameJSON,DRepJSON,DRepVotingThresholdsJSON,DatumHashJSON,DatumOptionJSON,Ed25519KeyHashJSON,Ed25519SignatureJSON,ExUnitPricesJSON,ExUnitsJSON,GenesisDelegateHashJSON,GenesisHashJSON,GovActionIdJSON,GovActionJSON,HardForkInitiationActionJSON,HeaderBodyJSON,HeaderJSON,IntJSON,Ipv4JSON,Ipv6JSON,KESSignatureJSON,KESVkeyJSON,LanguageJSON,LegacyRedeemerJSON,MetadataJSON,MultiHostNameJSON,NativeScriptJSON,NetworkIdJSON,NewConstitutionJSON,NoConfidenceJSON,NonEmptyVecCertificateJSON,NonEmptyVecEd25519KeyHashJSON,NonEmptyVecNativeScriptJSON,NonEmptyVecPlutusDataJSON,NonEmptyVecPlutusV1ScriptJSON,NonEmptyVecPlutusV2ScriptJSON,NonEmptyVecPlutusV3ScriptJSON,NonEmptyVecProposalProcedureJSON,NonEmptyVecTransactionInputJSON,NonceHashJSON,NonceJSON,OperationalCertJSON,ParameterChangeActionJSON,PlutusDataJSON,PlutusV1ScriptJSON,PlutusV2ScriptJSON,PlutusV3ScriptJSON,PoolMetadataHashJSON,PoolMetadataJSON,PoolParamsJSON,PoolRegistrationJSON,PoolRetirementJSON,PoolVotingThresholdsJSON,ProposalProcedureJSON,ProtocolMagicJSON,ProtocolParamUpdateJSON,ProtocolVersionJSON,PublicKeyJSON,RationalJSON,RedeemerKeyJSON,RedeemerTagJSON,RedeemerValJSON,RedeemersJSON,RegCertJSON,RegDrepCertJSON,RelayJSON,ResignCommitteeColdCertJSON,ScriptAllJSON,ScriptAnyJSON,ScriptDataHashJSON,ScriptHashJSON,ScriptInvalidBeforeJSON,ScriptInvalidHereafterJSON,ScriptJSON,ScriptNOfKJSON,ScriptPubkeyJSON,ScriptRefJSON,ShelleyMAFormatAuxDataJSON,SingleHostAddrJSON,SingleHostNameJSON,StakeDelegationJSON,StakeDeregistrationJSON,StakeRegDelegCertJSON,StakeRegistrationJSON,StakeVoteDelegCertJSON,StakeVoteRegDelegCertJSON,TransactionBodyJSON,TransactionHashJSON,TransactionInputJSON,TransactionJSON,TransactionMetadatumJSON,TransactionOutputJSON,TransactionWitnessSetJSON,TreasuryWithdrawalsActionJSON,UnitIntervalJSON,UnregCertJSON,UnregDrepCertJSON,UpdateCommitteeJSON,UpdateDrepCertJSON,UrlJSON,VRFCertJSON,VRFKeyHashJSON,VRFVkeyJSON,ValueJSON,VoteDelegCertJSON,VoteJSON,VoteRegDelegCertJSON,VoterJSON,VotingProcedureJSON
C006|type-only|chain,chain/mary,lib|MaryBlockJSON,MaryTransactionBodyJSON,MaryTransactionJSON,MaryTransactionOutputJSON
C006|type-only|chain,chain/shelley,lib|GenesisKeyDelegationJSON,MultisigAllJSON,MultisigAnyJSON,MultisigNOfKJSON,MultisigPubkeyJSON,MultisigScriptJSON,ProtocolVersionStructJSON,ShelleyBlockJSON,ShelleyCertificateJSON,ShelleyDNSNameJSON,ShelleyHeaderBodyJSON,ShelleyHeaderJSON,ShelleyMoveInstantaneousRewardJSON,ShelleyMoveInstantaneousRewardsCertJSON,ShelleyMultiHostNameJSON,ShelleyPoolParamsJSON,ShelleyPoolRegistrationJSON,ShelleyProtocolParamUpdateJSON,ShelleyRelayJSON,ShelleySingleHostNameJSON,ShelleyTransactionBodyJSON,ShelleyTransactionJSON,ShelleyTransactionOutputJSON,ShelleyTransactionWitnessSetJSON,ShelleyUpdateJSON
C006|type-only|chain/conway|ConwayInput,ConwayWireShape
C007|runtime|chain,chain/multi-era,lib|MapGenesisHashToMultiEraProtocolParamUpdate,MultiEraBlock,MultiEraBlockHeader,MultiEraBlockKind,MultiEraCertificate,MultiEraCertificateKind,MultiEraCertificateList,MultiEraProtocolParamUpdate,MultiEraTransactionBody,MultiEraTransactionBodyKind,MultiEraTransactionBodyList,MultiEraTransactionInput,MultiEraTransactionInputList,MultiEraTransactionOutput,MultiEraTransactionOutputList,MultiEraUpdate
C007|runtime|chain,lib|ByronGenesisRedeem,CardanoNodePlutusDatumSchema,LinearFee,MetadataJsonSchema,NetworkInfo,calc_script_data_hash,calc_script_data_hash_from_witness,compute_total_ex_units,decode_arbitrary_bytes_from_metadatum,decode_metadatum_to_json_str,decode_plutus_datum_to_json_str,encode_arbitrary_bytes_as_metadatum,encode_json_str_to_metadatum,encode_json_str_to_plutus_datum,genesis_txid_byron,genesis_txid_shelley,get_deposit,get_implicit_input,hash_auxiliary_data,hash_plutus_data,hash_script_data,hash_transaction,make_daedalus_bootstrap_witness,make_icarus_bootstrap_witness,make_vkey_witness,min_ada_required,min_fee,min_no_script_fee,min_script_fee
C007|type-only|chain,chain/multi-era,lib|MultiEraBlockJSON,MultiEraTransactionBodyJSON
C008|runtime|chain,lib|CertificateBuilderResult,ChangeSelectionAlgo,CoinSelectionStrategyCIP2,InputAggregateWitnessData,InputBuilderResult,MintBuilderResult,NativeScriptWitnessInfo,PartialPlutusWitness,PlutusScript,PlutusScriptWitness,ProposalBuilder,ProposalBuilderResult,RedeemerSetBuilder,RedeemerWitnessKey,RequiredWitnessSet,SignedTxBuilder,SingleCertificateBuilder,SingleInputBuilder,SingleMintBuilder,SingleOutputBuilderResult,SingleWithdrawalBuilder,TransactionBuilder,TransactionBuilderConfig,TransactionBuilderConfigBuilder,TransactionOutputAmountBuilder,TransactionOutputBuilder,TransactionUnspentOutput,TransactionWitnessSetBuilder,TxRedeemerBuilder,UntaggedRedeemer,VoteBuilder,VoteBuilderResult,WithdrawalBuilderResult
C009|runtime|cip|cip25,cip36,cip8
C009|runtime|cip/cip25,lib|CIP25ChunkableString,CIP25ChunkableStringKind,CIP25FilesDetails,CIP25FilesDetailsList,CIP25LabelMetadata,CIP25Metadata,CIP25MetadataDetails,CIP25MiniMetadataDetails,CIP25String64,CIP25String64List,CIP25Version
C009|runtime|cip/cip36,lib|CIP36Delegation,CIP36DelegationDistribution,CIP36DelegationDistributionKind,CIP36DelegationList,CIP36DeregistrationCbor,CIP36DeregistrationWitness,CIP36KeyDeregistration,CIP36KeyRegistration,CIP36RegistrationCbor,CIP36RegistrationWitness,NonEmptyCIP36DelegationList
C009|runtime|cip/cip8,lib|AlgorithmId,COSEKey,COSESign,COSESign1,COSESign1Builder,COSESignBuilder,COSESignature,COSESignatures,CounterSignature,CurveType,ECKey,EdDSA25519Key,HeaderMap,Headers,KeyOperation,KeyType,Label,LabelKind,Labels,ProtectedHeaderMap,SigContext,SigStructure,SignedMessage,SignedMessageKind
C009|type-only|cip/cip36|CIP36LegacyKeyRegistration,CIP36Nonce,CIP36StakeCredential,CIP36StakeWitness,CIP36StakingPubKey,CIP36VotingPubKey,CIP36VotingPurpose,CIP36Weight
C010|runtime|plutus,plutus/data|Constr,Data
C010|type-only|plutus,plutus/data|AnySchema,ArraySchema,BooleanSchema,BytesSchema,DataSchema,Datum,EnumItemSchema,EnumSchema,Exact,IntegerSchema,Json,LiteralSchema,MapSchema,NullableSchema,ObjectSchema,PlutusDataValue,Redeemer,SchemaProperties,StaticProperties,StaticSchema,TupleSchema
C011|runtime|lib,plutus|apply_params_to_script,eval_phase_two_raw
C011|runtime|lib,plutus,plutus/uplc|builtinCost,builtinTag,dataConstant,decodeFlatProgram,decodeProgramEnvelope,decodeProgramEnvelopeCompatible,defaultBuiltinCostModel,defaultMachineCosts,encodeFlatProgram,encodePlutusData,encodeProgramEnvelope,evaluateProgram,makeBuiltinCostModel,parseUplcText,validatePlutusDataNode
C011|type-only|lib,plutus|PhaseTwoEvaluation,PhaseTwoRawEvaluation,UplcExBudget
C011|type-only|lib,plutus,plutus/uplc|BuiltinCostModel,CostStream,FlatValue,MachineBudget,MachineCosts,MachineResult,ProgramDecodeOptions,SemanticsVariant,UplcConstant,UplcData,UplcProgram,UplcTerm,UplcType
```

## Frozen ledger operations, JSON, and builder contract

### Ledger JSON conversions

- JSON equality is semantic; whitespace/indentation is not an interoperability promise. Hex is lowercase without `0x` unless a schema below explicitly requires it. Shared input limits are depth 128 inclusive (`depth>128` rejects) and 100,000 visited values. Apply those limits to every C++ JSON entry point, including generic Conway JSON; this is an explicit hardening disposition because the frozen Conway helper lacked its own recursion budget.
- Historical generic JSON maps CBOR integers in the JavaScript safe range to JSON numbers and all other integers to decimal strings; bytes to lowercase hex strings; text/boolean/float/simple/null directly; arrays recursively to arrays; maps to arrays of two-item `[key,value]` arrays; tags to `{ "tag": <number>, "value": ... }`. Its inverse accepts safe integral numbers as integers, every string as CBOR text (decimal strings are not reinterpreted), arrays as arrays, objects as insertion-ordered maps with text keys, booleans/null directly. Conway generic output is the same except maps are arrays of `{ "k":..., "v":... }`; generic JSON objects on input still become text-keyed maps. Consequently large-integer strings and generic map output are not promised to round-trip through generic `from_json`; preserve that observable distinction rather than guessing types.
- Exact specialized JSON values are: `TransactionInput={"transaction_id":<64-char hash hex>,"index":<non-negative safe integer>}` (output converts uint64 to binary64 and can lose precision); `Address`/`RewardAddress` are Bech32 strings; raw-byte wrappers are lowercase hex strings; IPv4/IPv6 are normalized strings; `Rational={"numerator":number-or-decimal-string,"denominator":number-or-decimal-string}`, `UnitInterval={"start":...,"end":...}`, `ExUnits={"mem":...,"steps":...}`, `Credential={"PubKey":{"hash":hex}}|{"Script":{"hash":hex}}`, `Anchor={"anchor_url":string,"anchor_doc_hash":hex}`, `ProtocolVersion={"major":number,"minor":number}`, `VRFCert={"output":[byte...],"proof":[byte...]}`, and `Value={"coin":binary64-number,"multiasset":{policyHex:{assetNameHex:binary64-number}}}`. Value JSON is output-only in the frozen runtime; do not invent a JSON-to-Value coercion under the parity API. Bootstrap witness JSON is `{public_key:hex,signature:hex,chain_code:hex,attributes:<attributes-CBOR-hex>}`.
- Native-script JSON is the exact single-variant union `{ScriptPubkey:{ed25519_key_hash:hex}}`, `{ScriptAll:{native_scripts:[...]}}`, `{ScriptAny:{native_scripts:[...]}}`, `{ScriptNOfK:{n:number,native_scripts:[...]}}`, `{ScriptInvalidBefore:{before:number}}`, or `{ScriptInvalidHereafter:{after:number}}`. Output uses binary64 number conversion for `n`/slots. ScriptPubkey/All/Any/NOfK/timelock variant classes use the corresponding inner object without the outer variant tag.
- Detailed Plutus Data JSON is one of `{constructor:number,fields:[...]}`, `{map:[{k:...,v:...}]}`, `{list:[...]}`, `{int:number}`, `{bytes:lowercase-hex}`. Input tag precedence is constructor, map, list, int, then bytes; constructor/fields require a non-negative safe integer and array, int requires a safe integer, bytes requires a string; map entries require `k` and `v`; extra object members are ignored. Output converts arbitrary constructor/integer values to binary64 numbers and may lose precision. `ConstrPlutusData` separately uses `{alternative:number,fields:[...]}`. The JavaScript-only `to_js_value` serde-number wrapper is `{"$serde_json::private::Number":decimal}`; mark it `LANGUAGE_SPECIFIC` rather than exposing it in C++.
- Metadata conversion schemas are `NoConversions=0`, `BasicConversions=1`, `Detailed=2`. Detailed values are exact singleton tagged objects `{int:number|string}`, `{string:string}`, `{bytes:hex-without-0x}`, `{list:[...]}`, `{map:[{k:...,v:...}]}`; non-singletons/unknown tags reject, while map-entry objects may have extra fields. No/Basic input accepts safe integers, strings, arrays, and objects, rejecting null, booleans, and non-integral numbers. In Basic, only `/^0x(?:[0-9a-f]{2})*$/i` strings become bytes and object keys matching `/^-?\d+$/` become integer keys; other strings/keys are text. Object members are processed deterministically in ascending UTF-8 byte order (explicit C++ disposition replacing locale-dependent JavaScript `localeCompare`). Output integers are binary64 JSON numbers; Basic bytes are lowercase `0x...`, while NoConversions rejects bytes; lists recurse; maps require text keys, or in Basic decimal-integer / `0x...` byte keys, otherwise reject.
- Cardano-node Plutus JSON schemas are `Basic=0`, `Detailed=1`. Detailed is the Plutus schema above. Basic input maps safe integers to Plutus integers, arrays recursively to lists, and strings beginning with lowercase `0x` to decoded hex bytes (bad hex rejects), otherwise to UTF-8 bytes; objects/null/booleans reject. Basic output supports only integer (binary64 number), bytes (`0x` plus lowercase hex), and list; constructors/maps reject. Metadata and Plutus conversions use the 128/100,000 budgets above.

### C007 multi-era and ledger-operation algorithms

- Explicit-network block CBOR is exactly `[tag, block]`, a two-item array with unsigned tag `0..7`: `0` Byron epoch-boundary, `1` Byron main, `2` Shelley, `3` Allegra, `4` Mary, `5` Alonzo, `6` Babbage, `7` Conway. Validate `block` with that era owner. Preserve the entire accepted explicit envelope byte-for-byte for its preserved re-encoding; a newly constructed value emits a definite two-item array. Kind collapses Byron tags 0/1 to Byron and maps other tags to `tag-1`. A non-explicit block decoder means Conway only; it must not guess an era.
- Multi-era block views: tag 0 has no transactions. Tag 1 reads block[1][0], then each auxiliary item’s element 0 as the Byron transaction body. Tags 2..7 read block[1] as the transaction-body array; non-Byron witness sets, auxiliary-data map, and Alonzo-or-later invalid-index list are block elements 2, 3, and 4. Byron block hash is Blake2b-256 over bytes `82 || tag-byte || preserved-header-CBOR`; later-era block hash is Blake2b-256 over preserved header CBOR. Header block-number/slot paths are EBB `[3,1,0]`/`[3,0]*21600`, Byron-main `[3,2,0]`/`[3,0,0]*21600+[3,0,1]`, later-era header-body `[0]`/`[1]`; previous hash is path `[1]` for Byron and `[2]` otherwise; later body size/hash are `[7]`/`[8]`. Missing optional views return empty/none; wrong required scalar shape rejects.
- Common transaction input is standard `[32-byte tx-hash,uint]`; Byron input reads input[1] as embedded-CBOR bytes whose decoded array is `[hash,index]`. Output address/value are map keys 0/1 or legacy array positions 0/1; Byron address is the encoded address node and Byron amount must be unsigned coin. Body common fields are inputs 0, outputs 1, fee 2, ttl 3, certs 4, withdrawals 5, update 6, aux hash 7, validity start 8, mint 9, script-data hash 11, collateral 13, required signers 14, network 15, collateral return 16, total collateral 17, references 18, votes 19, proposals 20, treasury 21, donation 22. Hash a body as Blake2b-256 of preserved body CBOR. C++ collection views shall unwrap optional tag 258 as well as bare arrays; record this as an official-CDDL correction to the frozen helper’s failure to unwrap tagged sets.
- `NetworkInfo` constants are `(network-id,protocol-magic)`: testnet `(0,1097911063)`, mainnet `(1,764824073)`, preview `(0,2)`, preprod `(0,1)`, sancho `(0,4)`. IDs fit uint8.
- Hash auxiliary data, transaction body, and Plutus datum as Blake2b-256 of preserved `to_cbor_bytes`, never canonical bytes. Hash a script as Blake2b-224 of one namespace byte followed by raw script bytes; namespaces are native/V1/V2/V3 = `0/1/2/3`. A native script therefore hashes `00 || preserved-native-script-CBOR`.
- Script-data hash is Blake2b-256 of concatenated chunks. Normally use `redeemer-bytes` (or `80`, empty array, if absent), then if present a definite datum array wrapped in tag 258 encoded with tag-head width 2, retaining each datum’s decoded wire node, then `CostModels.language_views_encoding()`. The special datums-only case is exactly `a0 || tagged-datums || a0` and ignores cost models. If both redeemers and datums are absent, the optional calculator returns none. The witness helper returns none unless witness map keys 5 (redeemers) and 4 (datums) are both present; it unwraps an optional datum tag 258, requires an array, and then uses the same calculator.
- A vkey witness is definite CBOR `[bytes32-public-key,bytes64-Ed25519-signature]`; sign the exact 32 raw body-hash bytes. No-script fee is `checked_u64(tx_preserved_cbor_length * coefficient + constant)`. Total fee is checked sum of no-script, script, and reference-script fees.
- Reference-script fee validates size as uint64 and uses 25,600-byte tiers. Starting price is the exact rational `ref_script_cost_per_byte/1`; charge `tier*price`, multiply price by `6/5` for the next tier, accumulate/reduce exact arbitrary-precision rationals, then floor once at the end and require uint64. Thus cost 10 gives size 25,600 => 256,000 and 25,601 => 256,012.
- Redeemers may be a legacy array (ExUnits at each redeemer[3]) or map (ExUnits at each value[1]); ExUnits is exactly `[mem,steps]`, each non-negative signed-int64. Sum both dimensions with checked int64 arithmetic. Script fee is `ceil(total.mem*mem_n/mem_d + total.steps*steps_n/steps_d)` using exact integers and positive denominators; no redeemer field means zero.
- Minimum ADA obtains coin from output array[1] or map key 1, where amount is either an unsigned coin or `[coin,multiasset]`. CBOR unsigned-head sizes are 1 for `<24`, 2 through 255, 3 through 65535, 5 through `2^32-1`, otherwise 9. Let `old` be the head size of the supplied current coin or encoded output coin and `latest=old`; repeatedly compute checked uint64 `tentative=(preserved_output_length + 160 + latest-old)*coins_per_utxo_byte`, replace `latest` with its head size, and return when the size is stable.
- Arbitrary metadata bytes encode as a metadata list of consecutive byte chunks of at most 64; empty bytes produce an empty list. Decode only a list whose every element is bytes, concatenate in order, and otherwise return none. Apply the metadata/Plutus JSON schemas and 128-depth/100,000-value limits stated in the JSON contract.
- Shelley genesis transaction id is Blake2b-256(raw address bytes). Byron redeem genesis creates the redeem address from public key and optional protocol magic and returns Blake2b-256(address CBOR), plus that address. Icarus bootstrap witness uses the BIP32 key’s raw private key/public key/signature, original chain code, and address attributes. Daedalus uses the first 32 bytes of legacy extended public key, legacy signature, legacy chain code, and address attributes.
- Deposit accounting reads optional tag-258 arrays at body keys 4 and 20. Deposits: certificate tag 0 adds key deposit; tag 3 pool deposit; tag/coin-position `7/2`, `11/3`, `12/3`, `13/4`, `16/2` add explicit deposits; every proposal adds procedure[0]. Implicit input/refunds start with every withdrawal-map value; certificate tags 1 and 15 add key deposit, tag 4 pool deposit, and `8/2`, `17/2` explicit refunds. All coins and totals are uint64; malformed structures and overflow reject.
- Metadata duplicate keys retain their owning ordered/pair-map semantics; JSON conversion does not silently deduplicate. Every operation above consumes preserved bytes unless it explicitly says canonical, and intermediate fee arithmetic uses arbitrary precision before the stated final bound check.

### C008 transaction-builder contract

- Config requires, in order, uint64 linear-fee coefficient/constant/reference-script price, uint64 pool/key deposits, uint32 maximum Value size and transaction size, uint64 coins-per-UTxO-byte, positive-denominator ExUnit prices, uint32 collateral percentage, and uint32 maximum collateral inputs. Cost models default to an empty map and `prefer_pure_change` to false. Collateral percentage is deliberately required/range-checked/stored but has no effect on any frozen computation; do not invent a percentage gate. The sole change strategy is `Default=0`; its argument is otherwise a no-op.
- Canonical identity is lowercase hex of canonical CBOR. Unless stated otherwise, deterministic hex ordering is ascending ASCII (equivalent to byte order for these lowercase hex keys). Byte comparison used for governance action ordering compares encoded length first, then unsigned bytes lexicographically. Map-like deduplication replaces the value at its first insertion position; it does not move the key.
- Explicit inputs reject duplicate canonical input identity. Candidate UTxOs silently ignore a duplicate or an already explicit input. Reference inputs silently deduplicate by canonical input. Withdrawals reject duplicate raw reward-address bytes. Collateral, explicit-input, and reference-input sets are not cross-deduplicated by the builder; owning-model validation remains authoritative. Selection mutates the builder incrementally and does not roll back already selected inputs on later failure.
- An output is a Conway map with keys 0 address bytes, 1 Value, optional 2 datum option, optional 3 script reference. Communication datum creates datum option `[0, Blake2b-256(preserved-datum-CBOR)]` and queues the datum witness. Adding an output requires preserved Value CBOR length `<=max_value_size` and coin `>=min_ada_required`. `with_asset_and_min_required_coin` computes minimum twice: first on identical output with coin zero, then again with that first minimum as coin. Outputs otherwise preserve insertion order.
- Input witness choice must match the UTxO payment credential. Key/Byron input collects vkey/bootstrap requirement; native/Plutus script hash must equal the address script credential. A supplied external Plutus datum must hash to the datum-hash option; inline-datum path supplies no external datum and rejects an unresolved datum hash. Required signer hashes are appended. Native signer estimates are: explicit list exactly those hashes; numeric count takes the first `min(count, discovered-required-signers)` in script discovery order; assume-count takes all discovered signers.
- Mint builders require a nonempty asset map and every signed-int64 quantity nonzero. Merge equal policy/asset by addition; int64 overflow rejects and an exactly-zero combined result rejects rather than deleting the asset. Withdrawals are uint64. Certificate credential positions are tag 1/2/5..16 at element 1; tag 4 additionally requires the pool key at element 1; tag 3 requires pool operator at registration[0] and every owner in registration[6]. Tag 0 has no credential requirement. Proposals retain insertion order. A `VoteBuilder` deduplicates voter/action pairs by canonical voter+action with later value replacing in place; adding separate vote-builder results does not perform another deduplication.
- Collateral accepts only a result without native/Plutus aggregate witness data, enforces `count < max_collateral_inputs`, and adds its key/bootstrap requirement. Main balance excludes collateral. If collateral return exists, total collateral is `sum(collateral input coins)-return coin`, rejects return coin above sum, and deliberately ignores native assets. It does not apply collateral percentage.
- Witness identity is vkey by public-key hash, bootstrap by public key, native/Plutus script by script hash, datum by datum hash, redeemer by `tag:index`; later equal identities overwrite in place. A discovered reference script is recognized only when ScriptRef is a tag whose value is bytes containing a decodable `Script`; it satisfies/removes the matching inline-script requirement and dominates a later inline requirement. Checked witness build rejects any remaining vkey/bootstrap/script/datum/redeemer requirement; unchecked build emits what is present. Witness fields 0,1,2,3,4,6,7 are tag-258 definite arrays; legacy redeemers at key 5 are an untagged definite array. Fake sizing witnesses are created only for missing vkeys: 32-byte public key with byte 0 equal to an incrementing id modulo 256 and all other bytes zero, plus a zero 64-byte signature; missing non-vkey witnesses are not faked.
- Redeemer purpose order is Spend, Mint, Cert, Reward, Voting, Proposing. Within each purpose sort stably by: spend canonical input hex; mint policy hex; reward raw address hex; Plutus-certificate ordinal as zero-padded 10-digit decimal among Plutus certificate sources; proposal index as zero-padded 10-digit decimal local to each proposal-builder result (equal indices from separate batches retain insertion order); voting canonical voter hex + `:` + canonical action hex. Assign zero-based index within purpose. ExUnit overrides are keyed by final `tag:index`. Missing ExUnits reject normal/evaluation output; dummy/draft sizing substitutes `(0,0)`. A redeemer is `[tag,index,data,ExUnits]`. Script-data body hash is emitted only when at least one redeemer exists.
- Auxiliary-data merge replaces the right operand wholesale if either operand is not a map. For two maps, key identity is canonical-key CBOR hex; a right value replaces at the left entry’s original position and a new key appends. Withdrawals sort by raw reward-address hex. Mint policies and assets sort by lowercase hex. Voting groups sort by canonical voter hex; within a voter, actions sort by canonical action CBOR using length-first/bytewise ordering. Transaction inputs sort by canonical input hex; outputs/certificates/proposals/collateral/reference inputs retain insertion order. Body keys are emitted ascending: mandatory 0 inputs, 1 outputs, 2 fee; optional 3 ttl, 4 tag-258 certs, 5 withdrawals, 7 auxiliary hash, 8 validity start, 9 mint, 11 script-data hash, 13 tag-258 collateral, 14 tag-258 required signers, 15 network, 16 collateral return plus 17 computed total collateral, 18 tag-258 references, 19 votes, 20 tag-258 proposals, 21 treasury, 22 donation. Donation setter accepts uint64 but body construction rejects zero.
- Accounting is `total_input=explicit inputs+withdrawals/refunds+positive mint`; `total_output=explicit outputs+deposits+negative mint+donation`, with checked Value/uint64 arithmetic. Reference inputs and collateral do not enter main balance.

### Exact CIP-2 selection

- Strategies are `LargestFirst=0`, `RandomImprove=1`, `LargestFirstMultiAsset=2`, `RandomImproveMultiAsset=3`. Candidate order begins as unselected added-UTxO insertion order. The target is current `total_output`; fee is not pre-added. Coverage is tested dynamically by computing `min_fee(true)` for the currently selected set and checking `total_input >= total_output + fee` in every coin and asset dimension. Any exception during this test means “not covered.” Plain RandomImprove rejects if any explicit output contains native assets; plain LargestFirst does not reject and ignores assets until the final coverage test.
- `largest_first_by(q,target)` filters remaining candidates with `q>0`, sorts by descending quantity then ascending canonical input hex, starts from `q(current total_input)`, and selects in that order until total is at least target; insufficient quantity rejects. MultiAsset LargestFirst invokes this once for every target policy/asset in target MultiAsset insertion order, then both largest strategies scan remaining candidates by descending coin, tie canonical input hex ascending, adding until dynamic coverage. Final noncoverage rejects.
- Fisher-Yates shuffle copies the input and for `i=n-1..1` swaps `i` with `rng32 % (i+1)`. Production `rng32` is four OS-CSPRNG bytes interpreted little-endian unsigned uint32. The recorded deterministic source starts at `0x5eedc1f2` and on each call applies uint32 xorshift `s ^= s<<13; s ^= s>>>17; s ^= s<<5`, returning `s`.
- `random_improve_by(q,targets)` filters remaining `q>0`, Fisher-Yates shuffles once, sorts positive targets descending, and for each target repeatedly **pops from the end** of that same shuffled array until the group sum reaches the target; each popped candidate is removed from remaining. Exhaustion/overflow rejects. Improvement then shuffles the still-unselected positive candidates. Visit every selected slot in group order. Candidate is `improvements[cursor % size]`, advancing cursor. For that group target set `ideal=2*target`, `maximum=3*target`; compare absolute distance of the individual current UTxO quantity and individual candidate quantity to ideal (not group totals). Replace only when candidate distance is strictly smaller and candidate quantity is strictly below maximum. On replacement, return current to remaining, remove candidate, replace candidate with current in both leftover/shuffled pools, and continue. Equal distance and quantity exactly `3*target` do not replace. Finally add all associated groups in group/slot order.
- MultiAsset RandomImprove runs that routine for each target policy/asset in target insertion order; per-asset targets are the positive quantities of each explicit output in output order (then sorted descending by the routine). It next runs for positive explicit-output coin amounts. Plain RandomImprove runs only that coin phase. Finally both random strategies Fisher-Yates shuffle all remaining candidates (including zero quantities for prior dimensions) and add until dynamic fee/value coverage. Burn-only/deposit/donation/fee needs are therefore handled by this final fill, not by per-output target groups. Final noncoverage rejects.

### Fee sizing and change

- `min_fee(include_exunits)` temporarily places `UINT64_MAX` in body fee, builds without the max-size gate, includes current legacy redeemers and present/fake witnesses, and measures the preserved transaction bytes. If `include_exunits=false`, return `length*coefficient+constant`; otherwise add exact script and reference-script fees from C007. Reference-script size sums selected-input and reference-UTxO outputs; if ScriptRef is `tag(bytes)`, count embedded byte-string payload length, otherwise count the full ScriptRef CBOR length. `fee_for_input/output` is preserved object CBOR length times coefficient. Final max transaction size is enforced only when building the final/evaluation body, using the full transaction with fake vkeys.
- Change iterates at most eight times. Save original output count and start with set fee or zero. Each iteration removes previous provisional change outputs, computes checked `change=total_input-(total_output+fee)`, emits provisional change, sets body fee, recomputes minimum fee, stops if equal, otherwise uses the new fee. After eight iterations use the last computed fee even if not converged. Remove provisional outputs once more, recompute final change at that fee, and emit it. If no output was emitted and final change is nonzero, native assets reject; coin-only remainder is added to fee. Store fee and return whether any change output was added.
- Clean change assets by retaining only positive quantities. With assets, split greedily in policy/asset insertion order: append an asset iff `CBOR_length(Value(coin=0,trial_bundle)) <= max_value_size`; otherwise close the prior bundle, and reject if one asset alone exceeds the limit. This size test deliberately uses coin zero and Value only. For each bundle compute minimum coin twice (zero-coin probe, then first-minimum probe). If total change coin is below sum of minima, reject. Let remainder be coin minus minima. If `prefer_pure_change` and remainder is positive and a pure-coin output of exactly remainder meets its minimum ADA, emit that pure output first and set remainder zero. Emit asset bundles in order; the first receives its minimum plus all remaining coin and later bundles receive their minima. Without assets, emit one pure output only if coin meets minimum ADA; otherwise emit none so the final step absorbs it into fee. Native-asset change can never be absorbed into fee.

## Required compatibility dispositions

- Preserved and canonical CBOR are separate operations. Duplicate maps, map order, nonminimal
  heads, chunks, tags, and indefinite containers remain representable.
- Current public decoders are complete-input and resource-bounded.
- Accept the frozen legacy Byron SSC certificate/share shapes documented by `typescript/0004`.
- Accept wider historical Conway protocol-parameter fields only while ingesting the frozen block
  corpus; direct Conway APIs remain strict to the captured official CDDL.
- Preserve the accepted historical address trailing-byte whitelist and long-address behavior.
- CIP-25 uses its deliberately narrower normalization policy; accepted noisy input is not promised
  a byte-identical re-encoding.
- CIP-8 protected bytes, context spelling, external AAD, detached payload choice, and Sign versus
  Sign1 shape are signature-sensitive.
- Only internal V1/V2 phase-two script decoding accepts a valid first CBOR object with trailing
  bytes. V3, generic CBOR, and parameter application remain strict.
- V1 retains the historical oversized definite Data-leaf behavior; V2 and later enforce the
  64-byte on-wire rule.
- Protocol-11 universe/constructor limits and D/E byte/string semantics do not apply to earlier
  variants. V3 evaluation requires Unit; V1/V2 accept any successful result.
- Current cost-model mappings contain 332 V1, 332 V2, and 350 V3 parameters. Ignore extra tail
  values silently and fill missing tail values with signed `INT64_MAX` silently.
- Every redeemer receives the same independent maximum budget. Public CPU/memory ordering maps
  explicitly to ledger steps/memory ordering.
- Collection getters and returned byte containers do not expose mutable internal storage.
- The aggregate facade aliases the owning C++ types. JavaScript module identity, `Uint8Array`,
  garbage-collector disposal, Web Crypto, ESM, Node, bundler, and npm behavior are replaced by the
  C++ ownership, container, RAII, CMake, and installed-target contracts above.

## Implementation steps

1. Create the independent CMake workspace, public target graph, C++23 feature checks, exact vcpkg
   baseline/overrides, compiler presets, install/export configuration, and test-only Catch2 setup.
   Record every dependency purpose and keep third-party types private.
2. Implement core bytes, results/errors, numbers, collections, network values, Bech32, and the
   cursor-based lossless CBOR reader/writer. Complete malformed, limit, preservation, canonical,
   mutation, and property coverage before domain codecs depend on it.
3. Implement the owned cryptography adapters, fixed-size wrappers, keys, derivation, randomness,
   secret lifecycle, encryption, UPLC primitives, and Byron compatibility. Pass all positive,
   negative, malformed, and dependency-policy tests before enabling signing consumers.
4. Implement addresses, genesis, shared chain values, and each era independently from Byron
   through Conway. Map every captured official CDDL rule to an owner and disposition; then add
   multi-era dispatch and the exact historical compatibility exceptions.
5. Implement ledger operations and transaction/witness builders, including CIP-2, change,
   collateral, governance, metadata, deterministic redeemer indexing, draft valuation, and signing.
   Prove balance, fee, size, minimum-ADA, and signing-payload invariants.
6. Implement CIP-25 and CIP-36 from the frozen requirements and relevant captured CDDL, then CIP-8
   from the accepted result and message-signing evidence. Reuse core, crypto, and chain owners.
7. Implement typed Data without duplicating ledger nominal types. Implement iterative UPLC codecs,
   cost models, builtins, CEK, parameter application, private ledger context construction, and raw
   phase-two valuation from the accepted result and complete captured evidence.
8. Complete focused headers, the aggregate facade, installable components, examples, and
   `API_PARITY.md`. Give every explicit change, public-inventory row and owned binding, feature-map
   item, compatibility disposition, and type/JSON contract in this instruction one owner, C++
   mapping, compatibility note, and validation reference without consulting TypeScript source.
9. Run evidence integrity, focused tests, full conformance, deterministic hardening, sanitizers,
   static analysis, the macOS Apple Clang build, installed-consumer checks, and the completion workflow.
   Create the paired result only after every required change has a disposition and every required
   gate passes.

No implementation step may fetch, refresh, run, translate, or substitute upstream provider
material beyond the declared snapshots. Exact dependency archives may be resolved only through
the pinned vcpkg manifest and overlay above. Captured artifacts may be read only as evidence and
tests; they may not generate C++ source, headers, exports, tests, package metadata, or dependency
manifests.

## Validation

### Evidence and conformance

- Verify the exact nonempty inventory, regular-file type, provenance, license mapping, and
  checksums for every declared provider snapshot before consuming it.
- Validate all 92 frozen CML vectors directly from their immutable snapshot paths: six genesis
  vectors, 85 successful historical block decodes with byte-exact preserved round trips, and one
  recorded rejection. Do not duplicate provider fixtures under `libs/cpp/`.
- Validate all seven official era CDDL artifacts plus Ledger license/notice inventory. Record a
  disposition for every supported Byron-through-Conway rule.
- Validate the message-signing checksum inventory and all exact CIP-8 signing/wire examples.
- Validate all 139 UPLC artifact checksums, all 3,013 embedded corpus entry sizes and hashes, and
  all 1,003 applicable official UPLC programs, budgets, Flat goldens, cost models, and contexts.

### C++ gates

- `cmake --workflow --preset ci` is the authoritative C++ completion command. It configures with
  warnings as errors, builds every component and test, runs all CTest labels, installs the package,
  and builds/runs a clean `find_package` consumer against the install tree.
- `cmake --workflow --preset sanitizers` must pass with AddressSanitizer, UndefinedBehaviorSanitizer,
  and supported leak detection on Apple Clang.
- `cmake --workflow --preset hardening` must run deterministic malformed-CBOR, nested
  preservation/canonicalization, JSON/Data, signature mutation, UPLC depth, and transaction-builder
  invariant campaigns using seed `0xc0b012f0`, including an extended 50,000-case malformed-input run.
- Run the frozen macOS 26.5 ARM64 Apple Clang 21.0 environment above, including provider-backed
  tests.
- Run clang-tidy over owned source when available, check formatting, reject
  warnings, and scan installed/public headers for forbidden dependency types.
- Inspect the installed archive/shared library, headers, CMake metadata, license, and examples.
  Reject provider artifacts, TypeScript/JavaScript, Haskell/Rust source, test-only dependencies,
  secrets, build paths, or unplanned binaries in the installed package.

Expected evidence includes exact wire and hash/signature outputs, strict malformed-input failures,
preserved and canonical CBOR results, deterministic cost/budget results, era/context parity,
builder balance, API ownership, clean sanitizer output, and successful installed consumers.
TypeScript workspace metrics are neither implementation inputs nor C++ gates.

## Compatibility and human review

- Human review must confirm the complete instruction-requirement crosswalk, package ownership,
  era-rule dispositions, historical exceptions, JSON mappings, error categories, and every
  language-specific adaptation.
- CBOR canonicalization, preserved metadata, malformed inputs, allocation limits, address parsing,
  transaction construction, script-data hashes, UPLC Flat/CEK/cost accounting, phase-two contexts,
  and CPU/memory conversion are consensus- or security-sensitive.
- Independent cryptography review must cover key derivation, signing, randomness, secret handling,
  EMIP-3, compact secp256k1 acceptance, BIP-340 message rules, BLS subgroup/infinity/compression,
  hash-to-group domain separation, and dependency configuration. Automated vectors are not a
  security audit.
- C++ ownership and RAII improve control over memory but do not guarantee secret erasure in
  registers, temporary dependency buffers, core dumps, swapped memory, or copied caller storage.
- Review all MIT, Apache-2.0, and dependency license/notice obligations before distribution.
- Performance and memory use must be measured on historical blocks and the UPLC corpus. Resource
  limits and iterative algorithms may not be removed merely to improve benchmark results.
- Initial compatibility is semantic and wire-level. No stable C++ ABI, C ABI, or source-level
  compatibility with the TypeScript API is promised.

## Completion criteria

- Every `C001` through `C013` requirement has an `IMPLEMENTED` disposition in the paired result,
  or the implementation remains `PLANNED`; partial feature parity may not move this record to
  `REVIEW`.
- `libs/cpp/` is an independent, documented, installable C++23 workspace whose completion command
  does not build or invoke the TypeScript workspace.
- Every explicit requirement, public-inventory row and owned binding, feature-map item,
  compatibility disposition, and type/JSON contract in this instruction has an owned C++ mapping
  and validation reference in `API_PARITY.md`; only the language mechanics explicitly identified
  here have reviewed `LANGUAGE_SPECIFIC` dispositions.
- Core, crypto, chain, CIP, Plutus, and aggregate consumers compile independently and refer to the
  same owned nominal C++ types.
- Provider inventories and checksums pass; historical vectors retain their recorded outcomes;
  every supported official era rule has a disposition; all applicable UPLC conformance programs
  and budget vectors pass.
- Preserved/canonical CBOR, compatibility exceptions, cryptographic acceptance, builders, CIPs,
  typed Data, UPLC, phase-two valuation, malformed inputs, resource limits, and defensive
  ownership have focused regression coverage.
- Exact dependencies are pinned and private, all required toolchain/sanitizer/hardening/install
  gates pass, and installed packages contain only intended C++ runtime material and notices.
- The paired result names every input actually consumed, exact dependency and toolchain versions,
  every deviation, complete validation evidence, remaining human review, and a language-neutral
  exported change contract.

## Out of scope

- Dijkstra, Plutus V4, protocol major 12, nested transactions, provisional ledger behavior, or
  any provider material outside the declared snapshots
- Typed Plutus Core, a UPLC compiler, optimizer, pretty-printer, debugger, CLI, code generator,
  protocol override callback, or script replacement callback
- Full Cardano node consensus/state-transition operation, networking, wallet persistence, a
  CIP-30 wallet/provider API, mnemonic-word parsing, hardware-wallet integration, or key storage
- COSE encryption/recipients, private COSE key serialization, or builder-created private label `-4`
- JavaScript/TypeScript bindings, Node addons, WebAssembly, browser packaging, Python/Rust/Java
  bindings, a stable C ABI, or a stable C++ binary ABI
- Copying or mechanically translating TypeScript, Haskell, or Rust source; generated runtime
  source, headers, public exports, tests, or package metadata; loading provider artifacts at runtime
- Fetching new upstream material, refreshing provider snapshots, changing captured evidence, or
  accepting later TypeScript features without a new C++ implementation sequence
- Linux, Windows, x86-64, GCC, upstream Clang, MSVC, and other non-macOS-ARM64 platform/compiler
  support; those require later C++ implementation records

## Blockers

None. Any authority conflict, unresolved instruction-requirement mapping, unavailable required
dependency primitive, unsupported provider construct, or ambiguous compatibility behavior
discovered during implementation is a stop condition and requires a new instruction.
