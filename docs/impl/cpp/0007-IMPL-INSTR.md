# C++ implementation 0007 instruction

Implementation-Version: v1
Implementation-ID: cpp/0007
Created: 20260731T075521Z
Evidence-Mode: HYBRID
Depends-On: ./0001-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`cpp/0001`](./0001-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted asset-name, policy, typed Data, CIP-25, error, ownership, and packaging baseline |
| [`0001-cardano-cips`](https://github.com/xray-network/xray-cardano-lib/blob/main/updates/providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md) | `PROVIDER` | Yes | Captured proposed CIP-67 algorithm/registry/schema and active CIP-68 version-4 specification |
| `libs/cpp/include/cardano/chain/builder.hpp` and `libs/cpp/include/cardano/plutus/data.hpp` | `LOCAL` | Yes | Existing `AssetName`, `ScriptHash`, and typed `Data` owners to reuse |

## Objective

Add a focused provisional CIP-67 label codec and active CIP-68 relationship/datum helpers for
labels 100, 222, 333, and 444. Keep classification and metadata validation pure and bounded; the
library must not become a token registry, chain indexer, minting service, or URI fetcher.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C7-LBL1` | Implement the exact four-byte CIP-67 label encoder/decoder and labelled `AssetName` helpers. | Additive but visibly provisional API under `cardano::cip::experimental::cip67`. | `include/cardano/cip/cip67.hpp`, `src/cip/cip67.cpp` | Captured vectors, CRC, bracket, length, and mutation tests |
| `C7-REL1` | Classify CIP-68 assets and validate/construct reference-token relationships using existing policy and asset owners. | Additive pure helpers; no implicit ledger lookup or authenticity claim. | `include/cardano/cip/cip68.hpp`, `src/cip/cip68.cpp` | Label/policy/content/quantity/count table tests |
| `C7-DAT1` | Parse, validate, construct, and canonically emit CIP-68 constructor-0 datum versions 1–4 for 222/333 and 2–4 for 444. | Existing generic typed Data remains unchanged; helpers return and retain that owner. | CIP-68 owner using `plutus::Data` | Direct/nested metadata, version, malformed, and round-trip tests |
| `C7-MET1` | Validate class-specific known metadata fields, bounded-byte URI forms, v4 nested lookup, and arbitrary extra Data without network access. | Compatible extensions are retained; known keys cannot be duplicated or have the wrong type. | CIP-68 owner | Per-class field, URI, duplicate, nested-path, and resource tests |
| `C7-API1` | Install CIP-67 only as a focused experimental header, export active CIP-68 through component/aggregate facades, label proposal status, and update inventories/docs. | No duplicate `AssetName`, policy, metadata, or `Data` nominal type; provisional CIP-67 bindings do not enter stable umbrellas. | Focused CIP-67 header, CIP-68 facade, CMake, docs/inventory | Header identity, aggregate-absence, and installed-consumer tests |

## Exact supported contract

- A CIP-67 label is four bytes with bits
  `[0000 | uint16 label, big-endian | CRC-8 | 0000]`. CRC-8 uses polynomial `0x07` over the
  two-byte label value. Both bracket nibbles must be zero. Label values are `0..65535`.
- `make_asset_name(label, content)` prepends the four label bytes to `0..28` content bytes and
  returns the existing maximum-32-byte `chain::AssetName`. Parsing returns the label and an owned
  content copy and rejects short names, bad brackets, or checksum mismatch.
- CIP-68 recognizes reference NFT 100 and user classes NFT 222, FT 333, and RFT 444 only. A valid
  pair has one policy ID, identical bytes after the label, reference label 100, the required user
  label, reference quantity exactly one, and exactly one matching reference asset in the
  caller-supplied candidate set.
- A datum is existing typed Data constructor 0 with exactly three fields:
  `[metadata, version, extra]`. Extra is required and may be arbitrary bounded Data. Versions 1–3
  use direct metadata; version 4 uses the captured nested byte-key path
  `"721" -> policy_id -> unlabelled asset_name`. Classes 222 and 333 accept versions 1–4. Class
  444 accepts versions 2–4: the captured “since version 2” marker and changelog introducing RFT in
  version 2 take precedence over the stale local `version = 3 / 4` line.
- Class 222 known direct fields are required `name` and `image`, optional `description` and
  `files`; class 333 requires `name` and `description`, with optional `ticker`, `url`, `decimals`,
  and `logo`; class 444 requires `name` and `image`, with optional `description`, `decimals`, and
  `files`. Additional metadata properties and the extra field are retained.
- Versions 1–2 accept URI bytes directly. Version 3 adds a nonempty chunk-list form, and version 4
  retains it. URI bytes must decode as UTF-8 and use `https`, `ipfs`, `ar`, or `data`. Syntax and a
  declared data-URI media type can be checked offline; remote media content or MIME type is never
  fetched or asserted.

## Implementation steps

1. Define immutable provisional `AssetNameLabel` and `LabelledAssetName` values plus
   `encode`, `decode`, `make_asset_name`, and `split_asset_name` in the experimental CIP-67
   namespace. Implement CRC locally with unsigned fixed-width arithmetic and no lookup dependency.
2. Reproduce the captured labels `0`, `1`, `23`, `99`, `533`, `2000`, `4567`, `11111`,
   `49328`, and `65535`. Use registry entries 100, 222, 333, and 444 only as frozen constants;
   do not expose a mutable or generic registry API and do not implement label 500.
3. Define stable CIP-68 `TokenClass`, `TokenIdentity`, `Relationship`, `Datum`,
   `MetadataFormat`, and `Limits` values. Reuse `ScriptHash`, `AssetName`, and `plutus::Data`;
   retained policy/name/Data values are owned or defensively copied.
4. Make `reference_identity(user)` replace only a valid recognized user label with label 100 and
   preserve policy/content. Make `validate_relationship` require the caller to supply reference
   quantity and candidate-match count; reject zero/multiple matches rather than implying that the
   library searched chain state.
5. Parse Data iteratively with defaults of depth 128, 100,000 nodes, 10,000 metadata map entries,
   and 16,777,216 aggregate byte-string bytes. Callers may lower or explicitly increase checked
   limits. Reject duplicate known byte keys, invalid constructor/field count, non-integer or
   unsupported versions, and ambiguous v4 paths.
6. Validate class-specific metadata shapes and file details (`mediaType`, `src`, optional `name`)
   without normalizing unknown properties. Chunked URI bytes preserve their Data shape on parse;
   new construction uses one byte string unless a caller explicitly supplies chunks.
7. In v4, select only the metadata at the exact 28-byte policy and unlabelled asset-name byte keys.
   Reject a label-prefixed nested key, missing/multiple matching path, or a `"721"` key in direct
   versions. Canonical emission delegates to the accepted typed Data/CBOR owner.
8. Include active CIP-68 from the stable CIP/aggregate facades. Install CIP-67's focused header
   for explicit experimental use, but do not include it from `cardano/cip/cip.hpp` or
   `cardano/cardano.hpp`.
9. Document that label verification establishes formatting and relationship verification checks
   caller-supplied facts only; minting-policy authenticity and global uniqueness require ledger
   data outside this API.

## Validation

- Assert exact bytes for every captured CIP-67 vector and the CIP-68 examples for labels 100 and
  222. Flip each label/checksum/bracket bit and test 0/28/29-byte content boundaries.
- Table-test all reference/user label pairs, differing policies/content, reference quantities
  0/1/2, and candidate counts 0/1/2.
- Cover direct 222/333 versions 1–3, direct 444 versions 2–3, nested version 4 for every class, and
  every unsupported class/version/format combination. Assert chunked URIs reject in versions 1–2
  and pass in versions 3–4.
- Test required/optional/additional metadata, duplicate keys, direct versus chunked URIs, allowed
  and forbidden schemes, invalid UTF-8, file shapes, integer decimals, nested policy/name matching,
  arbitrary extra Data, and every limit.
- Prove parse-to-canonical-Data round trips, active CIP-68 component/aggregate nominal identity,
  focused CIP-67 direct-header availability and aggregate absence, no network or filesystem
  access, and no provider artifacts in install contents.
- Run completion, sanitizer, hardening, provider-integrity, API inventory, and installed-consumer
  gates.

## Compatibility and human review

CIP-67 remains proposed in the captured evidence, so its public surface is explicitly experimental.
CIP-68 is active, but helpers support only the captured classes and versions. Generic assets,
CIP-25 behavior, and typed Data behavior are unchanged.

Human review must verify CRC bit layout, proposal labeling, class/version matrices, v4 path matching,
URI claims, duplicate handling, resource limits, and the explicit no-chain-lookup boundary.

## Completion criteria

- All captured label vectors and supported CIP-68 shapes pass exact and negative validation.
- Unsupported/provisional behavior is isolated and fails closed.
- No new nominal policy, asset-name, metadata-JSON, or Data owner is introduced.
- All C++ workflows pass and the result identifies exact CIP-67/68 artifacts consumed.

## Out of scope

- Chain/indexer lookup, reference UTxO discovery, minting-policy validation, global uniqueness
  proof, transaction/mint construction, wallet display decisions, URI retrieval, or MIME probing.
- A general CIP-67 registry, label 500/CIP-102, future CIP-68 classes/versions, automatic CIP-25
  migration, or JSON metadata normalization.

## Blockers

None.
