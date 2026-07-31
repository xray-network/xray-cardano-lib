# TypeScript implementation 0003 instruction

Implementation-Version: v1
Implementation-ID: typescript/0003
Created: 20260727T100703Z
Evidence-Mode: DIRECT
Depends-On: NONE
Provider-Evidence: ../../providers/uplc/0001-uplc/SNAPSHOT.md

## Inputs and authority

| Input | Kind | Required | Purpose |
| --- | --- | --- | --- |
| [`0001-uplc`](https://github.com/xray-network/cardano-lib/blob/main/updates/providers/uplc/0001-uplc/SNAPSHOT.md) | `PROVIDER` | Yes | Official Plutus UPLC and Cardano Ledger phase-two evidence |

## Objective

Implement and expose the complete official UPLC runtime required by Cardano protocol majors 5
through 11 as browser-native TypeScript owned by `@xray-network/cardano-plutus`, including AST
types, text and Flat codecs, cost models, and budgeted CEK evaluation. The package also exposes:

- `apply_params_to_script`, which applies a CBOR array of Plutus Data parameters left-to-right to
  the term inside a serialized Plutus script and returns the canonical single-CBOR-wrapped Flat
  program; and
- `eval_phase_two_raw`, which values Alonzo, Babbage, or Conway transaction redeemers using
  official Cardano Ledger context construction and official protocol/language UPLC semantics.

No upstream code or runtime artifact is shipped or executed.

## Comparison sources

- No previous accepted `uplc` snapshot exists.
- Official Cardano Ledger repository `https://github.com/IntersectMBO/cardano-ledger.git`, commit
  `a624de4c8db7286a6c065da149679ea55f7d5629` from `refs/heads/master`, is the authoritative
  comparison for phase-two transaction integration.
- [`0001-cardano-ledger`](https://github.com/xray-network/cardano-lib/blob/main/updates/providers/cardano-ledger/0001-cardano-ledger/SNAPSHOT.md) pins the same Cardano
  Ledger commit for wire-schema work. Its CDDL-only artifacts are complementary; this snapshot
  captures the phase-two source paths that it deliberately excluded.

## Confirmed upstream delta

- Official Plutus release `1.66.0.0` is the normative UPLC source; no Aiken source or conformance
  artifact is used.
- Official Flat uses term tags 0-9, builtin tags 0-100, and default-universe support for arrays and
  values. Tags 89-91 and 94-100 are valid protocol-11 builtins rather than reserved holes.
- Language and evaluator behavior depends on both Plutus ledger language and protocol major:
  V1 starts at 5, V2 at 7, V3 at 9; semantics are A for pre-Conway V1/V2, B for protocol 9-10
  V1/V2, C for protocol 9-10 V3, D for protocol 11 V1/V2, and E for protocol 11 V3.
- UPLC 1.1.0, `Constr`, `Case`, array/value constants, the complete builtin set, a maximum
  32-node universe header, and a maximum 1,024 `Constr` fields become available according to the
  captured protocol-11 rules.
- Official script deserialization retains a narrow historical compatibility rule: V1 and V2
  accept a valid first CBOR script object with trailing bytes, while V3 rejects the remainder.
  V2 and later enforce the 64-byte on-wire constant limit; V1 cannot retroactively enforce it.
- Official evaluation applies Data arguments left-to-right, scope-checks the applied term, charges
  the CEK machine and builtins, and returns consumed CPU/memory. V1/V2 accept any successful term;
  V3 requires the result to be Unit.
- Current cost-model parameter enumerations contain 332 V1, 332 V2, and 350 V3 values. The official
  adapter ignores extra tail values and substitutes signed `INT64_MAX` for missing tail values,
  recording warnings internally; it does not reject non-initial lengths. Cardano Ledger's genesis
  initial counts remain 166, 175, and 251 respectively.
- Official `evalTxExUnitsWithLogs` ignores each redeemer's encoded ExUnits and independently
  evaluates it against the protocol parameter `maxTxExUnits`; it does not decrement one shared
  transaction pool. It returns used ExUnits as memory then steps, whereas the requested public
  budget tuple is CPU then memory.
- Cardano Ledger builds V1/V2 scripts with `[datum, redeemer, context]` for spending with a datum
  and `[redeemer, context]` otherwise. V3 receives one `ScriptContext` containing TxInfo, redeemer,
  and purpose-specific ScriptInfo. It supplies exact Alonzo, Babbage, and Conway translations and
  protocol-sensitive compatibility guards.
- Cardano Ledger comparison commit also contains provisional PlutusV4/protocol-12 and Dijkstra
  support. The primary Plutus release exposes only V1-V3 through protocol 11, so those newer ledger
  paths are excluded.

## Changes to implement

| Language | Change ID | Requirement | Compatibility | Local owner | Validation | Reason |
| --- | --- | --- | --- | --- | --- | --- |
| `typescript` | `C001` | `REQUIRED` | `compatible` | `libs/typescript/packages/plutus/` | `npm --prefix libs/typescript run check` | Initial UPLC baseline |

## Captured artifacts

| Artifact | Upstream source and revision | Preservation and integrity | License | Intended consumers |
| --- | --- | --- | --- | --- |
| `../../providers/uplc/0001-uplc/artifacts/plutus/LICENSE.md`, `NOTICE.md`, `README.adoc` | Official Plutus root at `91e8c2af9c7bec705b035c6cf8e679c35c4c2ad5` | Three byte-exact files; checksums recorded | Apache-2.0 | Legal and source review |
| `../../providers/uplc/0001-uplc/artifacts/plutus/plutus-conformance/` excluding test cases | Official conformance metadata and runner source at the Plutus commit | Four byte-exact files; checksums recorded | Apache-2.0 | Corpus semantics and test-reader design |
| `../../providers/uplc/0001-uplc/artifacts/plutus/plutus-core/` | Selected official Flat, UPLC, CEK, builtin, cryptographic, costing, default model, and golden paths at the Plutus commit | Exactly 68 byte-exact files; checksums recorded | Apache-2.0 | Internal codec, evaluator, builtins, costs, tests |
| `../../providers/uplc/0001-uplc/artifacts/plutus/plutus-ledger-api/` | Selected official ledger API, protocol, language, script, cost-model, and V1/V2/V3 Data paths at the Plutus commit | Exactly 38 byte-exact files; checksums recorded | Apache-2.0 | Protocol selection, application, contexts, tests |
| `../../providers/uplc/0001-uplc/artifacts/conformance/corpus.json` | Deterministic transform of all regular files below official `plutus-conformance/test-cases` at the Plutus commit | 3,013 entries, 807,307 decoded bytes, per-entry size/SHA-256/base64; artifact checksum recorded | Apache-2.0 | Evaluator and budget conformance tests |
| `../../providers/uplc/0001-uplc/artifacts/conformance/README.md` | Snapshot-local control artifact | Authored consumer and provenance guidance; checksum recorded | Repository metadata | Safe corpus consumption |
| `../../providers/uplc/0001-uplc/artifacts/cardano-ledger/` | Selected official Cardano Ledger phase-two/context paths at `a624de4c8db7286a6c065da149679ea55f7d5629` | Exactly 24 byte-exact files; checksums recorded | Apache-2.0 | Raw transaction valuation and context tests |
| `../../providers/uplc/0001-uplc/artifacts/SHA256SUMS` | Snapshot-local control artifact | 139 sorted entries covering every other artifact; final inventory is 140 regular files | Repository metadata | Preparation and implementation integrity |

## Public API contract

Add these functions and type-only result contracts to
`@xray-network/cardano-plutus`:

```ts
export interface UplcExBudget {
  readonly cpu: bigint;
  readonly memory: bigint;
}

export interface PhaseTwoEvaluation {
  readonly cost: UplcExBudget;
  readonly logs: readonly string[];
}

export type PhaseTwoRawEvaluation = readonly [
  redeemerBytes: Uint8Array,
  evaluation: PhaseTwoEvaluation,
];

export function apply_params_to_script(
  paramsBytes: Uint8Array,
  plutusScriptBytes: Uint8Array,
): Uint8Array;

export function eval_phase_two_raw(
  txBytes: Uint8Array,
  utxosBytes: readonly (
    readonly [inputBytes: Uint8Array, outputBytes: Uint8Array]
  )[],
  costModelsBytes: Uint8Array,
  maxBudget: readonly [cpu: bigint, memory: bigint],
  slotConfig: readonly [
    zeroTimeMilliseconds: bigint,
    zeroSlot: bigint,
    slotLengthMilliseconds: bigint,
  ],
  protocolMajorVersion: number,
  runPhaseOne: boolean,
): readonly PhaseTwoRawEvaluation[];
```

Expose immutable `UplcProgram`, `UplcTerm`, `UplcConstant`, `UplcData`, and `UplcType` values;
`parseUplcText`; Flat and serialized-script encode/decode functions; cost-model construction and
lookup; `defaultMachineCosts`; and `evaluateProgram` from both the Plutus root and
`@xray-network/cardano-plutus/uplc`. Expose the typed `Data` schema API from the root and
`@xray-network/cardano-plutus/data`.

Both functions are synchronous and clone caller-owned and returned bytes/arrays. Protocol majors
5-6 select Alonzo transaction decoding, 7-8 Babbage, and 9-11 Conway; every other major is rejected.
The cost-model input is the ledger `CostModels` CBOR map and is required. The slot tuple defines
the linear mapping `zeroTime + (slot - zeroSlot) * slotLength` in POSIX milliseconds.

`eval_phase_two_raw` sorts redeemers by ledger pointer tag then index, values every entry
independently against `maxBudget`, returns a CBOR redeemer fragment with calculated ExUnits, and
aborts on the first deterministic error. Existing redeemer ExUnits are ignored for calculation.
`logs` contains official `Trace` text in emission order; no non-official label/debug projection is
added.

Malformed byte inputs use `DeserializeError`; invalid numeric bounds use `CardanoBoundsError`.
Add `"EVALUATE"` and `"UNSUPPORTED"` to `CardanoErrorCode`: unsupported protocol/era/language
combinations use `CardanoError("UNSUPPORTED", ...)`, and script discovery, context, cost-model,
machine, return-value, or budget failures use `CardanoError("EVALUATE", ...)` with a stable path
such as `["redeemers", tag, index]` and the underlying cause.

Expose the Plutus functions and types by identity from the root runtime facade. Ledger
transaction decoding, script resolution, context construction, callbacks, and protocol overrides
remain package-private.

## Cardano Lib change map

| Upstream change | Evidence | Local owner | Required implementation | Compatibility | Tests |
| --- | --- | --- | --- | --- | --- |
| Stable Flat program, term, builtin, constant, list, pair, Data, array, value, BLS, and padding encodings | `../../providers/uplc/0001-uplc/artifacts/plutus/plutus-core/**/Flat*.hs`, `Default/{Builtins,Universe}.hs`, `Value.hs`, Flat golden | `libs/typescript/packages/plutus/src/uplc/{ast,flat,index}.ts` | Implement and expose iterative complete Flat decoding/encoding for UPLC 1.0.0/1.1.0, De Bruijn indices, term tags 0-9, builtin tags 0-100, universe tags and constant payloads; preserve exact canonical output | `apply_params_to_script` is strict and rejects CBOR/Flat remainder, malformed tags/padding, free variables, double wrapping, and inputs over 16 MiB | Flat golden, every constant/term, malformed/truncated/padding/tag/version, deep iterative and size-bound cases |
| Parameter application is iterated term application to Data constants inside one CBOR byte-string envelope | `Common/Eval.hs`, `SerialisedScript.hs`, `MkPlc.hs`, `MkUPlc.hs` | `libs/typescript/packages/plutus/src/api.ts`, existing chain Plutus Data owner | Decode a CBOR array of complete Plutus Data fragments, decode the complete serialized program, apply constants left-to-right without evaluation, scope-check, and return one canonical CBOR byte string | Preserve caller bytes; empty params still canonicalize the valid script | Empty/one/many/order-sensitive params, exact envelope, non-array/malformed/trailing/double-CBOR, defensive-copy tests |
| CEK compute/return machine, discharge, scope, logs, restricting budget, step batching, and memory accounting | UPLC `Cek*.hs`, `CommonAPI.hs`, machine budget/memory/cost paths, conformance corpus | `libs/typescript/packages/plutus/src/uplc/machine.ts` | Implement and expose all term states including Constr/Case and builtin application, official startup/step/builtin charging and final flush, bigint counters, out-of-budget behavior, and Trace log order | Avoid JS recursion for adversarial terms; charge before expensive primitives; evaluation is bounded by each supplied max budget | CEK unit cases, exact CPU/memory goldens, boundary exhaustion, nontermination guard, logs, deep terms, full applicable corpus |
| Builtin tags 0-100 and semantics variants A-E are protocol/language-sensitive | `Default/Builtins.hs`, `Bitwise.hs`, `Value.hs`, crypto paths, `ProtocolVersions.hs`, `Versions.hs`, `MachineParameters.hs` | `libs/typescript/packages/plutus/src/uplc/{machine,cost-model}.ts`; crypto primitives remain in `libs/typescript/packages/crypto/src/primitives/crypto.ts` | Implement exact force/arity/type/error order and denotation for every builtin; select A/B/C/D/E and builtin availability from `(language, protocolMajorVersion)` | No unavailable builtin may deserialize; protocol 11 bounded integer/bytes and UTF-8 semantics must not leak into earlier variants | All official builtin corpus vectors by semantics, availability matrix 5-11, malformed crypto/group/value/array inputs, browser execution |
| Additional hashes, secp256k1, and BLS12-381 are required by official builtins | Captured official crypto modules and corpus vectors | `docs/adr/0004-cryptography-primitives.md`, `libs/typescript/packages/crypto/src/primitives/{crypto,uplc}.ts` and exports | Add Cardano-owned SHA2-256, Keccak-256, RIPEMD-160, secp256k1 ECDSA/Schnorr, BLS group/pairing/hash/compression wrappers; reuse existing Blake2b/SHA3/Ed25519 | Keep exact noble 2.2.0 pins; no new dependency, WASM, native addon, Buffer, or Node builtin; subgroup/infinity and signature acceptance are security-sensitive | Official positive/negative/malformed vectors, subgroup/infinity/compression/hash-to-group cases, browser/dependency-policy tests |
| Cost model maps ordered signed values into CEK/builtin formulas and tolerate length drift | Plutus `ParamName.hs`, V1/V2/V3 ParamName and EvaluationContext, cost JSON/golden; Ledger `CostModels.hs` | Existing `libs/typescript/packages/chain/src/era/conway/model.ts` CostModels plus new `libs/typescript/packages/plutus/src/uplc/cost-model.ts` | Decode the map losslessly; select IDs 0/1/2; require signed-64 values; map current 332/332/350 names; ignore extra tail or pad short tail with `INT64_MAX`; select formulas per protocol | Required active model; no fallback defaults; unknown language IDs do not affect active evaluation; do not apply obsolete strict initial counts | Initial/current/short/extra models, first/last parameter sensitivity, missing language, signed bounds, protocol/semantics selection, exact budgets |
| Serialized-script validation depends on language and protocol | `SerialisedScript.hs`, `Versions.hs`, `Common/Eval.hs` | `libs/typescript/packages/plutus/src/uplc/flat.ts` | Enforce language/PV and PLC-version availability, builtin availability, V2+ 64-byte constant rule, PV11 universe/constr bounds, and scope; preserve V1/V2 remainder compatibility only inside phase-two script decoding | This is a documented consensus exception to generic complete-input CBOR; V3 and parameter application stay strict; V3 must return Unit while V1/V2 accept any success | Matrix across V1/V2/V3 and PV5-11, remainder regressions, constant/header/constr limits, bad scope/version, V3 non-Unit |
| Official valuation decodes a known era and resolves scripts, datums, reference scripts, UTxOs, and redeemer pointers | Ledger `Alonzo/Plutus/Evaluate.hs`, era `Scripts.hs`, `Rules/Utxos.hs`; existing chain models | Existing chain era/shared owners plus private `libs/typescript/packages/plutus/src/ledger/{context,evaluate}.ts` | Derive era from PV; strictly decode one tx and each input/output fragment with existing owners; reject duplicate/missing UTxOs; gather witness/reference scripts and datum sources; sort pointers by tag/index | No ambiguous trial decoding; native scripts are never phase-two; preserve valid list/map redeemer forms and caller bytes | Alonzo 5-6, Babbage 7-8, Conway 9-11 fixtures; list/map ordering; inline/reference datum/script; duplicate/missing/malformed/trailing inputs |
| V1/V2/V3 TxInfo, purpose, ScriptInfo, and Data encodings differ by era and feature | Captured Plutus V1/V2/V3 Data modules and Ledger Alonzo/Babbage/Conway `TxInfo.hs` plus specs | Private `libs/typescript/packages/plutus/src/ledger/context.ts` reusing chain nominal types | Reproduce inputs/outputs/value/address/credential/time/cert/withdrawal/datum/redeemer/governance translation and purpose indexes; use 3/2 legacy args and one V3 context exactly | Enforce official feature guards and known protocol-9 certificate compatibility; linear slot mapping must use bigint and millisecond truncation rules | Era/language structural fixtures for spend/mint/cert/reward/vote/propose, interval edges, Conway V1/V2 excluded features, protocol-9 cert regression |
| Official tx valuation ignores encoded ExUnits and uses maxTxExUnits independently per redeemer | Ledger `evalTxExUnitsWithLogs`, core `Evaluate.hs`, `TxInfo.transExUnits` | `libs/typescript/packages/plutus/src/ledger/evaluate.ts`, `libs/typescript/packages/plutus/src/api.ts` | Evaluate each sorted redeemer with the same `maxBudget`; return used cost/logs and rewritten redeemer CBOR; abort with stable contextual error on failure | Public CPU/memory maps to ledger steps/mem; no shared remaining budget or initial/remaining result fields | Multi-redeemer independent-cap regression, encoded-budget ignored, CPU/memory swap, exact returned CBOR, partial failure ordering, Trace logs |
| Optional collection checks compare needed Plutus purposes and supplied redeemers | Ledger `collectPlutusScriptsWithContext` and collection errors | `libs/typescript/packages/plutus/src/ledger/evaluate.ts` | When `runPhaseOne` is true, reject missing and extra redeemer pointers before evaluation, excluding native scripts | Explicitly not full phase-one transaction validation | Switch off/on, exact set, missing/extra, native exclusion, all six purpose tags |
| Public surface belongs to Plutus and the root runtime | UPLC and ledger API evidence | `libs/typescript/packages/plutus/src/{api,index}.ts`, `libs/typescript/packages/plutus/src/uplc/index.ts`, `libs/typescript/packages/runtime/src/{plutus,index}.ts` | Export Data, UPLC parsing/encoding/cost/evaluation, parameter application, and valuation by identity without new nominal ledger classes | Stable package and subpath ownership | Type/import tests, namespace/root identity, ESM NodeNext/bundler, pack and browser graph |
| Plutus tests mirror the source domain and remain discoverable | TypeScript workspace `test:built` script | `libs/typescript/packages/plutus/test/*.test.mjs`, `libs/typescript/package.json` | Keep Data, API, Flat, machine, ledger, and conformance tests under the Plutus owner | Avoid duplicate discovery and preserve all tests | `npm --prefix libs/typescript run test:built` and the targeted Plutus command prove all files run |
| Builder consumers map valuation back to witness keys | Existing `TxRedeemerBuilder.draft_tx`, `set_exunits`, `RedeemerWitnessKey`, `ExUnits` | Tests and documentation only | Demonstrate draft serialization, raw valuation, returned pointer decoding, `ExUnits.new(memory, cpu)`, `set_exunits`, and rebuild | No third helper or automatic builder mutation | End-to-end Alonzo/Babbage/Conway valuation-to-builder fixture where supported |

## Implementation steps

1. Implement the ADR 0004 Cardano-owned crypto wrappers with official positive, negative,
   malformed, subgroup,
   infinity, browser, and dependency-policy tests; retain all current dependency pins.
2. Place Plutus tests under `libs/typescript/packages/plutus/test/` and make root `test:built` discover both flat
   and nested test files.
3. Add public UPLC value/AST and iterative Flat modules under
   `libs/typescript/packages/plutus/src/uplc/`. Implement UPLC 1.0.0/1.1.0, stable term/builtin/universe
   encodings, CBOR script envelopes, De Bruijn scope checks, deterministic encoding, and the exact
   V1/V2 phase-two remainder exception.
4. Add cost-model, memory, CEK, and builtin modules. Implement current parameter order and tolerant
   length mapping, all formula shapes, tags 0-100, variants A-E, availability by language/PV,
   restricting budgets, logs, result rules, and protocol-11 bounds.
5. Add package-private ledger decode, resolution, Data translation, context, collection-check, and
   evaluation modules for Alonzo 5-6, Babbage 7-8, and Conway 9-11. Reuse existing chain/core types,
   use the linear slot mapping, resolve every official purpose, and convert all upstream partial
   failures into stable Cardano errors.
6. Add `libs/typescript/packages/plutus/src/api.ts` with the exact contract above. Implement strict
   left-to-right parameter application and deterministic raw valuation, clone boundaries, per-script
   maximum budgets, redeemer CBOR rewriting, CPU/memory conversion, and error paths.
7. Export the typed Data, UPLC, parameter-application, and phase-two APIs from the Plutus root,
   focused subpaths, and root runtime facade. Add import/type/identity/browser/packed-consumer tests and the
   draft/evaluate/set-ExUnits builder integration fixture.
8. Integrity-check and consume the entire conformance corpus through a private test reader.
   Run every applicable UPLC evaluation and budget vector, all captured Flat/cost goldens, focused
   era/context fixtures, and the required targeted and TypeScript completion commands.

## Validation

- Verify `../../providers/uplc/0001-uplc/artifacts/SHA256SUMS`; verify all 3,013 corpus entry sizes and SHA-256 digests before
  consuming a vector.
- `npm --prefix libs/typescript run build`
- `node --test libs/typescript/packages/crypto/test/crypto.test.mjs libs/typescript/packages/crypto/test/dependency-policy.test.mjs`
- `node --test libs/typescript/packages/plutus/test/*.test.mjs`
- `node --test libs/typescript/packages/runtime/test/imports.test.mjs libs/typescript/packages/runtime/test/api-contract.test.mjs libs/typescript/packages/runtime/test/browser-package.test.mjs`
- `node libs/typescript/packages/runtime/test/pack-smoke.mjs`
- `npm --prefix libs/typescript run test:built` after nested-test discovery is updated
- TypeScript completion command: `npm --prefix libs/typescript run check`
- Expected evidence: all applicable official conformance/budget vectors and captured goldens pass;
  transaction contexts and rewritten redeemer CBOR match era fixtures; budgets are independent and
  correctly swap CPU/memory into steps/mem; malformed, unavailable, missing-data, invalid-return,
  and out-of-budget cases fail deterministically; public identities match; and no new dependency,
  Node-only edge, upstream runtime code, WASM, native addon, or unintended packed artifact appears.

## Compatibility and human review

- `eval_phase_two_raw` requires protocol selection and cost models, supports three eras, uses a
  per-script cap, and follows official tolerant cost-model length behavior.
- `eval_phase_two_raw` is valuation, not transaction validation. `runPhaseOne` performs only the
  captured script/redeemer collection checks; callers still need complete ledger phase-one
  validation, balancing, fees, collateral, and script-data-hash handling.
- The public budget order is CPU then memory. Ledger ExUnits and `ExUnits.new` are memory then
  steps. Human review must verify every conversion.
- V1/V2 trailing serialized-script bytes are accepted only in the internal phase-two decoder for
  historical consensus compatibility. This narrow exception must not weaken Cardano Lib's generic
  complete-input CBOR APIs or `apply_params_to_script`.
- Cryptographic acceptance, BLS subgroup/infinity/compression behavior, cost formulas and indexing,
  Flat bit encoding, protocol availability, context Data shapes, protocol-9 certificate behavior,
  and era selection require independent security/consensus review before stable release.
- The 16 MiB standalone parameter/script cap is above on-chain transaction limits and protects the
  non-ledger `apply_params_to_script` entry point. Evaluation remains bounded by caller-supplied
  execution units and iterative parsing.
- Both official evidence sets are Apache-2.0 and remain snapshot-only. Published packages contain
  Cardano Lib-owned MIT TypeScript.

## Completion criteria

- The Plutus root, `./data`, `./uplc`, and root runtime facade expose the planned typed Data, UPLC,
  parameter-application, and valuation APIs with shared identities.
- The internal evaluator implements all official UPLC terms, constants, builtins 0-100, cost
  formulas, semantics A-E, language/PV availability, CEK accounting, logs, and return rules needed
  by protocol majors 5-11.
- Parameter application passes official/focused Flat and Data cases, applies arguments left-to-right,
  emits one canonical CBOR byte-string envelope, preserves no trailing input, and never mutates
  caller bytes.
- Raw valuation supports Alonzo 5-6, Babbage 7-8, and Conway 9-11 with V1/V2/V3 as available;
  builds official contexts for every era-supported purpose; ignores encoded ExUnits; uses the same
  max budget independently for every redeemer; and returns exact rewritten redeemer CBOR plus logs.
- Cost-model length drift, strict/tolerant script remainder boundaries, protocol-11 bounds,
  CPU/memory mapping, V3 Unit results, missing ledger data, and optional collection checks have
  explicit regression coverage.
- The complete captured corpus and goldens pass all applicable tests. Nested Plutus tests, ADR,
  dependency, browser, runtime identity, packed-consumer, builder integration, and `npm --prefix libs/typescript run check`
  all pass without new dependencies or non-TypeScript runtime artifacts.

## Observed but excluded

- Cardano Ledger's provisional PlutusV4 language tag, protocol 12, Dijkstra era, nested transaction
  contexts, and Guarding purpose because official Plutus release `1.66.0.0` does not expose them.
- Official typed Plutus Core, compiler, plugin, optimizer, parser, pretty-printer, debugger,
  metatheory, formal proof, benchmarking, cost-model generation, CLI, and code-generation APIs.
- Conformance suites for typed PLC, typechecking, erasure, or compile-time behavior.
- Aiken's APIs, patches, errors, tag omissions, default costs, shared-budget behavior, context
  snapshots, and test corpus.

## Out of scope

- A UPLC pretty-printer, optimizer, compiler, debugger, protocol override callback, script
  replacement callback, or third valuation helper
- PlutusV4/protocol 12/Dijkstra support
- Full phase-one ledger validation, fee calculation, minimum-fee selection, balancing, collateral
  selection, script-data-hash calculation, or automatic transaction-builder mutation
- Importing or executing upstream Haskell/Rust, WASM, native libraries, generated bindings,
  binaries, or loading snapshot artifacts at package runtime
- Adding or upgrading any runtime dependency

## Blockers

None.
