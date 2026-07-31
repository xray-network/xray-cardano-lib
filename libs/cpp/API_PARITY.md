# C++ API parity ledger

This file maps the frozen C++ 0001 instruction to independently owned C++ bindings and validation.
It is completed alongside the implementation. The canonical implementation status remains in
`updates/implementations/cpp/STATUS.md`.

| Change | Owner | Current mapping | Validation |
| --- | --- | --- | --- |
| C001 | `libs/cpp/` | C++23 CMake workspace, pinned vcpkg manifest, component targets, install exports, presets | Configure/build/install/consumer workflows |
| C002 | `cardano::core` | Owned bytes, integers, errors/results, collections, Bech32, network constants | `tests/core` |
| C003 | `cardano::core::cbor` | Lossless reader/value tree and preserved/canonical writer with limits | `tests/core` |
| C004 | `cardano::crypto` | Fixed hashes, Ed25519/BIP32, required hashes, secp256k1, BLS12-381, EMIP-3, secure randomness, legacy Daedalus signing, ABOR, and Byron proxy certificates | `tests/crypto`, `tests/hardening` |
| C005 | `cardano::chain` | Shelley and Byron addresses, typed Byron attributes/content/checksum, Base58/Bech32/CRC32, Byron and Shelley genesis parsing, bootstrap/vkey witnesses, and genesis witness helpers | `tests/chain/address_tests.cpp`, `tests/chain/byron_tests.cpp` |
| C006 | `cardano::chain` | Distinct Byron-through-Conway public model inventory; owned dynamic and specialized JSON DTOs; lossless/canonical CBOR; recursive Data/metadata/value; era-specific transaction body, output, witness, redeemer, certificate, pool, relay, header, block, protocol-parameter, and Conway-governance validation; provider-backed historical dispatch; complete supported-rule disposition below. | `tests/chain/multi_era_tests.cpp`, `tests/chain/plutus_data_tests.cpp`, `tests/api_inventory.cmake`, provider corpus |
| C007 | `cardano::chain` | Explicit multi-era blocks/common views, preserved hashes, fee/reference-fee/ExUnit/minimum-ADA operations, deposits/refunds, metadata and Plutus JSON, script-data hashes, genesis IDs, and witness helpers | `tests/chain/ledger_tests.cpp`, `tests/chain/multi_era_tests.cpp`, provider corpus |
| C008 | `cardano::chain` | Staged input/output/mint/withdrawal/certificate/governance builders, exact CIP-2 largest-first and RandomImprove, change, collateral, reference-script dominance/fees, stable auxiliary merge, redeemer indexing/ExUnits, evaluation drafts, size gates, and checked/unchecked signing | `tests/chain/builder_tests.cpp` |
| C009 | `cardano::cip` | CIP-8 COSE signing, CIP-25 metadata, and CIP-36 voting registration/deregistration with frozen compatibility behavior | `tests/cip` |
| C010 | `cardano::plutus` | Recursive typed Data, schemas, CBOR-hex, detailed JSON, large integers, chain-owner conversion, and limits | `tests/plutus` |
| C011 | `cardano::plutus` | UPLC 1.0/1.1 Flat/text codecs, builtins 0–100, semantics A–E, CEK budgets, exact cost mappings, parameter application, V1/V2/V3 contexts, and raw phase-two valuation for protocol 5–11 | `tests/plutus` and applicable provider conformance corpus |
| C012 | Focused owners | Component headers, aggregate facade, install metadata, examples, installed identity checks, and a checked crosswalk for all 975 frozen runtime/type-only bindings across all 60 inventory rows. Non-identical names are explicit C++ naming/ownership adaptations checked against their public owner. | Compile/link/runtime consumer tests and `tests/api_inventory.cmake` |
| C013 | Workspace | macOS 26.5 ARM64/Apple Clang 21/Ninja gates, warnings-as-errors, provider integrity, deterministic 50,000-case hardening, ASan/UBSan, Apple clang-format, conditional clang-tidy, installed-content/path/archive scan, and clean consumer | `ci`, `sanitizers`, and `hardening` workflows |

## Builder compatibility details

- `CoinSelectionStrategyCIP2` values are frozen at largest-first `0`, RandomImprove `1`,
  multi-asset largest-first `2`, and multi-asset RandomImprove `3`. RandomImprove uses the
  specified Fisher–Yates draws, target grouping, ideal-value replacement, and final dynamic-fee
  fill; production and tests both require an injected `SecureRandomSource`.
- Reference UTxOs retain their input for body field 18 and their output for reference-script
  discovery and tiered fee sizing. A recognized embedded Script satisfies and removes the
  matching inline witness even if the inline request is added later.
- Auxiliary maps merge by canonical-key identity: replacement keeps the left position and new
  keys append. A non-map on either side replaces the left value wholesale.
- `build_for_evaluation` emits dummy zero ExUnits and fake vkeys only for sizing/evaluation.
  `build_signed` retains all requirements; `SignedTxBuilder::build` is checked and
  `build_unchecked` is explicit.

## Frozen public-inventory crosswalk

[`tests/api_inventory.cmake`](tests/api_inventory.cmake) reads the immutable inventory block in the
0001 instruction and fails unless all 60 rows and all 975 bindings remain present. An exact public
C++ spelling owns the binding by default. `CARDANO_MODEL_JSON(Model)` owns the corresponding
`ModelJSON` DTO without generating a header. The script's `CARDANO_API_ADAPTATIONS` table is the
exhaustive exception list: it maps camel-case JavaScript functions to idiomatic C++ snake case,
maps the two declared entry aliases to their single nominal owner, and records C++ language
adaptations for copying, result propagation, protocol-magic values, and CBOR integer bounds.
Every mapped target must independently occur in an installed public header.

The type-only JSON inventory is owned by
[`era_json.hpp`](include/cardano/chain/era_json.hpp). Common structural DTOs such as transaction
inputs, rational values, credentials, anchors, protocol versions, VRF certificates, Values, Byron
outputs, address attributes, and bootstrap witnesses have explicit fields. Remaining era DTOs use
the owned recursive `JsonValue` tree parameterized by their distinct runtime model; no dependency
JSON type crosses the public boundary.

## Normative requirement crosswalk

| Instruction scope | Change owner | Implementation and validation |
| --- | --- | --- |
| Workspace, dependency, install, and macOS ARM64 contracts | C001, C013 | CMake workspace, pinned manifest/overlay, presets, install/export/consumer and release scans |
| Core values, errors, collections, Bech32, network, and randomness contracts | C002 | `cardano::core`; core, ownership, and hardening tests |
| Lossless/preserved/canonical CBOR and resource limits | C003 | `cardano::core::cbor`; CBOR golden, malformed, mutation, property, and hardening tests |
| Hashes, keys, signing, encryption, secp256k1, BLS, and Byron cryptography | C004 | `cardano::crypto`; crypto vectors, negative tests, UPLC primitive tests, and hardening |
| Shelley/Byron addresses, genesis, witnesses, and public text/JSON contracts | C005 | `cardano::chain`; address and Byron/genesis/witness tests |
| Byron-through-Conway wire/JSON models and official-era rules | C006 | `cardano::chain`; era/model, historical corpus, Data, malformed, and API-inventory tests |
| Multi-era views, hashing, fees, deposits, minimum ADA, and JSON conversions | C007 | `cardano::chain`; ledger and multi-era tests |
| Builder configuration, CIP-2, balancing, collateral, redeemers, valuation, and signing | C008 | `cardano::chain`; builder invariants and end-to-end tests |
| CIP-8, CIP-25, and CIP-36 contracts | C009 | `cardano::cip`; proposal-specific vector and malformed-input tests |
| Typed Data, schemas, conversions, and limits | C010 | `cardano::plutus`; Data/schema/round-trip/limit tests |
| UPLC codecs, CEK, costs, builtins, contexts, and phase two | C011 | `cardano::plutus`; unit, conformance, budget, and provider tests |
| Focused/aggregate ownership, public identities, and package consumption | C012 | Component headers, `cardano.hpp`, inventory gate, examples, and installed consumers |

## JSON compatibility disposition

The generic era facade exposes bounded CBOR-shaped JSON through
`cbor_value_to_json`/`cbor_value_from_json`. Public type-only DTO names are now fully owned, while
the specialized contracts above retain explicit C++ fields. Runtime generic JSON intentionally
retains the frozen large-integer/map round-trip distinction. `TransactionInput`, addresses,
credentials, rational/unit-interval values, ExUnits/prices, anchors, protocol versions, VRF
certificates, Values, IP addresses, scripts, and bootstrap witnesses own the exact specialized
runtime contracts required by 0001. Other era models intentionally use the frozen generic
CBOR-shaped conversion and their distinct owned `EraModelJSON<Model>` DTO.

The Byron bootstrap runtime codec follows the observable compact four-string form:
`public_key`, `signature`, and `chain_code` are lowercase hex and `attributes` is lowercase CBOR
hex. The separately declared array-shaped DTO is retained as `BootstrapWitnessJSON`; the runtime
codec continues to use the observable compact four-hex-string form.

No TypeScript, JavaScript, provider source, or dependency type crosses the public API boundary.

## Official era-rule disposition

The seven immutable Ledger CDDL artifacts are applied through four dispositions:

- `DIRECT`: the named public model validates the complete production.
- `NESTED`: the production is validated by its owning transaction, block, certificate, update,
  script, Data, or governance validator.
- `SHARED`: a lossless-CBOR primitive or reusable bounded validator owns the production.
- `EVIDENCE_ONLY`: the rule is not serialized ledger data.

Every retained rule in a later era is re-applied by that era's body/output/witness/block limits;
the delta lists below identify additions and removals without treating inherited rules as
unchecked.

### Byron

All 64 rules are `DIRECT`, `NESTED`, or `SHARED` in the focused modules below
`src/chain/era/`, `byron.cpp`, the address owner, and the lossless CBOR owner:

`block`, `mainblock`, `ebblock`, `u8`, `u16`, `u32`, `u64`, `blake2b-256`, `txid`, `blockid`,
`updid`, `certificateid`, `voteid`, `hash`, `blake2b-224`, `addressid`, `stakeholderid`, `epochid`,
`slotid`, `pubkey`, `signature`, `attributes`, `addrdistr`, `addrtype`, `addrattr`, `address`,
`txin`, `txout`, `tx`, `txproof`, `twit`, `vsspubkey`, `vsssec`, `vssenc`, `vssdec`, `vssproof`,
`ssccomm`, `ssccomms`, `sscopens`, `sscshares`, `ssccert`, `ssccerts`, `ssc`, `sscproof`, `dlg`,
`dlgsig`, `lwdlg`, `lwdlgsig`, `bver`, `txfeepol`, `bvermod`, `updata`, `upprop`, `upvote`, `up`,
`difficulty`, `blocksig`, `blockcons`, `blockheadex`, `blockproof`, `blockhead`, `blockbody`,
`ebbcons`, and `ebbhead`.

Embedded tag 24 values, nonempty transaction collections, witness alternatives, address
attributes, update payloads, header/body proofs, and block variants are validated. The declared
legacy SSC certificate field order and frozen nested shares map are accepted only inside Byron
SSC ingestion; direct official models remain structurally checked.

### Shelley

All 82 wire rules are `DIRECT`, `NESTED`, or `SHARED`:

`block`, `header`, `header_body`, `block_number`, `slot`, `hash32`, `vkey`, `vrf_vkey`,
`vrf_cert`, `operational_cert`, `kes_vkey`, `sequence_number`, `kes_period`, `signature`,
`protocol_version`, `major_protocol_version`, `kes_signature`, `transaction_body`, `set<a0>`,
`transaction_input`, `transaction_id`, `transaction_output`, `address`, `coin`, `certificate`,
`account_registration_cert`, `stake_credential`, `credential`, `addr_keyhash`, `hash28`,
`script_hash`, `account_unregistration_cert`, `delegation_to_stake_pool_cert`, `pool_keyhash`,
`pool_registration_cert`, `pool_params`, `vrf_keyhash`, `unit_interval`, `reward_account`, `relay`,
`single_host_addr`, `port`, `ipv4`, `ipv6`, `single_host_name`, `dns_name`, `multi_host_name`,
`pool_metadata`, `url`, `pool_retirement_cert`, `epoch`, `genesis_delegation_cert`, `genesis_hash`,
`genesis_delegate_hash`, `move_instantaneous_rewards_cert`, `move_instantaneous_reward`,
`delta_coin`, `withdrawals`, `update`, `proposed_protocol_parameter_updates`,
`protocol_param_update`, `epoch_interval`, `nonnegative_interval`, `positive_int`, `max_word64`,
`nonce`, `auxiliary_data_hash`, `transaction_witness_set`, `vkeywitness`, `native_script`,
`script_pubkey`, `script_all`, `script_any`, `script_n_of_k`, `int32`, `bootstrap_witness`,
`transaction_index`, `metadata`, `metadatum_label`, `metadatum`, and `transaction`.

`signkey_kes` is `EVIDENCE_ONLY` because it is a signing secret, not a serialized ledger value.
Protocol-major limits, required body keys, rational bounds, relays, certificate/script
discriminators, block witness counts, and metadata indexes are era-specific.

### Allegra and Mary

Allegra revalidates all retained Shelley productions and adds `int64`, `min_int64`, `max_int64`,
`script_invalid_before`, `script_invalid_hereafter`, `auxiliary_data`, `auxiliary_data_array`, and
`auxiliary_scripts` as `NESTED`/`SHARED`. Shelley-only `int32` and `signkey_kes` are absent.

Mary revalidates all retained Allegra productions and adds `value`, `multiasset<a0>`, `policy_id`,
`asset_name`, and `mint` as `DIRECT`/`NESTED`. Policy IDs are 28 bytes, asset names are bounded to
32 bytes, and mint quantities are nonzero signed int64 values.

### Alonzo and Babbage

Alonzo revalidates all retained Mary rules and adds `cost_models`, `language`, `cost_model`,
`ex_unit_prices`, `ex_units`, `script_data_hash`, `required_signers`, `network_id`,
`plutus_v1_script`, `plutus_data`, `constr<a0>`, `big_int`, `big_uint`, `bounded_bytes`, `big_nint`,
`redeemers`, `redeemer`, `redeemer_tag`, and `auxiliary_data_map` as `DIRECT`, `NESTED`, or
`SHARED`. Language 0 has exactly 166 cost entries, redeemer indexes are uint32, ExUnits are
nonnegative int64, and `signkey_kes` is `EVIDENCE_ONLY`.

Babbage revalidates all retained Alonzo rules and adds `alonzo_transaction_output`,
`babbage_transaction_output`, `datum_option`, `data`, `script_ref`, `script`, and
`plutus_v2_script`. It removes `nonce` and the single `cost_model` alias. Language 0/1 cost arrays
have exactly 166/175 entries; array/map outputs, inline data, tag-24 reference scripts, reference
inputs, and collateral fields are checked by their owning models.

### Conway

Conway revalidates every retained Babbage rule and adds these `DIRECT`/`NESTED` productions:

`positive_coin`, `plutus_v3_script`, `certificates`, `nonempty_oset<a0>`,
`account_registration_deposit_cert`, `account_unregistration_deposit_cert`,
`delegation_to_drep_cert`, `drep`, `delegation_to_stake_pool_and_drep_cert`,
`account_registration_delegation_to_stake_pool_cert`,
`account_registration_delegation_to_drep_cert`,
`account_registration_delegation_to_stake_pool_and_drep_cert`,
`committee_authorization_cert`, `committee_cold_credential`, `committee_hot_credential`,
`committee_resignation_cert`, `anchor`, `drep_registration_cert`, `drep_credential`,
`drep_unregistration_cert`, `drep_update_cert`, `nonzero_int64`, `negative_int64`,
`positive_int64`, `nonempty_set<a0>`, `voting_procedures`, `voter`, `gov_action_id`,
`voting_procedure`, `vote`, `proposal_procedures`, `proposal_procedure`, `gov_action`,
`parameter_change_action`, `pool_voting_thresholds`, `drep_voting_thresholds`,
`guardrails_script_hash`, `hard_fork_initiation_action`, `treasury_withdrawals_action`,
`no_confidence`, `update_committee`, `new_constitution`, `constitution`, `info_action`, and
`nonempty_list<a0>`.

`potential_languages` is `NESTED` in the uint8 cost-model-key validator and `signkey_kes` remains
`EVIDENCE_ONLY`. Conway removes genesis delegation, MIR, and update productions from its accepted
certificate/body choices. Direct decoding enforces the official protocol-parameter widths;
wider frozen fields are accepted only by the historical multi-era compatibility boundary.
Standard prelude names such as `uint`, `bool`, `nil`, `bigint`, and `encoded-cbor` are `SHARED`
through the complete-input, resource-bounded CBOR owner.
