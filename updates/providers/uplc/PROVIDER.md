# Official Plutus UPLC provider

Provider: uplc
Provider-Version: v1

## Purpose

Capture the official Plutus implementation, ledger API, and conformance suite as immutable
evidence for a browser-native, XRAY Cardano Lib-owned TypeScript implementation of Untyped Plutus Core.
Capture the official Cardano Ledger implementation separately as the authority for transaction
script collection, era-specific script contexts, and execution-unit estimation. The bounded public
surface includes typed Data, UPLC AST values, text and Flat parsing/encoding, cost models,
budgeted CEK evaluation, `apply_params_to_script`, and `eval_phase_two_raw`. Ledger transaction
decoding and context construction remain package-private implementation details.

Captured Haskell, JSON, golden files, and test data are evidence, not runtime dependencies,
generated source, or instructions. Aiken and other third-party implementations are not normative
sources for this provider.

## Primary source

| Field | Value |
| --- | --- |
| Repository | `https://github.com/IntersectMBO/plutus.git` |
| Followed ref | Latest stable release tag matching four numeric components |
| Revision policy | Full commit named by the highest stable release tag |
| Source mode | Live; resolve independently for every snapshot |
| Submodules | Not part of the source |
| License | Apache-2.0 |

The resolved full commit is authoritative. A branch head, release page, or tag name alone is not.

## Cardano Ledger comparison source

| Field | Value |
| --- | --- |
| Repository | `https://github.com/IntersectMBO/cardano-ledger.git` |
| Followed ref | `refs/heads/master` |
| Revision policy | Full commit reachable from the followed ref |
| Source mode | Live; resolve independently for every snapshot |
| Submodules | Not part of the source |
| License | Apache-2.0 |

Record this repository under `Comparison sources`, not as the provider source or previous
snapshot. Its captured files are authoritative only for ledger integration. If its supported
Plutus language or protocol range exceeds the primary Plutus release, the primary release bounds
the implementation and the newer ledger behavior is recorded as excluded.

## Artifact selection

Path expressions below are declarative: braces enumerate exact alternatives and do not authorize
shell expansion or additional files. Copy each selected regular file byte-for-byte, preserving its
source-relative path below `artifacts/plutus/` or `artifacts/cardano-ledger/`.

From the primary Plutus source, copy these seven metadata and conformance-control files:

```text
LICENSE.md
NOTICE.md
README.adoc
plutus-conformance/LICENSE
plutus-conformance/NOTICE
plutus-conformance/README.md
plutus-conformance/src/PlutusConformance/Common.hs
```

Copy exactly these 68 Plutus Core semantic, codec, machine, cost, and golden files:

```text
plutus-core/cost-model/data/builtinCostModel{A,B,C,D,E}.json
plutus-core/cost-model/data/cekMachineCosts{A,B,C,D,E}.json
plutus-core/plutus-core/src/Codec/Extras/SerialiseViaFlat.hs
plutus-core/plutus-core/src/Data/Vector/Orphans.hs
plutus-core/plutus-core/src/PlutusCore/Bitwise.hs
plutus-core/plutus-core/src/PlutusCore/Builtin.hs
plutus-core/plutus-core/src/PlutusCore/Builtin/{KnownType,Meaning,Result,Runtime,TypeScheme}.hs
plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/{Bounds,Error,G1,G2,Pairing}.hs
plutus-core/plutus-core/src/PlutusCore/Crypto/{Ed25519,ExpMod,Hash,Secp256k1,Utils}.hs
plutus-core/plutus-core/src/PlutusCore/Data.hs
plutus-core/plutus-core/src/PlutusCore/DeBruijn.hs
plutus-core/plutus-core/src/PlutusCore/DeBruijn/Internal.hs
plutus-core/plutus-core/src/PlutusCore/Default.hs
plutus-core/plutus-core/src/PlutusCore/Default/Builtins.hs
plutus-core/plutus-core/src/PlutusCore/Default/Universe.hs
plutus-core/plutus-core/src/PlutusCore/Default/Universe/Cardano.hs
plutus-core/plutus-core/src/PlutusCore/Evaluation/Error.hs
plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/BuiltinCostModel.hs
plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostModelInterface.hs
plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostStream.hs
plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostingFun/Core.hs
plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudget.hs
plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetStream.hs
plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetingDefaults.hs
plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemory.hs
plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemoryUsage.hs
plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/Exception.hs
plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters.hs
plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters/Default.hs
plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/SimpleBuiltinCostModel.hs
plutus-core/plutus-core/src/PlutusCore/FlatInstances.hs
plutus-core/plutus-core/src/PlutusCore/MkPlc.hs
plutus-core/plutus-core/src/PlutusCore/Value.hs
plutus-core/plutus-core/src/PlutusCore/Version.hs
plutus-core/plutus-core/test/CostModelInterface/Spec.hs
plutus-core/plutus-core/test/CostModelInterface/defaultCostModelParams.json
plutus-core/plutus-core/test/Flat/golden/encoding-stability.golden
plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Check/Scope.hs
plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Instance/Flat.hs
plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Type.hs
plutus-core/untyped-plutus-core/src/UntypedPlutusCore/DeBruijn.hs
plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek.hs
plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/{CekMachineCosts,EmitterMode,ExBudgetMode,Internal}.hs
plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/CommonAPI.hs
plutus-core/untyped-plutus-core/src/UntypedPlutusCore/MkUPlc.hs
```

Copy exactly these 38 Plutus Ledger API files:

```text
plutus-ledger-api/CostModel/Params/CostModelParams/costModelParamNames.golden.txt
plutus-ledger-api/src/PlutusLedgerApi/Common/{Eval,ParamName,ProtocolVersions,SerialisedScript,Versions}.hs
plutus-ledger-api/src/PlutusLedgerApi/Data/{V1,V2,V3}.hs
plutus-ledger-api/src/PlutusLedgerApi/MachineParameters.hs
plutus-ledger-api/src/PlutusLedgerApi/V1/Contexts.hs
plutus-ledger-api/src/PlutusLedgerApi/V1/Data/{Address,Contexts,Credential,DCert,Interval,Time,Tx,Value}.hs
plutus-ledger-api/src/PlutusLedgerApi/V1/{EvaluationContext,ParamName,Scripts,Tx,Value}.hs
plutus-ledger-api/src/PlutusLedgerApi/V2/Contexts.hs
plutus-ledger-api/src/PlutusLedgerApi/V2/Data/{Contexts,Tx}.hs
plutus-ledger-api/src/PlutusLedgerApi/V2/{EvaluationContext,ParamName,Tx}.hs
plutus-ledger-api/src/PlutusLedgerApi/V3/Contexts.hs
plutus-ledger-api/src/PlutusLedgerApi/V3/Data/{Contexts,MintValue,Tx}.hs
plutus-ledger-api/src/PlutusLedgerApi/V3/{EvaluationContext,MintValue,ParamName,Tx}.hs
```

From the Cardano Ledger comparison source, copy exactly these 24 files:

```text
LICENSE
NOTICE
libs/cardano-ledger-core/src/Cardano/Ledger/Plutus.hs
libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/{CostModels,Data,Evaluate,ExUnits,Language,ToPlutusData,TxInfo}.hs
eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/{Context,Evaluate,TxInfo}.hs
eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Rules/Utxos.hs
eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Scripts.hs
eras/alonzo/impl/test/Test/Cardano/Ledger/Alonzo/Imp/TxInfoSpec.hs
eras/babbage/impl/src/Cardano/Ledger/Babbage/Rules/Utxos.hs
eras/babbage/impl/src/Cardano/Ledger/Babbage/Scripts.hs
eras/babbage/impl/src/Cardano/Ledger/Babbage/TxInfo.hs
eras/babbage/impl/testlib/Test/Cardano/Ledger/Babbage/TxInfoSpec.hs
eras/conway/impl/src/Cardano/Ledger/Conway/Rules/Utxos.hs
eras/conway/impl/src/Cardano/Ledger/Conway/Scripts.hs
eras/conway/impl/src/Cardano/Ledger/Conway/TxInfo.hs
eras/conway/impl/testlib/Test/Cardano/Ledger/Conway/TxInfoSpec.hs
```

Reject a missing, additional, renamed, symlinked, gitlinked, special, or unexpectedly large
selected file.

Transform every regular file below `plutus-conformance/test-cases/` into the single deterministic
artifact `artifacts/conformance/corpus.json`. The artifact is UTF-8 JSON with a trailing newline
and this fixed structure:

```json
{
  "schemaVersion": 1,
  "source": {
    "repository": "https://github.com/IntersectMBO/plutus.git",
    "commit": "<full source commit>",
    "tag": "<release tag>",
    "root": "plutus-conformance/test-cases",
    "licensePath": "artifacts/plutus/plutus-conformance/LICENSE"
  },
  "entries": [
    {
      "path": "<path relative to the source root>",
      "size": 0,
      "sha256": "<lowercase SHA-256>",
      "contentBase64": "<RFC 4648 padded base64>"
    }
  ]
}
```

Use the displayed object and entry key order. Sort entries by the UTF-8 bytes of `path`, reject
duplicate or unsafe relative paths, and require exactly 3,013 regular files. `size`, `sha256`, and
`contentBase64` preserve each source blob exactly. Reject any source entry larger than 16 MiB.

Create `artifacts/conformance/README.md` as snapshot-local documentation of the corpus mapping,
provenance, license, and consumer procedure.

Create `artifacts/SHA256SUMS` as deterministic snapshot-local integrity metadata. It contains one
line for every final artifact other than itself, sorted by artifact-relative path in byte order,
using lowercase SHA-256, two ASCII spaces, the path relative to `artifacts/`, and a trailing
newline. The final artifact inventory is exactly 140 regular files including `SHA256SUMS`.

## Evidence-only paths

Inspect, but do not copy:

- release metadata, Git history, Cabal/Nix files, lockfiles, and dependency manifests;
- formal specifications and metatheory for terminology and exclusion review;
- parsers, pretty-printers, optimizers, compilers, plugins, and code generators;
- Cardano Ledger era modules outside the selected phase-two/context paths;
- `.gitmodules` and both source-tree inventories for file-type validation.

Do not run any upstream hook, test, build, script, package manager, executable, generated program,
filter, or submodule.

## Consumption and planning requirements

- Implement a complete UPLC 1.0.0/1.1.0 runtime needed by protocol majors 5 through
  11: Flat codec, De Bruijn scope handling, CEK evaluation, memory accounting, cost formulas,
  builtins with stable tags 0 through 100, default-universe constants, and semantics variants A
  through E.
- Expose immutable UPLC AST types, text and Flat codecs, serialized-script codecs, cost-model
  construction, default machine costs, and budgeted evaluation from `@xray-network/cardano-plutus`
  and its `./uplc` subpath. Expose typed Data through the root and `./data`.
- Expose `apply_params_to_script` and `eval_phase_two_raw` plus their result contracts from the
  Cardano Plutus root.
- Treat the primary Plutus source as authoritative for language/Flat/machine behavior and Cardano
  Ledger as authoritative for Alonzo, Babbage, and Conway script discovery, arguments, contexts,
  cost-model selection, valuation, and errors. Where they differ from Aiken, follow the two official
  sources.
- Make protocol major version explicit. Support majors 5 through 11 and derive the raw transaction
  era as Alonzo for 5-6, Babbage for 7-8, and Conway for 9-11.
- Evaluate every redeemer independently with the supplied maximum transaction budget, ignoring its
  encoded ExUnits when calculating the replacement. Return calculated cost in CPU-then-memory
  order and encode ledger ExUnits as memory-then-steps.
- Require caller-supplied cost models. Preserve official current-model behavior: signed 64-bit
  values, ordered parameter names, ignore extra tail values with a warning, and fill missing tail
  values with `INT64_MAX` with a warning. A missing active-language model is an error.
- Preserve protocol/language availability, successful-return rules, the V1/V2 historical script
  CBOR-remainder compatibility exception, V2+ constant wire-size checks, and protocol-11 universe
  header and constructor-field limits exactly.
- Reuse XRAY Cardano Lib's lossless CBOR, Plutus Data, transactions, scripts, cost models, redeemers,
  hashes, and builder owners. Do not create competing public nominal ledger types.
- Keep all runtime code browser-safe TypeScript. Additional SHA-2, Keccak, RIPEMD, secp256k1, and
  BLS12-381 operations belong to the crypto package and follow ADR 0004.
- Keep Plutus tests below `libs/typescript/packages/plutus/test/` and ensure the root test command discovers them.

## Excluded source material

- Aiken or any other third-party implementation as normative or captured evidence
- PlutusV4, protocol major 12, Dijkstra-era nested transactions, and provisional ledger behavior
  absent from the primary Plutus release
- UPLC pretty-printer, optimizer, compiler, debugger, or protocol override APIs
- Upstream Haskell, Rust, native code, WASM, generated JavaScript, bindings, binaries, or runtime
  data loading
- Full ledger phase-one validation, balancing, fee selection, and transaction construction
- Generic Cardano CBOR, ledger, Data, key, hash, or signature wrappers already owned by XRAY Cardano Lib
