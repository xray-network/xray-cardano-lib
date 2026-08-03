# TypeScript implementation 0006 result

Result-Version: v1
Implementation-ID: typescript/0006
Instruction: ./0006-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C001` | Implemented | Corrected `CertificateKind` and the transaction-builder witness routing to the official Conway tags `0..4` and `7..18`, with no aliases for gaps 5 and 6. | All 17 tags round-trip through typed variants; tags 5, 6, and 19 are rejected; full workspace checks pass. |
| `C002` | Implemented | Added typed factories, discriminants, variant accessors, and field getters to the existing certificate, DRep, voter, vote, governance-ID, and nested voting-map owners. | Certificate, DRep, voter, vote, uint16 boundary, typed-map, lossless-CBOR, and focused/root/aggregate identity tests pass. |
| `C003` | Implemented | Added typed governance-action, constitution, proposal, and voting-procedure construction directly to the accepted nominal classes, and updated builders to consume those typed APIs. | All seven governance actions, optional anchors and guardrails, proposal fields, packed consumers, and unchanged builder body assembly pass. |

## Outcome

The Conway public model now exposes typed construction and inspection for certificates and
governance values without introducing competing wrappers. Certificate discriminants match the
official ledger CDDL, including the flattened pool-registration parameter group. Transaction
builders use typed certificate, proposal, voter, governance-ID, and voting-procedure APIs while
retaining the accepted body encoding and witness behavior.

Existing generic `new(...ConwayInput[])` entry points remain as validated advanced raw
compatibility APIs. Lossless decode still preserves accepted noncanonical CBOR, while canonical
encoding remains explicit.

## Inputs consumed

- The current user instruction
- `typescript/0004` accepted implementation result
- `cardano-ledger/0001-cardano-ledger` provider snapshot and the captured Conway CDDL
- `libs/typescript/packages/chain/src/era/conway/` models and validation
- `libs/typescript/packages/chain/src/builder/transaction.ts`
- Existing chain, runtime, public-import, upstream-vector, and packed-consumer tests
- The adjacent `xray-js` Cardano call sites, inspected read-only for compatibility workarounds

## Project changes

- Corrected `CertificateKind` values and copied builder routing for post-pool certificate tags.
- Added typed constructors and field getters to all 17 existing certificate variant classes.
- Added `Certificate.kind()`, typed bridge factories, and matching `as_*()` accessors.
- Added typed DRep and voter factories, kinds, and hash accessors.
- Added uint16 `GovActionId` construction and defensive transaction-hash/index getters.
- Added typed `VotingProcedure` and mutable typed `VotingProcedures.insert/get` APIs.
- Added seven typed governance-action factories and discriminated accessors.
- Added typed constitution and proposal construction and field accessors.
- Added typed pool-parameter operator/owner inspection used by witness collection.
- Documented typed factories as the preferred Conway construction surface and raw constructors as
  advanced compatibility APIs.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C001` | Conway certificate tags are exactly `0..4` and `7..18`; tags 5 and 6 are invalid. | Numeric values previously assigned after tag 4 are corrected; enum names remain. | Replace persisted or hand-written shifted tag numbers with the official Conway values. |
| `C002` | Existing nominal certificate variants, `Certificate`, `DRep`, `Voter`, `GovActionId`, `VotingProcedure`, and `VotingProcedures` expose typed construction and inspection. | Additive; generic validated constructors remain available. | Prefer named factories and field accessors over positional numeric construction. |
| `C003` | Existing `GovAction`, `Constitution`, and `ProposalProcedure` owners expose typed factories, kinds, and accessors, and all public paths re-export the same bindings. | Additive and identity-preserving; accepted wire behavior is unchanged. | Remove local governance wrappers and numeric action construction where these APIs cover the use case. |

## Validation

The following checks passed from the TypeScript workspace:

```sh
npm run build
node --test packages/chain/test/conway-foundations.test.mjs \
  packages/chain/test/builders.test.mjs \
  packages/chain/test/upstream-vectors.test.mjs
npm run check
```

- Required focused chain suite: 20 passed.
- Complete built TypeScript suite: 151 passed.
- Packed package smoke suite: 534 intended files, 2,373,258 unpacked bytes; ESM, NodeNext, and
  bundler consumers passed.

## Deviations from instruction

None.

## Remaining human review

Confirm the corrected certificate numbers, flattened pool-registration mapping, typed-to-wire
factories, optional-field behavior, and compatibility status of the generic raw constructors,
then decide whether this result should move from `REVIEW` to `ACCEPTED`.

## Reproducibility

From the repository root with Node.js 20.19 or newer and locked dependencies installed, run
`npm --prefix libs/typescript run check`.
