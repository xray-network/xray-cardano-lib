# Official Plutus UPLC provider snapshot

Provider-Snapshot-Version: v1
Snapshot: 0001
Provider: uplc
Created: 20260727T100703Z
Previous-Snapshot: NONE
Source-Type: git
Source-Repository: https://github.com/IntersectMBO/plutus.git
Source-Commit: 91e8c2af9c7bec705b035c6cf8e679c35c4c2ad5
Source-Ref: refs/tags/1.66.0.0
Source-Tag: 1.66.0.0
Source-URL: NONE
Source-SHA256: NONE

## Evidence objective

Preserve official Plutus UPLC and Cardano Ledger phase-two evidence for protocol majors 5 through
11.

## Comparison source

Cardano Ledger commit `a624de4c8db7286a6c065da149679ea55f7d5629` from
`refs/heads/master` is captured separately within this snapshot for ledger integration behavior.

## Frozen capture specification

The rules below preserve this capture’s original source selection, mappings, transformations,
completeness, licensing and consumer boundaries. They apply only to the immutable identities
recorded above. Discovery/ref and future-planning statements describe the original capture context;
they cannot retarget this snapshot. Repository-root paths remain repository-root-relative;
`artifacts/` paths resolve from this directory. No mutable provider guide supplies normative rules.

### Purpose

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

### Primary source

| Field | Value |
| --- | --- |
| Repository | `https://github.com/IntersectMBO/plutus.git` |
| Followed ref | Latest stable release tag matching four numeric components |
| Revision policy | Full commit named by the highest stable release tag |
| Source mode | Immutable captured evidence; discovery policy below does not change the recorded commit |
| Submodules | Not part of the source |
| License | Apache-2.0 |

The resolved full commit is authoritative. A branch head, release page, or tag name alone is not.

### Cardano Ledger comparison source

| Field | Value |
| --- | --- |
| Repository | `https://github.com/IntersectMBO/cardano-ledger.git` |
| Followed ref | `refs/heads/master` |
| Revision policy | Full commit reachable from the followed ref |
| Source mode | Immutable captured evidence; discovery policy below does not change the recorded commit |
| Submodules | Not part of the source |
| License | Apache-2.0 |

Record this repository under `Comparison sources`, not as the provider source or previous
snapshot. Its captured files are authoritative only for ledger integration. If its supported
Plutus language or protocol range exceeds the primary Plutus release, the primary release bounds
the implementation and the newer ledger behavior is recorded as excluded.

### Artifact selection

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

### Evidence-only paths

Inspect, but do not copy:

- release metadata, Git history, Cabal/Nix files, lockfiles, and dependency manifests;
- formal specifications and metatheory for terminology and exclusion review;
- parsers, pretty-printers, optimizers, compilers, plugins, and code generators;
- Cardano Ledger era modules outside the selected phase-two/context paths;
- `.gitmodules` and both source-tree inventories for file-type validation.

Do not run any upstream hook, test, build, script, package manager, executable, generated program,
filter, or submodule.

### Consumption and planning requirements

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

### Excluded source material

- Aiken or any other third-party implementation as normative or captured evidence
- PlutusV4, protocol major 12, Dijkstra-era nested transactions, and provisional ledger behavior
  absent from the primary Plutus release
- UPLC pretty-printer, optimizer, compiler, debugger, or protocol override APIs
- Upstream Haskell, Rust, native code, WASM, generated JavaScript, bindings, binaries, or runtime
  data loading
- Full ledger phase-one validation, balancing, fee selection, and transaction construction
- Generic Cardano CBOR, ledger, Data, key, hash, or signature wrappers already owned by XRAY Cardano Lib

### Verified artifact inventory

This table freezes all 140 regular artifacts, including existing control files. Paths are
relative to this snapshot. Hashes and sizes were verified during the metadata conversion on
2026-09-09; this local verification does not claim a new source-tree capture or upstream audit.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `artifacts/SHA256SUMS` | 18341 | `b98a166763ea1f1be2922be663137a0b318d607411c070d4416bbcd5feeb8df8` |
| `artifacts/cardano-ledger/LICENSE` | 10174 | `0d542e0c8804e39aa7f37eb00da5a762149dc682d7829451287e11b938e94594` |
| `artifacts/cardano-ledger/NOTICE` | 575 | `58721f8b6ca67f0fcbe1cd739b384fec3126a35f7d79951aedaa2bc3863a3162` |
| `artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Context.hs` | 13713 | `46658c5dc9e9e8b983694748018b03c87da2ba9785c95df8b6cf5c2a2ab42a63` |
| `artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Evaluate.hs` | 13718 | `aa09e9ad884b4b097b37c0ea4d99d71fa631afeee851ebb632e7534dfc7addc2` |
| `artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/TxInfo.hs` | 14736 | `012494cd3a95706442cf8d9652e1eef98600270dcb2da17a23b9bc654dda5152` |
| `artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Rules/Utxos.hs` | 14029 | `0445cb18a2f4d9cbc77e6252142dc333e46f6f01237d0f556526e77f1fc3853d` |
| `artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Scripts.hs` | 25182 | `c5d9817ff60e3fcb1a16c2c17a1352cad4daf67889ba8ef9ccce70bd3f0460f3` |
| `artifacts/cardano-ledger/eras/alonzo/impl/test/Test/Cardano/Ledger/Alonzo/Imp/TxInfoSpec.hs` | 2838 | `0a9724dd41950b138c2d6457bfdada1ef27553b0f651ca6a3e98e85eb7435e41` |
| `artifacts/cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Rules/Utxos.hs` | 7983 | `52885862cb04b6ebcd7440de1ed645fe75423cd0bc697a267cb50b6472bb0317` |
| `artifacts/cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Scripts.hs` | 4530 | `5a2ec0c145872e202ca69626e495dcc0a815100c74036bf1fb4f92a9c69ae113` |
| `artifacts/cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/TxInfo.hs` | 15196 | `fd8ccc004a7a073d8386d3cb5d774677e93e3d887df69233b3785e7efac6dce5` |
| `artifacts/cardano-ledger/eras/babbage/impl/testlib/Test/Cardano/Ledger/Babbage/TxInfoSpec.hs` | 12944 | `0769977e970e33b13a94dbdab81a845dd5e7b5b2a9a35b3dab9a0f3379cb849b` |
| `artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Rules/Utxos.hs` | 7897 | `05b05bab4e821e32b96ed66767bbedafb5d83b82aa8539d09c8da5b2fce7e56a` |
| `artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Scripts.hs` | 10595 | `09c3ea360bffd0c3a52e473bb0bdb765b62b50c6f9d9b92c9bcdfdad5636f1c0` |
| `artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/TxInfo.hs` | 32404 | `127509935f8dc336e136619ea5d3f11612a34f07e497944ccae9edb39991b4a6` |
| `artifacts/cardano-ledger/eras/conway/impl/testlib/Test/Cardano/Ledger/Conway/TxInfoSpec.hs` | 4200 | `a4526cec991c15f9a492db4ef5d185aa0aeff7c11eaabb6deeab04458038806d` |
| `artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus.hs` | 504 | `009292938ecd818883aacd9aed07fa51abd553a04f1d26d0b9d6895eb5423654` |
| `artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/CostModels.hs` | 19684 | `5fd2ab01dabc4a7998b46f1bce8a8db2ddd89912fc6d680416d5f9d5a1becf47` |
| `artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Data.hs` | 11013 | `6cb38292ff6db086ba97b771df7b3480383ff986720c7dce1059d4f3864e7301` |
| `artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Evaluate.hs` | 16992 | `733bd37b4f534f3c8a70d53b007addd2393ccb4b6e1a6a3e780cc0682ed8d966` |
| `artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ExUnits.hs` | 7247 | `618964660c3e7445367563440c4c1994b8c50f6e129d7185cd05fb87a2a9be4c` |
| `artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Language.hs` | 20784 | `4810809113145806885f54154bb3fbdb2f553001a9fdc35730b348391d92ea06` |
| `artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ToPlutusData.hs` | 5814 | `509183c7b0ae1b1fea4b4c9ae0fa44b1575a93a675882fc17a84004079079e67` |
| `artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/TxInfo.hs` | 7286 | `0f358b95a13e726318ec1dd8677a296fb5378416143250ac882a6384b30a47b7` |
| `artifacts/conformance/README.md` | 1192 | `8b60dbbce1c2ee21f88eec91ed9e4948e9f00e2aa27d297177d2774ed4f79a88` |
| `artifacts/conformance/corpus.json` | 1827347 | `d1fb60fcd1f680667fe7133c292c24a8b484d1ba0585622d855f0aacb31d1ed5` |
| `artifacts/plutus/LICENSE.md` | 11356 | `43070e2d4e532684de521b885f385d0841030efa2b1a20bafb76133a5e1379c1` |
| `artifacts/plutus/NOTICE.md` | 560 | `9e1fe670abe06d2cbdffa2d724765b5e86aa7f65d8eccd34b391a37fbf7bab89` |
| `artifacts/plutus/README.adoc` | 4198 | `47811f8c530b2a96db0b347be916f6a4063948b2e4e834148cdf0fdb1ca112ed` |
| `artifacts/plutus/plutus-conformance/LICENSE` | 9139 | `69ce94606a661fa3290eeaca21f3f9ad7d73dc985ffa346a1104704548f72d93` |
| `artifacts/plutus/plutus-conformance/NOTICE` | 593 | `9f5112659daf50eec7866133b2c11432547d132ec8253121ffc8503970197b34` |
| `artifacts/plutus/plutus-conformance/README.md` | 7591 | `8bfe738336b8fc58745b18818082215d3b866e7a0651186af1b75b0d8c20f2ec` |
| `artifacts/plutus/plutus-conformance/src/PlutusConformance/Common.hs` | 11944 | `aea3b958aca12484855a12814ccdb6b43825bc56f77d24ccb5e9cea257574acf` |
| `artifacts/plutus/plutus-core/cost-model/data/builtinCostModelA.json` | 23728 | `0f6faf14e0d54a05bbcc3a3bd697b3bbe19e8b002f5f675f665c33e61522859a` |
| `artifacts/plutus/plutus-core/cost-model/data/builtinCostModelB.json` | 23711 | `c78c4490e04959f5979a5c0e3b81cf29f9dfe352705d21da7fab7e1a2a6e9c96` |
| `artifacts/plutus/plutus-core/cost-model/data/builtinCostModelC.json` | 24153 | `1556b0d8ee1652758a35fed8f894cef2335fab248575101aa0d1226fb2c996d7` |
| `artifacts/plutus/plutus-core/cost-model/data/builtinCostModelD.json` | 23711 | `d019c35e677d27124f8ff05e3835f9e2f538a50f1139676d9678c1779b30f0d0` |
| `artifacts/plutus/plutus-core/cost-model/data/builtinCostModelE.json` | 24161 | `56abe89beca140e9e9bbb7f573829397a3021d6340e22f868406dc4c10df83f9` |
| `artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsA.json` | 671 | `5457650b1e138f59888f2f7c486d8137fa0aa3c119b25931d130bbfd73e4a56f` |
| `artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsB.json` | 671 | `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` |
| `artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsC.json` | 671 | `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` |
| `artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsD.json` | 671 | `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` |
| `artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsE.json` | 671 | `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` |
| `artifacts/plutus/plutus-core/plutus-core/src/Codec/Extras/SerialiseViaFlat.hs` | 3725 | `0ed6622daa30737f5691f78056e3ff25538eab7e9c249273df180ca297b37d0c` |
| `artifacts/plutus/plutus-core/plutus-core/src/Data/Vector/Orphans.hs` | 1093 | `2d017a10136c1b516dfe0acdf501b67662495100540da982e532a32c59cf0381` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Bitwise.hs` | 57835 | `a18717fb9199d9489191372baa5974114a3b4ec90d15b6d8c9fe60e77f5d8ea9` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin.hs` | 613 | `8b20f7142c91dd12521e14f896ad9b9319813031e1a8271ecb06be3af8b1b2a6` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/KnownType.hs` | 22725 | `d5c08abc0be1a33663c708799a3aaba74d41348774696567f09b59b9ce64860a` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Meaning.hs` | 22056 | `e5ad4d68a2a8ab3aa71427ddfb37ad649f3f6a8fabd4b20e334aed816a05e5d9` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Result.hs` | 9734 | `79f8202a565a883bfc7e5aa033228d0243b214cbbe982c0450202d32b97a663e` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Runtime.hs` | 5624 | `29f6241a275645f5c339c96edebbfbfbaaeb3fc47e832e1165b63c234d8cf290` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/TypeScheme.hs` | 4473 | `b81f6fb30ae2b12f36e1fc74d7f9058a6e0386d5e8787f2509ee235e89370cf3` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Bounds.hs` | 651 | `8255a49552e7d28781c21c238370dff4b01d8fcb9571f514c3889bccb11eac21` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Error.hs` | 157 | `b2c02ed3ce7646902885f1727cf2654bd18ec0a428392b1938f3914652208a7e` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G1.hs` | 8427 | `4c497eb561d8485759069c1c719d56c1f879ef890a3dedb62b6c63f8694bc152` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G2.hs` | 6057 | `1ea930741576a7703515719edc4d92f5746a3e639ca1a306dd0d3c7330da8d18` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Pairing.hs` | 2680 | `8403c461db988da5f303b35adfd13c099798681ab23ed88cf30fbb9d842dfd4a` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Ed25519.hs` | 1302 | `9a6fa8a721e28d90483a6f23347e0f0aaf4cec7028e1717e7b586c7fcf028259` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/ExpMod.hs` | 1574 | `697727fe1e2aaa7c37f1abb3cc8d71d5333163fdd218d383fcce3f2b0eacaf08` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Hash.hs` | 1363 | `40d8f6e118f4d0e1f0f72714d19d3322e095691ec69c09a4118544f8acbd3113` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Secp256k1.hs` | 3730 | `e25b37a1056a14342b9f0e5591c9f9c0362b8c0edf1f4c5354bd83bbfcb7e644` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Utils.hs` | 662 | `06a2dae624cc3df9e02e64ad24d599c5485b12810a41381c2cd8cbe58bc463cf` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Data.hs` | 12814 | `e4633b7b293b4078387b107eb36d1f16864b93b7a129a7a2531493791fb5628c` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/DeBruijn.hs` | 9418 | `ae7d44eaabdc5b58fbb01898c3472972ba69df9d46de0c785b515392082c48b2` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/DeBruijn/Internal.hs` | 13569 | `55b2928e0d2d0af4a246bfa21a790270af243f3b9d9336490ef1468d223053ec` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default.hs` | 198 | `e11984b4e05701e101283742639fae96bb3d0c67122f79eb6ac771c0d74ee76d` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default/Builtins.hs` | 124051 | `5ee08d1f385a3372438d7fd2575ddcd32a2add779b9bb81fc45cd0ed4e5d0393` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default/Universe.hs` | 39224 | `88d257b863f3f5a70971a19b431725610cceec623ec132e2b8344a9efa2cc517` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default/Universe/Cardano.hs` | 740 | `353c896bb289591bb6e6ca323b63fa55bf7563fc77cd63f462aa61b74c031f3d` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Error.hs` | 3234 | `6b5d8035b535264fc133af41782ec5d03ea7baa40ebd0736db9367dc0e14c456` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/BuiltinCostModel.hs` | 9612 | `49d80fe1636117d57268cf997c1abe9a8a23efa9981574246045a6d088101610` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostModelInterface.hs` | 19435 | `f3286eacee23ab160a285195af0caf0c215f7395e0e6c88fcffe564bc96f925d` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostStream.hs` | 6652 | `46b33a311e77e1335dd4688e5e94cb966a48416ecdca3fdf32ea605af8116c3b` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostingFun/Core.hs` | 38655 | `acf2d2dd373d8763416d04125a8cc9a85a2bf59aebdf67859079eedc4b77e61b` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudget.hs` | 10894 | `45947e92230654f900799ff1c3d437e3bc287dc75f36c55a0d4567e45823c54e` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetStream.hs` | 3560 | `0f21b09e0252825c53ef83eeb19e23a8dd85fc269da495d172390b5ec4333da8` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetingDefaults.hs` | 19277 | `03fd2656f8c8aa82c5bedda46350bac03aed37b48d272795dd130e97c71a91f5` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemory.hs` | 5001 | `d6da66e49a1e38abd7a745999eb884433afc3a2306a73808e7016a5238ba7dac` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemoryUsage.hs` | 24245 | `63b72128955389b0fc4134f5559b6521a69020d87d186df25790b57822866027` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/Exception.hs` | 6348 | `58140a97be5898cbe0cb6057a9eb55f4c48c0b4d09cbe69bcc1737111f917cc1` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters.hs` | 5317 | `e63180de6672fb5e16f974a00f4678f9e89ca07220284ff62460d1dcf724cf34` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters/Default.hs` | 4673 | `a8fff1b96f9b014b4a2f9b459ba89659da090d7a796215ef382bb899bb3ce92a` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/SimpleBuiltinCostModel.hs` | 1507 | `8b0dd3fc51ba9ef980a6a5f2fbe2ddc08aedb4d96070362b8d4a54be912ab1ef` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/FlatInstances.hs` | 15431 | `693e1fa70b419e73de486bafec3f01d6e574ed0896a345d5688bc1925ed5107d` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/MkPlc.hs` | 12524 | `64ddf39f5a2a46114ead6113d95a76abcf24fc62a368315de5e06b4748a6aa50` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Value.hs` | 21808 | `325cd37de6c967f985c740e25fac30f9b68b9b4528af541d1294a63dd6883fd1` |
| `artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Version.hs` | 2751 | `f0a48a92483a323fab9caa502de11850362fb4c7cc7cc0fd4e102486dd137e7c` |
| `artifacts/plutus/plutus-core/plutus-core/test/CostModelInterface/Spec.hs` | 10888 | `fa0ea6df8addab0b586b24c407322ce16013899537097639cc2c863954328687` |
| `artifacts/plutus/plutus-core/plutus-core/test/CostModelInterface/defaultCostModelParams.json` | 11576 | `a47727bcdf68efeceac8c49f032d19682a374c14fede64392203d92279d3534f` |
| `artifacts/plutus/plutus-core/plutus-core/test/Flat/golden/encoding-stability.golden` | 381 | `794d5bdf0baa30dbe74478d3a414dc015e591456c7c99ca0859bff496325302d` |
| `artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Check/Scope.hs` | 1872 | `60442f821052511b5ccd73bca1d382570f17f8d4b719e8ff8c6b9cec24b0497f` |
| `artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Instance/Flat.hs` | 10756 | `ee762bda65323a0c6f270cad06b198b39728530980703ae235af26f695f32188` |
| `artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Type.hs` | 7857 | `902bb51a5ba193a5f7947b08c9b61d1632b358ddb9ac0bb7996841d51576f275` |
| `artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/DeBruijn.hs` | 4158 | `22f6cb8f0da939345c3affd221ecea005f3fa676ccf7bd388770831a2066ac56` |
| `artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek.hs` | 3842 | `567caff202fe0813c929c1af12e475fdaaf586cc75b89679c8630f96c827331c` |
| `artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/CekMachineCosts.hs` | 3579 | `96a1fa30b7736a1ed3674ddc200298033edc6ded4ba34c9f4b516b69526a32f7` |
| `artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/EmitterMode.hs` | 4403 | `52e453ee3b1d1378c570fe613b5a81ee25d81b9532ca9f1cf8ab7258614a62d6` |
| `artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/ExBudgetMode.hs` | 6792 | `dc79de42f831b3c197f4fb6d51f5f7f9b2381f9da06022bf8e359311ed35aa81` |
| `artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/Internal.hs` | 52018 | `f7483a9661dc5ff5291140dc4c6f4609911e6f6d5f54b043bf98a0cb88090527` |
| `artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/CommonAPI.hs` | 6213 | `20fd7fdccd4fb00c86ea158341f4802acffc03aeb03cb53a21083970fbdf2804` |
| `artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/MkUPlc.hs` | 794 | `f1eec0e2237a8e1d8065011a3cf51b159255124c63d0937037d9609cc1bfb7e2` |
| `artifacts/plutus/plutus-ledger-api/CostModel/Params/CostModelParams/costModelParamNames.golden.txt` | 12554 | `93898d8fd636685fae59dbdb861d2e64e8d45700e0fcc9f5eda584c65f6387f8` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Eval.hs` | 14193 | `9844d259cff614a988a042fff430ede2f9670f1757a06c5c0953c3900f17935b` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ParamName.hs` | 4859 | `cb63e7b54aafb67b3e69ef113845613db2eef40733b6a385d90d3a53ef3dcf28` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ProtocolVersions.hs` | 5076 | `ce06ce9bf577415903fa8241293a6a94f7b1f1452a4f9c570303e8f91b841f2b` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/SerialisedScript.hs` | 11490 | `7feb2ddd0001d48e81db2246a2fda592f2b6cae5f98422cfcbb744b7befdd412` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs` | 12333 | `137a302960f24aeb8b5ded13fd8431194876bde123bc27c5d030dc4e038c87c9` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V1.hs` | 6490 | `f0edecd396506e2e1aa38ff32e0ab9065a804cad76b79275cb721d43c652a231` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V2.hs` | 7361 | `d65557b5ca35ec395d785a6e9dcd337d39d19869c8a68f4d132dc2619fcbcced` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V3.hs` | 9814 | `47f85dfe0013df3adc94e032f7df591cc5f06c9d0451add0fb0162d2e2f0fe65` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/MachineParameters.hs` | 1458 | `7f76270784a1482ab1e38210ffa57510a542aee662cb614ac4dd05191c56d462` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Contexts.hs` | 11440 | `6b4d6c913959ff70e38b6fa6b0053d4bab957e7b7d7b1443c2d54c97092b332f` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Address.hs` | 3283 | `eb68fad824b70953c5b107bf5ed883d2475d915c8c0c176fd8f18d53a708161c` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Contexts.hs` | 11069 | `501df38b935fda1039d543bb341a1b2c720f833f6a98bbe5948d1e71a0eff6b8` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Credential.hs` | 3610 | `9930b6b0097aa83646defc958b82e71f47c14cb62fe4b2d7f9ced26235eefa81` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/DCert.hs` | 3607 | `858c7a0b2e58b6309b860be8c78b0f18fffe211ec1e258e91307f9593d48ec1d` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Interval.hs` | 19367 | `243952f5d415c106f4db54b477d0788ea7aec9c99014057e3e888e6576a78b8f` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Time.hs` | 3293 | `7286d160c1de7c69c00212c23176891229ed0286b3556b55ce0d3ddd5689b116` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Tx.hs` | 5545 | `782d3a6f7ee4c2f88d5c30165332b428c77f4bd80f8e4d2f272438c6fcf5a84e` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Value.hs` | 25984 | `5baed5f10b100d64622520ff62b905deebbce5207a4416a4841d11c255c7bb0b` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/EvaluationContext.hs` | 2115 | `cc3f68cfa670dea838715512597d32c1bce1c77515f8fcef6632fc4e640bfb63` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/ParamName.hs` | 13737 | `3a39cec1e520a42d860ad1ca9f2a9213e878a6e97dd00b35acedeccd938914db` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Scripts.hs` | 6857 | `562806a9b75d48187d8a1c4ac66778c0c7e71001a766d37061075504e32618dc` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Tx.hs` | 5972 | `95e69701c55f3762ff6d6c07127b7a78ad30e6b12fdc572bdbc6727e9d6a311f` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Value.hs` | 24703 | `046b52475911fa325ad909a06d87fcbdea47ace874a4280400f6e9e7c118b565` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Contexts.hs` | 10918 | `ca07745f836b291009a7df9986ceb1ae12f9605938222d8b370243194cc34767` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Contexts.hs` | 11397 | `81ecd549083deb3b436b96468545463fe9fff6b95bdbe8d8f3e7e809654ac31a` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Tx.hs` | 4629 | `d01f63d4d292795350cfdc5432ae418638c9057ae0fd8f8abe5f8f17d05a975c` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/EvaluationContext.hs` | 2115 | `5636a676507259548d9214d2493b9b442966350e30ed38dc8390d502a9dded1d` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/ParamName.hs` | 13846 | `5d5a8e3a1f4623a081b3812aef9f30c66c73bb3d39807abe1b3262a2f77402bc` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Tx.hs` | 4786 | `484e75d3c04874a034c5c7c07f1c741f8179a519f624e5f1ea02f3bac49c773e` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Contexts.hs` | 25064 | `206fe185a66c79df8f095b350b4d23796220001a254ab687a9893ce21b301af4` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Contexts.hs` | 23719 | `d0d049d095d1e91480ecfaa9e83e18afd19b49ebe3d2880638efa1d16c9b0261` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/MintValue.hs` | 4548 | `dd6674980a22a4e9d01a535652b03bef434b6c2ddd9f0f85d234a0d244786f02` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Tx.hs` | 3152 | `e6e942aba7f44cc03088455208e858638191a0ca0e41dbc6e1edf98aa5daa268` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/EvaluationContext.hs` | 1922 | `efc693f86c5b803f694d7a683f1fe53851f2a16e690fb1d2573a369decb23268` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/MintValue.hs` | 5394 | `0278e5fa319658f50ffad9ae856908ad3c483094a1a29b1ce1483c0e3c10aeea` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/ParamName.hs` | 14633 | `ebf3d1c938af1168f8e79f816e638366a89d2c6651e11e13bce5d2c039052369` |
| `artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Tx.hs` | 2934 | `e312e1cb5ab8e2b4bd48402235f2daf6e999d6c205a9a5c823f65fe954239313` |

## Captured scope

- 113 byte-exact Plutus metadata, conformance, Plutus Core, and Plutus Ledger API files below
  `artifacts/plutus/`.
- 24 byte-exact Cardano Ledger phase-two and context files below
  `artifacts/cardano-ledger/`.
- A deterministic 3,013-entry conformance corpus in `artifacts/conformance/corpus.json`.
- Snapshot-local corpus documentation and `artifacts/SHA256SUMS`.

The final inventory is 140 regular files. The checksum inventory covers every artifact other than
itself.

## Integrity and licensing

The exact selections and deterministic corpus transformation are defined by
[this frozen capture specification](#frozen-capture-specification). The captured Plutus and Cardano Ledger materials are Apache-2.0.

## Semantic evidence

The evidence covers UPLC 1.0.0 and 1.1.0, Flat encoding, CEK evaluation, builtin tags 0 through
100, cost models, protocol/language availability, Plutus V1/V2/V3 contexts, and raw transaction
valuation. It is evidence rather than runtime code.

## Exclusions

PlutusV4, protocol 12, Dijkstra, compiler and optimizer functionality, full phase-one validation,
third-party normative sources, generated bindings, native code, WASM, and upstream execution are
excluded.

## Change summary

| Change | Evidence | Observed significance |
| --- | --- | --- |
| Baseline | [Captured scope](#captured-scope) and [artifact inventory](#verified-artifact-inventory) | Initial same-provider snapshot; no previous snapshot exists. Original comparison-source observations remain above. |
| Metadata conversion | [Migration provenance](#migration-provenance) | Capture rules are now self-contained; artifact bytes and source identities did not change. No new upstream comparison was performed. |

## Consumer impact and recommended work

Migration-time guidance, not a newly performed upstream audit. Existing captured semantics and
consumer boundaries remain authoritative; this metadata conversion requires no runtime change.

| Evidence | Maintained target and owners | Validation | Recommendation |
| --- | --- | --- | --- |
| UPLC codecs, evaluation and costs; [semantic evidence](#semantic-evidence) | TypeScript: `libs/typescript/packages/plutus/src/uplc/` | `libs/typescript/packages/plutus/test/conformance.test.mjs` | No implementation change from this migration. Compare a separately captured update before planning behavior changes. |
| Script discovery and transaction contexts; [semantic evidence](#semantic-evidence) | TypeScript: `libs/typescript/packages/plutus/src/ledger/` | `libs/typescript/packages/plutus/test/ledger.test.mjs` | No implementation change from this migration. Compare a separately captured update before planning behavior changes. |

Behavioral evidence is language-neutral. C++ is an unmaintained, opt-in consumer.

## Unresolved questions

No fresh upstream completeness or delta audit was performed for this metadata migration.
The inventory verifies stored bytes; original capture claims are retained as historical claims.
Any future update must independently enumerate its pinned sources and resolve semantic mappings.

## Migration provenance

- Metadata conversion date: 2026-09-09, explicitly requested by the human while SPECTRE 1.0.0 is in development.
- Previous snapshot descriptor SHA-256: `a2fc7e20ffaedf73aee9f05f09dedd20d7b4ac012a1902d14c438b0d5678d123`.
- Original applicable capture-rules SHA-256: `2a673279a264236309bb355fdbc47c14b8eec9613b8d35cf794b45111a2ceb7b` (historical label v1).
- Embedded the original applicable rules, with snapshot-relative scope and obsolete provider-version
  routing removed. Added this exact artifact inventory and clearly labeled migration-time guidance.
- Corrected the initial snapshot’s self-referential Previous-Snapshot to NONE; filled inapplicable
  URL source fields with NONE. Preserved Created, snapshot ID, source identities, all artifact paths
  and every artifact byte. No new snapshot or upstream capture was created.
- The converted descriptor and its artifacts are frozen after this migration. Future changes
  require a new numbered snapshot with its own complete specification.
