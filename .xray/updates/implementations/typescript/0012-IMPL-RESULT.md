# TypeScript implementation 0012 result

Result-Version: v1
Implementation-ID: typescript/0012
Instruction: ./0012-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `TS12-PKG1` | Implemented | Named the aggregate runtime `@xray-network/xray-cardano-lib` and renamed the five domain manifests, dependencies, workspace path mappings, lockfile entries, source imports, tests, documentation, and packed-consumer fixtures to the matching `@xray-network/xray-cardano-lib-*` identities. | TypeScript build, package-boundary tests, all runtime tests, and packed NodeNext/bundler consumers passed. |
| `TS12-DOC1` | Implemented | Updated maintained TypeScript package headings, descriptions, examples, test output, active identity ADRs, and nonterminal planned instructions to XRAY Cardano Lib and its current package-family identities. | Maintained-name scan, mirror comparison, and packed package checks passed. |
| `TS12-ID1` | Implemented | Preserved all five Cardano domains' public bindings, nominal owners, package dependency direction, and runtime behavior while renaming their package specifiers. | Package-boundary, API, browser-package, lockfile-integrity, and full workspace tests passed. |

## Outcome

The TypeScript packages are now published and consumed locally as one XRAY package family. The
aggregate runtime is `@xray-network/xray-cardano-lib`; the domain packages append `-core`,
`-crypto`, `-chain`, `-cip`, and `-plutus`. Public exports, domain ownership, and runtime behavior
are unchanged.

## Inputs consumed

- `docs/adr/repository/0003-project-identity.md`
- `docs/adr/typescript/0003-upstream-evidence-and-package-ownership.md`
- The owned TypeScript manifests, lockfile, path mappings, package READMEs, runtime tests, and
  packed-consumer fixtures under `libs/typescript/`
- Nonterminal planned TypeScript instructions `0005` through `0011`

No provider evidence or external implementation result was consumed.

## Project changes

- Renamed the root workspace and all six published package manifests and workspace links in
  `package.json` and `package-lock.json`.
- Renamed the TypeScript path aliases and every maintained source, test, and example import.
- Updated packed-consumer package names, fixture values, and success output.
- Updated maintained TypeScript descriptions, active identity ADRs, and nonterminal planned work
  to XRAY Cardano Lib and the current package-family identities.
- Renamed the extended hardening environment variable to
  `XRAY_CARDANO_LIB_HARDENING_CASES`.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `TS12-PKG1` | The aggregate TypeScript runtime package is `@xray-network/xray-cardano-lib`; its five domain packages are `@xray-network/xray-cardano-lib-core`, `-crypto`, `-chain`, `-cip`, and `-plutus`. | Imports and dependency declarations using the former `@xray-network/cardano-*` package names or `@xray-network/cardano-lib` no longer resolve to this workspace package family. | Replace each old dependency and import specifier with its `@xray-network/xray-cardano-lib` family equivalent. |
| `TS12-DOC1` | The maintained TypeScript distribution display name is XRAY Cardano Lib. | No runtime behavior change. | Use the new display name in current integrations and documentation. |
| `TS12-ID1` | Cardano domain public bindings and nominal owners are unchanged. | Runtime behavior remains compatible after updating package specifiers. | Update direct domain-package imports without changing use of their public bindings. |

## Validation

The required completion command passed:

```sh
npm --prefix libs/typescript run check
```

It completed the clean TypeScript build, all 137 tests, and packed ESM smoke tests for NodeNext and
bundler consumers. The packed artifacts used the `@xray-network/xray-cardano-lib` package family;
the runtime contained 526 intended files.

## Deviations from instruction

None.

## Remaining human review

Confirm the intentional breaking npm package-family rename and decide whether the result should
move from `REVIEW` to `ACCEPTED`. Registry publication, deprecation, or forwarding-package work
remains outside this implementation.

## Reproducibility

From the repository root with Node.js 20.19 or newer and the locked npm dependencies installed,
run `npm --prefix libs/typescript run check`.
