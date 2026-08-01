# TypeScript implementation 0010 instruction

Implementation-Version: v1
Implementation-ID: typescript/0010
Created: 20260731T075521Z
Evidence-Mode: HYBRID
Depends-On: ./0003-IMPL-RESULT.md, ./0004-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`typescript/0003`](./0003-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted typed Plutus Data owner and immutable Data behavior |
| [`typescript/0004`](./0004-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted `ScriptHash`, `AssetName`, value, and Conway wire owners |
| [`0001-cardano-cips`](../../providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md) | `PROVIDER` | Yes | Immutable CIP-0067/0068 specifications, registry, and registry schema |
| Captured `CIP-0067/README.md`, `registry.json`, and `registry.schema.json` | `PROVIDER` | Yes | Label bits, CRC-8 vectors, proposal status, and registered values |
| Captured `CIP-0068/README.md` | `PROVIDER` | Yes | Active token relationships, versioned datum shapes, and metadata rules |
| `libs/typescript/packages/{cip,chain,plutus,runtime}/` | `LOCAL` | Yes | Existing package, asset, Data, export, browser, and test boundaries |

## Objective

Add a small pure codec for proposed CIP-0067 labels and an active CIP-0068 asset/datum API that
reuses existing asset and Plutus Data owners, validates versioned metadata locally, and performs no
chain lookup or URI fetching.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C001` | Add provisional CIP-0067 label encode/decode and asset-name split/join | Additive focused proposal API | `libs/typescript/packages/cip/src/cip67/` | Ten official vectors, CRC/brackets/range/length, registry evidence |
| `C002` | Add CIP-0068 token relationships and versioned datum/metadata validation | Additive active CIP API | `libs/typescript/packages/cip/src/cip68/` | Labels 100/222/333/444, versions 1-4, direct/nested metadata |
| `C003` | Publish proposal-aware focused exports and stable active-CIP re-exports | Additive exports | CIP manifest/barrels, runtime CIP facade/tests, READMEs | Export absence/presence, identity, browser, packed consumers |

## C001: provisional CIP-0067 contract

- Export `encode_asset_name_label(label): Uint8Array`,
  `decode_asset_name_label(bytes): number`, and
  `split_labeled_asset_name(assetName): { label, content }` from
  `@xray-network/xray-cardano-lib-cip/cip67`; joining must return the existing `AssetName` binding.
- Encode four bytes as `[0000 | 16-bit big-endian label | CRC-8 | 0000]`. CRC-8 uses polynomial
  `0x07`, initial value 0, no reflection, and final XOR 0 over the two padded label bytes.
- Require integer labels `0..65535`, both zero bracket nibbles, a matching checksum, exactly four
  prefix bytes, and content no longer than 28 bytes so the existing 32-byte `AssetName` maximum is
  preserved. Return defensive copies.
- Treat labels 0 through 15 as private-use, not invalid.
- CIP-0067 is Proposed. Add only the explicit `./cip67` export; do not expose its general codec
  from the stable CIP root or aggregate runtime.
- Use the captured registry only as test evidence. Production code must not load it or expose a
  mutable omnibus registry.

## C002: active CIP-0068 contract

- Export `CIP68TokenClass` values `NFT=222`, `FT=333`, and `RFT=444`, plus the reference label
  constant 100, under `@xray-network/xray-cardano-lib-cip/cip68`.
- Add pure helpers to construct user/reference `AssetName`s from content and to validate a pair.
  User and reference assets must use the same existing `ScriptHash` policy, have labels
  222/333/444 and 100 respectively, and have byte-identical unlabeled content.
- Add immutable `CIP68Datum` construction/parsing around the existing `PlutusData` binding.
  Canonical shape is constructor 0 with exactly `[metadata, version, extra]`; `extra` is required
  and defaults to constructor-0 unit only in the convenience constructor. Never omit it.
- Resolve stale two-field examples in the captured document in favor of its normative prose and
  CDDL: strict parsing rejects two-field datum examples.
- NFT and FT accept versions 1 through 4. RFT accepts versions 2 through 4, following the
  normative “since version 2” marker and changelog; the stale RFT `version = 3 / 4` line does not
  override that historical version-2 contract.
- Versions 1 and 2 use direct metadata and byte-string URIs. Version 3 uses direct metadata and
  permits a URI as one byte string or a nonempty list of byte-string chunks. Version 4 requires
  nested metadata at byte key `"721"`, then raw 28-byte policy ID, then unlabeled asset content;
  its URIs retain version-3 chunk support.
- URI chunks must concatenate to valid UTF-8 and a syntactically valid `https`, `ipfs`, `ar`, or
  RFC-2397 `data` URI. Validation is syntax-only and performs no lookup.
- NFT metadata requires byte keys `name` and `image`; optional `description` and `files`, whose
  entries require `mediaType` and `src`. FT requires `name` and `description`; `ticker`, `url`,
  `decimals`, and `logo` are optional. RFT requires `name` and `image`; `description`, `decimals`,
  and `files` are optional. Preserve additional metadata properties as Plutus Data.
- For nested version 4, select exactly the requested policy/content path and reject missing,
  duplicate, or mismatched paths. All decoded Data and byte views are defensive.

## Public ownership and exports

- `cip68` may use the local CIP-0067 implementation internally, but it must return the existing
  `AssetName`, `ScriptHash`, and `PlutusData` bindings rather than wrappers for those concepts.
- Add `./cip68`, namespace `cip68` at the stable CIP root, and the same active bindings through
  the runtime CIP facade and aggregate runtime.
- Keep general `cip67` absent from stable root/runtime exports until a later instruction responds
  to a status change. The focused `cip67` and `cip68` modules must not create an omnibus metadata
  API.

## Implementation steps

1. Implement CRC-8, four-byte label encoding, and strict asset-name splitting under `cip67`.
2. Verify the captured registry schema/inventory in tests and hard-code only the four CIP-0068
   label constants required by the active standard.
3. Implement pure CIP-0068 name/policy relationship helpers and immutable datum views.
4. Implement class/version-specific direct and nested metadata/URI validation over typed Data.
5. Wire proposal-aware exports, documentation, browser tests, and package identity tests.

## Validation

- Assert all ten captured CIP-0067 vectors plus labels 100, 222, 333, and 444; reject bad
  brackets, checksum, short prefixes, overlong content, and out-of-range labels.
- Validate the captured registry against its captured schema and assert unique labels; do not use
  uncaptured linked CIPs as runtime semantics.
- Cover same/different policy and content, every class/version combination, required/extra fields,
  two- versus three-field datum, direct versus nested shape, `"721"` path matching, duplicate map
  keys, URI schemes/chunks/UTF-8, and arbitrary `extra`.
- Assert focused `cip67` availability but stable-root/runtime absence; assert `cip68` identity
  across focused, root, runtime, browser, and packed consumers.
- Run targeted CIP/Plutus/runtime tests and `npm --prefix libs/typescript run check`.

## Compatibility and human review

Both APIs are additive, but CIP-0067 remains Proposed. Review the CRC parameters, status-scoped
exports, the explicit resolution of stale CIP-0068 examples/version text, duplicate-map handling,
URI parser behavior, and preservation of arbitrary Data without prototype-bearing conversion.

## Completion criteria

- Label bytes and all captured vectors match CIP-0067 exactly.
- CIP-0068 pair and datum validation covers labels 100/222/333/444 and historical versions 1-4.
- Existing policy, asset-name, and Plutus Data bindings remain the sole nominal owners.
- No registry, URI, provider, or chain-state access occurs in production.
- Provisional and active APIs have the prescribed export visibility and identity.
- `npm --prefix libs/typescript run check` passes.
- The paired result records proposal boundaries and every evidence conflict disposition.

## Out of scope

- Minting policies, transaction construction, UTxO/indexer discovery, or “exactly one reference”
  chain-state proof
- URI fetching, media verification, caches, gateways, or renderer APIs
- CIP-0025 as a standalone implementation or labels from uncaptured CIPs such as 500
- Stable promotion of the general CIP-0067 registry/codec

## Blockers

None.
