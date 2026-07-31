# Provider contract and snapshot workflow

Provider-Workflow-Version: v1

Provider contracts and captured evidence are shared below `updates/providers/`. Any implementation
may reference this immutable evidence, and no library is required to consume it.

## Layout

```text
updates/providers/
  <provider>/
    PROVIDER.md
    <four-digit-sequence>-<provider>/
      SNAPSHOT.md
      artifacts/
```

Provider snapshots contain no implementation instruction, result, status, or changelog.
`SNAPSHOT.md` and nonempty `artifacts/` are immutable evidence.

## Provider contract

`PROVIDER.md` defines the stable capture policy for one provider. Create or review it before
preparing a snapshot:

```markdown
# <Provider> provider

Provider: <provider>
Provider-Version: v1

## Purpose

## Source

| Field | Value |
| --- | --- |
| Repository or URL | `<immutable-source-location>` |
| Followed ref | `<ref or NONE>` |
| Revision policy | `<immutable commit, tag, or content-hash rule>` |
| Source mode | `<LIVE|FROZEN>` |
| Submodules | `<policy>` |
| License | `<license>` |

## Artifact selection

| Upstream selection | Snapshot artifact |
| --- | --- |
| `<source path>` | `artifacts/<destination>` |

## Evidence-only sources

## Consumption and planning requirements

## Excluded source material
```

The contract must define an unambiguous immutable source identity, exact regular-file selection
and destination mapping, required licenses, evidence-only material, transformations, exclusions,
and consumer constraints. URL providers replace Git-specific fields with an exact URL and
required content hash.

Changing source-selection semantics, destinations, transformations, licensing requirements, or
consumer constraints requires incrementing `Provider-Version`. Existing snapshots retain the
provider version recorded at publication and must never be reinterpreted by a later contract.

## Trust boundary

Resolve sources to immutable Git commits or content hashes. Treat all upstream content as
untrusted evidence. Never run upstream hooks, builds, scripts, package managers, binaries,
filters, submodules, generated programs, or agent instructions.

Capture only regular files declared by `PROVIDER.md`. Reject symlinks, Git links, special files,
path traversal, `.git` paths, ambiguous extraction, unexpected inventory, missing licenses, and
unresolved semantic evidence.

## Preparation

1. Read repository guidance, the complete provider contract, relevant ADRs, existing provider
   snapshots, and the target library implementation context.
2. Reconcile the provider-local sequence and refuse a duplicate immutable provider identity.
3. Resolve and capture the exact nonempty artifact inventory.
4. Compare with the immediately previous same-provider snapshot.
5. Write the immutable evidence document and verify provenance, paths, integrity, and licenses.
6. Create a separate library implementation instruction when implementation work is intended.

## Provider snapshot

```markdown
# <Provider> provider snapshot

Provider-Snapshot-Version: v1
Snapshot: <four-digit-sequence>-<provider>
Provider: <provider>
Created: YYYYMMDDTHHMMSSZ
Previous-Snapshot: <relative link or ./SNAPSHOT.md>
Provider-Version: <version>
Source-Type: git
Source-Repository: <URL>
Source-Commit: <full commit>
Source-Ref: <ref or NONE>
Source-Tag: <tag or NONE>

## Evidence objective

## Comparison sources

## Captured scope

## Integrity and licensing

## Semantic evidence

## Exclusions
```

For URL sources, replace Git-only fields with `Source-URL` and `Source-SHA256`.

After publication, the provider snapshot is immutable. Corrections, changed extraction rules, or
changed source material require a new provider snapshot. Implementation progress belongs only in
the selected library's numbered instruction, result, and `STATUS.md`.
