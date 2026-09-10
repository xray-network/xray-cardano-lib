# TypeScript implementation 0024 instruction

Implementation-Version: v1
Implementation-ID: typescript/0024
Created: 20260825T084339Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| Human request dated 2026-08-25 for complete intrinsic transaction parsing consumed by xray-js and XRAY App | `LOCAL` | Yes | Defines the typed-accessor, bigint, certificate, signedness, and non-resolution boundaries. |
| `libs/typescript/packages/chain/src/era/{multi-era,conway,shelley}` transaction, certificate, witness, and auxiliary owners | `LOCAL` | Yes | Define current lossless ledger owners and incomplete public accessors. |
| `libs/typescript/packages/chain/test/{multi-era,conway-foundations,witness-discovery}.test.mjs` | `LOCAL` | Yes | Define current cross-era, transaction decomposition, malformed-input, and witness coverage. |
| Accepted lossless-CBOR and package-ownership ADRs plus chain/runtime READMEs | `LOCAL` | Yes | Define byte preservation, nominal ownership, browser-safe types, and documentation requirements. |
| Read-only adjacent xray-js transaction parser and XRAY App approval projection | `LOCAL` | Yes | Demonstrate downstream low-level workarounds and the required intrinsic inspection coverage. |

## Objective

Complete typed, lossless transaction inspection accessors in the TypeScript Cardano chain library.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C01` | Complete `MultiEraTransactionBody` access to every standard transaction-body field through typed existing or newly owned bindings: spending inputs, outputs, fee, invalid-hereafter/legacy TTL, certificates, withdrawals, protocol update, auxiliary-data hash, invalid-before, mint, script-data hash, collateral inputs, required signers, network ID, collateral return, total collateral, reference inputs, voting procedures, proposal procedures, current treasury value, and donation. Replace generic `HistoricalData` returns for withdrawals, required signers, voting, and proposals with typed collections and extend protocol-update bindings across recognized fields. | Additive/breaking cleanup of the unreleased multi-era surface; do not retain generic compatibility returns alongside the typed contract. All ledger integers use `bigint`. | Chain multi-era/body and existing Conway owners | One sparse/full fixture per field, exact bigint bounds, malformed known fields, and nominal identity tests. |
| `C02` | Complete transaction input/output accessors without resolution: inputs expose transaction hash and index only; outputs expose address, value, legacy datum hash, Babbage/Conway datum option including inline data, and reference script. Apply the same output contract to collateral return and keep Byron/Shelley-family address behavior explicit. | No UTxO/provider/account DTO, input resolution, asset metadata, or application ownership classification. | Multi-era input/output owners | Byron and Shelley-family inputs/outputs, multi-assets, datum hash, inline datum, reference script, collateral return, malformed fields, and defensive ownership tests. |
| `C03` | Support every certificate tag `0..18` with typed variant accessors and complete semantic fields, including historical genesis-key delegation and move-instantaneous-rewards payloads plus pool parameters and Conway committee/DRep anchors and deposits. Add a non-throwing raw tag plus optional known kind so structurally valid future tags remain inspectable as owned CBOR bytes; malformed discriminants/payloads remain strict. | Do not collapse legacy tags `0/1` with explicit amount tags `7/8`, invent deposits from protocol parameters, or expose application display strings. | Multi-era, Shelley, and Conway certificate owners | One fixture per tag, historical payloads, full pool parameters, optional anchors, exact amounts, future unknown tag, and malformed tag/payload tests. |
| `C04` | Add typed defensive accessors to `TransactionWitnessSet` for vkey and bootstrap witnesses, native scripts, Plutus V1/V2/V3 scripts, Plutus data, and both redeemer encodings; add typed auxiliary-data variant/accessors for metadata and every native/Plutus script family. Expose component presence without interpreting it as complete signing or script validity. | Existing transaction `is_valid()` remains the ledger script-validity flag. No signature verification, required-witness discovery, script execution, or signed/unsigned classification is added. | Conway witness and auxiliary-data owners | Empty, partial vkey, bootstrap, native/Plutus, datum, both redeemer forms, metadata, and malformed/defensive tests. |
| `C05` | Add lossless unknown-field enumeration to transaction body, witness-set, and auxiliary-data map owners. Each entry retains defensively owned key/value CBOR bytes (and an unsigned bigint key where applicable), while known fields remain strictly decoded. Correct standalone multi-era body parsing so it does not claim Conway merely because no era context exists; block-derived bodies retain authoritative era identity. | Preserved serialization and hashes remain byte-for-byte; canonical serialization stays explicit. Unknown valid fields are observable, not silently accepted as known semantics. | Multi-era identity and shared lossless codec boundaries | Unknown keys/forms, map order, duplicate/malformed cases, standalone unknown-era, block-derived era, preserved/canonical bytes, and hash stability. |
| `C06` | Export all new bindings through the focused multi-era path, chain root, and aggregate runtime by nominal identity; document typed intrinsic inspection boundaries and update focused, browser, malformed-input, and packed-consumer coverage. Keep canonical implementation/status documentation mirrors and navigation synchronized. | No new package, dependency, JSON inspection DTO/serializer, provider API, xray-js model, UI, C++ parity work, or publication. | Chain/runtime exports, docs, tests, and XRAY mirrors | Focused tests, complete TypeScript gate, public identity/import checks, browser/pack smoke, mirror validation, docs navigation, and `git diff --check`. |

## Implementation steps

1. Replace generic multi-era body returns with typed chain-owned collections and complete known field accessors.
2. Extend input/output and certificate owners, including historical variants and future-tag preservation.
3. Add typed witness-set and auxiliary-data decomposition without signature or script interpretation.
4. Add unknown-field enumeration and correct standalone unknown-era semantics while preserving exact wire bytes.
5. Export by identity, document the low-level boundary, synchronize implementation mirrors/navigation, and run focused/full gates.

## Validation

- `npm --prefix libs/typescript run build`
- Focused chain tests for multi-era bodies, outputs, certificates, witnesses, auxiliary data, unknown fields, and preservation.
- `npm --prefix libs/typescript run check`
- Confirm every standard body key `0..22` used by the supported eras has a typed accessor or explicit absence semantics.
- Confirm all certificate tags `0..18` decode through their exact owner and a future structurally valid tag remains available as raw CBOR.
- Confirm standalone transaction bodies do not report a fabricated era and block-derived bodies retain their era.
- Confirm public APIs use `Uint8Array` and `bigint`, contain no JSON inspection serializer, and preserve focused/root/runtime nominal identity.
- Verify canonical records, Mintlify mirrors, status links, and `docs/docs.json` navigation.
- `git diff --check`

## Compatibility and human review

Review the typed field owners, standalone-era correction, legacy certificate semantics, unknown-field boundary, witness/auxiliary completeness, defensive ownership, exact bigint behavior, and preserved-versus-canonical CBOR contract.

## Completion criteria

- Normal consumers can inspect every intrinsic known transaction body, output, certificate, witness, and auxiliary-data field without reading generic CBOR nodes.
- Inputs remain references only and no network/application resolution concern enters Cardano Lib.
- All certificate tags `0..18` have complete typed access and future unknown tags/fields remain losslessly observable.
- Witness presence is factual and never presented as signature completeness.
- Standalone parsing does not fabricate an era, exact decoded bytes/hashes remain stable, and all TypeScript/package/mirror validation passes.

## Out of scope

- UTxO/provider resolution, application inspection DTOs or labels, signed/unsigned detection, signature/script validation, transaction balancing or mutation, JSON serialization, token metadata, UI, C++ feature parity, dependency upgrades, or package publication

## Blockers

None.
