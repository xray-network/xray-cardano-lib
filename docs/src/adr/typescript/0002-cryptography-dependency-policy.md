# ADR 0002: Security, cryptography dependency, and randomness policy

- Status: Accepted
- Date: 2026-07-22

## Context

The crypto package handles keys, signatures, derivation, hashes, Bech32, Byron behavior, and EMIP-3
in both Node and browsers. A dependency choice can silently introduce native/WASM artifacts,
runtime-specific behavior, permissive length handling, or insecure randomness. Selecting exact
libraries before the primitive-by-primitive vector audit would be premature, but the acceptance
policy must be fixed now.

## Decision

- Runtime cryptography must be native JavaScript/TypeScript or standards-based Web Crypto. No
  Rust, WASM, native addon, or network service is permitted in a published package.
- Put each external primitive behind a narrow internal interface. Public classes enforce Cardano
  lengths, formats, hardened/soft derivation rules, and error behavior themselves.
- A dependency is accepted only after it demonstrates Node/browser support, deterministic vectors,
  an actively maintained security posture, a compatible license, no hidden binary artifact, and
  acceptable bundle/performance characteristics.
- Pin every accepted dependency to an exact version in the single npm lockfile. Record its purpose,
  vectors, upstream audit/security evidence, and replacement boundary in a follow-up dependency
  decision.
- Use `globalThis.crypto.getRandomValues` through an injectable `SecureRandomSource`. Fail closed if
  it is unavailable. Never use `Math.random`, timestamp entropy, or an undocumented fallback.
- Random key generation is tested for CSPRNG availability and non-repetition; deterministic
  differential tests use recorded private material and derivation/signing vectors.
- Overwrite owned secret buffers on `free()`/dispose as best effort and avoid implicit string/log
  conversions. Document that JavaScript garbage collection and copying prevent guaranteed
  zeroization.

## Consequences

- No cryptographic implementation is considered complete merely because TypeScript compiles or a
  dependency exposes a similarly named function.
- Blake2b, Ed25519, Cardano Ed25519-BIP32, legacy Daedalus, Bech32, and EMIP-3 each need independent
  authoritative and independently published vectors.
- Adding a dependency that violates the binary-free browser graph requires rejecting it, not hiding
  it behind a facade.
- Stable release requires independent security review of wrappers and key handling.

## Vulnerability reporting

Report suspected vulnerabilities privately through the repository's security-reporting channel.
Do not open a public issue before maintainers have had an opportunity to investigate and
coordinate a fix. Never include private keys, seed phrases, or production signing material in a
report.

A report should include the affected package and version, impact, reproduction steps or a proof of
concept, and any suggested mitigation. Maintainers should acknowledge the report, assess severity,
coordinate a release, and credit the reporter unless anonymity is requested.

## Repository security controls

Published packages are ESM-only TypeScript/JavaScript and must not contain WebAssembly, native
addons, or platform-specific binaries. Production dependencies are exactly pinned and limited to
reviewed workspace and cryptography packages.

The complete validation gate is:

```sh
npm --prefix libs/typescript run check
```

It verifies:

- lossless and canonical CBOR behavior, malformed inputs, resource limits, duplicate keys,
  recursion, truncation, and allocation pressure;
- JSON conversion, recursive shape limits, prototype-pollution resistance, and error handling;
- Ed25519, extended keys, BIP32 derivation, EMIP-3, randomness failure, secret copying, and
  best-effort disposal;
- transaction and witness builders, balance preservation, coin selection, fees, change, and
  signing payloads; and
- dependency pinning, lockfile integrity, licenses, forbidden binaries, browser-safe package
  boundaries, and packed-package consumers.

## Hardening evidence

The deterministic hardening campaign uses seed `0xc0b012f0` and covers malformed and nested CBOR,
metadata and Plutus JSON, signatures, and transaction-builder seeds. Run the extended campaign
with:

```sh
XRAY_CARDANO_LIB_HARDENING_CASES=50000 node --test libs/typescript/packages/runtime/test/hardening.test.mjs
```

The recorded extended run completed 50,000 malformed-CBOR cases without crashes, hangs, or
semantic failures.

## Independent review

Automated tests and self-review are not an audit opinion. No independent XRAY Cardano Lib security
review has been commissioned. Before a stable security-sensitive release, an independent reviewer
must inspect:

- CBOR decoding and canonicalization;
- JSON and recursive data handling;
- cryptographic primitives, key derivation, randomness, and secret lifecycle;
- transaction construction and signing boundaries; and
- package contents and dependency controls.

The review record must identify the reviewer, reviewed revision, dates, methods, findings,
dispositions, and a verifiable report location or digest. No critical or high-severity finding may
remain unresolved at release.
