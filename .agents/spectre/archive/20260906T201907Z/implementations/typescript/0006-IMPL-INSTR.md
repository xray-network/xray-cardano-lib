# TypeScript implementation 0006 instruction

Implementation-Version: v1
Implementation-ID: typescript/0006
Created: 20260731T075521Z
Evidence-Mode: HYBRID
Depends-On: ./0004-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-ledger/0001-cardano-ledger/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`typescript/0004`](./0004-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted Conway wire models, package ownership, and lossless-codec contract |
| [`0001-cardano-ledger`](../../providers/cardano-ledger/0001-cardano-ledger/SNAPSHOT.md) | `PROVIDER` | Yes | Official Conway certificate and governance CDDL |
| `libs/typescript/packages/chain/src/era/conway/` and `libs/typescript/packages/chain/src/builder/transaction.ts` | `LOCAL` | Yes | Existing nominal owners and builder consumers |

## Objective

Replace generic Conway construction at public call sites with typed, discriminated factories and
accessors while retaining the accepted nominal owners, lossless wire behavior, and a compatibility
path for advanced raw construction.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C001` | Correct certificate discriminators to official Conway tags `0..4` and `7..18` | Correctness fix; enum names remain available with corrected numeric values | `libs/typescript/packages/chain/src/era/conway/types.ts`, `model.ts` | All 17 certificate tags and malformed gaps |
| `C002` | Add typed factories, kinds, and field accessors for certificates, DReps, voters, votes, and governance IDs | Additive; existing nominal classes and generic `new(...)` remain | `libs/typescript/packages/chain/src/era/conway/`, builder consumers | Factory/accessor/CBOR/identity tests |
| `C003` | Add typed governance-action, constitution, proposal, and voting-procedure construction | Additive typed surface over accepted wire owners | `libs/typescript/packages/chain/src/era/conway/`, public barrels, runtime facade | Variants `0..6`, maps, proposals, imports, and packed consumers |

## Required semantic map

### Certificates

`CertificateKind` must map:

| Tags | Variants |
| --- | --- |
| `0..4` | stake registration, stake deregistration, stake delegation, pool registration, pool retirement |
| `7..13` | registration-with-deposit, unregistration-with-deposit, vote delegation, stake+vote delegation, stake registration+pool delegation, stake registration+vote delegation, stake registration+pool+vote delegation |
| `14..18` | committee hot authorization, committee cold resignation, DRep registration, DRep unregistration, DRep update |

Tags 5 and 6 are invalid in Conway and must not be assigned aliases. Give each existing certificate
variant class a strongly typed `new(...)` signature and named field getters. Give `Certificate`
`kind()` plus matching `as_*()` accessors that return the existing variant binding or `undefined`.

### Credentials, voting, and identifiers

- `DRep`: `new_key`, `new_script`, `new_always_abstain`, `new_always_no_confidence`, `kind`,
  `as_key`, and `as_script`, for tags 0, 1, 2, and 3.
- `GovActionId`: typed construction from the existing `TransactionHash` plus an unsigned 16-bit
  index, with `transaction_id()` and `index()` defensive getters.
- `Voter`: named factories/accessors for CC-hot key (0), CC-hot script (1), DRep key (2), DRep
  script (3), and stake-pool key (4).
- `VotingProcedure`: typed `Vote` (`No=0`, `Yes=1`, `Abstain=2`) plus optional `Anchor`, with
  `vote()` and `anchor()` accessors.
- `VotingProcedures`: typed insertion and lookup by the existing `Voter`, `GovActionId`, and
  `VotingProcedure` bindings; retain nonempty nested-map validation.

### Governance actions and proposals

- `GovAction` named factories, `kind()`, and `as_*()` accessors for parameter change (0), hard
  fork (1), treasury withdrawals (2), no confidence (3), update committee (4), new constitution
  (5), and information (6).
- Factories must use existing owners including `GovActionId`, `ProtocolParamUpdate`, `ScriptHash`,
  `Credential`, `Anchor`, `UnitInterval`, `ProtocolVersion`, and reward-account/address bindings.
- `Constitution` gets typed construction and accessors for its anchor and optional guardrails
  script hash.
- `ProposalProcedure` gets typed construction and accessors for deposit, reward account,
  `GovAction`, and `Anchor`.
- Keep the existing generic `.new(...ConwayInput[])` entry points as explicitly documented raw
  compatibility APIs. They must pass the same validation and must not create competing nominal
  wrapper types.

## Implementation steps

1. Correct the enum and every tag-position table or builder branch that copied the old values.
2. Add typed signatures and accessors directly to existing Conway nominal classes.
3. Share one internal discriminant parser so factories, `kind()`, `as_*()`, JSON, and builder
   logic cannot diverge.
4. Update certificate, proposal, and vote builders to consume the typed APIs without changing
   encoded bodies.
5. Export each binding from `@xray-network/xray-cardano-lib-chain/conway`, the chain root, and the runtime
   facade as the same identity.
6. Add exhaustive positive and malformed coverage, then run the workspace gate.

## Validation

- Assert canonical CBOR for all certificate tags, explicit rejection of 5, 6, and values above 18,
  and round trips through `kind()`/`as_*()`.
- Cover all four DReps, five voters, three votes, seven governance actions, optional anchors and
  guardrail hashes, uint16 governance indexes, empty/nonempty voting maps, and every certificate
  field boundary.
- Retain byte-preserving decode and independent canonical-encode tests for noncanonical valid CBOR.
- Add compile-time/public-import and runtime identity assertions for focused, root, and aggregate
  exports.
- Run `node --test libs/typescript/packages/chain/test/conway-foundations.test.mjs
  libs/typescript/packages/chain/test/builders.test.mjs
  libs/typescript/packages/chain/test/upstream-vectors.test.mjs`.
- Run `npm --prefix libs/typescript run check`.

## Compatibility and human review

The only numeric breaking correction is the currently wrong `CertificateKind` assignment after
tag 4; review downstream code that persisted those enum numbers. Generic construction remains for
compatibility, but documentation must prefer typed factories. Review all typed-to-wire mappings,
optional fields, collection nonemptiness, and identity-preserving re-exports.

## Completion criteria

- All certificate and governance discriminators match the captured Conway CDDL.
- Each covered wire variant has a typed factory, kind, and lossless field accessor.
- Builders use typed owners without changing accepted canonical transaction bytes.
- No duplicate credential, action, voter, certificate, or identifier nominal type is introduced.
- Targeted tests and `npm --prefix libs/typescript run check` pass.
- The paired result records the enum correction and the exact added API contract.

## Out of scope

- CIP-0129 governance text identifiers or HD key derivation
- Governance metadata lookup, anchor fetching, or network/provider APIs
- Dijkstra-era governance changes
- Policy evaluation, voting advice, or transaction submission

## Blockers

None.
