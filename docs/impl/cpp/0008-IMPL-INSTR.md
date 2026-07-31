# C++ implementation 0008 instruction

Implementation-Version: v1
Implementation-ID: cpp/0008
Created: 20260731T075521Z
Evidence-Mode: HYBRID
Depends-On: ./0001-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`cpp/0001`](./0001-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted lossless CBOR metadata, era validation, transaction, CIP-36, limit, error, and package baseline |
| [`0001-cardano-cips`](https://github.com/xray-network/xray-cardano-lib/blob/main/updates/providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md) | `PROVIDER` | Yes | Captured active CIP-21 interoperability rules and explicitly time-sensitive device appendix |
| `libs/cpp/include/cardano/core/cbor.hpp` and `libs/cpp/include/cardano/chain/era_models.hpp` | `LOCAL` | Yes | Existing preserved-wire and transaction owners used for read-only inspection |

## Objective

Add an optional, deterministic, read-only CIP-21 compatibility diagnostic for serialized
Shelley-through-Conway transactions. It reports specification violations and migration advisories;
it never rewrites a transaction, signs it, selects a hardware device, or promises support from
particular firmware.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C8-DIA1` | Add bounded `diagnose_cip21` over an explicit era and complete serialized transaction, returning stable ordered diagnostics. | Additive opt-in API; no builder or decoder behavior changes. | `include/cardano/cip/cip21.hpp`, `src/cip/cip21.cpp` | Positive, one-rule-at-a-time, ordering, offset, and limit tests |
| `C8-CBR1` | Diagnose shortest integers/lengths, definite containers, map order, and transaction-wide consistency of optional tag 258 on semantic sets. | Inspection uses preserved decode metadata and does not canonicalize the caller's bytes. | CIP-21 owner using core CBOR metadata | Noncanonical-width/order/tag fixtures |
| `C8-BDY1` | Diagnose all captured body-entry, integer, cardinality, empty-collection, output, multiasset, certificate, withdrawal, and voting restrictions. | Valid ledger transactions may still be reported incompatible, as CIP-21 is narrower than ledger validity. | CIP-21 owner using existing era views | Rule matrix and boundary tests |
| `C8-AUX1` | Diagnose the captured Catalyst auxiliary-data tuple rule only when the caller selects Catalyst-registration context. | Hash-only auxiliary data has no invented structure requirement. | CIP-21 owner reusing CIP-36 recognition | Tuple, scripts-empty, mode, and malformed tests |
| `C8-API1` | Export focused/component/aggregate bindings and document the non-authoritative device boundary. | No implicit enforcement in builders or finalization. | CIP facade, CMake, docs/inventory | Header identity and installed-consumer tests |

## Public and diagnostic contract

- `diagnose_cip21(Cip21Era era, ByteSpan transaction, Cip21Context context = {},
  Cip21Limits limits = {}) -> Result<Cip21Report>` performs complete-input bounded decode followed
  by existing era validation and CIP-21 checks. It never mutates the input or returns replacement
  bytes.
- `Cip21Diagnostic` contains a stable code enum, `error` or `advisory` severity, structural path,
  byte offset when preserved metadata supplies one, and an owned message. Diagnostics sort by
  source offset, then structural path, then code; repeated runs return identical reports.
- Default limits are the accepted CBOR limits plus at most 100,000 diagnostics. Reaching any decode
  or diagnostic limit returns one terminal limit error and performs no unbounded allocation.
- Required error codes are:
  `noncanonical_integer`, `noncanonical_length`, `indefinite_item`, `map_key_order`,
  `inconsistent_set_tag`, `unsupported_body_entry`, `integer_range`, `element_count`,
  `voting_cardinality`, `empty_optional_collection`, `legacy_output_value_shape`,
  `empty_inline_datum`, `empty_reference_script`, `duplicate_policy`, `duplicate_asset`,
  `unsupported_certificate`, `pool_registration_combination`, `duplicate_withdrawal`, and
  `catalyst_auxiliary_shape`. `legacy_output_format` is advisory only.

## Exact rules to diagnose

1. Integers and major-type 2–5 lengths use their shortest heads, containers are definite, and map
   keys use the accepted canonical CBOR ordering. Every CDDL semantic set in one transaction
   either carries tag 258 or omits it; mixed use is an error.
2. Body keys 6 (`update`) and 20 (`proposal procedures`) are unsupported. Every signed integer
   fits `int64_t` and every unsigned integer fits `uint64_t`.
3. Each of inputs, outputs, policy groups per output/mint, asset names per policy, certificates,
   pool owners, pool relays, withdrawals, collateral inputs, required signers, reference inputs,
   and total witnesses is at most 65,535. Voting procedures contain exactly at most one voter and
   at most one procedure for that voter.
4. Optional lists/maps are absent or nonempty unless CIP-21 explicitly fixes their shape. Post-
   Alonzo output maps are preferred. A legacy output without assets uses `[address, coin,
   ?datum_hash]`, not `[address, [coin, {}], ?datum_hash]`. Present inline data and reference
   scripts are nonempty.
5. Policy and asset maps use canonical order and contain no duplicate key. Withdrawals likewise
   use canonical order and contain no duplicate reward account.
6. Unsupported certificate alternatives are genesis delegation, instantaneous rewards,
   stake-vote delegation, stake registration-delegation, vote registration-delegation, and
   stake-vote registration-delegation.
7. If a pool-registration certificate is present, it is the only certificate and the body has no
   withdrawal, mint, datum/datum-hash/reference-script output, script-data hash, collateral,
   required signer, collateral return, total collateral, reference input, voting procedure,
   treasury value, or donation value.
8. In `catalyst_registration` context, auxiliary data uses the tuple
   `[transaction_metadata, auxiliary_scripts]` and `auxiliary_scripts` is an array of length zero.
   Other contexts do not inspect auxiliary payload structure beyond ledger validity.

## Implementation steps

1. Add `Cip21Era`, `Cip21Context`, `Cip21Limits`, `Cip21Code`, `Cip21Severity`,
   `Cip21Diagnostic`, and `Cip21Report` as immutable/small value contracts. Context contains only
   auxiliary mode; it contains no brand, firmware, derivation path, key, or signing callback.
2. Decode once with preservation enabled, require complete input, dispatch through the requested
   existing era validator, and inspect the preserved tree iteratively. Do not diagnose an
   unvalidated value as a transaction.
3. Compare original heads, container forms, map order, and tag metadata directly. Do not use
   re-encoding equality as a substitute where it would erase the source location or duplicate key.
4. Build era-aware field visitors for Shelley, Allegra, Mary, Alonzo, Babbage, and Conway. A field
   unavailable in an older era is absent, not an error. Check cardinalities before descending into
   child collections.
5. Detect pool registration first, then collect every forbidden co-occurring field in body-key
   order so a report is comprehensive and deterministic. Treat each unsupported certificate and
   duplicate occurrence as a separately located diagnostic within the global limit.
6. Reuse the CIP-36 metadata recognition contract for Catalyst context but do not verify a
   signature, contact a device, or infer signing mode.
7. Keep diagnostics detached from `TransactionBuilder`: callers explicitly opt in after building
   or before a hardware-wallet workflow. Documentation must state that devices can impose newer or
   additional restrictions.

## Validation

- Add one known-compatible fixture per supported era and isolated fixtures for every diagnostic
  code, plus combinations proving stable comprehensive order.
- Test canonical head width boundaries, nested map ordering, indefinite strings/arrays/maps,
  all-tagged/all-untagged/mixed semantic sets, duplicates retained by the lossless decoder, and
  structural offsets.
- Test cardinalities 65,535 and 65,536 for every listed collection, signed/unsigned numeric
  boundaries, voting 0/1/2, optional empty collections, legacy/post-Alonzo outputs, and empty
  datum/script references.
- Test every unsupported certificate and every pool-registration exclusion independently and in
  combination. Test Catalyst tuple mode separately from hash-only mode.
- Prove input bytes and model state are unchanged; run completion, sanitizer, hardening,
  provider-integrity, API inventory, component, aggregate, and installed-content gates.

## Compatibility and human review

This diagnostic is stricter than ledger validity by design but has no enforcement side effect.
The captured device appendix is informational and time-sensitive; no device-specific row becomes
an API rule or support promise.

Human review must verify each rule against captured CIP-21, semantic-set coverage, duplicate/source
preservation, era routing, deterministic ordering, and wording that avoids firmware guarantees.

## Completion criteria

- Every captured normative restriction has one stable diagnostic path and boundary test.
- Compatible inputs produce no errors; advisory-only legacy output use remains distinguishable.
- Diagnostics are deterministic, bounded, read-only, and independent of device state.
- All C++ workflows pass and the paired result names the exact CIP-21 artifact consumed.

## Out of scope

- Transaction rewriting/canonicalization, builder enforcement, signing, APDU or transport support,
  key derivation/storage, signing-mode selection, firmware detection, device matrices, or a claim
  that any transaction will be accepted by a particular hardware wallet.
- Restrictions found only in uncaptured vendor documentation or later CIP revisions.

## Blockers

None.
