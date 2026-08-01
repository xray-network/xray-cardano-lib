# XRAY Updates status

Status-Version: v1

This is the only lifecycle and decision-proof ledger for all implementation records.

## C++ implementation status

Target: cpp

### Implementation ledger

| ID | Title | Instruction | State | Result | Evidence mode | Decision proof |
| --- | --- | --- | --- | --- | --- | --- |
| `0001` | Full TypeScript feature parity | [Instruction](./cpp/0001-IMPL-INSTR.md) | `ACCEPTED` | [Result](./cpp/0001-IMPL-RESULT.md) | `HYBRID` | Human acceptance was recorded on 2026-07-31 for the completed `C001`–`C013` result and its validation evidence. |
| `0002` | Builder and typed Conway construction hardening | [Instruction](./cpp/0002-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0003` | Portability, lean components, and benchmarks | [Instruction](./cpp/0003-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0004` | CIP-14 asset fingerprints | [Instruction](./cpp/0004-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0005` | Strict identities and HD derivation | [Instruction](./cpp/0005-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0006` | CIP-57 Plutus contract blueprints | [Instruction](./cpp/0006-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0007` | CIP-67/68 token metadata | [Instruction](./cpp/0007-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0008` | Optional CIP-21 compatibility diagnostics | [Instruction](./cpp/0008-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0009` | Local CMake test ownership | [Instruction](./cpp/0009-IMPL-INSTR.md) | `REVIEW` | [Result](./cpp/0009-IMPL-RESULT.md) | `LOCAL` | Implementation completed with local domain CMake ownership and the full 141-test CI workflow passing; awaiting human decision. |
| `0010` | XRAY Cardano Lib package rename | [Instruction](./cpp/0010-IMPL-INSTR.md) | `REVIEW` | [Result](./cpp/0010-IMPL-RESULT.md) | `LOCAL` | Rename completed with the full 141-test C++ workflow passing; awaiting human decision. |

## Repository implementation status

Target: repository

### Implementation ledger

| ID | Title | Instruction | State | Result | Evidence mode | Decision proof |
| --- | --- | --- | --- | --- | --- | --- |
| `0001` | Install XRAY Updates | [Instruction](./repository/0001-IMPL-INSTR.md) | `ACCEPTED` | [Result](./repository/0001-IMPL-RESULT.md) | `LOCAL` | Human requested installation of XRAY Updates. |

## TypeScript implementation status

Target: typescript

### Implementation ledger

| ID | Title | Instruction | State | Result | Evidence mode | Decision proof |
| --- | --- | --- | --- | --- | --- | --- |
| `0001` | CML baseline | [Instruction](./typescript/0001-IMPL-INSTR.md) | `ACCEPTED` | [Result](./typescript/0001-IMPL-RESULT.md) | `DIRECT` | Existing human acceptance preserved from the previous terminal TypeScript status record. |
| `0002` | Message signing | [Instruction](./typescript/0002-IMPL-INSTR.md) | `ACCEPTED` | [Result](./typescript/0002-IMPL-RESULT.md) | `DIRECT` | Existing human acceptance preserved from the previous terminal TypeScript status record. |
| `0003` | UPLC implementation | [Instruction](./typescript/0003-IMPL-INSTR.md) | `ACCEPTED` | [Result](./typescript/0003-IMPL-RESULT.md) | `DIRECT` | Existing human acceptance preserved from the previous terminal TypeScript status record. |
| `0004` | Cardano Ledger validation | [Instruction](./typescript/0004-IMPL-INSTR.md) | `ACCEPTED` | [Result](./typescript/0004-IMPL-RESULT.md) | `HYBRID` | Existing human acceptance preserved from the previous terminal TypeScript status record. |
| `0005` | Value and transaction-builder correctness | [Instruction](./typescript/0005-IMPL-INSTR.md) | `PLANNED` | — | `LOCAL` | Awaiting implementation. |
| `0006` | Typed Conway governance construction | [Instruction](./typescript/0006-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0007` | CIP-14 asset fingerprints | [Instruction](./typescript/0007-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0008` | Strict identities and HD derivation | [Instruction](./typescript/0008-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0009` | CIP-57 Plutus contract blueprints | [Instruction](./typescript/0009-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0010` | CIP-67/68 token metadata | [Instruction](./typescript/0010-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0011` | Optional CIP-21 compatibility diagnostics | [Instruction](./typescript/0011-IMPL-INSTR.md) | `PLANNED` | — | `HYBRID` | Awaiting implementation. |
| `0012` | XRAY Cardano Lib package-family rename | [Instruction](./typescript/0012-IMPL-INSTR.md) | `REVIEW` | [Result](./typescript/0012-IMPL-RESULT.md) | `LOCAL` | Package-family rename completed with the full TypeScript gate passing; awaiting human decision. |
| `0013` | Typed Plutus void and aggregate exports | [Instruction](./typescript/0013-IMPL-INSTR.md) | `REVIEW` | [Result](./typescript/0013-IMPL-RESULT.md) | `LOCAL` | Typed void schema and aggregate exports completed with TypeScript and C++ compatibility gates passing; awaiting human decision. |
| `0014` | CIP8Message facade | [Instruction](./typescript/0014-IMPL-INSTR.md) | `REVIEW` | [Result](./typescript/0014-IMPL-RESULT.md) | `LOCAL` | CIP8Message signing, verification, envelope typing, and aggregate exports completed with the full TypeScript gate passing; awaiting human decision. |
| `0015` | CostModels JSON parsing and validation | [Instruction](./typescript/0015-IMPL-INSTR.md) | `REVIEW` | [Result](./typescript/0015-IMPL-RESULT.md) | `LOCAL` | CostModels JSON parsing, symmetric serialization, and constructor validation completed with the full TypeScript gate passing; awaiting human decision. |
| `0016` | CIP-4 wallet checksum facade | [Instruction](./typescript/0016-IMPL-INSTR.md) | `REVIEW` | [Result](./typescript/0016-IMPL-RESULT.md) | `LOCAL` | Canonical CIP-4 checksum calculation, focused and aggregate exports, and the xray-js xpub adapter completed with all library and SDK gates passing; awaiting human decision. |
