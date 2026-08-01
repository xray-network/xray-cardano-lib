# ADR 0004: XRAY Updates v1 adoption

- Status: Accepted
- Date: 2026-08-01
- Supersedes: the canonical-root, template-location, and lifecycle-ledger sections of
  [ADR 0002](./0002-shared-update-ledger.md)

## Context

The repository developed its evidence-backed implementation workflow below `updates/` before the
workflow became the portable XRAY Updates standard. XRAY Updates v1 standardizes the bootstrap
document, installation root, target terminology, lifecycle permissions, templates, provider trust
boundary, and validation invariants for use across unrelated projects.

Adopting the standard must preserve this repository's existing implementation history, human
decisions, provider evidence, language ownership, and Mintlify integration. Installation is a
tracking-structure migration, not a TypeScript or C++ implementation.

## Decision

- The repository adopts `.xray/updates/XRAY-UPDATES.md` with `Standard-ID: xray/updates` and
  `Standard-Version: 1.0.0`.
- `.xray/updates/` is the canonical tracking root.
- The existing `typescript` and `cpp` libraries remain the implementation targets because each
  already has independent source ownership, validation, status, and numbering.
- Nested monorepo storage is used. The reserved `repo` target owns repository-wide XRAY governance
  and no product implementation work.
- Existing implementation directories and provider evidence move from `updates/` to
  `.xray/updates/`. The required accepted bootstrap record is `repo/0001`; no TypeScript or C++
  implementation is created for installation.
- The bootstrap record's `Created` value is `20260723T122735Z`, matching the earliest existing
  implementation timestamp as directed by the current human.
- Terminal instruction and result content and provider snapshot bytes remain unchanged. Historical
  plain-text references to the former root remain part of those immutable records.
- `.xray/updates/XRAY-UPDATES-STATUS.md` is the sole aggregate lifecycle ledger. Its ordered target
  sections preserve every existing ID, state, result link, evidence mode, and decision proof while
  adding the required objective Title column. Target-local status files are retired.
- The three installed templates below `.xray/updates/templates/` use the canonical XRAY Updates v1
  content. Repository-specific
  ownership, completion checks, and Mintlify mirror rules remain stricter instructions in
  `AGENTS.md` and `CONTRIBUTING.md`.
- Published Mintlify mirror links are corrected to the new canonical root as part of the structural
  migration. The mirrors remain noncanonical.
- The root `AGENTS.md` contains the idempotent XRAY developer standards pointer.

## Consequences

Future tracked work follows the locally pinned standard and uses the applicable section in the
aggregate status plus `.xray/updates/implementations/<target>/`. Existing sequences and lifecycle
states continue without renumbering. Provider contracts and snapshots remain shared below
`.xray/updates/providers/`.

The retired `updates/` root is not retained as an alias, avoiding two apparent sources of truth.
Documentation mirroring remains a repository integration rather than part of the XRAY Updates core
layout.
