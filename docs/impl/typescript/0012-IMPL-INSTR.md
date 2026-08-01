# TypeScript implementation 0012 instruction

Implementation-Version: v1
Implementation-ID: typescript/0012
Created: 20260731T120245Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| `docs/adr/repository/0003-project-identity.md` | `LOCAL` | Yes | Authoritative XRAY Cardano Lib display, repository, and package-family identities |
| `docs/adr/typescript/0003-upstream-evidence-and-package-ownership.md` | `LOCAL` | Yes | Authoritative TypeScript package ownership and published package-family identities |
| `libs/typescript/` | `LOCAL` | Yes | Owned package metadata, import mappings, documentation, lockfile, source, and tests |
| `updates/implementations/typescript/0005-IMPL-INSTR.md` through `0011-IMPL-INSTR.md` | `LOCAL` | Yes | Nonterminal planned work whose package references must use the current identities |

## Objective

Rename the TypeScript package family and its current branding from Cardano Lib to XRAY Cardano
Lib while preserving the independently owned Cardano protocol domains.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `TS12-PKG1` | Name the aggregate runtime `@xray-network/xray-cardano-lib` and every domain package `@xray-network/xray-cardano-lib-*` (`core`, `crypto`, `chain`, `cip`, and `plutus`) throughout manifests, the workspace lockfile, TypeScript path mappings, source imports, tests, and examples. | Intentional package-name break; no compatibility aliases are retained. | `libs/typescript/packages/`, workspace configuration, and lockfile | Build, imports, packed consumers, and complete workspace gate |
| `TS12-DOC1` | Use XRAY Cardano Lib and the current package-family identities in maintained TypeScript package descriptions, documentation, active ADRs, and nonterminal planned work. | Documentation and planning-reference update. | `libs/typescript/`, `docs/adr/`, and nonterminal TypeScript instructions | Repository name scan and package smoke tests |
| `TS12-ID1` | Preserve each domain package's public bindings, nominal ownership, and dependency direction while applying the XRAY package-family names. | Package specifiers change, but public domain ownership and runtime behavior remain unchanged. | TypeScript workspace | Package-boundary tests, API tests, and complete workspace gate |

## Implementation steps

1. Update every package manifest, workspace path alias, and npm lockfile entry to the XRAY package
   family, keeping the aggregate runtime name unsuffixed.
2. Update source imports, tests, packed-consumer fixtures, and TypeScript examples to import the
   renamed packages.
3. Update maintained TypeScript READMEs, package-description branding, active identity ADRs, and
   nonterminal planned instructions while preserving domain ownership and public symbols.
4. Run the complete TypeScript validation command.

## Validation

Run:

```sh
npm --prefix libs/typescript run check
```

Also scan maintained TypeScript source, manifests, documentation, and tests for the retired
`@xray-network/cardano-*` and `@xray-network/cardano-lib` package names.

## Compatibility and human review

This is an intentional breaking package-family rename requested by the project owner. Reviewers
must confirm that no compatibility packages or aliases are required and that domain ownership and
public bindings remain unchanged.

## Completion criteria

- The aggregate npm identity is `@xray-network/xray-cardano-lib`, and each domain npm identity is
  `@xray-network/xray-cardano-lib-*`, everywhere in maintained TypeScript configuration,
  documentation, source, and tests.
- The workspace lockfile resolves all six packages under the new identities.
- The complete TypeScript validation gate passes.

## Out of scope

- Renaming Cardano protocol concepts, public TypeScript symbols, or domain ownership boundaries.
- Rewriting terminal implementation records, provider snapshots, captured artifacts, or fixed
  compatibility vectors.
- Publishing or deprecating packages in the npm registry.

## Blockers

None.
