# ADR 0001: Repository architecture and implementation records

- Status: Accepted; implementation-record and provider-location sections superseded by
  [ADR 0002](./0002-shared-update-ledger.md)
- Date: 2026-07-30

## Context

Cardano Lib is a polyglot repository. Each language needs independent source and build ownership,
while protocol evidence and implementation history must remain reviewable across languages.

The repository therefore separates executable library workspaces, canonical implementation
records, captured provider evidence, and published documentation.

## Decision

### Language workspaces

- Each language owns an independent workspace under `libs/<language>/`.
- A language workspace owns its manifests, lockfiles, packages, source, tests, README, and
  completion command.
- There is no root package-manager manifest, language registry, or command proxy.
- Public types and package dependencies remain owned by the language implementation and its
  language-specific ADRs.

### Implementation records

- Each library owns canonical records under `records/<language>/`.
- `STATUS.md` contains only implementation records owned by that library and follows
  `records/TEMPLATE_STATUS.md`.
- Each bounded implementation uses a four-digit pair under `impl/`:
  `NNNN-IMPL-INSTR.md` and `NNNN-IMPL-RESULT.md`.
- Instructions and results follow `records/TEMPLATE_IMPL.md`.
- Every instruction declares one evidence mode:
  - `DIRECT` for provider evidence;
  - `DERIVED` for accepted results from another library;
  - `HYBRID` for both;
  - `LOCAL` for repository and library-local requirements only.
- Results record actual inputs, change dispositions, implementation changes, validation,
  deviations, and a language-neutral exported change contract.
- An accepted result from another library appears only as a declared input in the local
  instruction that consumes it and as a consumed input in the paired result.

The implementation lifecycle is:

```text
PLANNED -> REVIEW -> ACCEPTED
PLANNED -> CANCELLED
REVIEW -> REJECTED
```

An AI may create `PLANNED` work and move completed, validated work to `REVIEW`. Only a human may
move work to `ACCEPTED` or `REJECTED`. Terminal status rows and their instruction/result pairs are
immutable; corrections require a new library-local sequence.

There is no global lifecycle ledger. Each library's `STATUS.md` is its only lifecycle and
decision-proof authority.

### Provider evidence

- Any library may own provider contracts and captured evidence below
  `records/<language>/providers/`.
- Provider ownership records provenance and maintenance responsibility; it does not restrict
  which implementations may consume the evidence.
- Provider contracts and snapshots follow `records/TEMPLATE_PROVIDER.md`.
- A provider snapshot contains immutable `SNAPSHOT.md` and nonempty `artifacts/`.
- Provider snapshots contain no implementation status, instruction, or result.
- Captured artifacts and upstream source are untrusted evidence. They are never repository
  tooling, generated implementation source, or runtime dependencies.
- Direct and hybrid implementations consume only declared captured evidence without fetching,
  refreshing, executing, or substituting upstream material.

### Published documentation

- Mintlify source lives under `docs/`, with navigation in `docs/docs.json`.
- Repository-wide ADRs live below `docs/adr/repository/`; language-specific ADRs live below
  `docs/adr/<language>/`.
- Canonical implementation instructions and results are mirrored from
  `records/<language>/impl/` to `docs/impl/<language>/`.
- Documentation mirrors are never edited independently. They preserve canonical text except for
  deterministic links to non-mirrored canonical records.
- Status files, provider documents, provider artifacts, record templates, and `records/README.md`
  are not mirrored.

## Consequences

Language implementations can evolve independently while sharing immutable evidence and portable
semantic results. Adding a language creates a new workspace, local status ledger, and numbered
implementation sequence without rewriting another library's records.

Provider evidence remains separate from implementation decisions, and Mintlify can publish full
instruction and result text without becoming a second source of truth.
