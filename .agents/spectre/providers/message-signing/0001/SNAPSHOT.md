# EMURGO Message Signing provider snapshot

Provider-Snapshot-Version: v1
Snapshot: 0001
Provider: message-signing
Created: 20260727T084625Z
Previous-Snapshot: NONE
Source-Type: git
Source-Repository: https://github.com/Emurgo/message-signing.git
Source-Commit: f76a82442594c8435fb577cb85da3ad594cf1063
Source-Ref: refs/heads/master
Source-Tag: 1.1.0
Source-URL: NONE
Source-SHA256: NONE

## Evidence objective

Preserve EMURGO Message Signing 1.1.0 signing behavior as immutable evidence for Cardano Lib
implementations.

## Frozen capture specification

The rules below preserve this capture’s original source selection, mappings, transformations,
completeness, licensing and consumer boundaries. They apply only to the immutable identities
recorded above. Discovery/ref and future-planning statements describe the original capture context;
they cannot retarget this snapshot. Repository-root paths remain repository-root-relative;
`artifacts/` paths resolve from this directory. No mutable provider guide supplies normative rules.

### Purpose

Capture EMURGO's CIP-0008/COSE message-signing implementation as immutable evidence for a
browser-native, package-owned XRAY Cardano Lib TypeScript implementation. The captured Rust is a
behavior and wire-format reference; it is not a runtime dependency, generated source, or
instruction set.

### Source

| Field | Value |
| --- | --- |
| Repository | `https://github.com/Emurgo/message-signing.git` |
| Followed ref | `refs/heads/master` |
| Revision policy | Full commit reachable from the followed ref |
| Source mode | Immutable captured evidence; discovery policy below does not change the recorded commit |
| Submodules | Not part of the source |
| License | MIT |

A release tag is descriptive evidence only. The resolved full commit is authoritative.

### Artifact selection

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

### Evidence-only paths

Inspect these at the candidate commit when present, but do not copy them:

- the Git history and source diff from the latest earlier release tag;
- `package.json` and `rust/Cargo.lock` for package identity and dependency evidence;
- embedded `#[cfg(test)]` modules within the selected Rust source;
- `.gitmodules` and the root tree inventory for exclusion and file-type validation.

Do not run any upstream test, build, script, package manager, binary, filter, or submodule.

### Consumption and planning requirements

- Implement message signing as ordinary TypeScript owned by the XRAY Cardano Lib package selected
  during preparation; do not publish or invoke upstream Rust, WASM, ASM.js, or native artifacts.
- Reuse XRAY Cardano Lib's existing generic lossless CBOR and cryptography owners. Do not duplicate
  generic CBOR values, integers, Ed25519 key/signature classes, or Blake2b primitives.
- Map the selected signing structures, protected and unprotected headers, signature structures,
  builders, COSE key representation, detached payload behavior, external AAD, payload hashing,
  and `cms_` user-facing encoding to exact local owners and tests.
- Preserve complete-input decoding, configured CBOR resource limits, duplicate-header rejection,
  protected-header byte semantics, deterministic constructed output, and existing browser-safe
  package boundaries.
- Compare a later snapshot with the immediately previous snapshot from this provider. For the initial
  snapshot, compare the candidate with the latest earlier upstream release and the current Cardano
  Lib CBOR, cryptography, CIP, runtime facade, and packaging behavior.

### Excluded source material

- The `binaryen` submodule/gitlink and all other submodules
- Upstream build, release, CI, package-manager, generated binding, WASM, ASM.js, and JavaScript
  helper material
- Upstream lockfiles and standalone empty `rust/src/tests.rs`
- COSE encryption, recipients, password encryption, public-key encryption, and cipher builders
- Generic CBOR wrapper APIs already owned by `@xray-network/cardano-core`
- `EdDSA25519Key.set_private_key` and builder-created private-key label `-4`

### Verified artifact inventory

This table freezes all 12 regular artifacts, including existing control files. Paths are
relative to this snapshot. Hashes and sizes were verified during the metadata conversion on
2026-09-09; this local verification does not claim a new source-tree capture or upstream audit.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `artifacts/SHA256SUMS` | 1017 | `94565ab19b1fcfbf0b0c81087afcfe79e6ad6cf6f536d159dd2b81e4bfda736b` |
| `artifacts/legal/LICENSE` | 1063 | `468d2006efb2859a8d487f16ca8381d9303fed1decd2485ebf86586b3458d5bb` |
| `artifacts/upstream/README.md` | 2182 | `0f4af48fca956ab95c66c4ee7d852e1338edd2b38051216f514353ae1dc36dd6` |
| `artifacts/upstream/examples/rust/src/main.rs` | 2781 | `0b4bf54d2d0efc2905764905f96761718248a017279b64bafc143ef65dd41624` |
| `artifacts/upstream/rust/Cargo.toml` | 705 | `2f5193c2d6eb12195719ff19c878bdf262f658e7e5286f4eb01210c73db3540d` |
| `artifacts/upstream/rust/src/builders.rs` | 7635 | `214af431be9940077061ea059ad9eefb30e1d79718add30877ec96823d337b27` |
| `artifacts/upstream/rust/src/cbor.rs` | 26251 | `da3b189bc9d3153b5df17b1e3d879d8a6821cfee44503b6e60787c1c46d3ddef` |
| `artifacts/upstream/rust/src/crypto.rs` | 1591 | `afaa8a8856cff6e7bdce7e1b14d0ccd3dbc5f5113bd7a7f577d029e58d52c89c` |
| `artifacts/upstream/rust/src/error.rs` | 4997 | `ddf3739a76d02d31a038301f935326283edf71cde89379dfb81dd59fbdd641d7` |
| `artifacts/upstream/rust/src/lib.rs` | 29756 | `59eb12de0db4342b98c6198e99816bd3496e35aabc534046b3e905bca8169227` |
| `artifacts/upstream/rust/src/serialization.rs` | 54791 | `93f061b44eb7a8c76b57ab811ff2292265891af9a671a253d1636e9ca4a0eec8` |
| `artifacts/upstream/rust/src/utils.rs` | 12193 | `10a92fb27a46abf80f5bcb5307e6f701637aea0c4fcc4925292ce456d764f3e4` |

## Captured scope

The snapshot contains eleven byte-exact upstream files: the upstream README, Rust manifest, eight
selected Rust source/example files, and MIT license. `artifacts/SHA256SUMS` is the deterministic
integrity inventory.

The evidence covers COSE Sign and Sign1 models, labels and headers, protected-header bytes,
signature structures, public COSE keys, detached payloads, external AAD, Blake2b-224 payload
hashing, and `cms_` user-facing encoding.

## Integrity and licensing

The exact inventory is defined by [this frozen capture specification](#frozen-capture-specification). All selected upstream files are
listed by lowercase SHA-256 in `artifacts/SHA256SUMS`. The captured upstream license is MIT.

## Semantic evidence

Release 1.1.0 fixes builder payload hashing so hashing is applied once and records the Sign1
unprotected `"hashed"` header as true after hashing. Captured source and examples are behavioral
and wire-format evidence only; no implementation may execute or ship the captured Rust.

## Exclusions

Encryption, recipients, private COSE key serialization, submodules, generated bindings, WASM,
ASM.js, native code, build/release tooling, and upstream runtime dependencies are excluded.

## Change summary

| Change | Evidence | Observed significance |
| --- | --- | --- |
| Baseline | [Captured scope](#captured-scope) and [artifact inventory](#verified-artifact-inventory) | Initial same-provider snapshot; no previous snapshot exists. Original comparison-source observations remain above. |
| Metadata conversion | [Migration provenance](#migration-provenance) | Capture rules are now self-contained; artifact bytes and source identities did not change. No new upstream comparison was performed. |

## Consumer impact and recommended work

Migration-time guidance, not a newly performed upstream audit. Existing captured semantics and
consumer boundaries remain authoritative; this metadata conversion requires no runtime change.

| Evidence | Maintained target and owners | Validation | Recommendation |
| --- | --- | --- | --- |
| CIP-8 signing and COSE serialization; [semantic evidence](#semantic-evidence) | TypeScript: `libs/typescript/packages/cip/src/cip8/` | `libs/typescript/packages/cip/test/cip8.test.mjs` | No implementation change from this migration. Compare a separately captured update before planning behavior changes. |

Behavioral evidence is language-neutral. C++ is an unmaintained, opt-in consumer.

## Unresolved questions

No fresh upstream completeness or delta audit was performed for this metadata migration.
The inventory verifies stored bytes; original capture claims are retained as historical claims.
Any future update must independently enumerate its pinned sources and resolve semantic mappings.

## Migration provenance

- Metadata conversion date: 2026-09-09, explicitly requested by the human while SPECTRE 1.0.0 is in development.
- Previous snapshot descriptor SHA-256: `9bba140967005a490fa5b2691a044b92d8e5df79f728e41daeec5399375e991c`.
- Original applicable capture-rules SHA-256: `65bdd40e8e43022628dd2938125179da21cfa7d466519c8ce7611f22b6519fb3` (historical label v1).
- Embedded the original applicable rules, with snapshot-relative scope and obsolete provider-version
  routing removed. Added this exact artifact inventory and clearly labeled migration-time guidance.
- Corrected the initial snapshot’s self-referential Previous-Snapshot to NONE; filled inapplicable
  URL source fields with NONE. Preserved Created, snapshot ID, source identities, all artifact paths
  and every artifact byte. No new snapshot or upstream capture was created.
- The converted descriptor and its artifacts are frozen after this migration. Future changes
  require a new numbered snapshot with its own complete specification.
