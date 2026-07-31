# C++ implementation 0002 instruction

Implementation-Version: v1
Implementation-ID: cpp/0002
Created: 20260731T075521Z
Evidence-Mode: HYBRID
Depends-On: ./0001-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-ledger/0001-cardano-ledger/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`cpp/0001`](./0001-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted builder, ledger-model, error, ownership, and compatibility baseline |
| [`0001-cardano-ledger`](https://github.com/xray-network/xray-cardano-lib/blob/main/updates/providers/cardano-ledger/0001-cardano-ledger/SNAPSHOT.md) | `PROVIDER` | Yes | Frozen Conway transaction-body grammar, protocol-parameter field shapes, collateral field shapes, and governance shapes; not phase-one calculations |
| `libs/cpp/include/cardano/chain/builder.hpp` and `libs/cpp/src/chain/builder.cpp` | `LOCAL` | Yes | Existing public construction API and implementation to harden |
| `libs/cpp/include/cardano/chain/era/conway.hpp` and `libs/cpp/include/cardano/chain/era/shared/models.hpp` | `LOCAL` | Yes | Existing Conway nominal owners and validated wire models |

The provider authorizes wire shapes only. Collateral calculations, return requirements, exact
rational arithmetic, and rejection rules below deliberately define XRAY Cardano Lib's repository-local
checked-builder policy; they are not presented as provider-backed or complete phase-one ledger
validation.

## Objective

Harden transaction construction so every successful mutation preserves input-role, collateral,
balance, fee, and wire-validity invariants, and add typed Conway construction paths that return the
existing chain-owned era models. This sequence closes construction gaps; it does not broaden
consensus validation or replace lossless raw-CBOR ingestion.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C2-BLD1` | Make spending, reference, and collateral input identities pairwise disjoint and make every builder mutation transactional. | Existing accepted call sequences keep their result; newly rejected overlap and partial-mutation cases violate the checked-builder policy. | `include/cardano/chain/builder.hpp`, `src/chain/builder.cpp` | State-snapshot, duplicate, cross-role, and failure-injection tests |
| `C2-COL1` | Enforce the checked-builder collateral percentage, maximum-count, total-collateral, return, and native-asset preservation policy. | The existing configuration fields remain; successful output becomes stricter and deterministic. | Builder owner plus existing chain `Value` owner | Boundary, overflow, script/no-script, return, and multi-asset tests |
| `C2-FEE1` | Replace the silent eight-pass fee/change loop with bounded fixed-point convergence that either proves stability or fails without changing builder state. | Stable transactions retain canonical bytes; non-convergent construction now returns a dedicated error. | `src/chain/builder.cpp` | Convergence, cycle, size-boundary, overflow, and rollback tests |
| `C2-CFG1` | Add an exact nonnegative rational fee primitive and a validated adapter from a Conway protocol-parameter model into `TransactionBuilderConfig`. | Keep the integer reference-script-cost setter as a deprecated exact-denominator-one convenience; do not silently round rational inputs. | `include/cardano/chain/ledger.hpp`, `include/cardano/chain/builder.hpp`, corresponding sources | Rational arithmetic, missing-field, range, and provider-shape tests |
| `C2-CWY1` | Close unvalidated raw-value constructors and add typed builders for Conway transaction bodies, proposal procedures, voting procedures, and protocol-parameter updates; return the existing `TransactionBody`, `ProposalProcedure`, `VotingProcedures`, and `ProtocolParamUpdate` owners. | Lossless validated decoding remains; direct use of an unvalidated `core::cbor::Value` constructor migrates to `from_value`, while typed construction is additive. | `include/cardano/chain/era/shared/models.hpp`, `include/cardano/chain/era/conway.hpp`, `src/chain/era/conway/construction.ipp` | Direct-header compile tests and positive/negative canonical-CBOR tests |
| `C2-API1` | Export the additions through `cardano::chain` and the aggregate facade, document construction versus ingestion, and extend the API inventory without creating duplicate model types. | Source-compatible additive API, except for calls that previously produced an invalid transaction. | Chain headers, `cardano.hpp`, `API_PARITY.md`, API inventory | Focused-component, aggregate-identity, and installed-consumer tests |

## Implementation steps

1. Introduce one private input-role index keyed by `TransactionInput::canonical_identity()`.
   Preflight an entire requested mutation against both current and staged state before updating
   inputs, witnesses, redeemers, datums, scripts, or required signers. A duplicate within one role
   and an identity reused across roles return stable, distinct builder errors.
2. Stage every public multi-item mutation (`add_inputs_from`, certificate, withdrawal, proposal,
   vote, collateral, and witness aggregation) in temporary state and commit only after all
   validation and checked arithmetic succeed. Document and test the strong failure guarantee.
3. Define `NonnegativeRational { uint64_t numerator; uint64_t denominator; }` in the ledger owner.
   Construction rejects denominator zero. Multiplication followed by ceiling division uses checked
   wide arithmetic and never floating point.
4. Under the repository-local checked-builder policy, compute required collateral as
   `ceil(final_fee * collateral_percentage / 100)`. When Plutus execution is present, require at
   least one collateral input, at most `max_collateral_inputs`, and enough lovelace. Emit
   `total_collateral` equal to the requirement. If a return is supplied, require its lovelace to
   equal aggregate collateral lovelace minus total collateral and require it to contain exactly
   all native assets from the collateral inputs. Require a return whenever collateral contains a
   native asset. Reject collateral fields on a no-Plutus transaction unless the caller uses an
   explicitly named unchecked finalization path.
5. Compute fee and change against a staged builder snapshot. Continue until fee, serialized body
   size, change outputs, and reference-script fee are identical in two consecutive passes. Detect
   repeated states and cap work at 32 passes; either condition returns `non_convergent` and restores
   the original state. Absorbing coin-only dust into fee is one final state that must itself be
   rechecked.
6. Change reference-script fee calculation to exact rational ceiling arithmetic as a
   repository-local builder guarantee. The protocol-parameter adapter validates every required
   field, including the captured Conway key-33 field shape used as the reference-script byte
   price, and rejects negative, non-integral where an integer is required, zero-denominator,
   missing, duplicate, or out-of-range values. Do not describe this adapter or its arithmetic as a
   complete provider-backed phase-one calculation.
7. Make macro-generated raw `core::cbor::Value` constructors non-public and keep
   `from_value`, `from_cbor`, `from_cbor_hex`, and `from_json` as the fallible validated ingestion
   paths. Add compile-fail coverage for direct raw construction and source-migration coverage for
   `from_value`. Then implement `ConwayTransactionBodyBuilder`, `ConwayProposalProcedureBuilder`,
   `ConwayVotingProceduresBuilder`, and `ConwayProtocolParamUpdateBuilder`. Their setters use
   existing nominal chain/crypto/value types, enforce required/nonempty fields and unique map keys,
   and build the existing era model only after validation against the captured CDDL shapes. No
   public setter accepts an unlabelled `core::cbor::Value`; deliberately open extension fields use
   an explicitly named `validated_extension` method and re-run the owning model validator.
8. Keep `era_models.hpp` a compatibility umbrella, add focused and installed-consumer coverage,
   update construction examples and the frozen API crosswalk, and do not modify captured evidence.

## Validation

- Add focused tests below `tests/chain/builder_tests.cpp` and a Conway construction test source.
  Cover same-role duplicates, every pair of cross-role collisions, batch-internal collisions, and
  byte-for-byte state equality before and after each failing call.
- Cover collateral percentages `0`, `1`, `100`, and `UINT32_MAX`; zero and maximum input counts;
  exact, deficient, and overflowing collateral; absent/wrong/exact returns; native assets; and
  transactions with and without Plutus execution.
- Exercise a normal fixed point, fee-CBOR-width transitions, minimum-ADA change changes, repeated
  states, the 32-pass limit, and overflow. Assert no successful build is returned from an unstable
  state.
- Decode representative captured Conway shapes into existing models, reconstruct equivalent
  values through typed builders, and compare canonical CBOR. Add malformed and missing-field cases
  for each typed builder and the protocol-parameter adapter.
- Run `cmake --workflow --preset ci`, `cmake --workflow --preset sanitizers`, and
  `cmake --workflow --preset hardening` from `libs/cpp/`.

## Compatibility and human review

The accepted `cpp/0001` types retain ownership and raw lossless decode behavior. Direct raw-value
constructors are intentionally removed from the public construction surface because they bypass
the library's `Result` validation boundary; `from_value` is the source-migration path. This
sequence may reject builder operations that violate the checked-builder policy or create
economically unsafe state, but it must not rewrite decoded historical CBOR. The integer
reference-script price remains a deprecated denominator-one adapter for one release; canonical
calculations use the rational owner.

Human review must verify the strong failure guarantee, collateral/native-asset accounting, exact
rounding, fixed-point termination, Conway field mapping, and the absence of duplicate public
nominal types.

## Completion criteria

- Every public builder mutation is atomic and all input roles are pairwise disjoint.
- Collateral and fee/change construction either satisfies the stated invariants or returns an
  owned error with original state unchanged.
- No public era-model constructor can retain an unvalidated raw CBOR value; all ingestion remains
  fallible and lossless through the validated factories.
- Typed Conway builders produce only existing model types valid against the captured provider
  shapes.
- The component, aggregate, inventory, completion, sanitizer, and hardening gates pass.
- A paired result records each change disposition and the exact evidence consumed.

## Out of scope

- Full node phase-one validation, chain-state lookup, protocol-parameter network fetching, wallet
  UTxO discovery, or transaction submission.
- A new coin-selection algorithm, automatic ExUnit evaluation, new era support, stable ABI, or
  changes to lossless historical ingestion.
- Generic schema-generated era builders or replacement of existing chain-owned Conway types.

## Blockers

None.
