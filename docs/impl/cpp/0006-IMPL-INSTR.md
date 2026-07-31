# C++ implementation 0006 instruction

Implementation-Version: v1
Implementation-ID: cpp/0006
Created: 20260731T075521Z
Evidence-Mode: HYBRID
Depends-On: ./0001-IMPL-RESULT.md
Provider-Evidence: ../../providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`cpp/0001`](./0001-IMPL-RESULT.md) | `IMPLEMENTATION_RESULT` | Yes | Accepted typed Data, UPLC script, hashing, JSON-boundary, limits, ownership, and package baseline |
| [`0001-cardano-cips`](https://github.com/xray-network/cardano-lib/blob/main/updates/providers/cardano-cips/0001-cardano-cips/SNAPSHOT.md) | `PROVIDER` | Yes | Captured CIP-57 document and the complete five-file blueprint schema set |
| `libs/cpp/include/cardano/plutus/data.hpp` and `libs/cpp/include/cardano/plutus/uplc.hpp` | `LOCAL` | Yes | Existing Data, schema, constant, program, and script owners to reuse |

## Objective

Implement a bounded, offline CIP-57 contract-blueprint model and validator. Support exactly the
captured blueprint vocabulary, local definitions, Plutus Data schemas, and UPLC builtin parameter
schemas without turning the library into a general JSON Schema engine, compiler, or code generator.

## Changes to implement

| Change ID | Requirement | Compatibility | Local owner | Validation |
| --- | --- | --- | --- | --- |
| `C6-DOC1` | Parse, validate, retain, and emit CIP-57 preamble, validators, arguments, parameters, definitions, compiled code, and hashes. | New additive model; existing JSON and script APIs remain unchanged. | `include/cardano/plutus/blueprint.hpp`, `src/plutus/blueprint.cpp` | Captured meta-schema, round-trip, malformed, and extension tests |
| `C6-SCH1` | Compile the captured Plutus Data and builtin schema vocabulary into a bounded internal validation graph with local references only. | No public generic JSON Schema abstraction or competing `Data` type. | Blueprint owner, private schema module | Schema-family, reference, applicator, ambiguity, and limit tests |
| `C6-DAT1` | Validate existing typed `plutus::Data` values and `plutus::UplcConstant` parameter values against named validator schemas. | Read-only validation; values are not coerced or mutated. | Blueprint public API using existing owners | Positive/negative recursive and large-integer tests |
| `C6-COD1` | Validate compiled-code envelopes and require/match the declared script hash using the preamble Plutus version. | Existing raw UPLC decode remains; blueprint admission is stricter. | Blueprint owner using UPLC and chain hash utilities | V1/V2/V3 code/hash and malformed-envelope tests |
| `C6-API1` | Export the focused API through `cardano::plutus` and aggregate headers and document the trust/limit boundary. | One nominal blueprint binding across entry points. | Plutus facade, CMake, docs/inventory | Component, aggregate, and installed-consumer tests |

## Exact supported contract

- `BlueprintPreamble` requires `title`, `version`, and `plutusVersion`; supported versions are
  `v1`, `v2`, and `v3`. It models optional description, compiler name/version, and license.
  `BlueprintValidator` requires `title` and `redeemer`; datum, parameters, description,
  `compiledCode`, and `hash` follow the captured schema. A compiled code value requires a hash.
- Argument purposes are the captured `spend`, `mint`, `withdraw`, and `publish` alternatives.
  Purpose combinations must satisfy the captured non-overlapping `oneOf` rules; omission retains
  the schema-defined unrestricted argument behavior.
- Data schemas support integer, bytes, list, map, and constructor forms plus the captured
  titles/descriptions, bounds, item/key/value/field schemas, local references, and `allOf`,
  `anyOf`, `oneOf`, and `not`. Parameter schemas additionally support captured builtin unit,
  boolean, integer, bytes, string, pair, and list forms.
- References may resolve only within the parsed document's `definitions` or to the five captured
  schema identities. URI, file, network, environment, dynamic, recursive-external, and
  caller-supplied resolver paths are rejected.
- Defaults are: JSON/schema depth 128, 100,000 total parsed nodes, 16,777,216 aggregate UTF-8
  string bytes, 10,000 validators plus definitions, and 1,000,000 schema-evaluation steps. Callers
  may lower named limits; increases require explicit values and checked allocation arithmetic.

## Implementation steps

1. Define immutable `ContractBlueprint`, `BlueprintPreamble`, `BlueprintValidator`,
   `BlueprintArgument`, `BlueprintParameter`, `BlueprintSchema`, `BlueprintLimits`, and
   path-bearing `BlueprintDiagnostic` values. Factories return `core::Result`; retained collections
   and strings are owned.
2. Parse with the existing private JSON dependency and reject duplicate object keys, trailing
   input, invalid UTF-8, non-integral numeric schema bounds, limit overflow, and wrong required
   fields before constructing the public model. Preserve permitted unknown validator/schema
   annotation properties for re-emission, but never interpret them as executable rules.
3. Resolve definitions to indexed nodes in a private graph. Detect missing references and cycles
   during compilation. A cycle is accepted only as a reference graph and is controlled at value
   evaluation by depth and step limits; no native recursion may bypass those limits.
4. Implement exact applicator semantics: `allOf` requires every branch, `anyOf` at least one,
   `oneOf` exactly one, and `not` zero matches. Diagnostics identify validator, argument/parameter,
   schema path, Data/constant path, and the decisive failing rule in deterministic order.
5. Validate typed `Data` without JSON conversion or coercion. Constructor indexes and integer
   bounds use arbitrary integers where the captured schema permits them. Lists, maps, bytes, and
   constructor fields enforce all declared lengths and nested schemas.
6. Validate builtin parameter schemas directly against existing UPLC constants, including nested
   pair/list shapes. A Plutus Data parameter continues through the Data schema path; unsupported
   constant kinds fail explicitly.
7. Decode `compiledCode` as strict even-length hex, then through the accepted complete-input
   serialized-script/program envelope with existing limits. Compute the `ScriptHash` using
   namespace byte 1, 2, or 3 for `v1`, `v2`, or `v3`; require the declared 28-byte lowercase hex
   hash to match. Never execute compiled code while loading a blueprint.
8. Emit deterministic JSON with known fields in documented order and preserved annotations.
   Emission never dereferences external resources or embeds captured schemas/artifacts.

## Validation

- Validate representative accepted documents against each captured schema file and reject missing
  preamble/validator requirements, invalid versions/purposes, duplicate keys, bad hashes, malformed
  compiled code, references outside the captured set, and all resource limits.
- Add table tests for every Data form, builtin form, bound, local definition, and applicator,
  including `oneOf` zero/one/multiple matches and deterministic error paths.
- Round-trip documents with optional fields and permitted annotations; reparse emitted JSON and
  compare the public model and code/hash identity.
- Test V1/V2/V3 script namespace hashes, code-without-hash rejection, hash-without-code handling
  exactly as allowed by the captured schema, and prove parsing never evaluates the program.
- Run completion, sanitizer, hardening, provider-integrity, API inventory, component, aggregate,
  and installed-content gates.

## Compatibility and human review

This is additive and does not alter UPLC, Data, script hashing, or general JSON behavior. Schema
validation is a bounded CIP-57 interpretation, not a claim of JSON Schema conformance. Unknown
annotations may round-trip but cannot grant capability or weaken a known rule.

Human review must verify schema-to-model mapping, applicator semantics, reference/cycle limits,
compiled-code hash namespaces, deterministic diagnostics, and absence of network/filesystem access.

## Completion criteria

- Documents, code hashes, Data arguments, and parameters validate exactly within the captured
  vocabulary and declared limits.
- External references and unsupported JSON Schema behavior fail closed.
- Existing nominal Data/UPLC/hash owners are reused and provider artifacts are absent from builds
  and installs.
- All required C++ workflows pass and the paired result records the exact five schemas consumed.

## Out of scope

- A general JSON Schema validator, remote/file reference resolver, blueprint code generation,
  source compilation, parameter application, contract execution, transaction construction, IDE
  service, registry, or network fetch.
- Validation against uncaptured schema versions or compiler-specific blueprint extensions with
  executable meaning.

## Blockers

None.
