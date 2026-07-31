# Cardano Lib updates

This directory is the permanent canonical ledger for implementation instructions, results,
library lifecycle state, and shared provider evidence.

## Layout

```text
updates/
  README.md
  TEMPLATE_IMPL.md
  TEMPLATE_PROVIDER.md
  TEMPLATE_STATUS.md
  implementations/
    <library>/
      STATUS.md
      <four-digit-sequence>-IMPL-INSTR.md
      <four-digit-sequence>-IMPL-RESULT.md
  providers/
    <provider>/
      PROVIDER.md
      <provider-snapshot>/
        SNAPSHOT.md
        artifacts/
```

Instruction and result sequences are independent for each library. The files live under the
library's directory below [`implementations/`](./implementations/) and use the same four-digit
sequence.

## Authority

- `NNNN-IMPL-INSTR.md` defines one bounded implementation objective, its inputs, required changes,
  compatibility constraints, and validation.
- `NNNN-IMPL-RESULT.md` records actual inputs, change dispositions, project changes, exported
  semantic changes, validation, deviations, and reproducibility.
- Each library's `STATUS.md` is the only lifecycle authority for that library and follows
  [`TEMPLATE_STATUS.md`](./TEMPLATE_STATUS.md).
- An implementation result may be consumed by another library as a portable change contract.
- A library may instead consume provider evidence directly, combine provider evidence with prior
  results, or perform a local-only implementation.

Provider contracts and captured artifacts are shared below [`providers/`](./providers/). Their
contracts and snapshots record selection and provenance without assigning evidence to a language.
Any implementation may reference a provider snapshot, and no library is required to use provider
evidence. Shared placement does not broaden a snapshot's captured scope, provider version, or
consumer constraints.

## Evidence modes

- `DIRECT`: provider snapshots or artifacts are normative inputs.
- `DERIVED`: accepted implementation results are normative inputs.
- `HYBRID`: both provider evidence and implementation results are normative inputs.
- `LOCAL`: no provider evidence or external implementation result is required.

Read [`TEMPLATE_IMPL.md`](./TEMPLATE_IMPL.md) before preparing or implementing a numbered record.
Read [`TEMPLATE_STATUS.md`](./TEMPLATE_STATUS.md) before creating or updating a library status.
Read [`TEMPLATE_PROVIDER.md`](./TEMPLATE_PROVIDER.md) and the selected provider
contract before capturing or consuming provider evidence.

Numbered implementation instructions and results are mirrored under `docs/impl/<library>/` for
Mintlify. Files below `updates/implementations/` remain canonical; documentation mirrors must
never be edited independently.
