# Cardano CIPs provider

Provider: cardano-cips
Provider-Version: v1

## Purpose

Capture the official specifications needed for Cardano Lib's focused encoding, native-asset,
governance, Plutus-blueprint, and hardware-wallet interoperability plans. The snapshot is
normative evidence for the selected standards only; it is not an instruction to implement every
CIP, copy reference implementations, or expose a generic CIP registry.

## Source

| Field | Value |
| --- | --- |
| Repository | `https://github.com/cardano-foundation/CIPs.git` |
| Followed ref | `refs/heads/master` |
| Revision policy | Full commit reachable from the followed ref |
| Source mode | Live; resolve independently for every snapshot |
| Submodules | Not part of the source |
| License | Per-CIP frontmatter and copyright notice: Apache-2.0 for CIP-0005 and CIP-0016; CC-BY-4.0 for the other selected CIPs and repository license |
| Supplementary license text | Frozen Apache-2.0 text at `updates/providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/LICENSE-APACHE-2.0.txt`, SHA-256 `4541e95aa81113643b71a96d7ff673c4a83ede3d2e8f0df2ad676e7970e2b1fa` |

A branch name is discovery metadata only. Every snapshot records one full commit as its
authoritative source identity.

## Artifact selection

Copy these regular files byte-for-byte:

| Upstream path | Snapshot artifact |
| --- | --- |
| `LICENSE` | `artifacts/legal/LICENSE` |
| `CIP-0005/README.md` | `artifacts/upstream/CIP-0005/README.md` |
| `CIP-0014/README.md` | `artifacts/upstream/CIP-0014/README.md` |
| `CIP-0016/README.md` | `artifacts/upstream/CIP-0016/README.md` |
| `CIP-0019/README.md` | `artifacts/upstream/CIP-0019/README.md` |
| `CIP-0019/CIP-0019-byron-addresses.cddl` | `artifacts/upstream/CIP-0019/CIP-0019-byron-addresses.cddl` |
| `CIP-0019/CIP-0019-cardano-addresses.abnf` | `artifacts/upstream/CIP-0019/CIP-0019-cardano-addresses.abnf` |
| `CIP-0021/README.md` | `artifacts/upstream/CIP-0021/README.md` |
| `CIP-0057/README.md` | `artifacts/upstream/CIP-0057/README.md` |
| `CIP-0057/schemas/README.md` | `artifacts/upstream/CIP-0057/schemas/README.md` |
| `CIP-0057/schemas/plutus-blueprint-argument.json` | `artifacts/upstream/CIP-0057/schemas/plutus-blueprint-argument.json` |
| `CIP-0057/schemas/plutus-blueprint-parameter.json` | `artifacts/upstream/CIP-0057/schemas/plutus-blueprint-parameter.json` |
| `CIP-0057/schemas/plutus-blueprint.json` | `artifacts/upstream/CIP-0057/schemas/plutus-blueprint.json` |
| `CIP-0057/schemas/plutus-builtin.json` | `artifacts/upstream/CIP-0057/schemas/plutus-builtin.json` |
| `CIP-0057/schemas/plutus-data.json` | `artifacts/upstream/CIP-0057/schemas/plutus-data.json` |
| `CIP-0067/README.md` | `artifacts/upstream/CIP-0067/README.md` |
| `CIP-0067/registry.json` | `artifacts/upstream/CIP-0067/registry.json` |
| `CIP-0067/registry.schema.json` | `artifacts/upstream/CIP-0067/registry.schema.json` |
| `CIP-0068/README.md` | `artifacts/upstream/CIP-0068/README.md` |
| `CIP-0105/README.md` | `artifacts/upstream/CIP-0105/README.md` |
| `CIP-0105/test-vectors.md` | `artifacts/upstream/CIP-0105/test-vectors.md` |
| `CIP-0105/test-vectors/test-vector-1.md` | `artifacts/upstream/CIP-0105/test-vectors/test-vector-1.md` |
| `CIP-0105/test-vectors/test-vector-2.md` | `artifacts/upstream/CIP-0105/test-vectors/test-vector-2.md` |
| `CIP-0105/test-vectors/test-vector-3.md` | `artifacts/upstream/CIP-0105/test-vectors/test-vector-3.md` |
| `CIP-0105/test-vectors/test-vector-4.md` | `artifacts/upstream/CIP-0105/test-vectors/test-vector-4.md` |
| `CIP-0129/README.md` | `artifacts/upstream/CIP-0129/README.md` |
| `CIP-1852/README.md` | `artifacts/upstream/CIP-1852/README.md` |

The selected upstream inventory is exactly twenty-seven byte-exact regular files. Reject a
missing, additional, renamed, symlinked, gitlinked, special, or empty selected file.

Copy the frozen supplementary Apache-2.0 text byte-for-byte to
`artifacts/legal/LICENSE-APACHE-2.0.txt` after rechecking its declared SHA-256 and 11,347-byte
length. This is legal metadata only, not a second semantic source.

Create `artifacts/SHA256SUMS` as deterministic snapshot-local integrity metadata. It contains one
line for each of the twenty-eight byte-exact artifacts, sorted by artifact-relative path in byte
order, using lowercase SHA-256, two ASCII spaces, the path relative to `artifacts/`, and a
trailing newline. It does not list itself.

## Evidence-only sources

Inspect these at the candidate commit when present, but do not copy them:

- the commit metadata, selected-file Git object types, root tree, and previous same-provider
  snapshot comparison;
- the small `CIP-NNNN/CIP-NNNN.md` redirect files;
- CIP frontmatter, changelogs, status, path-to-active sections, and links needed to distinguish
  normative rules from historical or provisional material;
- CIP-0068's extension boilerplate and reference-implementation directory;
- repository contribution, validation, rendering, and automation material.

Do not run any upstream hook, build, test, validator, script, package manager, binary, filter,
reference implementation, or generated program.

## Consumption and planning requirements

- Consume only the selected CIPs relevant to a bounded instruction. This provider does not justify
  an omnibus CIP API or automatic exports for every captured standard.
- Reuse Cardano Lib's existing Bech32, Blake2b, key, address, ledger, Plutus Data, CBOR, and JSON
  owners. Do not create competing nominal primitives or ship snapshot artifacts.
- Treat CIP-0005 as the prefix registry, CIP-0016 as the key-serialization contract, and CIP-0019
  as the address-format contract. Typed decoders must validate semantic HRP, payload shape, and
  network where the selected specifications define them.
- Treat CIP-0067 and CIP-0129 as `Proposed` at the captured commit. Their planned public surface
  must be visibly provisional, or confined to a focused proposal subpath, until a later reviewed
  instruction deliberately promotes it. CIP-0068 may use the captured CIP-0067 labels required by
  its active specification without representing the entire registry as stable.
- CIP-0105 supersedes none of CIP-0129's identifier bytes. Use CIP-1852 and CIP-0105 for typed
  derivation paths and role-specific keys; use CIP-0129 for current governance identifiers.
  Deprecated CIP-0105 identifier encodings, if accepted at all, are explicit decode-only
  compatibility paths and are never canonical output.
- CIP-0057 consumers must validate its captured document and Plutus Data vocabulary without
  fetching remote schemas. References are restricted to the captured schema set and
  document-local definitions; arbitrary network or filesystem resolution is forbidden.
- CIP-0021 consumers implement proposal-level transaction diagnostics only. Do not claim support
  for a named device or firmware version, transform transactions silently, or weaken ledger
  validity. The device-specific appendix is informational and time-sensitive.
- Compare a later snapshot with the latest earlier snapshot from this provider. A new snapshot at
  the same exact commit and provider version is a duplicate.

## Excluded source material

- Every CIP, CPS, schema, registry, test vector, and legal file not listed above
- CIP-0068 reference TypeScript/Haskell implementations and extension boilerplate
- Generic JSON Schema implementations and live remote schema resolution
- CIP-0030 wallet injection, mnemonic dictionaries, hardware-device SDKs, firmware matrices, and
  signing transports
- Repository validators, website/rendering code, tests, build configuration, automation, Git
  metadata, and agent files
- Unselected links, linked repositories, package releases, and external reference implementations
