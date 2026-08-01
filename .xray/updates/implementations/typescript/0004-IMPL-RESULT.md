# TypeScript implementation 0004 result

Result-Version: v1
Implementation-ID: typescript/0004
Instruction: ./0004-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C001` | `IMPLEMENTED` | `libs/typescript/packages/chain/`, `libs/typescript/packages/plutus/src/typed_data/` | `npm --prefix libs/typescript run check` |

## Outcome

Implemented direct, era-specific validation for the captured official Byron-through-Conway ledger
CDDL while preserving Cardano Lib's existing nominal type ownership, lossless CBOR trees,
canonical-output API, historical JSON contracts, and multi-era facade.

Previously permissive historical wrappers now reject unsupported map keys, missing mandatory
fields, wrong discriminators and tuple lengths, invalid nested structures, incorrect fixed-size
bytes, invalid integer/rational bounds, empty containers where the CDDL requires nonempty values,
and inconsistent block collection indexes. The validation applies to CBOR, hex, JSON, factories,
list mutation, direct block decoding, and explicit multi-era network decoding.

The implementation retains two documented read-only compatibility paths required by the frozen
CML corpus: legacy Byron SSC certificate/shares forms and historical Conway block protocol
parameter widths. Direct public Conway protocol-parameter decoding remains strict to the official
widths. All 86 frozen multi-era block fixtures retain their recorded outcome: 85 byte-exact
round trips and one expected rejection.

UPLC remains a separate Plutus-domain concern. Program/term/data types, text parsing, Flat and
script-envelope codecs, cost models, machine costs, budgeted CEK evaluation,
`apply_params_to_script`, and `eval_phase_two_raw` are exported by
`@xray-network/cardano-plutus`; ADR 0003 records that ownership without attributing UPLC
implementation to this ledger snapshot.

Package-owned typed Data schemas and codecs live under `src/typed_data/` and emit under
`dist/esm/typed_data/`. Root exports and the public `@xray-network/cardano-plutus/data` subpath
expose the same bindings.

## Artifacts consumed

- `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/byron.cddl` —
  `bc6f7fc1c6295046a2944ad784ce4b5ea544a185400add4aa7b122cd8e46a107`
- `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/shelley.cddl` —
  `3a4723732bcd9dafbbb5d2e6c29d9ae3347575212adbf6bbd5e4de16a6790791`
- `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/allegra.cddl` —
  `e71cbf08f4fe62bb654f8371f4934eba750c17546b7d44c0c4f38b470eaabcd6`
- `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/mary.cddl` —
  `aa13e8687343658c5195b115d54fa1f4dfd7beeb70020f3e6c57f63f9be7aef1`
- `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/alonzo.cddl` —
  `7460f60206160f3b459ee58befb1b912acf1812402113faa3963bf4bda0cf98d`
- `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/babbage.cddl` —
  `fcca168539a91a16c45b55c724b52e34bd85ff6499148a976ae5e01b66cff272`
- `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/cddl/eras/conway.cddl` —
  `316ed8ee090ea172983083329e849f24f4360a236d26be0a6f2094c6078f1e1f`
- `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/legal/LICENSE` —
  `0d542e0c8804e39aa7f37eb00da5a762149dc682d7829451287e11b938e94594`
- `../../providers/cardano-ledger/0001-cardano-ledger/artifacts/legal/NOTICE` —
  `58721f8b6ca67f0fcbe1cd739b384fec3126a35f7d79951aedaa2bc3863a3162`

The exact provider inventory is present, every selected artifact is a regular file, and all nine
SHA-256 digests were rechecked. Provenance remains official Cardano Ledger commit
`a624de4c8db7286a6c065da149679ea55f7d5629`; the captured legal material is Apache-2.0.

## Official-to-CML-to-owned-source dispositions

The frozen CML snapshot remains the source of the existing public class names and historical
facade. The official CDDL is now the authority for accepted wire structure. A rule listed below
has one of these dispositions:

- **owned/direct**: a public era class invokes an exact validator on every decode/factory path;
- **owned/nested**: the rule is enforced inside its owning block, transaction, certificate,
  script, data, metadata, or governance validator and does not require a new nominal public type;
- **shared/no-op**: an existing fixed-size crypto, address, Plutus-data, native-script, CBOR
  preservation, or scalar owner already matched the official rule and is now called by the era
  validator; or
- **evidence-only**: the rule names a non-wire signing secret or grammar range and has no standalone
  public ledger value.

This records all captured supported rules. Era deltas inherit the preceding era's dispositions,
but each retained rule is applied through that era's validator rather than assumed equivalent.

### Byron baseline

All 64 rules are owned/direct, owned/nested, or shared/no-op under
`libs/typescript/packages/chain/src/era/byron/validation.ts`, the existing Byron runtime/address owners, and the
shared lossless codec:

`block`, `mainblock`, `ebblock`, `u8`, `u16`, `u32`, `u64`, `blake2b-256`, `txid`, `blockid`,
`updid`, `certificateid`, `voteid`, `hash`, `blake2b-224`, `addressid`, `stakeholderid`, `epochid`,
`slotid`, `pubkey`, `signature`, `attributes`, `addrdistr`, `addrtype`, `addrattr`, `address`,
`txin`, `txout`, `tx`, `txproof`, `twit`, `vsspubkey`, `vsssec`, `vssenc`, `vssdec`, `vssproof`,
`ssccomm`, `ssccomms`, `sscopens`, `sscshares`, `ssccert`, `ssccerts`, `ssc`, `sscproof`, `dlg`,
`dlgsig`, `lwdlg`, `lwdlgsig`, `bver`, `txfeepol`, `bvermod`, `updata`, `upprop`, `upvote`, `up`,
`difficulty`, `blocksig`, `blockcons`, `blockheadex`, `blockproof`, `blockhead`, `blockbody`,
`ebbcons`, and `ebbhead`.

Compared with the CML surface, no nominal owner was moved. Official embedded-CBOR tags,
nonempty transaction inputs/outputs, witnesses, block variants, bounds, and hashes are enforced.
The official and legacy SSC certificate field orders and both official/frozen shares shapes are
accepted only at those historical compatibility points.

### Shelley baseline

All 82 rules are owned/direct, owned/nested, or shared/no-op under
`libs/typescript/packages/chain/src/era/shelley/validation.ts`, shared era models, and the shared lossless codec:

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

`signkey_kes` is evidence-only: it is a signing secret, not serialized ledger data. Compared with
CML, the same public classes remain, while mandatory body fields, protocol parameter indexes,
rational bounds, certificate/script discriminators, relay and hash sizes, and block body/witness
and metadata-index consistency are now checked directly.

### Allegra delta

All Shelley rules retained by Allegra use the Allegra-specific direct/nested validator. The eight
added rules—`int64`, `min_int64`, `max_int64`, `script_invalid_before`,
`script_invalid_hereafter`, `auxiliary_data`, `auxiliary_data_array`, and `auxiliary_scripts`—are
owned/nested in `libs/typescript/packages/chain/src/era/shared/post-shelley-validation.ts` and
`libs/typescript/packages/chain/src/era/allegra/validation.ts`. Shelley-only `int32` and evidence-only
`signkey_kes` are absent, matching the official artifact. The CML public Allegra wrappers and JSON
shape remain unchanged.

### Mary delta

All Allegra rules retained by Mary use the Mary-specific direct/nested validator. The five added
rules—`value`, `multiasset<a0>`, `policy_id`, `asset_name`, and `mint`—are owned/nested in the
shared post-Shelley validator and Mary runtime. Policy IDs are 28 bytes, asset names are at most
32 bytes, and mint amounts are signed int64. Existing CML public ownership remains unchanged.

### Alonzo delta

All Mary rules retained by Alonzo use the Alonzo-specific direct/nested validator. These added
rules are owned/nested in the Alonzo and shared ledger-data owners:

`cost_models`, `language`, `cost_model`, `ex_unit_prices`, `ex_units`, `script_data_hash`,
`required_signers`, `network_id`, `plutus_v1_script`, `plutus_data`, `constr<a0>`, `big_int`,
`big_uint`, `bounded_bytes`, `big_nint`, `redeemers`, `redeemer`, `redeemer_tag`,
and `auxiliary_data_map`.

Alonzo's reintroduced `signkey_kes` is evidence-only. The implementation enforces language-0 cost
model length, ExUnits bounds, redeemer purpose/index/data shape, Plutus Data structure,
required-signer sets, tagged auxiliary data, transaction validity, and block index consistency.
UPLC parsing/evaluation is not inferred from these ledger rules.

### Babbage delta

All Alonzo rules retained by Babbage use the Babbage-specific direct/nested validator. The seven
added rules—`alonzo_transaction_output`, `babbage_transaction_output`, `datum_option`, `data`,
`script_ref`, `script`, and `plutus_v2_script`—are owned/nested in the Babbage and shared
ledger-data owners. Babbage removes `nonce` and the fixed `cost_model` alias while retaining
`cost_models`, matching the artifact. Array/map output choices, embedded CBOR for inline data and
reference scripts, V1/V2 scripts, reference inputs, collateral return/total collateral, and
protocol versions are validated without changing lossless encoding.

### Conway delta

All Babbage rules retained by Conway use the Conway-specific direct/nested validators in
`libs/typescript/packages/chain/src/era/conway/validation.ts` and the existing Conway models. These added rules are
owned/direct or owned/nested:

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

`potential_languages` is an owned/nested range used for cost-model map keys; `signkey_kes` remains
evidence-only. Conway removes the obsolete Shelley genesis-delegation, MIR, and update rules
(`genesis_delegation_cert`, `genesis_hash`, `genesis_delegate_hash`,
`move_instantaneous_rewards_cert`, `move_instantaneous_reward`, `delta_coin`, `update`, and
`proposed_protocol_parameter_updates`) from its accepted choices.

The implementation additionally enforces protocol major/minor bounds, network IDs 0/1, unit
interval ordering and positive denominators, signed-64 ExUnits, UTF-8 anchor/URL/DNS limits,
nonempty governance maps/sets/lists, certificate and governance discriminators, proposal deposits,
mint nonzero values, current witness alternatives, and block collection invariants. Existing CML
nominal class names and JSON forms are retained. Frozen Conway block ingestion alone accepts the
legacy uint64 widths at protocol parameter keys 8, 22, 23, 24, and 27; direct
`ProtocolParamUpdate` decoding enforces the official narrower bounds.

No captured supported rule is deferred or silently ignored. Standard CDDL prelude names such as
`uint`, `bool`, `nil`, `bigint`, and `encoded-cbor` are enforced by the shared CBOR codec and
validator primitives rather than represented as separate public classes.

## Project changes

- Added reusable CDDL validation combinators for exact arrays/maps, tagged sets, choices,
  discriminators, embedded CBOR, fixed-size bytes, UTF-8 text limits, integer widths, rational
  values, and recursive Plutus data.
- Added independent Byron, Shelley, Allegra, Mary, Alonzo, Babbage, and Conway validators and
  connected them to their existing public runtime owners.
- Made historical CBOR/hex/JSON factories and list mutation invoke the owning era validator while
  keeping original-byte and independent canonical encodings.
- Validated direct and explicitly tagged multi-era blocks through the selected era, added
  multi-era certificate CBOR/hex factories, and prevented multi-era JSON from bypassing era
  validation.
- Tightened Conway constructor and decoder behavior for network IDs, rationals, protocol versions,
  ExUnits, anchors, protocol parameters, outputs, certificates, scripts, redeemers, and governance
  values. Checked ExUnits addition now detects signed-64 overflow.
- Updated synthetic builder/ledger fixtures to use valid official transaction-body structure;
  production builder behavior and output ownership did not otherwise change.
- Added focused ledger-CDDL malformed/boundary tests and extended the historical corpus tests for
  official validation, duplicate-map preservation, and all eight explicit network tags.
- Documented the chain validation boundary and Plutus package ownership in ADR 0003. The
  `@xray-network/cardano-plutus/uplc` export has parser, Flat codec, cost-model, evaluator API,
  and packed-consumer coverage.
- Kept package-owned typed Data schemas and codecs under `src/typed_data/`, with package targets
  under `dist/esm/typed_data/` and the compatible public `./data` export.
- Added no dependency, generated source, WASM, native addon, Node-only production edge, or runtime
  dependency on snapshot artifacts. Public ledger class ownership and JSON type aliases remain
  stable.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C001` | Enforce official Byron-through-Conway ledger CDDL through era-specific validation, with package-owned typed Data under the internal `typed_data` layout, while retaining the documented historical Byron SSC and frozen Conway block-ingestion compatibility paths. | Compatible hardening of malformed-input behavior; valid historical bytes, the public `./data` subpath, and public nominal ownership remain stable. | Apply equivalent validation for supported eras, consume the official provider evidence directly for an independent mapping, or record unsupported eras as not applicable. |

## Validation

| Check | Result | Evidence |
| --- | --- | --- |
| Provider artifact inventory and SHA-256 review | PASS | Nine expected regular files present; hashes listed above |
| `npm --prefix libs/typescript run build` | PASS | All TypeScript project references built |
| focused ledger and multi-era tests | PASS | Mandatory fields, bounds, malformed choices, JSON validation, preservation, 86 historical fixtures, and all 8 network tags passed |
| `node --test libs/typescript/packages/plutus/test/*.test.mjs` | PASS | Public API plus parser, Flat, CEK, cost model, context, phase-two, and all 1,003 applicable official UPLC programs passed |
| `npm --prefix libs/typescript run test:built` | PASS | 137 workspace tests passed |
| `node libs/typescript/packages/runtime/test/pack-smoke.mjs` | PASS | 526 intended files; ESM, NodeNext, and bundler consumers passed |
| `npm --prefix libs/typescript run check` | PASS | Authoritative build, 137-test workspace suite, and packed-consumer gate passed |
| Typed Data layout review | PASS | Source and emitted modules use `typed_data`; the public `./data` export resolves to `dist/esm/typed_data/index.js`; no stale internal `data` directory exists |
| `git diff --check` | PASS | No whitespace errors |

## Deviations from plan

- The plan grouped rule reconciliation by era but did not prescribe the implementation form.
  Validation is implemented as package-owned runtime combinators layered onto existing nominal
  owners, rather than generating TypeScript from CDDL. This preserves ownership and byte-lossless
  behavior while making malformed-input rejection explicit.
- The frozen corpus exposed two historical differences from the captured official grammar. Rather
  than changing fixture outcomes, the narrow Byron SSC and Conway block-ingestion compatibility
  paths described above are retained and tested. Strict direct Conway APIs still follow the
  official CDDL.
- No builder or ledger-operation algorithm required a production change after validation was
  connected; only invalid synthetic fixtures were corrected. This is a justified no-op for those
  plan owners.
- UPLC parsing and evaluation are outside this ledger snapshot and are owned by
  `@xray-network/cardano-plutus` under ADR 0003.
- Legal artifacts were reviewed as evidence only. They introduce no new runtime material or
  dependency and required no repository legal-file change.

## Remaining human review

- Review the compatibility decision to accept legacy Byron SSC forms and wider historical Conway
  protocol-parameter fields only while ingesting frozen blocks.
- Review the consensus-sensitive Conway governance choices, nonempty-container enforcement,
  protocol bounds, and malformed-input behavior.
- Review the UPLC public API and package-size impact under ADR 0003; generic CEK callers
  must select protocol-appropriate costs, semantics, and budgets.
- Confirm the internal `typed_data` source and emitted layout retains the public
  `@xray-network/cardano-plutus/data` binding identity.
- Confirm the captured Apache-2.0 LICENSE/NOTICE handling. No changed obligation was identified,
  but this result is not legal advice.

## Reproducibility

Implementation used only the captured snapshot artifacts and the frozen local CML comparison
snapshot. It did not fetch, refresh, execute, generate from, or substitute upstream material.
Published packages contain only reviewed Cardano Lib TypeScript output and do not read snapshot
artifacts at runtime.
