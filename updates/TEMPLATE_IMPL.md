# Implementation instruction and result workflow

Implementation-Workflow-Version: v1

This is the authoritative workflow for library implementation records.

## Fixed layout

```text
updates/implementations/<library>/
  STATUS.md
  0001-IMPL-INSTR.md
  0001-IMPL-RESULT.md
  0002-IMPL-INSTR.md
  0002-IMPL-RESULT.md
```

Sequences are four digits, start at `0001`, increase by one, and are local to the library.
Instructions and results are direct children of the library's implementation directory.

## Evidence modes

Every instruction declares exactly one mode:

- `DIRECT`: consume one or more provider snapshots or artifacts.
- `DERIVED`: consume one or more accepted implementation results.
- `HYBRID`: consume both provider evidence and accepted implementation results.
- `LOCAL`: consume only repository requirements and owned source.

Inputs are explicit. A library is never required to consume provider artifacts merely because
another library did. An accepted result is a semantic change contract, not authorization to copy
another language's source or nominal types.

Provider inputs reference the shared `updates/providers/` tree. Provider contracts and snapshots
record capture and provenance; consuming instructions and results remain library-owned.

## Instruction document

```markdown
# <Library> implementation <NNNN> instruction

Implementation-Version: v1
Implementation-ID: <library>/<NNNN>
Created: YYYYMMDDTHHMMSSZ
Evidence-Mode: <DIRECT|DERIVED|HYBRID|LOCAL>
Depends-On: <result links or NONE>
Provider-Evidence: <snapshot links or NONE>

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| `<path>` | `PROVIDER` | Yes | Exact purpose |

## Objective

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |

## Implementation steps

## Validation

## Compatibility and human review

## Completion criteria

## Out of scope

## Blockers

None.
```

Input kinds are `PROVIDER`, `IMPLEMENTATION_RESULT`, and `LOCAL`. Change IDs are unique within the
instruction. The instruction must be implementation-ready: unresolved source selection, semantic
mapping, ownership, compatibility, or validation design is a blocker.

## Result document

Create a result only after implementation and required validation:

```markdown
# <Library> implementation <NNNN> result

Result-Version: v1
Implementation-ID: <library>/<NNNN>
Instruction: ./<NNNN>-IMPL-INSTR.md
Evidence-Mode: <DIRECT|DERIVED|HYBRID|LOCAL>

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |

## Outcome

## Inputs consumed

## Project changes

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |

## Validation

## Deviations from instruction

## Remaining human review

## Reproducibility
```

Every required instruction change has one result disposition. The exported change contract must
be language-neutral enough for another library to evaluate without reading provider artifacts.
The result names every input actually consumed and every deviation from the instruction.

## Per-library status

Every library status follows [`TEMPLATE_STATUS.md`](./TEMPLATE_STATUS.md). That template is the
single authority for status metadata, section order, table columns, lifecycle states, decision
proof, and empty sections.

## Preparation

1. Read repository guidance, relevant ADRs, this workflow, the target library README, manifest,
   source, and tests.
2. Reconcile the library's sequence, status ledger, and declared dependencies.
3. Select the evidence mode and resolve every input to an immutable provider snapshot, accepted
   implementation result, or exact local requirement.
4. Create the complete instruction and add its `PLANNED` status row without changing library
   source.

## Implementation

1. Require one matching `PLANNED` row, a complete instruction, and all declared inputs.
2. Consume only the declared inputs. Do not silently fetch, refresh, substitute, or broaden them.
3. Implement every required change and run targeted plus library completion validation.
4. Create the result, reconcile every change disposition and actual input, then move the status
   row to `REVIEW`.
