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
| `docs/adr/repository/0003-project-identity.md` | `LOCAL` | Yes | Authoritative XRAY Cardano Lib display, repository, and aggregate package identities |
| `libs/typescript/` | `LOCAL` | Yes | Owned aggregate runtime metadata, import mapping, documentation, lockfile, and tests |

## Objective

Rename the TypeScript aggregate runtime and its current branding from Cardano Lib to XRAY Cardano
Lib without changing the independently owned Cardano domain package identities.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `TS12-PKG1` | Rename the aggregate package to `@xray-network/xray-cardano-lib` in its manifest, workspace lockfile, TypeScript path mapping, tests, and examples. | Intentional package-name break; no compatibility alias is retained. | `libs/typescript/packages/runtime/`, workspace configuration, and lockfile | Build, imports, packed consumers, and complete workspace gate |
| `TS12-DOC1` | Use XRAY Cardano Lib in maintained TypeScript package descriptions and documentation. | Documentation-only display-name change. | `libs/typescript/` | Repository name scan and package smoke tests |
| `TS12-ID1` | Keep `@xray-network/cardano-core`, `cardano-crypto`, `cardano-chain`, `cardano-cip`, and `cardano-plutus` unchanged. | Preserves domain-package ownership and imports. | TypeScript workspace | Lockfile inspection, package-boundary tests, and complete workspace gate |

## Implementation steps

1. Update the runtime package manifest, workspace path aliases, and npm lockfile to the new
   aggregate package identity.
2. Update runtime tests, packed-consumer fixtures, and TypeScript examples to import the renamed
   package.
3. Update maintained TypeScript README and package-description branding while preserving domain
   package names.
4. Run the complete TypeScript validation command.

## Validation

Run:

```sh
npm --prefix libs/typescript run check
```

Also scan maintained TypeScript source, manifests, documentation, and tests for the retired
aggregate package name.

## Compatibility and human review

This is an intentional breaking package rename requested by the project owner. Reviewers must
confirm that no compatibility package or alias is required and that domain package identities
remain unchanged.

## Completion criteria

- The aggregate npm identity is `@xray-network/xray-cardano-lib` everywhere in maintained
  TypeScript configuration, documentation, and tests.
- The workspace lockfile resolves the runtime under the new package identity.
- The complete TypeScript validation gate passes.

## Out of scope

- Renaming Cardano protocol domain packages or public TypeScript symbols.
- Rewriting terminal implementation records, provider snapshots, captured artifacts, or fixed
  compatibility vectors.
- Publishing or deprecating packages in the npm registry.

## Blockers

None.
