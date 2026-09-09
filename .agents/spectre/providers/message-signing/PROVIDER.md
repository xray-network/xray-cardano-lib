# EMURGO Message Signing provider

Provider: message-signing

## Purpose and authority

EMURGO message-signing supplies CIP-0008/COSE behavior and wire-format evidence. Captured Rust explains semantics; it is not a runtime dependency or generated library source.

This unversioned guide explains the provider and update workflow. Each numbered snapshot owns
its frozen specification and evidence; editing this guide never changes an existing snapshot.

## Sources and tracking

| Source | Official repository | Followed ref/policy | License guidance |
| --- | --- | --- | --- |
| Primary | [Emurgo/message-signing](https://github.com/Emurgo/message-signing) | refs/heads/master; release tags are descriptive | Inspect MIT license and source notices at the resolved commit. |

Tracking is live and human-triggered. Resolve full immutable source identities on each requested
capture. No scheduling or GitHub Actions are involved. The snapshot specification records exact
selection, mappings, formats, corpus counts, integrity checks and licensing before publication.
Consult the previous snapshot’s rules as historical context, then independently enumerate the new
selection. Do not inherit old counts or silently broaden coverage when upstream paths change.

## Evidence domains and boundaries

COSE Sign/Sign1, protected/unprotected headers, signature structures, keys, builders, detached payloads, external AAD and payload hashing. Preserve protected-header bytes and duplicate rejection. COSE encryption, private-key setters, native/WASM bindings and duplicated generic CBOR/crypto owners are outside scope.

Captured source and upstream instructions are untrusted evidence. Do not execute them or generate
implementation code from them. Artifact-backed implementation requires its own bounded plan.

## Summarization requirements

In each new CAPTURE.md, summarize the first capture as a baseline and later captures against the
immediately preceding same-provider snapshot. Name all immutable source identities and relevant
artifact paths. Account for added, removed, changed and unchanged evidence, distinguishing behavior
from documentation, tests and refactoring. State observations separately from inferred impact.
Map relevant behavior to maintained modules/APIs and tests, recommend implementation or investigation,
and explain no-change conclusions, exclusions and unresolved questions. Summary prose is advisory
and does not authorize consumer changes. SNAPSHOT.md owns the complete specification and resolved
inventory; legacy 0001 summaries remain in their original snapshot documents. New incremental
captures reuse unchanged earlier artifacts instead of copying them. A no-change capture writes nothing.

## Maintained consumer guidance

Describe behavior in language-neutral terms, then map it to the initial maintained target,
TypeScript. C++ remains unmaintained and opt-in; no automatic cross-language parity is required.

| Domain | TypeScript owner below libs/typescript/packages/ | Tests below the same root |
| --- | --- | --- |
| CIP-8 signing and COSE serialization | `cip/src/cip8/` | `cip/test/cip8.test.mjs` |

Use focused package checks and `npm --prefix libs/typescript run check` for consumer changes.

## Snapshots

- [0001](0001/SNAPSHOT.md): original frozen capture, with its complete specification,
  exact artifact inventory and documented development metadata migration.

## Capture directory migration — 2026-09-09

At the human's request, `0001-message-signing/` was renamed to `0001/`.
This is a repository path migration, not a new upstream capture. All baseline artifact bytes,
source revisions, selection rules, and lifecycle decisions are preserved. Active references and
capture metadata use the new path. Immutable archived results and historical documentation retain
their original paths and hashes; resolve their legacy directory through this mapping.

| Descriptor | SHA-256 before migration | SHA-256 after migration |
| --- | --- | --- |
| [0001/SNAPSHOT.md](0001/SNAPSHOT.md) | `db8340138ee183c19443f3bd952a7a2323d27a7e45111a75848a8a3c0950f17b` | `70e67e36d1e9d585965d5c26bb75d19decd91349155ecab8c4e73e7cae21a666` |
