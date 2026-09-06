# TypeScript implementation 0013 result

Result-Version: v1
Implementation-ID: typescript/0013
Instruction: ./0013-IMPL-INSTR.md
Evidence-Mode: LOCAL

## Change dispositions

| Change ID | Disposition | Implementation | Validation |
| --- | --- | --- | --- |
| `TS13-VOID1` | Implemented | Added `VoidSchema` with static type `undefined`, included it in `DataSchema`, exposed `Data.Void()`, and encoded/decoded only constructor alternative zero with no fields. | Typecheck, focused round-trip tests, invalid JavaScript value tests, wrong-shape and malformed-CBOR tests, full built suite, and packed type consumers passed. |
| `TS13-RAW1` | Implemented | Preserved lowercase `Data.void()` unchanged as the raw `d87980` helper. | Existing raw-helper assertion and the new focused compatibility assertion passed. |
| `TS13-AGG1` | Implemented | Explicitly re-exported the Plutus-owned `Data` and `Constr` bindings and typed-data types through the runtime Plutus facade and aggregate root without adding another implementation or a conflicting star export. | Workspace identity assertions, aggregate named imports, generated declarations, packed runtime identity assertions, and NodeNext/bundler type consumers passed. |
| `TS13-TEST1` | Implemented | Added void success/failure coverage, aggregate import and identity coverage, and packed runtime/type-consumer coverage. | All 138 TypeScript tests and the packed-package suite passed. |

## Outcome

Consumers can now replace a local Plutus Data implementation with named `Data` and `Constr`
imports from `@xray-network/xray-cardano-lib`. `Data.Void()` has the static value type
`undefined`, round-trips through `d87980`, and rejects every other JavaScript value or Plutus
constructor shape. The pre-existing lowercase `Data.void()` raw helper is unchanged.

## Inputs consumed

- The current user instruction
- `docs/adr/typescript/0001-lossless-cbor-and-encoding-metadata.md`
- `docs/adr/typescript/0003-upstream-evidence-and-package-ownership.md`
- The owned Plutus typed-data source, exports, tests, and README under
  `libs/typescript/packages/plutus/`
- The owned aggregate source, tests, packed-consumer harness, and README under
  `libs/typescript/packages/runtime/`

No provider evidence or external implementation result was consumed.

## Project changes

- Added `VoidSchema` and the `"void"` schema variant.
- Added `Data.Void()` with exact `undefined` and `Constr(0, [])` cast behavior.
- Preserved `Data.void()` as the raw `d87980` helper.
- Added explicit aggregate `Data`, `Constr`, and typed-data type exports.
- Added invalid-value, invalid-CBOR, identity, named-import, generated-declaration, and packed
  consumer coverage.
- Documented typed void use in the Plutus and aggregate package READMEs.

The independent C++ implementation was audited but not changed. Its language-native
`DataSchema::constructor(0, {})` already validates the exact zero-alternative, zero-field shape;
JavaScript `undefined` static typing and npm named exports do not map to its public C++ API. Its
complete workflow passed unchanged.

## Exported change contract

| Change ID | Semantic change | Compatibility | Downstream action |
| --- | --- | --- | --- |
| `TS13-VOID1` | `Data.Void()` denotes exactly Plutus constructor alternative zero with no fields; its TypeScript value is `undefined`, and its canonical CBOR is `d87980`. | Additive. Other constructors, variants, field counts, and JavaScript values remain invalid for this schema. | Replace a local void schema with `Data.Void()` and pass `undefined` to `Data.to`. |
| `TS13-RAW1` | `Data.void()` still returns raw CBOR hex `d87980`. | Unchanged. | No migration needed for raw-helper consumers. |
| `TS13-AGG1` | The aggregate package re-exports the Plutus owner's `Data`, `Constr`, and typed-data schema types by identity. | Additive and nominally identity-preserving. | Import `Data`, `Constr`, and schema types from `@xray-network/xray-cardano-lib` when using the aggregate package. |
| `TS13-TEST1` | Packed ESM and declaration consumers verify the new aggregate API under NodeNext and bundler resolution. | Test-only. | No downstream action. |

## Validation

The following checks passed from the repository root:

```sh
npx --prefix libs/typescript tsc -b libs/typescript/tsconfig.json --pretty false
npm --prefix libs/typescript run typecheck
npm --prefix libs/typescript run lint
node --test libs/typescript/packages/plutus/test/typed_data.test.mjs \
  libs/typescript/packages/runtime/test/imports.test.mjs
npm --prefix libs/typescript run test:built
npm --prefix libs/typescript run pack:smoke
npm --prefix libs/typescript run check
git diff --check
```

- Focused typed-data and import tests: 6 passed.
- Complete built TypeScript suite: 138 passed.
- Packed suite: 526 intended files; ESM runtime plus NodeNext and bundler type consumers passed.
- Generated declarations expose `VoidSchema extends SchemaType<undefined>` and the aggregate
  schema-type exports.
- Documentation JSON parsing, implementation mirror equality, and canonical/mirror inventory
  checks passed.

The C++ compatibility audit used the repository's cached pinned toolchain:

```sh
env PATH=/private/tmp/cardano-build-tools/bin:/usr/bin:/bin:/usr/sbin:/sbin \
  VCPKG_ROOT=/private/tmp/cardano-vcpkg \
  /private/tmp/cardano-build-tools/bin/cmake --workflow --preset ci --fresh
```

Configuration, formatting, build, provider integrity, installation, aggregate consumption, and
all 141 C++ tests passed.

## Deviations from instruction

The TypeScript workspace has no formatter script or formatter dependency. Formatting was checked
with `git diff --check`, the existing TypeScript lint gate, and the C++ workflow's repository
format check; all passed. No source-format rewrite was required.

The first C++ workflow attempt encountered a stale CMake cache created under the repository's
former `cardano-lib` path. The documented `--fresh` option regenerated the cache under
`xray-cardano-lib`; the prescribed configure, build, and test workflow then passed unchanged.

## Remaining human review

Confirm the exact void-constructor acceptance rule, the `undefined` declaration type, and the
identity-preserving aggregate exports, then decide whether this result should move from `REVIEW`
to `ACCEPTED`.

## Reproducibility

From the repository root with Node.js 20.19 or newer and locked dependencies installed, run
`npm --prefix libs/typescript run check`. For C++ compatibility, provide the pinned vcpkg and
documented CMake/Ninja toolchain, then run `cmake --workflow --preset ci` from `libs/cpp/`.
