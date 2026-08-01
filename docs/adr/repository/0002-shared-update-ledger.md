# ADR 0002: Shared update ledger and provider evidence

- Status: Accepted; canonical-root and lifecycle-ledger sections superseded by
  [ADR 0004](./0004-xray-updates-standard.md)
- Date: 2026-07-31
- Supersedes: the implementation-record and provider-location sections of
  [ADR 0001](./0001-repository-architecture.md)

## Context

The original repository layout placed implementation records below a library-local `impl/`
directory and provider evidence below the capturing library. In practice, provider evidence is
language-neutral and is consumed by multiple implementations. Keeping the shared evidence below
`records/typescript/providers/` incorrectly suggests TypeScript ownership.

The old library-local `impl/` level also adds no ownership information: the containing library
directory and record filenames already identify each implementation sequence. A single
`updates/implementations/` namespace instead separates all language-local lifecycle ledgers from
shared provider evidence and update workflow templates.

## Decision

### Canonical update ledger

- `updates/` is the canonical, permanent update root.
- Language-local implementation ledgers are grouped below `updates/implementations/`.
- Each library owns `updates/implementations/<language>/STATUS.md`.
- Numbered instructions and results are direct children of the library directory:
  `updates/implementations/<language>/NNNN-IMPL-INSTR.md` and
  `updates/implementations/<language>/NNNN-IMPL-RESULT.md`.
- Sequences, implementation IDs, lifecycle states, evidence modes, decision authority, and
  terminal immutability remain library-local and otherwise unchanged.
- There is no global implementation status ledger.

### Shared provider evidence

- Provider contracts and immutable snapshots live below
  `updates/providers/<provider>/`.
- Provider evidence has no owning implementation language. Provenance belongs to the provider
  contract and snapshot, while consumption belongs to each instruction and result.
- Providers do not maintain consumer lists, lifecycle statuses, implementation instructions, or
  results.
- Direct and hybrid implementations declare every consumed provider snapshot explicitly.

### Published documentation

- Mintlify mirrors remain below `docs/impl/<language>/` to preserve published documentation paths.
- Every canonical numbered instruction and result has exactly one mirror with the same filename.
- Status files, providers, artifacts, templates, and `updates/README.md` are not mirrored.

### Structural migration

The repository migration from `records/` to `updates/implementations/` and
`updates/providers/` is an architecture change, not a numbered TypeScript or C++ implementation.
It therefore creates no library result and changes no lifecycle state.

For this migration only, accepted instructions and results may receive path-only link corrections
required by the new canonical layout. Their implementation IDs, evidence, semantic requirements,
outcomes, validation claims, and decision proofs remain immutable. Files are moved with history;
no legacy `records/` alias or duplicate provider tree is retained.

## Consequences

Implementation history is visibly grouped and library-owned, while provider evidence is visibly
shared. Relative provider links are uniform across languages, and adding a library requires only
its workspace, `updates/implementations/<language>/STATUS.md`, and local numbered sequence.

The migration must update repository guidance, status links, cross-library inputs, documentation
mirrors, absolute repository URLs, and every test or integrity gate that resolves captured
evidence. Provider inventories and artifact bytes must remain unchanged.
