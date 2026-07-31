# EMURGO Message Signing provider

Provider: message-signing
Provider-Version: v1

## Purpose

Capture EMURGO's CIP-0008/COSE message-signing implementation as immutable evidence for a
browser-native, package-owned Cardano Lib TypeScript implementation. The captured Rust is a
behavior and wire-format reference; it is not a runtime dependency, generated source, or
instruction set.

## Source

| Field | Value |
| --- | --- |
| Repository | `https://github.com/Emurgo/message-signing.git` |
| Followed ref | `refs/heads/master` |
| Revision policy | Full commit reachable from the followed ref |
| Source mode | Live; resolve independently for every snapshot |
| Submodules | Not part of the source |
| License | MIT |

A release tag is descriptive evidence only. The resolved full commit is authoritative.

## Artifact selection

Copy these regular files byte-for-byte:

| Upstream path | Snapshot artifact |
| --- | --- |
| `README.md` | `artifacts/upstream/README.md` |
| `rust/Cargo.toml` | `artifacts/upstream/rust/Cargo.toml` |
| `rust/src/builders.rs` | `artifacts/upstream/rust/src/builders.rs` |
| `rust/src/cbor.rs` | `artifacts/upstream/rust/src/cbor.rs` |
| `rust/src/crypto.rs` | `artifacts/upstream/rust/src/crypto.rs` |
| `rust/src/error.rs` | `artifacts/upstream/rust/src/error.rs` |
| `rust/src/lib.rs` | `artifacts/upstream/rust/src/lib.rs` |
| `rust/src/serialization.rs` | `artifacts/upstream/rust/src/serialization.rs` |
| `rust/src/utils.rs` | `artifacts/upstream/rust/src/utils.rs` |
| `examples/rust/src/main.rs` | `artifacts/upstream/examples/rust/src/main.rs` |
| `LICENSE` | `artifacts/legal/LICENSE` |

The selected upstream inventory is exactly eleven byte-exact regular files. Reject a missing,
additional, renamed, symlinked, gitlinked, special, or unexpectedly large selected file.

Create `artifacts/SHA256SUMS` as deterministic snapshot-local integrity metadata. It contains one
line for each of the eleven byte-exact artifacts, sorted by artifact-relative path in byte order,
using lowercase SHA-256, two ASCII spaces, the path relative to `artifacts/`, and a trailing
newline. It does not list itself.

## Evidence-only paths

Inspect these at the candidate commit when present, but do not copy them:

- the Git history and source diff from the latest earlier release tag;
- `package.json` and `rust/Cargo.lock` for package identity and dependency evidence;
- embedded `#[cfg(test)]` modules within the selected Rust source;
- `.gitmodules` and the root tree inventory for exclusion and file-type validation.

Do not run any upstream test, build, script, package manager, binary, filter, or submodule.

## Consumption and planning requirements

- Implement message signing as ordinary TypeScript owned by the Cardano Lib package selected
  during preparation; do not publish or invoke upstream Rust, WASM, ASM.js, or native artifacts.
- Reuse Cardano Lib's existing generic lossless CBOR and cryptography owners. Do not duplicate
  generic CBOR values, integers, Ed25519 key/signature classes, or Blake2b primitives.
- Map the selected signing structures, protected and unprotected headers, signature structures,
  builders, COSE key representation, detached payload behavior, external AAD, payload hashing,
  and `cms_` user-facing encoding to exact local owners and tests.
- Preserve complete-input decoding, configured CBOR resource limits, duplicate-header rejection,
  protected-header byte semantics, deterministic constructed output, and existing browser-safe
  package boundaries.
- Compare a later snapshot with the latest accepted snapshot from this provider. For the initial
  snapshot, compare the candidate with the latest earlier upstream release and the current Cardano
  Lib CBOR, cryptography, CIP, runtime facade, and packaging behavior.
- A new snapshot at the same exact commit and provider version is a duplicate.

## Excluded source material

- The `binaryen` submodule/gitlink and all other submodules
- Upstream build, release, CI, package-manager, generated binding, WASM, ASM.js, and JavaScript
  helper material
- Upstream lockfiles and standalone empty `rust/src/tests.rs`
- COSE encryption, recipients, password encryption, public-key encryption, and cipher builders
- Generic CBOR wrapper APIs already owned by `@xray-network/cardano-core`
- `EdDSA25519Key.set_private_key` and builder-created private-key label `-4`
