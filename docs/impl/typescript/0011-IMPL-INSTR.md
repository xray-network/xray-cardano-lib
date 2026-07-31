# TypeScript implementation 0011 instruction

Implementation-Version: v1
Implementation-ID: typescript/0011
Created: 20260731T075521Z
Evidence-Mode: HYBRID
Depends-On: ./0004-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`typescript/0004`](./0004-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted Byron-through-Conway models, lossless CBOR, and transaction ownership |
| [`0001-cardano-cips`](https://github.com/xray-network/cardano-lib/blob/main/updates/providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md) | `PROVIDER` | Yes | Immutable active CIP-0021 transaction restrictions |
| Captured `CIP-0021/README.md` | `PROVIDER` | Yes | Exact deterministic restrictions and explicitly time-sensitive appendix boundary |
| `libs/typescript/packages/{core,chain,cip,runtime}/` | `LOCAL` | Yes | Existing lossless CBOR nodes, transaction owner, package exports, and tests |

## Objective

Add an optional, pure CIP-0021 diagnostic pass for hardware-wallet interoperability that reports
all deterministic captured violations without mutating, rebuilding, signing, or claiming support
for any device or firmware version.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C001` | Diagnose canonical CBOR, set tags, integer ranges, and collection limits | Additive read-only API | `libs/typescript/packages/cip/src/cip21/` using core/chain owners | Raw-CBOR mutation matrix and boundary counts |
| `C002` | Diagnose CIP-0021 body/output/asset/certificate/withdrawal/auxiliary restrictions | Additive read-only API | `libs/typescript/packages/cip/src/cip21/` | One positive and one negative fixture per rule/combination |
| `C003` | Publish a namespace-scoped optional diagnostic surface | Additive focused/namespace exports | CIP manifest/barrels, runtime CIP facade/tests, READMEs | Import, identity, browser, packed-consumer checks |

## Public contract

- Export
  `diagnose_cip21_transaction(transaction, options?): readonly CIP21Violation[]` from
  `@xray-network/cardano-cip/cip21`.
- Accept the existing lossless Conway `Transaction` binding. Never create a competing transaction,
  certificate, output, or witness type.
- `CIP21Violation` is deeply immutable and contains stable `code`, structural JSON-pointer `path`,
  and human-readable `message`. Sort results by path then code, independent of map insertion order.
- `options.auxiliaryDataMode` is `"hash-only"` by default or `"catalyst-registration"`. The latter
  enables the captured Catalyst auxiliary-data shape checks; the default makes no claim about
  metadata content that a hardware wallet does not serialize.
- Return at most 4096 detailed violations, followed by one `DIAGNOSTIC_LIMIT` violation when more
  exist. Diagnostics must not throw for a structurally decodable transaction; malformed bytes
  still fail through the existing transaction decoder before this API is called.
- Export namespace `cip21` from the CIP root and runtime CIP facade. The aggregate runtime may
  expose that namespace but must not add the diagnostic function as an unscoped top-level name.

## Stable violation codes

Use exactly these public codes:

- `NON_CANONICAL_INTEGER`, `NON_CANONICAL_LENGTH`, `UNSORTED_MAP`,
  `INDEFINITE_LENGTH`
- `INCONSISTENT_SET_TAG`, `UNSUPPORTED_BODY_ENTRY`, `INTEGER_OUT_OF_RANGE`,
  `TOO_MANY_ELEMENTS`, `EMPTY_OPTIONAL`
- `LEGACY_EMPTY_MULTI_ASSET`, `EMPTY_INLINE_DATUM`, `EMPTY_SCRIPT_REF`
- `DUPLICATE_POLICY`, `DUPLICATE_ASSET`, `UNSUPPORTED_CERTIFICATE`,
  `POOL_REGISTRATION_COMBINATION`
- `DUPLICATE_WITHDRAWAL`, `MULTIPLE_VOTERS`, `MULTIPLE_VOTES`
- `INVALID_CATALYST_AUXILIARY_DATA`, `DIAGNOSTIC_LIMIT`

One violation may cover a repeated instance at its exact path; do not collapse distinct paths.

## C001: encoding and bound rules

- Inspect preserved CBOR rather than only canonical re-encoding. Integers and lengths for major
  types 2 through 5 must use their shortest forms; all containers must be definite length.
- Every map must use deterministic RFC-7049 canonical key order: shorter encoded key first, then
  bytewise lexical order. Numeric transaction-body keys therefore remain lowest-to-highest.
- Optional tag 258 on every semantic set identified by the accepted Conway grammar must be either
  present everywhere or absent everywhere within the transaction.
- Every signed integer must fit int64 and every unsigned integer must fit uint64 in addition to
  its field-specific accepted ledger bound.
- Independently limit to 65535: body inputs, outputs, policy groups per output or mint, assets per
  policy, certificates, pool owners, pool relays, withdrawals, collateral inputs, required
  signers, reference inputs, and the total number of witnesses across all witness collections.
- Voting procedures may contain exactly one voter and that voter exactly one governance-action
  procedure.
- Diagnose included optional empty lists/maps unless CIP-0021 explicitly requires the empty value.

## C002: transaction restrictions

- Body keys 6 (`update`) and 20 (`proposal procedures`) are unsupported.
- A legacy output with no native assets must encode its amount as coin, not `[coin, {}]`.
- A present post-Alonzo inline datum and present reference script must each contain nonempty
  encoded content.
- Policy and asset maps in outputs and mint must be sorted and contain no duplicate policy IDs or
  asset names, including duplicates visible only in lossless raw CBOR.
- Diagnose certificate types genesis-key delegation, instantaneous rewards,
  stake+vote delegation, stake-registration+pool delegation,
  stake-registration+vote delegation, and stake-registration+pool+vote delegation.
- If a pool-registration certificate is present, diagnose every other certificate, withdrawal,
  mint, output datum/hash/reference script, script-data hash, collateral input, required signer,
  collateral return, total collateral, reference input, voting procedure, treasury value, or
  donation in the same transaction.
- Withdrawals must be canonically ordered and contain no duplicate reward account.
- In `"catalyst-registration"` mode, auxiliary data must use the captured tuple
  `[transaction_metadata, auxiliary_scripts]` and `auxiliary_scripts` must be an empty array.
  Do not infer that arbitrary label-61284/61285 metadata is valid Catalyst registration beyond
  this captured structural rule.

## Implementation steps

1. Build a read-only walker over preserved CBOR nodes with stable structural paths and bounded
   violation collection.
2. Implement canonical-form, tag-consistency, integer, and count checks without re-encoding the
   caller's transaction.
3. Implement the exact body, output, asset, certificate, withdrawal, vote, and optional Catalyst
   checks above.
4. Add the focused module and namespace-only root/runtime exports, documentation, and explicit
   “diagnostic, not device support” wording.
5. Add rule-isolated, interaction, browser, identity, and packed-consumer tests.

## Validation

- For every stable code, add a positive control and a minimally changed negative raw-CBOR fixture
  proving its exact path and deterministic order.
- Cover 65535 and 65536 for every counted collection, int64/uint64 boundaries, every nonminimal
  width, indefinite containers, map-key order, duplicate raw keys, and mixed tag-258 usage.
- Cover all unsupported body/certificate cases and every forbidden pool-registration
  combination independently and together.
- Cover legacy/post-Alonzo outputs, empty optionals, one/multiple voters and votes, withdrawal
  duplicates, both auxiliary modes, and diagnostic-limit truncation.
- Assert byte-for-byte transaction immutability before/after diagnostics.
- Run targeted core/chain/CIP/runtime tests, browser and packed-consumer checks, then
  `npm --prefix libs/typescript run check`.

## Compatibility and human review

This is additive and advisory. A clean report means only that the captured deterministic CIP-0021
rules passed; it is not a promise that a particular wallet, firmware, signing mode, or future
device supports the transaction. Review raw duplicate detection, canonical ordering, semantic-set
inventory, total-witness counting, and the intentionally narrow Catalyst mode.

## Completion criteria

- Every deterministic restriction before the device appendix has one implementation and test
  disposition.
- Diagnostics are stable, bounded, exhaustive within the cap, and leave caller state untouched.
- The public surface remains namespace-scoped and reuses accepted transaction identities.
- Documentation excludes device/firmware support claims and identifies snapshot time.
- Browser, import, identity, packed-consumer, targeted, and workspace checks pass.
- `npm --prefix libs/typescript run check` passes.
- The paired result records the exact captured CIP-0021 file consumed and every excluded appendix
  item.

## Out of scope

- Transaction mutation, canonicalization, builder enforcement, signing, or submission
- Ledger/Trezor transports, APDUs, firmware profiles, support matrices, or device discovery
- Signing-mode credential/path policy beyond deterministic captured transaction restrictions
- Future CIP-0021 revisions or uncaptured wallet-specific limits

## Blockers

None.
