# TypeScript implementation 0009 result

Result-Version: v1
Implementation-ID: typescript/0009
Instruction: ./0009-IMPL-INSTR.md
Evidence-Mode: HYBRID

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C001` | Implemented | Added bounded, immutable CIP-57 document parsing/validation with duplicate-key, shape, extension, code, hash, and resource checks. | Blueprint document tests pass in the full suite. |
| `C002` | Implemented | Added local-reference Data/builtin schema evaluation with applicators, exact integer bounds, recursion state, and structural constraints. | Data, constants, recursion, and malformed cases pass. |
| `C003` | Implemented | Published identity-preserving focused, Plutus-root, runtime-Plutus, and aggregate-runtime exports. | Build, import/browser tests, and packed consumers pass. |

## Outcome

TypeScript applications can parse and validate untrusted CIP-57 blueprints and existing Plutus values through a bounded, browser-safe, offline API.

## Inputs consumed

- `typescript/0003` accepted result
- `0001-cardano-cips` snapshot, captured CIP-57 document, and five schema files
- Existing TypeScript Plutus Data, UPLC, hash, package, runtime, and documentation owners

## Project changes

- Added the `plutus/blueprint` focused module and package export.
- Added document, schema, value, code/hash, reference, and limit tests.
- Added root/runtime exports and trust-boundary documentation.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C001` | Blueprint admission is typed, immutable, and bounded. | Additive. | Parse untrusted JSON before consuming validators. |
| `C002` | Existing Data/constants validate without coercion or external resolution. | Existing nominal owners retained. | Pass established Plutus values. |
| `C003` | All intended paths expose identical bindings. | Additive exports. | Import from focused or aggregate paths without adapters. |

## Validation

`npm --prefix libs/typescript run check` passed: TypeScript build, 184 tests, and packed ESM/NodeNext/bundler consumers (566 files, 2,599,775 unpacked bytes).

## Deviations from instruction

None.

## Remaining human review

Review captured-schema fidelity, recursive/reference behavior, resource ceilings, code/hash validation, and export identity before acceptance.

## Reproducibility

Run `npm --prefix libs/typescript run check` from the repository root.
