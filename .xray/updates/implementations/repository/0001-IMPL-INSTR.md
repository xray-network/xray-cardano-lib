# Repository implementation 0001 instruction

Implementation-Version: v1
Implementation-ID: repository/0001
Created: 20260723T122735Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [XRAY Updates v1](../../XRAY-UPDATES.md) | `LOCAL` | Yes | Canonical installation, storage, lifecycle, template, and validation requirements. |
| [Repository instructions](../../../../AGENTS.md) | `LOCAL` | Yes | Repository ownership, documentation-mirror, provider, and validation constraints. |
| [Repository architecture](../../../../README.md) | `LOCAL` | Yes | Evidence that independently owned TypeScript and C++ workspaces require monorepo storage. |
| [XRAY Updates adoption decision](../../../../docs/adr/repository/0001-xray-updates-standard.md) | `LOCAL` | Yes | Repository decision to adopt the standard while preserving existing records and evidence. |
| Human installation request | `LOCAL` | Yes | Explicit authority to install XRAY Updates and accept this bootstrap record. |

## Objective

Install and validate the XRAY Updates v1 tracking structure in monorepo mode, preserve the existing
TypeScript and C++ implementation histories and provider evidence, and make no product-source
changes.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C01` | Install the pinned standard, canonical templates, tracking README, and required `AGENTS.md` pointer at their v1 paths. | Preserve all unrelated repository instructions and use the exact downloaded standard and canonical template fields. | `.xray/updates/`, `AGENTS.md` | Compare the installed standard with the canonical download; inspect template locations and the pointer. |
| `C02` | Select nested monorepo storage, consolidate target-local statuses into the sole aggregate ledger, and create the reserved `repository/0001` bootstrap record. | Preserve every existing TypeScript and C++ ID, state, result link, evidence mode, and decision proof; do not create another product implementation. | `.xray/updates/XRAY-UPDATES-STATUS.md`, `.xray/updates/implementations/` | Validate target ordering, IDs, links, state/result rules, record metadata, and absence of target-local statuses or flat records. |
| `C03` | Synchronize repository governance and the existing Mintlify implementation-mirror integration with the installed layout. | Preserve canonical ownership and provider evidence; documentation mirrors remain noncanonical and product source remains unchanged. | `README.md`, `CONTRIBUTING.md`, `docs/`, `.gitattributes` | Resolve repository-relative links, compare canonical records with mirrors, validate navigation, and inspect the scoped diff. |

## Implementation steps

1. Install the exact standard at `.xray/updates/XRAY-UPDATES.md` and place the three canonical
   templates below `.xray/updates/templates/`.
2. Preserve the nested `cpp` and `typescript` sequences, add reserved target `repository`, and consolidate
   lifecycle rows into `.xray/updates/XRAY-UPDATES-STATUS.md` in target-slug order.
3. Remove obsolete target-local status files and obsolete root-level template and standard paths.
4. Update active repository guidance and Mintlify integration for the aggregate ledger and template
   paths without changing product source.
5. Validate the complete installation, then create the matching result and accepted bootstrap row.

## Validation

- Verify that `.xray/updates/XRAY-UPDATES.md` is byte-identical to the canonical v1 download.
- Verify that exactly three canonical templates exist, all below `.xray/updates/templates/`.
- Verify aggregate target ordering, ledger-to-record links, IDs, titles, states, results, evidence
  modes, decision proofs, change dispositions, and accepted derived dependencies.
- Verify that no target-local `STATUS.md`, flat implementation record, or forbidden provider record
  exists.
- Verify repository-relative Markdown links, Mintlify mirror coverage, and `docs/docs.json` syntax.
- Verify existing provider artifact inventories and SHA-256 manifests.
- Review the scoped installation diff and confirm that this update changes no product source.

## Compatibility and human review

The installation changes governance and tracking structure only. Existing terminal decisions,
implementation semantics, provider bytes, library ownership, and public APIs remain unchanged. The
human installation request is the explicit acceptance decision for this bootstrap record only.

## Completion criteria

- The repository has one valid monorepo XRAY Updates v1 installation under `.xray/updates/`.
- The aggregate ledger contains ordered `cpp`, `repository`, and `typescript` sections and exactly one
  accepted installation row at `repository/0001`.
- Every existing target record remains represented with matching state and evidence metadata.
- All required structure, link, mirror, provider-integrity, and metadata checks pass.
- No product source is changed by this standard-structure update.

## Out of scope

- Creating or implementing any TypeScript or C++ product plan
- Changing an existing implementation lifecycle decision
- Fetching, refreshing, or rewriting provider evidence
- Changing library behavior, APIs, manifests, dependencies, or tests

## Blockers

None.
