# ADR 0004: Cryptography primitive selection

- Status: Accepted
- Date: 2026-07-27

## Context

XRAY Cardano Lib requires browser-native Ed25519, Cardano Ed25519-BIP32, hashing, encryption, UPLC
secp256k1, and BLS12-381 operations. These primitives are security-sensitive, and UPLC operations
are consensus-visible.

## Decision

- Pin `@noble/curves`, `@noble/hashes`, and `@noble/ciphers` at exactly `2.2.0`. They are pure ESM
  JavaScript packages with no native addon or WASM artifact.
- Require Node.js 20.19 or newer, matching the exact dependency pins, while retaining browser
  support without `Buffer`, `process`, native code, or WASM.
- Use Cardano-owned wrappers for Blake2b, SHA-2, SHA-3, Keccak-256, RIPEMD-160, HMAC/PBKDF2,
  ChaCha20-Poly1305, Ed25519, secp256k1 ECDSA/Schnorr verification, and BLS12-381 group, hash,
  compression, and pairing operations.
- Keep Cardano Ed25519-BIP32 derivation, extended signing, protocol-specific lengths, formats,
  domain separation, and acceptance rules in owned TypeScript.
- Keep dependency-specific points, fields, and cipher objects private. Public boundaries use owned
  values, `Uint8Array`, and canonical compressed encodings.
- Treat malformed encodings, invalid subgroups, infinity restrictions, ECDSA low-S acceptance,
  BIP-340 message length, BLS domain separation, key handling, and randomness as
  security-sensitive behavior.
- Use `globalThis.crypto.getRandomValues` through the injectable secure-random boundary and fail
  closed when it is unavailable.
- Require authoritative positive, negative, malformed, subgroup, browser, and dependency-policy
  tests for every primitive and wrapper.

## Consequences

The production graph remains pure universal ESM and supports Node.js 20.19 or newer and modern
browsers. Dependency upgrades or changes to wrapper acceptance rules require a security review,
updated vectors, browser checks, and the complete TypeScript gate.

JavaScript cannot guarantee constant-time execution or secret zeroization. Owned secret arrays
are overwritten on disposal as a best effort, but runtime copying and garbage collection may
retain inaccessible copies. Automated vector coverage does not replace independent review before
a stable release.
