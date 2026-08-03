# TypeScript implementation 0020 instruction

Implementation-Version: v1
Implementation-ID: typescript/0020
Created: 20260803T104819Z
Evidence-Mode: LOCAL
Depends-On: ./0003-IMPL-RESULT.md, ./0004-IMPL-RESULT.md
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| `typescript/0003` | `IMPLEMENTATION_RESULT` | Yes | Accepted phase-two evaluator and raw compatibility API |
| `typescript/0004` | `IMPLEMENTATION_RESULT` | Yes | Accepted redeemer/transaction/CostModels owners |
| Plutus phase-two API and chain `RedeemerWitnessKey` | `LOCAL` | Yes | Existing evaluator and typed identity owner |
| Adjacent xray-js evaluator loop | `LOCAL` | Yes | Read-only manual redeemer tag/index decoding evidence |

## Objective

Return phase-two evaluation results with the existing typed `RedeemerWitnessKey` identity so normal
consumers never decode legacy redeemer arrays manually.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `TS20-EVAL1` | Add typed `evaluatePhaseTwo` over existing chain owners | Additive; raw API remains | Plutus API | Evaluation parity and malformed tests |
| `TS20-ID1` | Return existing `RedeemerWitnessKey` with typed tag/index accessors | Identity-preserving | Chain owner re-used by Plutus | All tags/bounds and identity tests |
| `TS20-EXPORT1` | Export typed API/types through Plutus root and aggregate runtime | Additive by identity | Existing barrels/runtime | Browser and packed tests |

## Implementation steps

1. Add a typed result contract containing `RedeemerWitnessKey` and `PhaseTwoEvaluation`.
2. Accept existing `Transaction`, resolved `TransactionUnspentOutput`, and `CostModels` bindings.
3. Delegate evaluation to the raw API and strictly decode returned legacy redeemers through existing
   chain validation.
4. Retain `evaluatePhaseTwoRaw` unchanged for byte-oriented compatibility.
5. Add parity, malformed, identity, mutation, browser, and packed-consumer coverage.

## Validation

- Compare typed and raw evaluation costs/logs and tag/index identity.
- Reject malformed evaluator redeemer identities through strict existing owners.
- Prove returned identities are chain-owned and frozen result data remains defensively owned.
- Run focused Plutus/runtime tests and the full TypeScript gate.

## Compatibility and human review

Additive. Review budget tuple order, UTxO conversion, strict redeemer decoding, and raw API parity.

## Completion criteria

Normal evaluation consumers receive a typed existing redeemer key and no manual CBOR decoding is
needed; raw compatibility and all validation remain green.

## Out of scope

- Remote evaluation, transaction balancing, or execution-unit policy

## Blockers

None.
