# TypeScript implementation 0022 instruction

Implementation-Version: v1
Implementation-ID: typescript/0022
Created: 20260807T105137Z
Evidence-Mode: LOCAL
Depends-On: NONE
Provider-Evidence: NONE

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| Human request to restructure unreleased Cardano Lib and xray-js exports without compatibility aliases | `LOCAL` | Yes | Authorizes a breaking public-surface cleanup before publication. |
| `libs/typescript/packages/{core,crypto,chain,cip,plutus,runtime}` source and manifests | `LOCAL` | Yes | Define domain ownership, focused entry points, and the aggregate facade. |
| TypeScript package READMEs and accepted package-ownership ADR | `LOCAL` | Yes | Define public ownership, browser boundaries, and documentation contracts. |
| TypeScript package-boundary, import, browser, and packed-consumer tests | `LOCAL` | Yes | Define the required public identity and packaging evidence. |

## Objective

Normalize protocol export namespaces across the unreleased TypeScript Cardano packages.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C01` | Export every stable implemented CIP, including CIP-67, as a numerically ordered namespace from the CIP root while keeping provisional CIP-129 focused-only. | Intentionally changes the unreleased CIP root contract; focused subpaths remain canonical. | CIP source, manifest, tests, and README | Root/focused identity tests and packed import smoke. |
| `C02` | Add explicit `data`, `uplc`, and `blueprint` namespaces to the Plutus root without creating new nominal owners. | Additive within the owning package. | Plutus source, tests, and README | Identity and package-boundary tests. |
| `C03` | Replace the aggregate runtime's mixed flat surface with `core`, `crypto`, `chain`, `cips`, and `plutus` namespaces that re-export domain owners by identity. | Breaking by request; do not retain flat aggregate compatibility exports. | Aggregate runtime source, tests, and README | Imports, type identity, browser, and packed-consumer tests. |
| `C04` | Correct public documentation and export-contract coverage for every declared stable CIP and Plutus namespace. | Documentation and validation follow the new unreleased API only. | Package READMEs and runtime/CIP tests | Full TypeScript completion gate and documentation scan. |

## Implementation steps

1. Normalize CIP and Plutus owning-package barrels.
2. Reduce the aggregate runtime to domain namespaces.
3. Rewrite public-contract, browser, and packed-consumer tests for the namespace contract.
4. Update package documentation and implementation mirrors.
5. Run the complete TypeScript gate and record the result.

## Validation

- `npm --prefix libs/typescript run check`
- Import-smoke all aggregate domain namespaces and every stable CIP namespace.
- Confirm CIP-129 is absent from stable roots and remains available from its focused subpath.
- Scan the aggregate source and declarations for retired flat protocol exports.
- `git diff --check`

## Compatibility and human review

This is intentionally breaking before first publication. Review the namespace names, retained focused
entry points, nominal binding identity, and the exclusion of provisional CIP-129 from stable roots.

## Completion criteria

All stable domains are reachable through predictable namespaces, focused packages remain canonical,
no retired aggregate aliases remain, and the full TypeScript completion gate passes.

## Out of scope

- New protocol implementations, behavioral changes, dependency upgrades, or npm publication

## Blockers

None.
