# TypeScript implementation 0005 result

Result-Version: v1
Implementation-ID: typescript/0005
Instruction: ./0005-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `C001` | Implemented | Snapshotted nested value/metadata inputs and encoded every ledger integer as a safe JSON number or canonical decimal string. | Mutation, boundary, JSON, and CBOR cases pass in the full suite. |
| `C002` | Implemented | Enforced disjoint spending/collateral/reference/selectable inputs and exact checked collateral accounting with native-asset conservation. | Role-order, rollback, and collateral tests pass. |
| `C003` | Implemented | Added exact fee/change fixed-point detection with cycle/64-pass rejection and failure-atomic output/fee state. | Convergence and rollback tests pass with package consumers. |

## Outcome

Values preserve caller isolation and exact JSON integers, while transaction construction rejects ambiguous roles, incomplete collateral, and unstable fees without leaking partial state.

## Inputs consumed

- Current TypeScript workspace/package ownership and completion command
- Existing Conway `Value`, `MultiAsset`, metadata, and transaction-builder implementations
- Existing chain and runtime regression/package-consumer tests

## Project changes

- Hardened Conway value/metadata JSON and defensive ownership.
- Hardened transaction input roles, selection, collateral, convergence, and rollback.
- Extended focused chain regression tests.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `C001` | Unsafe ledger integers serialize as canonical decimal strings. | Safe numbers retain numeric JSON. | Accept both exact JSON representations. |
| `C002` | Input roles are disjoint and collateral is fully conserved. | Invalid ambiguous builders now reject. | Keep each UTxO in one role. |
| `C003` | A built transaction proves fee/change stability. | Stable outputs are unchanged. | Handle deterministic convergence errors. |

## Validation

`npm --prefix libs/typescript run check` passed: TypeScript build, 184 tests, and packed ESM/NodeNext/bundler consumers (566 files, 2,599,775 unpacked bytes).

## Deviations from instruction

None.

## Remaining human review

Review exact-integer JSON compatibility, collateral policy boundaries, and rollback/convergence semantics before acceptance.

## Reproducibility

Run `npm --prefix libs/typescript run check` from the repository root.
