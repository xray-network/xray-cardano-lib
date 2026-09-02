# TypeScript implementation 0018 instruction

Implementation-Version: v1
Implementation-ID: typescript/0018
Created: 20260803T104819Z
Evidence-Mode: LOCAL
Depends-On: ./0004-IMPL-RESULT.md
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| `typescript/0004` | `IMPLEMENTATION_RESULT` | Yes | Accepted Conway transaction, certificate, address, and witness wire owners |
| `libs/typescript/packages/chain/src/builder/transaction.ts` | `LOCAL` | Yes | Existing `RequiredWitnessSet`, builders, and resolved UTxO owner |
| Adjacent xray-js signing-key discovery helper | `LOCAL` | Yes | Read-only downstream evidence for covered fields and workarounds |

## Objective

Add chain-owned required-witness discovery from a transaction plus resolved inputs, returning only
existing ledger, hash, address, and witness owners.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `TS18-DISC1` | Add `discover_required_witnesses(transaction, resolvedInputs)` | Additive; no SDK DTOs | Chain builder/witness owner | Spending, collateral, resolution tests |
| `TS18-FIELD1` | Cover certificates, withdrawals, required signers, native scripts, datums, redeemers, and reference scripts | Additive protocol analysis | Existing nominal types | Positive and malformed field tests |
| `TS18-SET1` | Add canonical empty `RequiredSigners.new()` construction | Additive compatibility helper | Existing Conway owner | Tag-258 canonical test |

## Implementation steps

1. Index resolved `TransactionUnspentOutput` values structurally and reject conflicting duplicates.
2. Discover spending/collateral credentials and datum requirements from resolved outputs.
3. Discover typed certificate, withdrawal, explicit signer, native-script, redeemer, and reference
   script requirements without account objects.
4. Deduplicate through `RequiredWitnessSet` and preserve all existing nominal owners.
5. Add malformed, missing-resolution, duplicate, script, certificate, withdrawal, collateral,
   native signer, datum, redeemer, and script-reference tests.

## Validation

- Cover key, bootstrap, native, Plutus, datum-hash, redeemer, and reference-script requirements.
- Cover primary inputs, collateral, certificates, withdrawals, explicit signers, and witness native
  scripts.
- Reject malformed or missing resolution and conflicting duplicate UTxOs.
- Run focused chain/runtime tests, browser/package tests, and the full TypeScript gate.

## Compatibility and human review

Additive. Review conservative behavior for unresolved script language, redeemer indexing, duplicate
resolution, and the distinction between script witnesses and reference scripts.

## Completion criteria

Downstream accounts can intersect discovered `Ed25519KeyHash` values locally without reimplementing
ledger parsing, and all witness categories are represented by existing owners.

## Out of scope

- Account ownership, key derivation, UTxO discovery, wallet connectors, or signing
- Phase-one validation or script execution

## Blockers

None.
