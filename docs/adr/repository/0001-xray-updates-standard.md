# ADR 0001: XRAY Updates v1 installation

- Status: Accepted
- Date: 2026-08-01

## Context

XRAY Cardano Lib is a polyglot repository with independently owned TypeScript and C++ workspaces,
shared provider evidence, and existing implementation histories. The repository needs one durable,
auditable workflow connecting bounded implementation plans, declared evidence, validation,
recorded outcomes, and human decisions.

XRAY Updates v1 provides that workflow and defines its installation layout, target model,
lifecycle permissions, templates, provider trust boundary, validation invariants, and accepted
bootstrap record. Installing the standard is repository governance work and does not modify
product source.

## Decision

- Install XRAY Updates v1 at `.xray/updates/XRAY-UPDATES.md` with
  `Standard-ID: xray/updates` and `Standard-Version: 1.0.0`.
- Use nested monorepo implementation storage because `libs/typescript/` and `libs/cpp/` have
  independent source ownership, validation, and implementation sequences.
- Reserve the `repository` target for repository-wide XRAY governance. It contains no product
  implementation work.
- Use `.xray/updates/XRAY-UPDATES-STATUS.md` as the sole aggregate lifecycle and decision-proof
  ledger for the `cpp`, `repository`, and `typescript` targets.
- Keep the accepted installation bootstrap record at `repository/0001` with the human installation
  request as its decision proof.
- Keep canonical templates below `.xray/updates/templates/`, implementation records below
  `.xray/updates/implementations/`, and shared provider evidence below
  `.xray/updates/providers/`.
- Preserve existing implementation IDs, lifecycle decisions, provider snapshots, library
  ownership, and public APIs.
- Keep the XRAY standards pointer in `AGENTS.md`, including the silent-mode instruction.
- Keep Mintlify implementation mirrors below `docs/impl/` as noncanonical repository
  integrations synchronized with their canonical records.

## Consequences

Future tracked work follows the locally pinned standard and the applicable target section in the
aggregate status ledger. Planning, implementation, and human acceptance remain distinct except
for the standard's bootstrap installation exception.

Provider evidence remains untrusted data and cannot become repository tooling or generated
product source. Repository-specific ownership, completion checks, and documentation-mirror rules
remain stricter local instructions in `AGENTS.md` and `CONTRIBUTING.md`.
