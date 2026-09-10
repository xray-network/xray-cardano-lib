# TypeScript implementation 0009 instruction

Implementation-Version: v1
Implementation-ID: typescript/0009
Created: 20260731T075521Z
Evidence-Mode: HYBRID
Depends-On: ./0003-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`typescript/0003`](./0003-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted typed Data, UPLC, serialized-script, and Plutus V1/V2/V3 ownership |
| [`0001-cardano-cips`](../../providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md) | `PROVIDER` | Yes | Immutable CIP-0057 specification and complete captured meta-schema set |
| Captured `CIP-0057/README.md` and `schemas/README.md` | `PROVIDER` | Yes | Document structure, vocabulary, validation semantics, and schema inventory |
| Captured five `CIP-0057/schemas/*.json` files | `PROVIDER` | Yes | Exact blueprint, argument, parameter, Data, and builtin schema constraints |
| `libs/typescript/packages/plutus/` and runtime facade | `LOCAL` | Yes | Existing package, Data, script codec, hash, export, and security boundaries |

## Objective

Add a bounded CIP-0057 blueprint parser and Plutus-value validator under the existing Plutus
package, without importing a generic JSON Schema engine, resolving external resources, generating
code, or duplicating typed Data/UPLC owners.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C001` | Parse and validate complete CIP-0057 blueprint documents | Additive focused API | `libs/typescript/packages/plutus/src/blueprint/` | Captured meta-schemas, examples, malformed documents, bounds |
| `C002` | Validate accepted typed Data and parameter constants against the CIP-0057 vocabulary | Additive; reuses existing Data/UPLC identities | `libs/typescript/packages/plutus/src/blueprint/` | Primitive, applicator, reference, recursion, and keyword matrix |
| `C003` | Publish focused/root/runtime identity-preserving exports | Additive exports | Plutus manifest/barrels, runtime Plutus facade/tests, READMEs | Imports, identity, browser, and packed consumers |

## Public contract

- Export `parse_plutus_blueprint(json: string): PlutusBlueprint`,
  `validate_plutus_blueprint(value: unknown): PlutusBlueprint`, and
  `validate_blueprint_value(schema, value): readonly BlueprintViolation[]`.
- `PlutusBlueprint`, its validator/argument/schema views, and `BlueprintViolation`
  (`code`, JSON-pointer `path`, `message`) are deeply immutable and return defensive byte/list/map
  views. A successful parse returns the normalized typed view; it does not rewrite source JSON.
- `validate_blueprint_value` accepts the existing typed `PlutusData` binding for Data schemas and
  the existing UPLC constant representation for parameter-only builtin schemas.
- Own the API at `@xray-network/xray-cardano-lib-plutus/blueprint`; export namespace `blueprint` from the
  Plutus root and the exact same bindings through the runtime Plutus facade and aggregate runtime.

## Required document semantics

- Require `preamble` and `validators`. Preamble requires string `title`, string `version`, and
  `plutusVersion` `v1|v2|v3`; optional description/compiler/license follow the captured
  additional-property rules. Each validator requires title and redeemer; datum, parameters,
  description, compiled code, and hash remain optional under the captured rules.
- Arguments/parameters require `schema`. Purposes are `spend|mint|withdraw|publish`; `oneOf`
  purpose branches must be nonempty, contain no duplicate purpose, and argument alternatives must
  have pairwise non-overlapping purpose sets.
- Datum and redeemer schemas may use Data types only. Parameters may additionally use `#unit`,
  `#boolean`, `#integer`, `#bytes`, `#string`, `#pair`, and `#list`.
- Validate base16 `compiledCode`; when it is present, require a 56-hex-character hash and verify
  Blake2b-224 over the serialized script with the `v1=1`, `v2=2`, or `v3=3` language prefix from
  the preamble. A hash without compiled code is accepted after shape validation.
- `$schema`, `$id`, and `$vocabulary` are metadata. If `$schema` is present, accept only the
  captured CIP-0057 meta-schema identifier; reject any unknown vocabulary marked required.
  Preserve allowed unknown extension members, ignore unknown validation keywords, and reject
  malformed known keywords. Enforce `additionalProperties: false` where the captured schemas do.

## Required vocabulary semantics

- A schema without `dataType` describes opaque Plutus Data.
- Implement Data types `integer`, `bytes`, `list`, `map`, and `constructor`; builtin types are
  parameter-only.
- Implement `allOf`, `anyOf`, `oneOf`, and `not` exactly, with nonempty schema arrays and exactly
  one success for `oneOf`.
- Bytes: base16 `enum`, `minLength`, and `maxLength` over byte length.
- Integers: positive `multipleOf`, inclusive `minimum`/`maximum`, and exclusive bounds using
  arbitrary-precision integers without JSON-number loss.
- Lists: homogeneous-schema or positional-schema `items`, `minItems`, `maxItems`, and structural
  `uniqueItems`. Maps: `keys`, `values`, and item-count bounds. Constructors: required nonnegative
  `index` and positional `fields`.
- Support document-local JSON Pointer references rooted at `#/definitions/`, including recursive
  definitions. Resolve the captured five meta-schemas from reviewed in-code tables for document
  validation only. Reject every other relative file, absolute HTTP(S), filesystem, or custom
  reference; production code must never read provider artifacts.
- Apply existing 16 MiB input, 128 nesting-depth, and 100,000-node safety ceilings to JSON,
  schemas, reference traversal, and value validation. Detect reference cycles by
  schema-and-value state so productive recursive schemas work while nonproductive cycles fail
  deterministically.

## Implementation steps

1. Translate the five captured schemas into small reviewed validators while recording each
   property, required field, reference, and additional-property disposition in tests.
2. Implement immutable document/schema views, local JSON Pointer resolution, and stable
   path-addressed violations.
3. Implement vocabulary evaluation over existing typed Data and UPLC constant bindings.
4. Add compiled-code/hash consistency through existing serialized-script and Cardano hash owners.
5. Wire focused/root/runtime exports and documentation, then add browser and package tests.

## Validation

- Validate each captured meta-schema against the implemented shape table and test the captured
  Aiken blueprint example.
- Cover every document field, purpose alternative, Data/builtin type, validation keyword,
  unknown-keyword rule, local reference escape, missing definition, productive recursion,
  nonproductive cycle, depth/node/input limit, and prototype-pollution key.
- Cover compiled code with correct/missing/wrong hashes for V1, V2, and V3, malformed/odd base16,
  and oversized script envelopes.
- Assert value-validation structural equality for list uniqueness and map/constructor nesting.
- Assert focused/root/runtime binding identity and browser-safe production imports.
- Run targeted Plutus/runtime tests, `node libs/typescript/packages/runtime/test/pack-smoke.mjs`,
  and `npm --prefix libs/typescript run check`.

## Compatibility and human review

This is additive but processes untrusted recursive JSON. Review limits, JSON Pointer escaping,
prototype-safe objects, applicator error accounting, arbitrary-precision bounds, script hashing,
and the decision to preserve allowed extensions. Confirm that no provider file or generic schema
runtime is shipped.

## Completion criteria

- Captured document and vocabulary rules have one tested local disposition each.
- Valid blueprints parse to immutable views and invalid input yields stable path diagnostics.
- Data/constant validation covers the full captured CIP-0057 vocabulary and local definitions.
- External reference resolution, code generation, and runtime artifact access are impossible.
- Focused, root, runtime, browser, and packed-consumer checks pass.
- `npm --prefix libs/typescript run check` passes.
- The paired result records the exact five schemas and specification files consumed.

## Out of scope

- A general Draft 2020-12 JSON Schema engine
- Remote/filesystem reference resolution or schema registries
- TypeScript code generation, compiler integration, script execution, or parameter application
- Blueprint discovery, repository scanning, rendering, or network APIs

## Blockers

None.
