# TypeScript implementation 0017 instruction

Implementation-Version: v1
Implementation-ID: typescript/0017
Created: 20260803T104819Z
Evidence-Mode: LOCAL
Depends-On: ./0004-IMPL-RESULT.md
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| `typescript/0004` | `IMPLEMENTATION_RESULT` | Yes | Accepted Conway transaction owner and lossless CBOR contract |
| `libs/typescript/packages/chain/src/era/conway/model.ts` | `LOCAL` | Yes | Existing `Transaction`, body, witness, and auxiliary owners |
| Adjacent xray-js transaction-parts helper | `LOCAL` | Yes | Read-only downstream evidence for the manual CBOR workaround |

## Objective

Expose typed, defensive transaction decomposition and reconstruction through the existing Conway
owners so consumers never decode the transaction array manually.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `TS17-PARTS1` | Add typed `Transaction` construction and body/witness/validity/auxiliary accessors | Additive typed surface; generic raw construction remains | Chain Conway `Transaction` | Field, optional, malformed, and mutation tests |
| `TS17-WIRE1` | Preserve accepted child wire bytes when decomposing noncanonical transactions | Existing lossless contract | Chain Conway codec | Preserved-wire and canonical tests |
| `TS17-EXPORT1` | Prove focused, chain-root, and aggregate identity | No new nominal owner | Existing barrels/runtime | Import, browser, and packed tests |

## Implementation steps

1. Add a typed `Transaction.new(body, witnessSet, isValid, auxiliaryData)` overload.
2. Add defensive `body()`, `witness_set()`, `is_valid()`, and `auxiliary_data()` accessors.
3. Decode fields through the existing nominal owners and retain generic validated construction.
4. Add focused, malformed, mutation, lossless, identity, and packed-consumer coverage.

## Validation

- Cover true/false validity and present/absent auxiliary data.
- Reject malformed transaction arrays through the existing validator.
- Prove child mutations do not alter the transaction.
- Prove accepted noncanonical child bytes survive decomposition and independently canonicalize.
- Run focused chain/runtime tests and the full TypeScript gate.

## Compatibility and human review

Additive. Review typed-to-wire field order, defensive ownership, optional auxiliary handling, and
lossless child extraction.

## Completion criteria

Consumers can decompose and reconstruct a transaction without direct CBOR array access, public
bindings retain identity, and all required validation passes.

## Out of scope

- Transaction building policy, signing, submission, or UTxO lookup
- Required-witness discovery

## Blockers

None.
