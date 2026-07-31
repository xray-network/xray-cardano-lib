# Library implementation status

Status-Template-Version: v1

Every `updates/implementations/<library>/STATUS.md` uses this schema. It is the library's only
lifecycle and decision-proof ledger. It contains only implementation records owned by that
library.

## Required document shape

```markdown
# <Library> implementation status

Status-Version: v1
Library: <library>

This is the only lifecycle and decision-proof ledger for <Library> implementation records.

## Implementation ledger

| ID | Instruction | State | Result | Evidence mode | Decision proof |
| --- | --- | --- | --- | --- | --- |
| `0001` | [Instruction](./0001-IMPL-INSTR.md) | `PLANNED` | — | `DIRECT` | Awaiting implementation. |
```

The implementation section and table header are required, even when the table contains no rows.
Put `No implementation records.` after an empty implementation table.

## Implementation ledger rules

- IDs are four digits, unique within the library, and ordered ascending.
- Each row links one matching instruction and, once created, its result.
- Evidence mode is `DIRECT`, `DERIVED`, `HYBRID`, or `LOCAL` and matches the instruction.
- Lifecycle state is `PLANNED`, `REVIEW`, `ACCEPTED`, `CANCELLED`, or `REJECTED`.
- A `PLANNED` or `CANCELLED` row may use `—` for Result. `REVIEW`, `ACCEPTED`, and `REJECTED`
  require a result link.
- Decision proof states the exact reason for the current state. Terminal proof records the human
  decision or the preserved prior human decision.

The lifecycle is:

```text
PLANNED -> REVIEW -> ACCEPTED
PLANNED -> CANCELLED
REVIEW -> REJECTED
```

An AI may create a complete instruction in `PLANNED` and move completed, validated work to
`REVIEW`. Only a human may move `REVIEW` to `ACCEPTED` or `REJECTED`. Terminal rows and their
instruction/result pairs are immutable. Corrections require a new local sequence.

`STATUS.md` must not contain provider inventories, external-result feeds, cross-library coverage,
or repository-wide planning. When a local implementation consumes another library's accepted
result, the local instruction and result record that dependency; the local status row identifies
only the resulting library-owned implementation.
