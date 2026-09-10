# TypeScript implementation 0027 instruction

Implementation-Version: v1
Implementation-ID: typescript/0027
Created: 20260909T114243Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

All code-formatted repository paths are repository-root-relative. Inputs describe local requirements
and owned files. The preceding upstream audit motivates this plan; uncaptured upstream material is
not a normative implementation input. No new upstream conformance claim follows from completing it.

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| Current human request (2026-09-09): plan the preceding provider-audit recommendations; the bounded local requirement is reproduced in Objective | `LOCAL` | Yes | User-requested behavior and scope. |
| `AGENTS.md` | `LOCAL` | Yes | Repository ownership, lifecycle, and validation rules. |
| `docs/src/adr/typescript/0001-lossless-cbor-and-encoding-metadata.md` | `LOCAL` | Yes | Preserved versus canonical encoding and mutation compatibility. |
| `docs/src/adr/typescript/0003-upstream-evidence-and-package-ownership.md` | `LOCAL` | Yes | Package ownership and evidence boundaries. |
| `libs/typescript/README.md` | `LOCAL` | Yes | Maintained workspace and completion gate. |
| `libs/typescript/package.json` | `LOCAL` | Yes | Build, test discovery, and package smoke commands. |
| `libs/typescript/packages/chain/README.md` | `LOCAL` | Yes | Current chain construction and lossless-decoding contract. |
| `libs/typescript/packages/chain/package.json` | `LOCAL` | Yes | Chain ownership, exports and dependencies. |
| `libs/typescript/packages/chain/src/builder/transaction.ts` | `LOCAL` | Yes | Transaction, proposal, witness, and redeemer builders. |
| `libs/typescript/packages/chain/src/era/shared/validation.ts` | `LOCAL` | Yes | Existing tagged-set wire validation; compatibility boundary. |
| `libs/typescript/packages/chain/src/era/shared/models.ts` | `LOCAL` | Yes | Existing duplicate-preserving wire collections. |
| `libs/typescript/packages/chain/test/builders.test.mjs` | `LOCAL` | Yes | Builder balances, input uniqueness, witnesses and evaluation tests. |

## Objective

Reject duplicate builder certificates and proposals.

Prevent duplicate certificates and proposal procedures from entering constructed transactions,
while retaining existing input guards and lossless decoder behavior. Equality at the construction
boundary uses the existing canonical CBOR representation of each complete value, not object
identity or preserved-width differences. A rejected addition must leave builder state unchanged.
This is bounded local construction hardening requested in the audit; it is not adoption of every
new CML collection API or decoder rejection rule.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C01` | Reject a certificate matching an already-added complete certificate before changing certificates, deposits/refunds, required witnesses or redeemers. | Distinct certificates sharing a credential remain distinct; preserve insertion order. | `libs/typescript/packages/chain/src/builder/transaction.ts` | Test repeated instance, semantic clone with different CBOR hints, and valid distinct certificates. |
| `C02` | Prevalidate an entire proposal batch for duplicates within the batch and against prior additions, then apply it atomically. | Do not partially add valid batch prefixes or double-count deposits; preserve accepted order. | `libs/typescript/packages/chain/src/builder/transaction.ts` | Test intra-batch and cross-call duplicates, late duplicate rollback, unique batch acceptance and deposit totals. |
| `C03` | Retain stable certificate/proposal redeemer positions across multiple successful additions and after rejected additions; ensure proposing indices use the full transaction sequence. | Preserve current spending-input ordering and encoded redeemer identity. | `libs/typescript/packages/chain/src/builder/transaction.ts` | Build script-bearing certificates/proposals across calls; inspect pointer indices and evaluate supported cases. |
| `C04` | Verify witness construction still emits unique witnesses, scripts and datums under the existing keyed-map policy; retain existing duplicate-input guards and raw decoder preservation. | Do not change witness replacement semantics or add a universal duplicate rejection to taggedSet. | `libs/typescript/packages/chain/test/builders.test.mjs` | Repeat add/merge operations and inspect output collections; retain existing input-role and raw-round-trip tests. |

## Implementation steps

1. Add failing tests showing unchecked certificate/proposal append and verify all affected aggregate state before/after a rejected call.
2. Use existing canonical value helpers to preflight complete values; validate proposal batches before mutation.
3. Ensure redeemer positions follow the accumulated transaction sequence and remain stable after failed additions.
4. Add witness uniqueness regression coverage without changing the existing merge policy; run focused and workspace gates.

## Validation

From the repository root:

```sh
npm --prefix libs/typescript run build
node --test libs/typescript/packages/chain/test/builders.test.mjs
npm --prefix libs/typescript run check
```

Require negative tests to fail on the pre-change implementation. Compare output bodies, deposits,
refunds, required witnesses and redeemer indices before/after rejected additions. Validate the
SPECTRE instruction/result mapping and record exact outcomes.

## Compatibility and human review

Applications attempting duplicate certificate/proposal construction now receive a TypeError
before state changes. Do not silently deduplicate these operations: that could change deposits
and redeemer identities. Raw CBOR models keep their existing era-specific acceptance and byte
preservation. A broader set-policy change requires separate evidence and planning.

## Completion criteria

Constructed certificate/proposal sets are unique, batches fail atomically, economic totals and
redeemer positions are correct, witness uniqueness remains intact, and all current input-role,
lossless wire and workspace tests pass. Record C01–C04 and the precise compatibility change.

## Out of scope

Generic set classes, rewriting historical decoders, blanket taggedSet rejection, new witness
replacement rules, full ledger phase-one validity, protocol expansion, capture, and C++ work.

## Blockers

None for this bounded LOCAL objective. Adoption of newly captured upstream behavior is separate;
see [cardano-multiplatform-lib consumer guidance](../../providers/cardano-multiplatform-lib/PROVIDER.md#maintained-consumer-guidance).
