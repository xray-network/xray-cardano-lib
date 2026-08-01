# TypeScript implementation status

Status-Version: v1
Library: typescript

This is the only lifecycle and decision-proof ledger for TypeScript implementation records.

## Implementation ledger

| ID | Instruction | State | Result | Evidence mode | Decision proof |
| --- | --- | --- | --- | --- | --- |
| `0001` | [CML baseline](./0001-IMPL-INSTR.md) | `ACCEPTED` | [Result](./0001-IMPL-RESULT.md) | `DIRECT` | Existing human acceptance preserved from the previous terminal TypeScript status record. |
| `0002` | [Message signing](./0002-IMPL-INSTR.md) | `ACCEPTED` | [Result](./0002-IMPL-RESULT.md) | `DIRECT` | Existing human acceptance preserved from the previous terminal TypeScript status record. |
| `0003` | [UPLC](./0003-IMPL-INSTR.md) | `ACCEPTED` | [Result](./0003-IMPL-RESULT.md) | `DIRECT` | Existing human acceptance preserved from the previous terminal TypeScript status record. |
| `0004` | [Cardano Ledger validation](./0004-IMPL-INSTR.md) | `ACCEPTED` | [Result](./0004-IMPL-RESULT.md) | `HYBRID` | Existing human acceptance preserved from the previous terminal TypeScript status record. |
| `0005` | [Value and transaction-builder correctness](./0005-IMPL-INSTR.md) | `PLANNED` | — | `LOCAL` | Awaiting implementation. |
| `0006` | [Typed Conway governance construction](./0006-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0007` | [CIP-14 asset fingerprints](./0007-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0008` | [Strict identities and HD derivation](./0008-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0009` | [CIP-57 Plutus contract blueprints](./0009-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0010` | [CIP-67/68 token metadata](./0010-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0011` | [Optional CIP-21 compatibility diagnostics](./0011-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0012` | [XRAY Cardano Lib package-family rename](./0012-IMPL-INSTR.md) | `REVIEW` | [Result](./0012-IMPL-RESULT.md) | `LOCAL` | Package-family rename completed with the full TypeScript gate passing; awaiting human decision. |
| `0013` | [Typed Plutus void and aggregate exports](./0013-IMPL-INSTR.md) | `REVIEW` | [Result](./0013-IMPL-RESULT.md) | `LOCAL` | Typed void schema and aggregate exports completed with TypeScript and C++ compatibility gates passing; awaiting human decision. |
| `0014` | [CIP8Message facade](./0014-IMPL-INSTR.md) | `REVIEW` | [Result](./0014-IMPL-RESULT.md) | `LOCAL` | CIP8Message signing, verification, envelope typing, and aggregate exports completed with the full TypeScript gate passing; awaiting human decision. |
| `0015` | [CostModels JSON parsing and validation](./0015-IMPL-INSTR.md) | `REVIEW` | [Result](./0015-IMPL-RESULT.md) | `LOCAL` | CostModels JSON parsing, symmetric serialization, and constructor validation completed with the full TypeScript gate passing; awaiting human decision. |
