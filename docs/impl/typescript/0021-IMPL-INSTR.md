# TypeScript implementation 0021 instruction

Implementation-Version: v1
Implementation-ID: typescript/0021
Created: 20260803T104819Z
Evidence-Mode: LOCAL
Depends-On: ./0004-IMPL-RESULT.md
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| `typescript/0004` | `IMPLEMENTATION_RESULT` | Yes | Accepted Conway `CostModels` nominal owner |
| Existing `CostModels.from_json()` / `to_json()` implementation | `LOCAL` | Yes | JSON consumption surface requiring public identity proof |
| Chain/runtime barrels and packed consumers | `LOCAL` | Yes | Export and packaging boundary |

## Objective

Ensure the existing `CostModels` JSON API is consumable from the Conway focus, chain root, and
aggregate runtime as the same binding, without SDK-shaped parameter types or duplicate owners.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `TS21-EXPORT1` | Prove/reinforce focused, chain-root, runtime-chain, and aggregate `CostModels` exports | Additive if a barrel is missing | Existing chain owner | Strict identity tests |
| `TS21-CONSUME1` | Demonstrate numeric-language JSON consumption through all public paths | No new model or DTO | Existing `CostModels` API | JSON symmetry/malformed tests |
| `TS21-PACK1` | Exercise JSON construction in packed ESM/NodeNext/bundler consumers | Packaging-only | Runtime tests | Browser and pack gate |

## Implementation steps

1. Audit all intended public paths and add only missing re-exports by identity.
2. Add identity and JSON-consumption tests using numeric language keys.
3. Add packed consumer coverage and concise documentation; do not add an SDK protocol-parameter
   adapter.

## Validation

- Assert strict binding identity across focused, root, runtime-chain, and aggregate paths.
- Assert `from_json`/`to_json` symmetry and existing malformed-input rejection.
- Run CostModels, import, browser, packed, and full TypeScript checks.

## Compatibility and human review

Additive or validation-only. Review that no duplicate model or SDK-shaped parameter object was
introduced and language IDs retain the accepted numeric-key JSON contract.

## Completion criteria

Packed consumers can call the existing JSON API through intended public paths with one nominal
binding and the full gate passes.

## Out of scope

- Provider DTO mapping, protocol-parameter fetching, or cost-model semantic validation changes

## Blockers

None.
