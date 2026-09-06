# EMURGO Message Signing provider snapshot

Provider-Snapshot-Version: v1
Snapshot: 0001-message-signing
Provider: message-signing
Created: 20260727T084625Z
Previous-Snapshot: ./SNAPSHOT.md
Provider-Version: v1
Source-Type: git
Source-Repository: https://github.com/Emurgo/message-signing.git
Source-Commit: f76a82442594c8435fb577cb85da3ad594cf1063
Source-Ref: refs/heads/master
Source-Tag: 1.1.0

## Evidence objective

Preserve EMURGO Message Signing 1.1.0 signing behavior as immutable evidence for Cardano Lib
implementations.

## Captured scope

The snapshot contains eleven byte-exact upstream files: the upstream README, Rust manifest, eight
selected Rust source/example files, and MIT license. `artifacts/SHA256SUMS` is the deterministic
integrity inventory.

The evidence covers COSE Sign and Sign1 models, labels and headers, protected-header bytes,
signature structures, public COSE keys, detached payloads, external AAD, Blake2b-224 payload
hashing, and `cms_` user-facing encoding.

## Integrity and licensing

The exact inventory is defined by [`PROVIDER.md`](../PROVIDER.md). All selected upstream files are
listed by lowercase SHA-256 in `artifacts/SHA256SUMS`. The captured upstream license is MIT.

## Semantic evidence

Release 1.1.0 fixes builder payload hashing so hashing is applied once and records the Sign1
unprotected `"hashed"` header as true after hashing. Captured source and examples are behavioral
and wire-format evidence only; no implementation may execute or ship the captured Rust.

## Exclusions

Encryption, recipients, private COSE key serialization, submodules, generated bindings, WASM,
ASM.js, native code, build/release tooling, and upstream runtime dependencies are excluded.

