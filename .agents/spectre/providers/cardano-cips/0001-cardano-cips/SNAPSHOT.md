# Cardano CIPs provider snapshot

Provider-Snapshot-Version: v1
Snapshot: 0001-cardano-cips
Provider: cardano-cips
Created: 20260731T075521Z
Previous-Snapshot: ./SNAPSHOT.md
Provider-Version: v1
Source-Type: git
Source-Repository: https://github.com/cardano-foundation/CIPs.git
Source-Commit: b491a839708eb0296597008e7b6b093eda5e3363
Source-Ref: refs/heads/master
Source-Tag: NONE

## Evidence objective

Preserve the official standards evidence needed to plan strict Cardano text encodings, asset
fingerprints and labels, typed wallet/governance identities, Plutus contract blueprints, datum
metadata, and proposal-level hardware-wallet compatibility diagnostics in TypeScript and C++.

## Comparison sources

This is the initial `cardano-cips` snapshot. There is no previous same-provider snapshot. The
candidate commit was compared with the current Cardano Lib Bech32, cryptography, address, ledger,
builder, CIP, typed Data, UPLC, package, and C++ component owners only to bound future
instructions; that comparison did not change captured bytes.

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

The exact inventory is defined by [`PROVIDER.md`](../PROVIDER.md). All twenty-eight byte-exact
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

Proposed status is evidence, not approval for a stable aggregate API. The provider contract
requires focused, provisional ownership for CIP-0067 and CIP-0129.

## Exclusions

Unselected CIPs and CPSs, redirect stubs, CIP-0068 extension/reference implementations, generic
JSON Schema engines, remote schema resolution, device transports and firmware profiles, wallet
injection, mnemonic dictionaries, repository validators, rendering/build automation, and all
linked external source are excluded. No upstream program was executed.
