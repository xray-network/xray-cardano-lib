# TypeScript implementation 0012 result

Result-Version: v1
Implementation-ID: typescript/0012
Instruction: ./0012-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `TS12-PKG1` | Implemented | Renamed the aggregate runtime manifest, workspace path mapping, npm lockfile entry, imports, and packed-consumer fixtures to `@xray-network/xray-cardano-lib`. | TypeScript build, all runtime tests, and packed NodeNext/bundler consumers passed. |
| `TS12-DOC1` | Implemented | Updated maintained TypeScript package headings, descriptions, examples, and test output to XRAY Cardano Lib. | Maintained-name scan and packed package checks passed. |
| `TS12-ID1` | Implemented | Left all five Cardano domain package identities and their ownership boundaries unchanged. | Package-boundary, browser-package, lockfile-integrity, and full workspace tests passed. |

## Outcome

The TypeScript aggregate runtime is now published and consumed locally as
`@xray-network/xray-cardano-lib`. Its public exports and domain-package dependencies are unchanged;
only the aggregate package identity and current project branding changed.

## Inputs consumed

- `docs/adr/repository/0003-project-identity.md`
- The owned TypeScript manifests, lockfile, path mappings, package READMEs, runtime tests, and
  packed-consumer fixtures under `libs/typescript/`

No provider evidence or external implementation result was consumed.

## Project changes

- Renamed the runtime package and workspace link in `package.json` and `package-lock.json`.
- Renamed the TypeScript path alias and every maintained runtime import/example.
- Updated packed-consumer package names, fixture values, and success output.
- Updated maintained TypeScript descriptions and documentation to XRAY Cardano Lib.
- Renamed the extended hardening environment variable to
  `XRAY_CARDANO_LIB_HARDENING_CASES`.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `TS12-PKG1` | The aggregate TypeScript runtime package identity is `@xray-network/xray-cardano-lib`. | Imports and dependency declarations using `@xray-network/cardano-lib` no longer resolve to this workspace package. | Replace the old aggregate dependency and import specifier with `@xray-network/xray-cardano-lib`. |
| `TS12-DOC1` | The maintained TypeScript distribution display name is XRAY Cardano Lib. | No runtime behavior change. | Use the new display name in current integrations and documentation. |
| `TS12-ID1` | Cardano domain package identities and nominal owners are unchanged. | Existing direct domain-package consumers remain compatible. | No action for direct domain-package imports. |

## Validation

The required completion command passed:

```sh
npm --prefix libs/typescript run check
```

It completed the clean TypeScript build, all 137 tests, and packed ESM smoke tests for NodeNext and
bundler consumers. The packed runtime used `@xray-network/xray-cardano-lib` and contained 526
intended files.

## Deviations from instruction

None.

## Remaining human review

Confirm the intentional breaking npm package rename and decide whether the result should move from
`REVIEW` to `ACCEPTED`. Registry publication, deprecation, or forwarding-package work remains
outside this implementation.

## Reproducibility

From the repository root with Node.js 20.19 or newer and the locked npm dependencies installed,
run `npm --prefix libs/typescript run check`.
