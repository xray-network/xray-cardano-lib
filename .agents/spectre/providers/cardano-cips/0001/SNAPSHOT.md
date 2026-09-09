# Cardano CIPs provider snapshot

Provider-Snapshot-Version: v1
Snapshot: 0001
Provider: cardano-cips
Created: 20260731T075521Z
Previous-Snapshot: NONE
Source-Type: git
Source-Repository: https://github.com/cardano-foundation/CIPs.git
Source-Commit: b491a839708eb0296597008e7b6b093eda5e3363
Source-Ref: refs/heads/master
Source-Tag: NONE
Source-URL: NONE
Source-SHA256: NONE

## Evidence objective

Preserve the official standards evidence needed to plan strict Cardano text encodings, asset
fingerprints and labels, typed wallet/governance identities, Plutus contract blueprints, datum
metadata, and proposal-level hardware-wallet compatibility diagnostics in TypeScript and C++.

## Comparison sources

This is the initial `cardano-cips` snapshot. There is no previous same-provider snapshot. The
candidate commit was compared with the current Cardano Lib Bech32, cryptography, address, ledger,
builder, CIP, typed Data, UPLC, package, and C++ component owners only to bound future
instructions; that comparison did not change captured bytes.

## Frozen capture specification

The rules below preserve this capture’s original source selection, mappings, transformations,
completeness, licensing and consumer boundaries. They apply only to the immutable identities
recorded above. Discovery/ref and future-planning statements describe the original capture context;
they cannot retarget this snapshot. Repository-root paths remain repository-root-relative;
`artifacts/` paths resolve from this directory. No mutable provider guide supplies normative rules.

### Purpose

Capture the official specifications needed for XRAY Cardano Lib's focused encoding, native-asset,
governance, Plutus-blueprint, and hardware-wallet interoperability plans. The snapshot is
normative evidence for the selected standards only; it is not an instruction to implement every
CIP, copy reference implementations, or expose a generic CIP registry.

### Source

| Field | Value |
| --- | --- |
| Repository | `https://github.com/cardano-foundation/CIPs.git` |
| Followed ref | `refs/heads/master` |
| Revision policy | Full commit reachable from the followed ref |
| Source mode | Immutable captured evidence; discovery policy below does not change the recorded commit |
| Submodules | Not part of the source |
| License | Per-CIP frontmatter and copyright notice: Apache-2.0 for CIP-0005 and CIP-0016; CC-BY-4.0 for the other selected CIPs and repository license |
| Supplementary license text | Frozen Apache-2.0 text at `.agents/spectre/providers/cardano-multiplatform-lib/0001/artifacts/test-vectors/LICENSE-APACHE-2.0.txt`, SHA-256 `4541e95aa81113643b71a96d7ff673c4a83ede3d2e8f0df2ad676e7970e2b1fa` |

A branch name is discovery metadata only. Every snapshot records one full commit as its
authoritative source identity.

### Artifact selection

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

### Evidence-only sources

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

### Consumption and planning requirements

- Consume only the selected CIPs relevant to a bounded instruction. This provider does not justify
  an omnibus CIP API or automatic exports for every captured standard.
- Reuse XRAY Cardano Lib's existing Bech32, Blake2b, key, address, ledger, Plutus Data, CBOR, and JSON
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
- Compare a later snapshot with the latest earlier snapshot from this provider.

### Excluded source material

- Every CIP, CPS, schema, registry, test vector, and legal file not listed above
- CIP-0068 reference TypeScript/Haskell implementations and extension boilerplate
- Generic JSON Schema implementations and live remote schema resolution
- CIP-0030 wallet injection, mnemonic dictionaries, hardware-device SDKs, firmware matrices, and
  signing transports
- Repository validators, website/rendering code, tests, build configuration, automation, Git
  metadata, and agent files
- Unselected links, linked repositories, package releases, and external reference implementations

### Verified artifact inventory

This table freezes all 29 regular artifacts, including existing control files. Paths are
relative to this snapshot. Hashes and sizes were verified during the metadata conversion on
2026-09-09; this local verification does not claim a new source-tree capture or upstream audit.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `artifacts/SHA256SUMS` | 2882 | `4f451a14d9426ef6f056c27dec98897ced500152d29e03782a2c7303f79695d0` |
| `artifacts/legal/LICENSE` | 18657 | `97d24386ff776d7160b87031b259a942bd03c9939796cab6724ad5d17d2cf785` |
| `artifacts/legal/LICENSE-APACHE-2.0.txt` | 11347 | `4541e95aa81113643b71a96d7ff673c4a83ede3d2e8f0df2ad676e7970e2b1fa` |
| `artifacts/upstream/CIP-0005/README.md` | 21470 | `693bb303b311f9af00675e31f03207db6861e931b29afb99d185a8cc6e2b3523` |
| `artifacts/upstream/CIP-0014/README.md` | 6877 | `2b2a872f67a4f027b4e76498e6ddc3114521354dffa69a403418f424074123ea` |
| `artifacts/upstream/CIP-0016/README.md` | 5169 | `1945c502e8903e1d68ed1e305d799ffd2bcc7730381833b2446088c9a50c4cce` |
| `artifacts/upstream/CIP-0019/CIP-0019-byron-addresses.cddl` | 1049 | `984e7ce4134acf4d4178a8a6459b9e27e18a36b2e637a0aa5eacbb0be08ad635` |
| `artifacts/upstream/CIP-0019/CIP-0019-cardano-addresses.abnf` | 1540 | `95ae2437aa50c01756736a826012f9b58f903037408455b190b2c0de6f73e38e` |
| `artifacts/upstream/CIP-0019/README.md` | 17601 | `9fbc8c69f261650b5ccab975fe1e291358a2e5e09088690b000013ea1cb72ce5` |
| `artifacts/upstream/CIP-0021/README.md` | 13673 | `8f9a6c420cd67fcd8ef212aba459d451709e34a9267f3a3950288e62d315185b` |
| `artifacts/upstream/CIP-0057/README.md` | 27754 | `36ffa58ac88c2170415a70b4d14c3076aba0de2aeb60c22db85445a20b013f7a` |
| `artifacts/upstream/CIP-0057/schemas/README.md` | 1138 | `29010c7a40b0939b27a26a07b0fdb015ee21674bf963657798e2eb4a692bfd58` |
| `artifacts/upstream/CIP-0057/schemas/plutus-blueprint-argument.json` | 2025 | `340866276cb7a1a02042f472b7470e7a16184a5a7e8a9b5ead234832d275a9af` |
| `artifacts/upstream/CIP-0057/schemas/plutus-blueprint-parameter.json` | 2113 | `f4319de02ea0835b4e94df14e44800a7dbb372766006c4afb72dbbf7f88b93d4` |
| `artifacts/upstream/CIP-0057/schemas/plutus-blueprint.json` | 3589 | `53f65e9895a25e86a13615e9a666c068baa2e5e04d58f205a30f70b134690270` |
| `artifacts/upstream/CIP-0057/schemas/plutus-builtin.json` | 4836 | `0927b24a304e62a3565674838edffa4bdb6ebae32bd89fe2a43d42d75bb8dae3` |
| `artifacts/upstream/CIP-0057/schemas/plutus-data.json` | 4636 | `632c14e1fa9744b7fddf3776d7eba67965bbbb7f663ad11441f06278a6f9832d` |
| `artifacts/upstream/CIP-0067/README.md` | 7744 | `85bf889d01fc88a7ffa2bce12bceb316110d60dac5a8ec690cb0cdc5cdb95d85` |
| `artifacts/upstream/CIP-0067/registry.json` | 1103 | `e0e60303cd54b41476564178b0b5fb1024e1a1ef42cfd78c6b5b16c1b6e6aadc` |
| `artifacts/upstream/CIP-0067/registry.schema.json` | 2282 | `5f727fb9ce2139741162ea301b088389d494c4ff692d9891f54d17857730f404` |
| `artifacts/upstream/CIP-0068/README.md` | 32971 | `1dea95c424739656266eb2a662a48d1950e1c96680b75fbda4ef6317f43ce198` |
| `artifacts/upstream/CIP-0105/README.md` | 21414 | `410d54518614e4244b56fe0b09bdfa949b5d18b0d220e5584b3def29bb2e8e44` |
| `artifacts/upstream/CIP-0105/test-vectors.md` | 722 | `5c7f7a463126a6f58a4646c1686436d59d80232a5d01c279866f038d23131b24` |
| `artifacts/upstream/CIP-0105/test-vectors/test-vector-1.md` | 9224 | `6b1b0bf4dfe8d64cbf19734fbe9ea7ef48aeb690d867fd281eeb82b20d7eb738` |
| `artifacts/upstream/CIP-0105/test-vectors/test-vector-2.md` | 9179 | `2d0553aca947c627bf23d2df0b80b05d37cfc75d3af7b63f4cabe1210286efe6` |
| `artifacts/upstream/CIP-0105/test-vectors/test-vector-3.md` | 9253 | `8a914a49a73116654784949933867433235e658eba93138b80e03c75259d85ce` |
| `artifacts/upstream/CIP-0105/test-vectors/test-vector-4.md` | 9251 | `54961ee60b5fcd5fabc82cda26dcde9fc55df002bd41cef61d6eb32fc28a4775` |
| `artifacts/upstream/CIP-0129/README.md` | 11164 | `2be349e5a971328d44309b41713be1d986e271d9552705a845e365a4aafc233c` |
| `artifacts/upstream/CIP-1852/README.md` | 4802 | `1246d058ad32ca533ccc796a9ad2dabcc1bc36b21e6ff15a55f3460d211ad6ff` |

## Captured scope

The snapshot contains twenty-seven byte-exact upstream regular files:

- CIP-0005, CIP-0014, CIP-0016, CIP-0021, CIP-0068, CIP-0129, and CIP-1852 specifications;
- the CIP-0019 specification plus its Shelley-address ABNF and Byron-address CDDL;
- the CIP-0057 specification, schema guide, and complete five-file captured meta-schema set;
- the CIP-0067 specification, registry, and registry schema;
- the CIP-0105 specification, vector index, and four vector documents; and
- the repository's CC-BY-4.0 license text.

The snapshot also contains one byte-exact supplementary Apache-2.0 license text and deterministic
`artifacts/SHA256SUMS`. No captured file was transformed.

## Integrity and licensing

The exact inventory is defined by [this frozen capture specification](#frozen-capture-specification). All twenty-eight byte-exact
artifacts are nonempty regular files and are listed by lowercase SHA-256 in
`artifacts/SHA256SUMS`.

CIP-0005 and CIP-0016 declare Apache-2.0 in their frontmatter and copyright notices. Their
supplementary license text is 11,347 bytes with SHA-256
`4541e95aa81113643b71a96d7ff673c4a83ede3d2e8f0df2ad676e7970e2b1fa`. Every other selected CIP
declares CC-BY-4.0, and the captured root `LICENSE` preserves the repository's CC-BY-4.0 legal
text. The selected README frontmatter preserves author and source attribution.

## Semantic evidence

At the captured commit:

- CIP-0005, CIP-0014, CIP-0016, CIP-0019, CIP-0021, CIP-0057, CIP-0068, CIP-0105, and CIP-1852
  are `Active`;
- CIP-0067 and CIP-0129 are `Proposed`;
- CIP-0005 defines the common HRP registry, CIP-0016 defines role-aware Cardano key byte layouts,
  and CIP-0019 defines address bytes, network tags, conventional text encodings, and vectors;
- CIP-0014 defines `asset` as Bech32 over Blake2b-160 of policy ID concatenated with asset name;
- CIP-1852 and CIP-0105 define typed wallet roles and Conway governance derivation paths, while
  CIP-0129 defines the current header-bearing governance identifier bytes and `gov_action` form;
- CIP-0057 defines the blueprint document, validator arguments, Plutus Data vocabulary, local
  definitions, and captured mutually referencing schemas;
- active CIP-0068 consumes the proposed CIP-0067 four-byte label format and defines versioned
  reference/user-token datum metadata relationships; and
- CIP-0021 defines deterministic transaction restrictions independent of the time-sensitive
  device appendix.

Proposed status is evidence, not approval for a stable aggregate API. The snapshot specification
requires focused, provisional ownership for CIP-0067 and CIP-0129.

## Exclusions

Unselected CIPs and CPSs, redirect stubs, CIP-0068 extension/reference implementations, generic
JSON Schema engines, remote schema resolution, device transports and firmware profiles, wallet
injection, mnemonic dictionaries, repository validators, rendering/build automation, and all
linked external source are excluded. No upstream program was executed.

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
| Plutus blueprint schemas and local references; [semantic evidence](#semantic-evidence) | TypeScript: `libs/typescript/packages/plutus/src/blueprint/index.ts` | `libs/typescript/packages/plutus/test/blueprint.test.mjs` | No implementation change from this migration. Compare a separately captured update before planning behavior changes. |
| Focused encoding, asset and governance standards; [semantic evidence](#semantic-evidence) | TypeScript: `libs/typescript/packages/cip/src/` | `libs/typescript/packages/cip/test/` | No implementation change from this migration. Compare a separately captured update before planning behavior changes. |

Behavioral evidence is language-neutral. C++ is an unmaintained, opt-in consumer.

## Unresolved questions

No fresh upstream completeness or delta audit was performed for this metadata migration.
The inventory verifies stored bytes; original capture claims are retained as historical claims.
Any future update must independently enumerate its pinned sources and resolve semantic mappings.

## Migration provenance

- Metadata conversion date: 2026-09-09, explicitly requested by the human while SPECTRE 1.0.0 is in development.
- Previous snapshot descriptor SHA-256: `6efb20708f1427e2d53ed55b6d619f742c88906876b531d6b000c21a75a0d8f2`.
- Original applicable capture-rules SHA-256: `961734f22256ce25d771be826cf1c7da7f81bd12ddb2892b3582a82a6406b427` (historical label v1).
- Embedded the original applicable rules, with snapshot-relative scope and obsolete provider-version
  routing removed. Added this exact artifact inventory and clearly labeled migration-time guidance.
- Corrected the initial snapshot’s self-referential Previous-Snapshot to NONE; filled inapplicable
  URL source fields with NONE. Preserved Created, snapshot ID, source identities, all artifact paths
  and every artifact byte. No new snapshot or upstream capture was created.
- The converted descriptor and its artifacts are frozen after this migration. Future changes
  require a new numbered snapshot with its own complete specification.
