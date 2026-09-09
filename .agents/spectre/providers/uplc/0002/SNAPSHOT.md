# Official Plutus UPLC provider snapshot

Provider-Snapshot-Version: v1
Snapshot: 0002
Provider: uplc
Created: 20260909T153557Z
Previous-Snapshot: 0001/SNAPSHOT.md
Previous-Snapshot-SHA256: b74c009eb9ebd6f42ae8e63c7fc902dc477a50b9d38563b3fda9bf2c8a82db5d
Source-Type: git
Source-Repository: https://github.com/IntersectMBO/plutus.git
Source-Commit: 9e17e2404dc6988c908b1fea099dde202df73b6a
Source-Ref: refs/tags/1.68.0.0
Source-Tag: 1.68.0.0
Source-URL: NONE
Source-SHA256: NONE

Capture-Summary: CAPTURE.md
Capture-Summary-SHA256: 39b65844f3ed7ba66da8e453b9bfde654591c995cd801cdbb9d3f1997d02cb92

## Evidence objective

Capture the latest stable official Plutus release and independently resolved Cardano Ledger
integration evidence. Compare with 0001 and the current maintained TypeScript implementation,
including its recently added text/Flat corpus reader, to identify the remaining adoption work.
This captures evidence; it does not switch the library’s pinned corpus or implement new semantics.

## Comparison sources

| Role | Repository / immutable identity | Selection and license |
| --- | --- | --- |
| Ledger integration authority | https://github.com/IntersectMBO/cardano-ledger.git, commit `ae2c8912b204d11e102faeb617b6faa6a8c9f934`, refs/heads/master, tree `a74d35187f5680451f7401b5b098bdafa1eb315f` | 24 explicit files below, Apache-2.0 LICENSE and NOTICE included |
| Previous same-provider snapshot | [0001](../0001/SNAPSHOT.md): Plutus 1.66.0.0 / `91e8c2af9c7bec705b035c6cf8e679c35c4c2ad5`; Ledger `a624de4c8db7286a6c065da149679ea55f7d5629` | Frozen prior 113/24 selection and 3,013-entry corpus |

Ledger is independently pinned comparison evidence, not the previous UPLC snapshot or authority
for UPLC builtin implementation. The current TS source is consumer context, not another upstream.

## Complete capture specification

### Source selection and mapping

Plutus was selected by enumerating all official tags and choosing the numerically greatest stable
four-component tag, 1.68.0.0. The full commit above points to Git tree `e4b96129a4c755afc4d6651a0287009edd6ddced`.
Ledger master resolved independently as above. Both complete recursive Git tree responses were
non-truncated; selected source membership was established before downloading artifact contents.
The 113 Plutus and 24 Ledger path allowlists retain 0001’s exact bounded source selection.

Every listed source below is a regular Git mode-100644 blob preserved byte-exact under its logical
artifact path, resolved through the complete physical inventory below. All regular blobs recursively below Plutus `plutus-conformance/test-cases/` are separately
selected for corpus.json: 4,821 mode-100644 and one mode-100755 blob. The latter is inert encoded
support data, never executable tooling. Complete corpus source membership is the ordered `entries`
path inventory in corpus.json prefixed by that source root; no extension or feature filtering occurs.

| Source role | Exact upstream path | Logical artifact path | Bytes | Git blob SHA-1 |
| --- | --- | --- | ---: | --- |
| `cardano-ledger` | `LICENSE` | `cardano-ledger/LICENSE` | 10174 | `f433b1a53f5b830a205fd2df78e2b34974656c7b` |
| `cardano-ledger` | `NOTICE` | `cardano-ledger/NOTICE` | 575 | `f4b8daf6a77e50caeed70c3094e4ec32cfc6de64` |
| `cardano-ledger` | `eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Context.hs` | `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Context.hs` | 15638 | `b9b4e70853161f6f94edf372009a15fc78420694` |
| `cardano-ledger` | `eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Evaluate.hs` | `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Evaluate.hs` | 14164 | `7325e4619646011a22ee89fe4187851bdcbf728e` |
| `cardano-ledger` | `eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/TxInfo.hs` | `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/TxInfo.hs` | 14959 | `65e0a629690e30a00418ba235b66b0005cd0f8c9` |
| `cardano-ledger` | `eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Rules/Utxos.hs` | `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Rules/Utxos.hs` | 13948 | `787258f86cd709c9dbe3a32dc13e9f0c11c9f8d3` |
| `cardano-ledger` | `eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Scripts.hs` | `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Scripts.hs` | 25897 | `cec2ce060248e67da75fc68ee0296b86417a05c5` |
| `cardano-ledger` | `eras/alonzo/impl/test/Test/Cardano/Ledger/Alonzo/Imp/TxInfoSpec.hs` | `cardano-ledger/eras/alonzo/impl/test/Test/Cardano/Ledger/Alonzo/Imp/TxInfoSpec.hs` | 2474 | `cece3069ff88a34429ad84ff16a780b98c107229` |
| `cardano-ledger` | `eras/babbage/impl/src/Cardano/Ledger/Babbage/Rules/Utxos.hs` | `cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Rules/Utxos.hs` | 8001 | `37c3e341e055abdbc28cef0397c8872e8fc151bf` |
| `cardano-ledger` | `eras/babbage/impl/src/Cardano/Ledger/Babbage/Scripts.hs` | `cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Scripts.hs` | 4530 | `1527d180c020ddb62f168e8105809092d87a51e9` |
| `cardano-ledger` | `eras/babbage/impl/src/Cardano/Ledger/Babbage/TxInfo.hs` | `cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/TxInfo.hs` | 15859 | `6613eb7faa5844568a4a10c933d4c67389966b17` |
| `cardano-ledger` | `eras/babbage/impl/testlib/Test/Cardano/Ledger/Babbage/TxInfoSpec.hs` | `cardano-ledger/eras/babbage/impl/testlib/Test/Cardano/Ledger/Babbage/TxInfoSpec.hs` | 12885 | `ca94c6b56e9925933bd8a46f4b6e810d0a3790f1` |
| `cardano-ledger` | `eras/conway/impl/src/Cardano/Ledger/Conway/Rules/Utxos.hs` | `cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Rules/Utxos.hs` | 8039 | `5a18f3fb544fb45e1c31ab9a25364251d6b2cef1` |
| `cardano-ledger` | `eras/conway/impl/src/Cardano/Ledger/Conway/Scripts.hs` | `cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Scripts.hs` | 11623 | `8a007db63528de77165ad4d5002503371e15bd0e` |
| `cardano-ledger` | `eras/conway/impl/src/Cardano/Ledger/Conway/TxInfo.hs` | `cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/TxInfo.hs` | 33429 | `8e9acf9f93741462745f6369d5825bbb711f0253` |
| `cardano-ledger` | `eras/conway/impl/testlib/Test/Cardano/Ledger/Conway/TxInfoSpec.hs` | `cardano-ledger/eras/conway/impl/testlib/Test/Cardano/Ledger/Conway/TxInfoSpec.hs` | 4200 | `f0e9eede691ec2a33e274e1a724f32027ea1ad10` |
| `cardano-ledger` | `libs/cardano-ledger-core/src/Cardano/Ledger/Plutus.hs` | `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus.hs` | 504 | `ffb2f12191195268ae453bbf29b1b45b5517d563` |
| `cardano-ledger` | `libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/CostModels.hs` | `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/CostModels.hs` | 19684 | `134d26b86915dfe48e9d2d7c9e9683b1a616ca3a` |
| `cardano-ledger` | `libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Data.hs` | `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Data.hs` | 11216 | `e1ff833521709eda8612842dbad5f28358015363` |
| `cardano-ledger` | `libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Evaluate.hs` | `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Evaluate.hs` | 16332 | `9511882dc04cf72054e8480ee5e6a257d28e9a22` |
| `cardano-ledger` | `libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ExUnits.hs` | `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ExUnits.hs` | 7866 | `76e090672f8afa8cb0b900ef3feaaf0a1c2af60c` |
| `cardano-ledger` | `libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Language.hs` | `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Language.hs` | 22874 | `072186304f995a1c1ea3f4743c83e3682e7fe325` |
| `cardano-ledger` | `libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ToPlutusData.hs` | `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ToPlutusData.hs` | 6255 | `f59fc5ed91cbcf6f81c9ca1f22ebd6d71c5040c9` |
| `cardano-ledger` | `libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/TxInfo.hs` | `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/TxInfo.hs` | 7291 | `9dda5bbf7061dfe36a53923c60833b4ed3bc1a94` |
| `plutus` | `LICENSE.md` | `plutus/LICENSE.md` | 11356 | `f49a4e16e68b128803cc2dcea614603632b04eac` |
| `plutus` | `NOTICE.md` | `plutus/NOTICE.md` | 560 | `a0a74865bef037095ec1cc4bc16cf1c528aa4d41` |
| `plutus` | `README.adoc` | `plutus/README.adoc` | 4198 | `0b4f68c0c1435173f0166478d7e14d689d47fee6` |
| `plutus` | `plutus-conformance/LICENSE` | `plutus/plutus-conformance/LICENSE` | 9139 | `0c8a80022eaad1406ec334ff3017890b48f80120` |
| `plutus` | `plutus-conformance/NOTICE` | `plutus/plutus-conformance/NOTICE` | 593 | `7bfbc260968d015cbab257d4c228f559b8c69e55` |
| `plutus` | `plutus-conformance/README.md` | `plutus/plutus-conformance/README.md` | 11151 | `ba2a86bbdf5e0628f5b26756f18923e0d9e586b8` |
| `plutus` | `plutus-conformance/src/PlutusConformance/Common.hs` | `plutus/plutus-conformance/src/PlutusConformance/Common.hs` | 26579 | `6abe8d1d15c0f5cf118a07ea7591a7aa53faa64a` |
| `plutus` | `plutus-core/cost-model/data/builtinCostModelA.json` | `plutus/plutus-core/cost-model/data/builtinCostModelA.json` | 24019 | `3cf708c8960c1fd46b9c495bdaa56ccff78d2259` |
| `plutus` | `plutus-core/cost-model/data/builtinCostModelB.json` | `plutus/plutus-core/cost-model/data/builtinCostModelB.json` | 24002 | `6cbe5636d7ec7a197e73e983604d556d155713ff` |
| `plutus` | `plutus-core/cost-model/data/builtinCostModelC.json` | `plutus/plutus-core/cost-model/data/builtinCostModelC.json` | 24444 | `24071b72982f13674246cb634c27ef0d502c0b9a` |
| `plutus` | `plutus-core/cost-model/data/builtinCostModelD.json` | `plutus/plutus-core/cost-model/data/builtinCostModelD.json` | 24002 | `d3b6f2e45d1c920666ec8ae9edb7e6ddaaeb873d` |
| `plutus` | `plutus-core/cost-model/data/builtinCostModelE.json` | `plutus/plutus-core/cost-model/data/builtinCostModelE.json` | 24452 | `e225e1f580eae77836ebd9cfcb8dcf02f7f80e7c` |
| `plutus` | `plutus-core/cost-model/data/cekMachineCostsA.json` | `plutus/plutus-core/cost-model/data/cekMachineCostsA.json` | 671 | `27f4cd01140f1bbe4cb42bb42a3bdaa34ae0d8c4` |
| `plutus` | `plutus-core/cost-model/data/cekMachineCostsB.json` | `plutus/plutus-core/cost-model/data/cekMachineCostsB.json` | 671 | `428b872b202e26f4d2b04456b04834112c813293` |
| `plutus` | `plutus-core/cost-model/data/cekMachineCostsC.json` | `plutus/plutus-core/cost-model/data/cekMachineCostsC.json` | 671 | `428b872b202e26f4d2b04456b04834112c813293` |
| `plutus` | `plutus-core/cost-model/data/cekMachineCostsD.json` | `plutus/plutus-core/cost-model/data/cekMachineCostsD.json` | 671 | `428b872b202e26f4d2b04456b04834112c813293` |
| `plutus` | `plutus-core/cost-model/data/cekMachineCostsE.json` | `plutus/plutus-core/cost-model/data/cekMachineCostsE.json` | 671 | `428b872b202e26f4d2b04456b04834112c813293` |
| `plutus` | `plutus-core/plutus-core/src/Codec/Extras/SerialiseViaFlat.hs` | `plutus/plutus-core/plutus-core/src/Codec/Extras/SerialiseViaFlat.hs` | 3725 | `2d4b19d583c6d54e83a7c3172a38d08f5da8de38` |
| `plutus` | `plutus-core/plutus-core/src/Data/Vector/Orphans.hs` | `plutus/plutus-core/plutus-core/src/Data/Vector/Orphans.hs` | 1093 | `be95f1c8830beac826e6c83515927f269cdcd25b` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Bitwise.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Bitwise.hs` | 57835 | `7fb28575220ae66aa18b8eec703f3168cd3ce938` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Builtin.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin.hs` | 613 | `3b7279b9f4b60be655f733c844ddd60c3f54beaa` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Builtin/KnownType.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/KnownType.hs` | 22725 | `53da30087d8bb99ed62674663914ef494950c940` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Builtin/Meaning.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Meaning.hs` | 22056 | `b14b521803f3fe558d53be9859f070bd5faa401c` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Builtin/Result.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Result.hs` | 9734 | `852d51192b0a92156a13e190afef53c296534f88` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Builtin/Runtime.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Runtime.hs` | 5624 | `75a0a73bc238c41c1740c380679b75288ea57697` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Builtin/TypeScheme.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/TypeScheme.hs` | 4473 | `7a90da261c8ed5a0eb76c724bbf341d71199dbbb` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Bounds.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Bounds.hs` | 651 | `a923ed11cf10098ac0afee6f290e3db6d982e972` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Error.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Error.hs` | 157 | `35eb1d9a326e9a86628441ef73b426f559aa624f` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G1.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G1.hs` | 8427 | `c97bd17c3c13f6aa09ecc754db147bf57c1f6895` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G2.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G2.hs` | 6057 | `66e41dd09b1ea904acf60779a71272392beb30db` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Pairing.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Pairing.hs` | 2680 | `c5caf1eb233ef8c12e162c0842d6361020af10f6` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Crypto/Ed25519.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Ed25519.hs` | 1302 | `12a6aca1926fc39a460cfe9da1744c7bdf15b4ae` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Crypto/ExpMod.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/ExpMod.hs` | 1574 | `117cf573579e9efe6dcc7b488f4181562a2f0390` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Crypto/Hash.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Hash.hs` | 1363 | `a8104f8f1500f4b17796d5e076ad7eb100e719ce` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Crypto/Secp256k1.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Secp256k1.hs` | 3730 | `009fd80539c0097d54d1a9ff6b436685a2b81ad7` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Crypto/Utils.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Utils.hs` | 662 | `9cfe12d0441501a76b233f10f7087f6bf80ca10f` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Data.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Data.hs` | 12814 | `a994354fb3c87a55ed6d1b4b6b544c7cd94c1c60` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/DeBruijn.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/DeBruijn.hs` | 9418 | `1e7041319cf290e3811b4299f057906125a23126` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/DeBruijn/Internal.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/DeBruijn/Internal.hs` | 13569 | `0a3a811c3b3c5b9993758c4f76cfc6a533c77d98` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Default.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Default.hs` | 198 | `1370da03c9a606b31132b1d476c53edea81db6fd` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Default/Builtins.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Default/Builtins.hs` | 126008 | `83b764a904466a004c4eefea4926d51652c8b80e` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Default/Universe.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Default/Universe.hs` | 39224 | `f2f941110d88579825630472300d4350edd7baab` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Default/Universe/Cardano.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Default/Universe/Cardano.hs` | 740 | `931528d55a91827854bfdebfb145c2bc57d4e09f` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Evaluation/Error.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Error.hs` | 3234 | `46bdacaf47e97a476c2efde696abba63c09f7958` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/BuiltinCostModel.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/BuiltinCostModel.hs` | 9973 | `fc57352fd58632d1aafeab711b73460431bc00ea` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostModelInterface.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostModelInterface.hs` | 19435 | `6234c99937f06c65afc74a3b153b2f50cadeb1d0` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostStream.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostStream.hs` | 6652 | `1a468ae59220e0f4f8b00e16877a8b85800dd8ea` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostingFun/Core.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostingFun/Core.hs` | 38663 | `59aed78d340eaef6bb3fda0e3b0b507779489586` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudget.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudget.hs` | 10923 | `0923eb9d37b78c9cd51e7026622a6f263ba020eb` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetStream.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetStream.hs` | 3560 | `b1b0d3178e0434b1bd5fd874d9021fe65fd75986` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetingDefaults.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetingDefaults.hs` | 19344 | `dc08e6c3c8e5357031720432b98ad8ee91b067f1` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemory.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemory.hs` | 5001 | `65eccab2b7f66336f394d092d2a49324a0048c9e` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemoryUsage.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemoryUsage.hs` | 24245 | `22e14a6fd828b97c2aa755129489fb7a55eced38` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/Exception.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/Exception.hs` | 6348 | `c09d3d639c7decc2d069e62570ca06da0a625f68` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters.hs` | 5317 | `dff7ea163955552b107aa8061ab4f285fc64ef00` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters/Default.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters/Default.hs` | 4673 | `15986db0818b4068bf5942b1a1d4f2039f01563a` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/SimpleBuiltinCostModel.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/SimpleBuiltinCostModel.hs` | 1507 | `b9323f8d9e636cb4a2244a739d4100aeae6d6a80` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/FlatInstances.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/FlatInstances.hs` | 15431 | `670553da0aa2c6136d20a6c11d0d9dc60e372855` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/MkPlc.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/MkPlc.hs` | 12524 | `1825bd73964d674a9adb30ec0bc8a77cfa615a21` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Value.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Value.hs` | 22113 | `3f25ebbb9b5818e12860aae83d47be3a6608593f` |
| `plutus` | `plutus-core/plutus-core/src/PlutusCore/Version.hs` | `plutus/plutus-core/plutus-core/src/PlutusCore/Version.hs` | 2751 | `1dac3fe8e7e25ffa7f04faef6eebd7bc9c7b59da` |
| `plutus` | `plutus-core/plutus-core/test/CostModelInterface/Spec.hs` | `plutus/plutus-core/plutus-core/test/CostModelInterface/Spec.hs` | 10888 | `04469aac4cc963b818702aa1b587c7db60bfa9d9` |
| `plutus` | `plutus-core/plutus-core/test/CostModelInterface/defaultCostModelParams.json` | `plutus/plutus-core/plutus-core/test/CostModelInterface/defaultCostModelParams.json` | 11576 | `7b91b1512a8031faa29bfdc9afeafd9f7f803be2` |
| `plutus` | `plutus-core/plutus-core/test/Flat/golden/encoding-stability.golden` | `plutus/plutus-core/plutus-core/test/Flat/golden/encoding-stability.golden` | 381 | `dae9b509ad53f39be1384fce29c88c9669d22a84` |
| `plutus` | `plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Check/Scope.hs` | `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Check/Scope.hs` | 1872 | `ac6f7e3eeab05bf706ee031913c0cd0e1b3c4717` |
| `plutus` | `plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Instance/Flat.hs` | `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Instance/Flat.hs` | 10756 | `6d109c660f7c5c3af6e181cae7c24f97f6d5841c` |
| `plutus` | `plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Type.hs` | `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Type.hs` | 7857 | `79b7f81d40b61cfe1e506844892677aedbd72717` |
| `plutus` | `plutus-core/untyped-plutus-core/src/UntypedPlutusCore/DeBruijn.hs` | `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/DeBruijn.hs` | 4158 | `f31da1012d703a91488647e17909a9e2f91abc5a` |
| `plutus` | `plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek.hs` | `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek.hs` | 3842 | `ae0a28b040efffe7d5fb289e9cb89ee142c0255b` |
| `plutus` | `plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/CekMachineCosts.hs` | `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/CekMachineCosts.hs` | 3921 | `676d6bf2c8d7cb7737b9c7e8da58254a16fe82f7` |
| `plutus` | `plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/EmitterMode.hs` | `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/EmitterMode.hs` | 4403 | `90e34e8e34aa43c05eb1852f653d691d3dafd1f9` |
| `plutus` | `plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/ExBudgetMode.hs` | `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/ExBudgetMode.hs` | 6792 | `1e259897bc584cf80ed386abcc2633b59ff36d94` |
| `plutus` | `plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/Internal.hs` | `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/Internal.hs` | 52018 | `071744a494c5a93478f95d1d2fe017055107c5f2` |
| `plutus` | `plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/CommonAPI.hs` | `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/CommonAPI.hs` | 6213 | `3d161359d1726aca4f66024eb1af56b7f22a98b3` |
| `plutus` | `plutus-core/untyped-plutus-core/src/UntypedPlutusCore/MkUPlc.hs` | `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/MkUPlc.hs` | 794 | `007b76fca310ef08a24770c2fc0814aa80bda2ff` |
| `plutus` | `plutus-ledger-api/CostModel/Params/CostModelParams/costModelParamNames.golden.txt` | `plutus/plutus-ledger-api/CostModel/Params/CostModelParams/costModelParamNames.golden.txt` | 12554 | `ec31dae2c2052f81842e519b24935a4541078130` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/Common/Eval.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Eval.hs` | 14193 | `a3f9e73f8b5d58baafde7344da4749075b531db9` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/Common/ParamName.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ParamName.hs` | 4859 | `d54324f91b3661ff3506cb4daf928cabfd386701` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/Common/ProtocolVersions.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ProtocolVersions.hs` | 5286 | `f9bdd78a91e6a93e1a0ba397a45c040bda7ef7cc` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/Common/SerialisedScript.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/SerialisedScript.hs` | 11490 | `7987bec5bd711a2eb670b6387ac399c225b5d821` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs` | 13092 | `b2393e16c25813100d089a2457fdc347547fa620` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/Data/V1.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V1.hs` | 6815 | `5addf1383ff2e9288a06a99daf492c83a7989e18` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/Data/V2.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V2.hs` | 7749 | `a840734846062941b20664169ae94626199ba61c` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/Data/V3.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V3.hs` | 10202 | `b58d57b1eab630ecfdd926a582412816712afd8d` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/MachineParameters.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/MachineParameters.hs` | 1779 | `e3905dd8e74f4d0af46ff809ef3c42f006edac32` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V1/Contexts.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Contexts.hs` | 11440 | `688cb68c85df8245995bdaedfdad13f622f9be8d` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Address.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Address.hs` | 3283 | `02e9a0149604f24d1475e91aa06e22d149ceafb0` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Contexts.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Contexts.hs` | 11069 | `ad00406c5ccee80916369c4cab2bb1781e153187` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Credential.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Credential.hs` | 3610 | `6dd1069e7112070243a07f919465d84a0a0670a8` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V1/Data/DCert.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/DCert.hs` | 3607 | `e10a91340760321bce4e86f4704d4f116b557f25` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Interval.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Interval.hs` | 19367 | `f869f20257255919a841954fa4fd696942a0c366` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Time.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Time.hs` | 3293 | `ac8c4a987285d302213f2796d894317fb7ddc632` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Tx.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Tx.hs` | 5545 | `586a51be986a56edb8b8c05585f2d7c41a630002` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Value.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Value.hs` | 26516 | `475e9cab3256e1779dcc0e4430aa7ae018a5235a` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V1/EvaluationContext.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/EvaluationContext.hs` | 2115 | `e27f0dc6b481cfd2f36e5a79bef87befaae33467` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V1/ParamName.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/ParamName.hs` | 13971 | `f47c26fb9437aaf978b58aa34cf57750fa51d698` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V1/Scripts.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Scripts.hs` | 6857 | `dca487cb5eea439cccbfb2f3497d86e93d04d3cb` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V1/Tx.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Tx.hs` | 5972 | `ba7be936a1488185ef176cf2360cfac317060111` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V1/Value.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Value.hs` | 25161 | `21af3c9d11c2ba7788dbe7a7a2f57d54e40d90bf` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V2/Contexts.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Contexts.hs` | 10918 | `a806b1ef3603d836ec513d512866bf3dd63a49bf` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Contexts.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Contexts.hs` | 11397 | `e0f0f369b5167981eeb8280dfac255070e3e24d3` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Tx.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Tx.hs` | 4629 | `e820a720fa9407ef08f23135ede2f9bdcc638246` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V2/EvaluationContext.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/EvaluationContext.hs` | 2115 | `1628c6ce088dcab25d1fb60713a75f2c0fa171a6` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V2/ParamName.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/ParamName.hs` | 14080 | `0340acbb80270c7b347bc9c71a38d46988b7995d` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V2/Tx.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Tx.hs` | 4786 | `9f9ae9c2b3ac1d9ac54b2ad6003085b9ee4e00b3` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V3/Contexts.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Contexts.hs` | 25064 | `db3959ce88a46c1a94a60765704a6e72cab754a2` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Contexts.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Contexts.hs` | 23719 | `22ebed2447f888f785d64b1f59b9dd373724bc3b` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V3/Data/MintValue.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/MintValue.hs` | 4548 | `ea2c2a4f2a9de31d371d5180cea876cbc51b842f` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Tx.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Tx.hs` | 3152 | `bfeaf5244a55386186eeebab522f529f5b9e0d8b` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V3/EvaluationContext.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/EvaluationContext.hs` | 1922 | `c6c4a3e60548e13f7084ec96a7a7db268b1e7315` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V3/MintValue.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/MintValue.hs` | 5394 | `2216347a4e35ef9a65a699b3d041811e94f1b707` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V3/ParamName.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/ParamName.hs` | 14867 | `5c16e227b7873c0e5f0eddd62d067405159359d4` |
| `plutus` | `plutus-ledger-api/src/PlutusLedgerApi/V3/Tx.hs` | `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Tx.hs` | 2934 | `b162f7d2ca15bbe4947c19291b136cb718704415` |

### Completeness and format rules

Reject truncated inventories, missing/extra selected source paths, duplicate/unsafe paths,
symlinks, gitlinks, submodules, special files and selected entries above 16 MiB. Empty corpus data
is representable; the selected standalone source files must be nonempty. Verify source byte lengths
and Git blob SHA-1 against the independently obtained tree, then SHA-256 for every stored artifact
and transformed entry. Plutus archive contents were read selectively without extract-all or hooks;
Ledger files were read by immutable raw URLs. No upstream code, shell script or generated tool ran.

corpus.json preserves every selected source blob using the following exact schema and key order:

```json
{
  "schemaVersion": 1,
  "source": {
    "repository": "https://github.com/IntersectMBO/plutus.git",
    "commit": "9e17e2404dc6988c908b1fea099dde202df73b6a",
    "tag": "1.68.0.0",
    "root": "plutus-conformance/test-cases",
    "licensePath": "artifacts/plutus/plutus-conformance/LICENSE"
  },
  "entries": [
    { "path": "<source-root-relative path>", "size": 0, "sha256": "<lowercase SHA-256>", "contentBase64": "<canonical padded base64>" }
  ]
}
```

Serialize as UTF-8 JSON with two-space indentation and a final LF, and order entries by UTF-8 path
bytes. Reject repeated paths, absolute paths, empty/dot/dot-dot components, backslashes and control
characters. RFC 4648 padded base64 must round-trip canonically. `size` and `sha256` refer to original
bytes; do not decode Flat or arbitrary expected-result bytes as text. Schema version is a transport
schema, not a provider version. No provider guide is needed to interpret these frozen rules.

The corpus transport bytes are unchanged by this storage reconciliation. source.licensePath is a
logical artifact path with an artifacts/ prefix; strip that prefix and resolve through the full
inventory below. The physical license and notices reside in 0001. Corpus README is generated local
documentation. SHA256SUMS lists every effective artifact except itself as lowercase SHA-256, two
spaces, provider-root-relative physical path and LF, ordered by UTF-8 physical path bytes. Verify
from the provider root. Its own hash is frozen below. SNAPSHOT.md/CAPTURE.md are not artifacts.

### Case discovery and associations

Select each .uplc and .flat entry as a distinct format case. Require exactly its appended
`.expected` result and exactly one matching `.budget.expected` after removing the program extension.
The shared budget may serve both text and Flat representations of the same stem. For this snapshot
there are no competing historical `<program>.budget.expected` paths, missing companions or orphan
expected-result/budget files. The eight exact auxiliary paths below are the entire non-case set;
account for them explicitly, without executing or dropping them. No future-feature exclusion was
applied during capture. Future consumer plans must classify availability from captured evidence.

- `uplc/evaluation/builtin/interleaving/README.md`
- `uplc/evaluation/builtin/parser/README.md`
- `uplc/evaluation/builtin/semantics/README.md`
- `uplc/evaluation/builtin/semantics/bls12_381-cardano-crypto-tests/README.md`
- `uplc/evaluation/builtin/semantics/bls12_381_G1_multiScalarMul/files`
- `uplc/evaluation/builtin/semantics/ren.sh`
- `uplc/evaluation/builtin/semantics/verifyEcdsaSecp256k1Signature/README.md`
- `uplc/evaluation/term/parser/README.md`

Six auxiliaries are Markdown documentation, one is a case-development note named `files`, and one
is rename tooling named `ren.sh`. All eight are inert corpus bytes, not library code.

### Inventory and counts

| Property | Exact value |
| --- | --- |
| Standalone source blobs | 113 Plutus + 24 Ledger = 137 |
| Corpus source blobs / packaged entries | 4,822, totaling 1,162,961 bytes; maximum 19,938 bytes |
| Total selected source blobs | 4,959 (137 standalone + 4,822 corpus) |
| Effective logical artifacts | 140 (137 source files + corpus.json + corpus README + SHA256SUMS) |
| Program-format cases | 1,905: 1,004 text and 901 Flat |
| Distinct logical stems / shared budget files | 1,004; 103 text-only stems |
| Expected-result files | 1,905 |
| Auxiliary files | 8, explicitly named above |
| Text expected outcomes | 721 success, 219 evaluation failure, 64 parse/decode failure |
| Flat expected outcomes | 677 success, 219 evaluation failure, 5 parse/decode failure |

1,905 programs + 1,905 results + 1,004 shared budgets + 8 auxiliaries = 4,822 entries. These are
inventory and fixture-expectation counts; the new runtime conformance cases were not executed.
Missing Flat counterparts are recorded as text-only, not generated or silently omitted.

### Integrity and licensing rules

Preserve Apache-2.0 licenses/notices for Plutus, its conformance corpus and Ledger. Their selected
legal files remain byte-identical to 0001. Verify final SHA256SUMS, exact inventory and corpus
entry length/hash/base64 before consumption. The complete resolved inventory freezes paths and hashes for all effective artifacts.
Source-tree/Git-blob verification establishes complete capture membership; counting output alone
does not. Supplementary third-party code/licenses and linked repositories are not implicitly added.

### Consumption boundaries and exclusions

Official Plutus supplies language, Flat/CEK, builtins and costs; Ledger supplies the selected
script-discovery/context/valuation integration. Maintain UPLC 1.0/1.1, Plutus V1–V3 and protocols
5–11 until a separately authorized scope decision changes them. Preserve historical codecs and
lossless Cardano wire owners. Capturing V4/protocol-12 or future-builtin source does not enable it.
C++ remains unmaintained and opt-in; behavior descriptions are language-neutral.

## Captured scope

All 137 original standalone source paths exist and are captured at the new independent revisions.
The full selected corpus, including binary contents and support material, is packaged without
extension-based omission. No new library source, tests, public exports or lifecycle state changed.

## Original capture validation evidence

The following records the original capture checks, not new execution during this reconciliation.
Current migration checks verify local bytes, mappings and counts; no fresh upstream audit is claimed.


All 4,959 selected source blobs passed byte-length and Git blob SHA-1 checks against their complete
source inventories. Corpus schema/order/base64/size/SHA-256 and all 1,905 companion associations
were verified locally. All 140 artifacts are covered by the final inventory; SHA256SUMS covers 139.

`npm --prefix libs/typescript run check` passed: 211 tests, zero failures, plus package smoke checks
for ESM/NodeNext/bundler consumers. This still tests the pinned 0001 corpus (1,003 text cases).
A read-only call to the existing `readConformanceCorpus` with the new exact 4,822/1,905 inventory
rejected `uplc/evaluation/builtin/semantics/bls12_381_G1_multiScalarMul/files` as an invalid auxiliary
path because the current helper only permits .md auxiliaries. No new conformance evaluation was
performed after that rejection, and no parity claim or missing-case suppression follows from it.

## Exclusions

Upstream code/tool execution, source generation, automatic adoption, full phase-one validation,
third-party normative implementations, new runtime support beyond protocols 5–11, scheduling and
GitHub Actions. Auxiliary tooling remains inert base64 evidence. Old snapshots and active REVIEW
implementation records remain unchanged.

## Resolved evidence storage

Logical artifact paths in the source mapping and transport metadata resolve through the complete
inventory below. Physical paths are relative to this provider root. Resolve files directly; no
patch replay or latest alias is permitted. Missing or mismatched inherited files are errors.
The source mapping, selected revisions and all byte-exact upstream files retain their original
capture identities. Only generated transport controls are rebased to the incremental storage.
SNAPSHOT.md and CAPTURE.md are immutable metadata, not artifact inventory members.

### Referenced snapshots

| Earlier snapshot path from provider root | SHA-256 | Role |
| --- | --- | --- |
| `0001/SNAPSHOT.md` | `614c88a7ae99d209e8328caa873eefa7db6a9140cea8bd9131ec2f4f34a674e5` | Immediate predecessor and owner of every reused artifact |

### Physical storage counts

- Effective logical artifacts: 140.
- Newly stored physical artifacts in 0002: 46.
- Logical entries reusing 0001 physical files: 94.
- Logical comparison: 0 added, 46 changed, 0 removed, 94 unchanged.
- Removed entries are absent here; their 0001 files remain available.

### Complete resolved artifact inventory

| Logical path | Physical path from provider root | Bytes | SHA-256 |
| --- | --- | --- | --- |
| `SHA256SUMS` | `0002/artifacts/SHA256SUMS` | 20426 | `f34a243d66e20c9ae7dd03a1fcdd7ada482f0a95a1d57cd4bf7b3e0900290f9b` |
| `cardano-ledger/LICENSE` | `0001/artifacts/cardano-ledger/LICENSE` | 10174 | `0d542e0c8804e39aa7f37eb00da5a762149dc682d7829451287e11b938e94594` |
| `cardano-ledger/NOTICE` | `0001/artifacts/cardano-ledger/NOTICE` | 575 | `58721f8b6ca67f0fcbe1cd739b384fec3126a35f7d79951aedaa2bc3863a3162` |
| `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Context.hs` | `0002/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Context.hs` | 15638 | `d4af885bdf2c53b226e705db07be6c0448e4af8fb28171aeaab3ad336ae65111` |
| `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Evaluate.hs` | `0002/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Evaluate.hs` | 14164 | `e6945f6b8961d9aa2234dfc88ee83fb37552ae72fc864457ca4f2fda844d9aba` |
| `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/TxInfo.hs` | `0002/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/TxInfo.hs` | 14959 | `5bccda99e315ecddc7870e17f706862299739ad2e897248cba04539636fb2639` |
| `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Rules/Utxos.hs` | `0002/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Rules/Utxos.hs` | 13948 | `5cb5eaf17a872838baa4d2b01dcff81ca6560ff2af0f1792a7d0856a57bfe339` |
| `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Scripts.hs` | `0002/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Scripts.hs` | 25897 | `e1923d516f46c4c13d238a790836cb2bd93fd829ca1e805797dd17eb6ab88273` |
| `cardano-ledger/eras/alonzo/impl/test/Test/Cardano/Ledger/Alonzo/Imp/TxInfoSpec.hs` | `0002/artifacts/cardano-ledger/eras/alonzo/impl/test/Test/Cardano/Ledger/Alonzo/Imp/TxInfoSpec.hs` | 2474 | `03a64f4252576abfeedfb9dd8ac48077475bbbd4de655b87701c0ac560a99f6b` |
| `cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Rules/Utxos.hs` | `0002/artifacts/cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Rules/Utxos.hs` | 8001 | `b27579439ad26eb294ea642e48ab1b70546bc05a8c5d9965aef465c11343237e` |
| `cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Scripts.hs` | `0001/artifacts/cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Scripts.hs` | 4530 | `5a2ec0c145872e202ca69626e495dcc0a815100c74036bf1fb4f92a9c69ae113` |
| `cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/TxInfo.hs` | `0002/artifacts/cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/TxInfo.hs` | 15859 | `7ed513a85c2c8fe17c0adb3fb52ff124cd8fba63c86071b6053f71660395864c` |
| `cardano-ledger/eras/babbage/impl/testlib/Test/Cardano/Ledger/Babbage/TxInfoSpec.hs` | `0002/artifacts/cardano-ledger/eras/babbage/impl/testlib/Test/Cardano/Ledger/Babbage/TxInfoSpec.hs` | 12885 | `1f84cd90dc752051eb4ca7cb2788d6e4db0fa10ad9abde9b8a6d5559a7a2ea32` |
| `cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Rules/Utxos.hs` | `0002/artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Rules/Utxos.hs` | 8039 | `af32f7740cef782e64722b2895015b7594b156eb00d03e88e74dd35e87b11c3f` |
| `cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Scripts.hs` | `0002/artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Scripts.hs` | 11623 | `fe6bf56c3e4d1fb66567b74ff979a885d721bc3fb6b98f237c66f6dce5c6ab88` |
| `cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/TxInfo.hs` | `0002/artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/TxInfo.hs` | 33429 | `ad44dd79cfb4598bf34f9706fc01e29ef397a24f80288ff3ef8dbe02297642c9` |
| `cardano-ledger/eras/conway/impl/testlib/Test/Cardano/Ledger/Conway/TxInfoSpec.hs` | `0001/artifacts/cardano-ledger/eras/conway/impl/testlib/Test/Cardano/Ledger/Conway/TxInfoSpec.hs` | 4200 | `a4526cec991c15f9a492db4ef5d185aa0aeff7c11eaabb6deeab04458038806d` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus.hs` | `0001/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus.hs` | 504 | `009292938ecd818883aacd9aed07fa51abd553a04f1d26d0b9d6895eb5423654` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/CostModels.hs` | `0001/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/CostModels.hs` | 19684 | `5fd2ab01dabc4a7998b46f1bce8a8db2ddd89912fc6d680416d5f9d5a1becf47` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Data.hs` | `0002/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Data.hs` | 11216 | `55355d7d12508d5bec88d6046fb5a2856247fc02a9f7b23c6b40f837105e125f` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Evaluate.hs` | `0002/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Evaluate.hs` | 16332 | `f82bd7d74515d1bd92e107185655f750772a774c1ff0a12e0e532b6df59ff867` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ExUnits.hs` | `0002/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ExUnits.hs` | 7866 | `b969c078d9fe8a71fc2dadeb05fddaa892411266c03f458fd65b5a86aaeb1a3e` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Language.hs` | `0002/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Language.hs` | 22874 | `233660eff9726f07c0c4eec6db851f011aa00d6dd4b42d67584df18ff9c0f8f4` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ToPlutusData.hs` | `0002/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ToPlutusData.hs` | 6255 | `13f1fa98c7d64177c90eee3d5813a900d7648edcd821c527d5e5fddd38890709` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/TxInfo.hs` | `0002/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/TxInfo.hs` | 7291 | `e37ab30f595ae4aee1a71a494a1700530f0fc2606fd70ddcf3c894be79504a9c` |
| `conformance/README.md` | `0002/artifacts/conformance/README.md` | 3969 | `e78b63737bfe74dd37c4bb46b512699fdcd15f508df235e5ff7bec15b7a1062c` |
| `conformance/corpus.json` | `0002/artifacts/conformance/corpus.json` | 2740361 | `974aeeb28b16ab8d0232b20535f382122f7329222399bc7839ae800c8e1df759` |
| `plutus/LICENSE.md` | `0001/artifacts/plutus/LICENSE.md` | 11356 | `43070e2d4e532684de521b885f385d0841030efa2b1a20bafb76133a5e1379c1` |
| `plutus/NOTICE.md` | `0001/artifacts/plutus/NOTICE.md` | 560 | `9e1fe670abe06d2cbdffa2d724765b5e86aa7f65d8eccd34b391a37fbf7bab89` |
| `plutus/README.adoc` | `0001/artifacts/plutus/README.adoc` | 4198 | `47811f8c530b2a96db0b347be916f6a4063948b2e4e834148cdf0fdb1ca112ed` |
| `plutus/plutus-conformance/LICENSE` | `0001/artifacts/plutus/plutus-conformance/LICENSE` | 9139 | `69ce94606a661fa3290eeaca21f3f9ad7d73dc985ffa346a1104704548f72d93` |
| `plutus/plutus-conformance/NOTICE` | `0001/artifacts/plutus/plutus-conformance/NOTICE` | 593 | `9f5112659daf50eec7866133b2c11432547d132ec8253121ffc8503970197b34` |
| `plutus/plutus-conformance/README.md` | `0002/artifacts/plutus/plutus-conformance/README.md` | 11151 | `852f4bdb7afba613b9cb4199a22821b208cb991950594529b1d3241771df72c9` |
| `plutus/plutus-conformance/src/PlutusConformance/Common.hs` | `0002/artifacts/plutus/plutus-conformance/src/PlutusConformance/Common.hs` | 26579 | `8eb319dd9b6fbe838513b4f5ba789d1efa15332b924bb180ea2575f290cc4b2a` |
| `plutus/plutus-core/cost-model/data/builtinCostModelA.json` | `0002/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelA.json` | 24019 | `76e166858b763cc3ef815c4bcc903bef4b36dbd2d09411bb00c7dce9db5371a4` |
| `plutus/plutus-core/cost-model/data/builtinCostModelB.json` | `0002/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelB.json` | 24002 | `2faa8ff8d81f4f7c07fb5f13369a8c0367b721e21a152e2c5cb2cd5437398656` |
| `plutus/plutus-core/cost-model/data/builtinCostModelC.json` | `0002/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelC.json` | 24444 | `b35dd8a6c2755f6241b1fcbacb24d501d1d38b976da58903f2cb60032f0889ea` |
| `plutus/plutus-core/cost-model/data/builtinCostModelD.json` | `0002/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelD.json` | 24002 | `428d796d5656731d44b763279ae146d663d7e39c96858255aec1d57fe89b5a9b` |
| `plutus/plutus-core/cost-model/data/builtinCostModelE.json` | `0002/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelE.json` | 24452 | `e3ba8e7095b04e3a9d468cf64389f2ebcd94a894cda40e3a5c85e477ac8472d6` |
| `plutus/plutus-core/cost-model/data/cekMachineCostsA.json` | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsA.json` | 671 | `5457650b1e138f59888f2f7c486d8137fa0aa3c119b25931d130bbfd73e4a56f` |
| `plutus/plutus-core/cost-model/data/cekMachineCostsB.json` | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsB.json` | 671 | `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` |
| `plutus/plutus-core/cost-model/data/cekMachineCostsC.json` | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsC.json` | 671 | `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` |
| `plutus/plutus-core/cost-model/data/cekMachineCostsD.json` | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsD.json` | 671 | `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` |
| `plutus/plutus-core/cost-model/data/cekMachineCostsE.json` | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsE.json` | 671 | `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` |
| `plutus/plutus-core/plutus-core/src/Codec/Extras/SerialiseViaFlat.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/Codec/Extras/SerialiseViaFlat.hs` | 3725 | `0ed6622daa30737f5691f78056e3ff25538eab7e9c249273df180ca297b37d0c` |
| `plutus/plutus-core/plutus-core/src/Data/Vector/Orphans.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/Data/Vector/Orphans.hs` | 1093 | `2d017a10136c1b516dfe0acdf501b67662495100540da982e532a32c59cf0381` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Bitwise.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Bitwise.hs` | 57835 | `a18717fb9199d9489191372baa5974114a3b4ec90d15b6d8c9fe60e77f5d8ea9` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin.hs` | 613 | `8b20f7142c91dd12521e14f896ad9b9319813031e1a8271ecb06be3af8b1b2a6` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/KnownType.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/KnownType.hs` | 22725 | `d5c08abc0be1a33663c708799a3aaba74d41348774696567f09b59b9ce64860a` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Meaning.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Meaning.hs` | 22056 | `e5ad4d68a2a8ab3aa71427ddfb37ad649f3f6a8fabd4b20e334aed816a05e5d9` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Result.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Result.hs` | 9734 | `79f8202a565a883bfc7e5aa033228d0243b214cbbe982c0450202d32b97a663e` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Runtime.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Runtime.hs` | 5624 | `29f6241a275645f5c339c96edebbfbfbaaeb3fc47e832e1165b63c234d8cf290` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/TypeScheme.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/TypeScheme.hs` | 4473 | `b81f6fb30ae2b12f36e1fc74d7f9058a6e0386d5e8787f2509ee235e89370cf3` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Bounds.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Bounds.hs` | 651 | `8255a49552e7d28781c21c238370dff4b01d8fcb9571f514c3889bccb11eac21` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Error.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Error.hs` | 157 | `b2c02ed3ce7646902885f1727cf2654bd18ec0a428392b1938f3914652208a7e` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G1.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G1.hs` | 8427 | `4c497eb561d8485759069c1c719d56c1f879ef890a3dedb62b6c63f8694bc152` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G2.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G2.hs` | 6057 | `1ea930741576a7703515719edc4d92f5746a3e639ca1a306dd0d3c7330da8d18` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Pairing.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Pairing.hs` | 2680 | `8403c461db988da5f303b35adfd13c099798681ab23ed88cf30fbb9d842dfd4a` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Ed25519.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Ed25519.hs` | 1302 | `9a6fa8a721e28d90483a6f23347e0f0aaf4cec7028e1717e7b586c7fcf028259` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/ExpMod.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/ExpMod.hs` | 1574 | `697727fe1e2aaa7c37f1abb3cc8d71d5333163fdd218d383fcce3f2b0eacaf08` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Hash.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Hash.hs` | 1363 | `40d8f6e118f4d0e1f0f72714d19d3322e095691ec69c09a4118544f8acbd3113` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Secp256k1.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Secp256k1.hs` | 3730 | `e25b37a1056a14342b9f0e5591c9f9c0362b8c0edf1f4c5354bd83bbfcb7e644` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Utils.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Utils.hs` | 662 | `06a2dae624cc3df9e02e64ad24d599c5485b12810a41381c2cd8cbe58bc463cf` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Data.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Data.hs` | 12814 | `e4633b7b293b4078387b107eb36d1f16864b93b7a129a7a2531493791fb5628c` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/DeBruijn.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/DeBruijn.hs` | 9418 | `ae7d44eaabdc5b58fbb01898c3472972ba69df9d46de0c785b515392082c48b2` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/DeBruijn/Internal.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/DeBruijn/Internal.hs` | 13569 | `55b2928e0d2d0af4a246bfa21a790270af243f3b9d9336490ef1468d223053ec` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Default.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default.hs` | 198 | `e11984b4e05701e101283742639fae96bb3d0c67122f79eb6ac771c0d74ee76d` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Default/Builtins.hs` | `0002/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default/Builtins.hs` | 126008 | `396226f91c048eee40b9bc3288e3441a46e737d6eb4a84d386ed9d2493da2218` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Default/Universe.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default/Universe.hs` | 39224 | `88d257b863f3f5a70971a19b431725610cceec623ec132e2b8344a9efa2cc517` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Default/Universe/Cardano.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default/Universe/Cardano.hs` | 740 | `353c896bb289591bb6e6ca323b63fa55bf7563fc77cd63f462aa61b74c031f3d` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Error.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Error.hs` | 3234 | `6b5d8035b535264fc133af41782ec5d03ea7baa40ebd0736db9367dc0e14c456` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/BuiltinCostModel.hs` | `0002/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/BuiltinCostModel.hs` | 9973 | `e4589f78ccebc3683f290b67d842cc759d436fe16e13b71ce2d5639986dfb28c` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostModelInterface.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostModelInterface.hs` | 19435 | `f3286eacee23ab160a285195af0caf0c215f7395e0e6c88fcffe564bc96f925d` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostStream.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostStream.hs` | 6652 | `46b33a311e77e1335dd4688e5e94cb966a48416ecdca3fdf32ea605af8116c3b` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostingFun/Core.hs` | `0002/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostingFun/Core.hs` | 38663 | `2b4a422186ebfd3bff4b889567cf6d1c3e2f5e5c06e8ba4ae6767a7913bc7178` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudget.hs` | `0002/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudget.hs` | 10923 | `747b74d7920b3be825e796a60e9699ea56a55cc2993bb84fb7ec36cab311d02a` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetStream.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetStream.hs` | 3560 | `0f21b09e0252825c53ef83eeb19e23a8dd85fc269da495d172390b5ec4333da8` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetingDefaults.hs` | `0002/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetingDefaults.hs` | 19344 | `4d24c16963cdd46a7b6669c2d0d93845ac263dfe201c9415787fc7eac70ad8b6` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemory.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemory.hs` | 5001 | `d6da66e49a1e38abd7a745999eb884433afc3a2306a73808e7016a5238ba7dac` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemoryUsage.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemoryUsage.hs` | 24245 | `63b72128955389b0fc4134f5559b6521a69020d87d186df25790b57822866027` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/Exception.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/Exception.hs` | 6348 | `58140a97be5898cbe0cb6057a9eb55f4c48c0b4d09cbe69bcc1737111f917cc1` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters.hs` | 5317 | `e63180de6672fb5e16f974a00f4678f9e89ca07220284ff62460d1dcf724cf34` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters/Default.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters/Default.hs` | 4673 | `a8fff1b96f9b014b4a2f9b459ba89659da090d7a796215ef382bb899bb3ce92a` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/SimpleBuiltinCostModel.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/SimpleBuiltinCostModel.hs` | 1507 | `8b0dd3fc51ba9ef980a6a5f2fbe2ddc08aedb4d96070362b8d4a54be912ab1ef` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/FlatInstances.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/FlatInstances.hs` | 15431 | `693e1fa70b419e73de486bafec3f01d6e574ed0896a345d5688bc1925ed5107d` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/MkPlc.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/MkPlc.hs` | 12524 | `64ddf39f5a2a46114ead6113d95a76abcf24fc62a368315de5e06b4748a6aa50` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Value.hs` | `0002/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Value.hs` | 22113 | `b1e15d2bf4fdeae9cdb79d1866426ba44200819ac9d5606f538d6b6cab25b1f0` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Version.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Version.hs` | 2751 | `f0a48a92483a323fab9caa502de11850362fb4c7cc7cc0fd4e102486dd137e7c` |
| `plutus/plutus-core/plutus-core/test/CostModelInterface/Spec.hs` | `0001/artifacts/plutus/plutus-core/plutus-core/test/CostModelInterface/Spec.hs` | 10888 | `fa0ea6df8addab0b586b24c407322ce16013899537097639cc2c863954328687` |
| `plutus/plutus-core/plutus-core/test/CostModelInterface/defaultCostModelParams.json` | `0001/artifacts/plutus/plutus-core/plutus-core/test/CostModelInterface/defaultCostModelParams.json` | 11576 | `a47727bcdf68efeceac8c49f032d19682a374c14fede64392203d92279d3534f` |
| `plutus/plutus-core/plutus-core/test/Flat/golden/encoding-stability.golden` | `0001/artifacts/plutus/plutus-core/plutus-core/test/Flat/golden/encoding-stability.golden` | 381 | `794d5bdf0baa30dbe74478d3a414dc015e591456c7c99ca0859bff496325302d` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Check/Scope.hs` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Check/Scope.hs` | 1872 | `60442f821052511b5ccd73bca1d382570f17f8d4b719e8ff8c6b9cec24b0497f` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Instance/Flat.hs` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Instance/Flat.hs` | 10756 | `ee762bda65323a0c6f270cad06b198b39728530980703ae235af26f695f32188` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Type.hs` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Type.hs` | 7857 | `902bb51a5ba193a5f7947b08c9b61d1632b358ddb9ac0bb7996841d51576f275` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/DeBruijn.hs` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/DeBruijn.hs` | 4158 | `22f6cb8f0da939345c3affd221ecea005f3fa676ccf7bd388770831a2066ac56` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek.hs` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek.hs` | 3842 | `567caff202fe0813c929c1af12e475fdaaf586cc75b89679c8630f96c827331c` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/CekMachineCosts.hs` | `0002/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/CekMachineCosts.hs` | 3921 | `b3bb420a2bb58ee650512984d565c004b88485dd5d26f5b1e4b2082619f3c8b1` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/EmitterMode.hs` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/EmitterMode.hs` | 4403 | `52e453ee3b1d1378c570fe613b5a81ee25d81b9532ca9f1cf8ab7258614a62d6` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/ExBudgetMode.hs` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/ExBudgetMode.hs` | 6792 | `dc79de42f831b3c197f4fb6d51f5f7f9b2381f9da06022bf8e359311ed35aa81` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/Internal.hs` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/Internal.hs` | 52018 | `f7483a9661dc5ff5291140dc4c6f4609911e6f6d5f54b043bf98a0cb88090527` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/CommonAPI.hs` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/CommonAPI.hs` | 6213 | `20fd7fdccd4fb00c86ea158341f4802acffc03aeb03cb53a21083970fbdf2804` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/MkUPlc.hs` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/MkUPlc.hs` | 794 | `f1eec0e2237a8e1d8065011a3cf51b159255124c63d0937037d9609cc1bfb7e2` |
| `plutus/plutus-ledger-api/CostModel/Params/CostModelParams/costModelParamNames.golden.txt` | `0001/artifacts/plutus/plutus-ledger-api/CostModel/Params/CostModelParams/costModelParamNames.golden.txt` | 12554 | `93898d8fd636685fae59dbdb861d2e64e8d45700e0fcc9f5eda584c65f6387f8` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Eval.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Eval.hs` | 14193 | `9844d259cff614a988a042fff430ede2f9670f1757a06c5c0953c3900f17935b` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ParamName.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ParamName.hs` | 4859 | `cb63e7b54aafb67b3e69ef113845613db2eef40733b6a385d90d3a53ef3dcf28` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ProtocolVersions.hs` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ProtocolVersions.hs` | 5286 | `db59d72270205b84d06c861301cf71c720659eea3eff1d5c009002ccf90e2927` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/SerialisedScript.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/SerialisedScript.hs` | 11490 | `7feb2ddd0001d48e81db2246a2fda592f2b6cae5f98422cfcbb744b7befdd412` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs` | 13092 | `c9ac5701e9d8554fc40ecaed7a92d705b7096d553502aa39a54a5f5d141d5a8a` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V1.hs` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V1.hs` | 6815 | `da8f638e2bcf537dbc0640e96e61ffded579abd51f9657ce8aedff9fc4677595` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V2.hs` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V2.hs` | 7749 | `de749c8f1b2d5f6d81b6d5760935c748f60fc0e7346e3e41b9c13aa9f5536974` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V3.hs` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V3.hs` | 10202 | `2766e0a170470dad2b7aea34b4ab196f4d4ec9f33e055dc0b05b05b1708fdb17` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/MachineParameters.hs` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/MachineParameters.hs` | 1779 | `39749ecc7d171a033622265393604ef4bf6722f0da8e4dc225588b604bca26ce` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Contexts.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Contexts.hs` | 11440 | `6b4d6c913959ff70e38b6fa6b0053d4bab957e7b7d7b1443c2d54c97092b332f` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Address.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Address.hs` | 3283 | `eb68fad824b70953c5b107bf5ed883d2475d915c8c0c176fd8f18d53a708161c` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Contexts.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Contexts.hs` | 11069 | `501df38b935fda1039d543bb341a1b2c720f833f6a98bbe5948d1e71a0eff6b8` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Credential.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Credential.hs` | 3610 | `9930b6b0097aa83646defc958b82e71f47c14cb62fe4b2d7f9ced26235eefa81` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/DCert.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/DCert.hs` | 3607 | `858c7a0b2e58b6309b860be8c78b0f18fffe211ec1e258e91307f9593d48ec1d` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Interval.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Interval.hs` | 19367 | `243952f5d415c106f4db54b477d0788ea7aec9c99014057e3e888e6576a78b8f` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Time.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Time.hs` | 3293 | `7286d160c1de7c69c00212c23176891229ed0286b3556b55ce0d3ddd5689b116` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Tx.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Tx.hs` | 5545 | `782d3a6f7ee4c2f88d5c30165332b428c77f4bd80f8e4d2f272438c6fcf5a84e` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Value.hs` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Value.hs` | 26516 | `b6015c7acfbde9edc772bf7c48c756fea1df991300a84a492ccd80d7b0201eed` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/EvaluationContext.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/EvaluationContext.hs` | 2115 | `cc3f68cfa670dea838715512597d32c1bce1c77515f8fcef6632fc4e640bfb63` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/ParamName.hs` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/ParamName.hs` | 13971 | `f6a74dfe889fcaa4061fbd32b63fdeb022e57dbb5e8bd38385bf96df8ea29126` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Scripts.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Scripts.hs` | 6857 | `562806a9b75d48187d8a1c4ac66778c0c7e71001a766d37061075504e32618dc` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Tx.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Tx.hs` | 5972 | `95e69701c55f3762ff6d6c07127b7a78ad30e6b12fdc572bdbc6727e9d6a311f` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Value.hs` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Value.hs` | 25161 | `06168acf83fd8f00580a988376decde17216a767c118f34f0b75c78d076bba1a` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Contexts.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Contexts.hs` | 10918 | `ca07745f836b291009a7df9986ceb1ae12f9605938222d8b370243194cc34767` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Contexts.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Contexts.hs` | 11397 | `81ecd549083deb3b436b96468545463fe9fff6b95bdbe8d8f3e7e809654ac31a` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Tx.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Tx.hs` | 4629 | `d01f63d4d292795350cfdc5432ae418638c9057ae0fd8f8abe5f8f17d05a975c` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/EvaluationContext.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/EvaluationContext.hs` | 2115 | `5636a676507259548d9214d2493b9b442966350e30ed38dc8390d502a9dded1d` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/ParamName.hs` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/ParamName.hs` | 14080 | `b6ae8d3524dce04ede0f320c7958c4bafc1ac75b18da4a5280c5ef6dd946c712` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Tx.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Tx.hs` | 4786 | `484e75d3c04874a034c5c7c07f1c741f8179a519f624e5f1ea02f3bac49c773e` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Contexts.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Contexts.hs` | 25064 | `206fe185a66c79df8f095b350b4d23796220001a254ab687a9893ce21b301af4` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Contexts.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Contexts.hs` | 23719 | `d0d049d095d1e91480ecfaa9e83e18afd19b49ebe3d2880638efa1d16c9b0261` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/MintValue.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/MintValue.hs` | 4548 | `dd6674980a22a4e9d01a535652b03bef434b6c2ddd9f0f85d234a0d244786f02` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Tx.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Tx.hs` | 3152 | `e6e942aba7f44cc03088455208e858638191a0ca0e41dbc6e1edf98aa5daa268` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/EvaluationContext.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/EvaluationContext.hs` | 1922 | `efc693f86c5b803f694d7a683f1fe53851f2a16e690fb1d2573a369decb23268` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/MintValue.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/MintValue.hs` | 5394 | `0278e5fa319658f50ffad9ae856908ad3c483094a1a29b1ce1483c0e3c10aeea` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/ParamName.hs` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/ParamName.hs` | 14867 | `1b02b46b25a51a90259a2947efa4df1affc60af498895be93d1a58b0a26f4d87` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Tx.hs` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Tx.hs` | 2934 | `e312e1cb5ab8e2b4bd48402235f2daf6e999d6c205a9a5c823f65fe954239313` |

### Changes from predecessor

| Logical path | Change | Previous physical path / SHA-256 | Current physical path / SHA-256 |
| --- | --- | --- | --- |
| `SHA256SUMS` | changed | `0001/artifacts/SHA256SUMS` / `b98a166763ea1f1be2922be663137a0b318d607411c070d4416bbcd5feeb8df8` | `0002/artifacts/SHA256SUMS` / `f34a243d66e20c9ae7dd03a1fcdd7ada482f0a95a1d57cd4bf7b3e0900290f9b` |
| `cardano-ledger/LICENSE` | unchanged | `0001/artifacts/cardano-ledger/LICENSE` / `0d542e0c8804e39aa7f37eb00da5a762149dc682d7829451287e11b938e94594` | `0001/artifacts/cardano-ledger/LICENSE` / `0d542e0c8804e39aa7f37eb00da5a762149dc682d7829451287e11b938e94594` |
| `cardano-ledger/NOTICE` | unchanged | `0001/artifacts/cardano-ledger/NOTICE` / `58721f8b6ca67f0fcbe1cd739b384fec3126a35f7d79951aedaa2bc3863a3162` | `0001/artifacts/cardano-ledger/NOTICE` / `58721f8b6ca67f0fcbe1cd739b384fec3126a35f7d79951aedaa2bc3863a3162` |
| `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Context.hs` | changed | `0001/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Context.hs` / `46658c5dc9e9e8b983694748018b03c87da2ba9785c95df8b6cf5c2a2ab42a63` | `0002/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Context.hs` / `d4af885bdf2c53b226e705db07be6c0448e4af8fb28171aeaab3ad336ae65111` |
| `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Evaluate.hs` | changed | `0001/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Evaluate.hs` / `aa09e9ad884b4b097b37c0ea4d99d71fa631afeee851ebb632e7534dfc7addc2` | `0002/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/Evaluate.hs` / `e6945f6b8961d9aa2234dfc88ee83fb37552ae72fc864457ca4f2fda844d9aba` |
| `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/TxInfo.hs` | changed | `0001/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/TxInfo.hs` / `012494cd3a95706442cf8d9652e1eef98600270dcb2da17a23b9bc654dda5152` | `0002/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Plutus/TxInfo.hs` / `5bccda99e315ecddc7870e17f706862299739ad2e897248cba04539636fb2639` |
| `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Rules/Utxos.hs` | changed | `0001/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Rules/Utxos.hs` / `0445cb18a2f4d9cbc77e6252142dc333e46f6f01237d0f556526e77f1fc3853d` | `0002/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Rules/Utxos.hs` / `5cb5eaf17a872838baa4d2b01dcff81ca6560ff2af0f1792a7d0856a57bfe339` |
| `cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Scripts.hs` | changed | `0001/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Scripts.hs` / `c5d9817ff60e3fcb1a16c2c17a1352cad4daf67889ba8ef9ccce70bd3f0460f3` | `0002/artifacts/cardano-ledger/eras/alonzo/impl/src/Cardano/Ledger/Alonzo/Scripts.hs` / `e1923d516f46c4c13d238a790836cb2bd93fd829ca1e805797dd17eb6ab88273` |
| `cardano-ledger/eras/alonzo/impl/test/Test/Cardano/Ledger/Alonzo/Imp/TxInfoSpec.hs` | changed | `0001/artifacts/cardano-ledger/eras/alonzo/impl/test/Test/Cardano/Ledger/Alonzo/Imp/TxInfoSpec.hs` / `0a9724dd41950b138c2d6457bfdada1ef27553b0f651ca6a3e98e85eb7435e41` | `0002/artifacts/cardano-ledger/eras/alonzo/impl/test/Test/Cardano/Ledger/Alonzo/Imp/TxInfoSpec.hs` / `03a64f4252576abfeedfb9dd8ac48077475bbbd4de655b87701c0ac560a99f6b` |
| `cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Rules/Utxos.hs` | changed | `0001/artifacts/cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Rules/Utxos.hs` / `52885862cb04b6ebcd7440de1ed645fe75423cd0bc697a267cb50b6472bb0317` | `0002/artifacts/cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Rules/Utxos.hs` / `b27579439ad26eb294ea642e48ab1b70546bc05a8c5d9965aef465c11343237e` |
| `cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Scripts.hs` | unchanged | `0001/artifacts/cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Scripts.hs` / `5a2ec0c145872e202ca69626e495dcc0a815100c74036bf1fb4f92a9c69ae113` | `0001/artifacts/cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/Scripts.hs` / `5a2ec0c145872e202ca69626e495dcc0a815100c74036bf1fb4f92a9c69ae113` |
| `cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/TxInfo.hs` | changed | `0001/artifacts/cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/TxInfo.hs` / `fd8ccc004a7a073d8386d3cb5d774677e93e3d887df69233b3785e7efac6dce5` | `0002/artifacts/cardano-ledger/eras/babbage/impl/src/Cardano/Ledger/Babbage/TxInfo.hs` / `7ed513a85c2c8fe17c0adb3fb52ff124cd8fba63c86071b6053f71660395864c` |
| `cardano-ledger/eras/babbage/impl/testlib/Test/Cardano/Ledger/Babbage/TxInfoSpec.hs` | changed | `0001/artifacts/cardano-ledger/eras/babbage/impl/testlib/Test/Cardano/Ledger/Babbage/TxInfoSpec.hs` / `0769977e970e33b13a94dbdab81a845dd5e7b5b2a9a35b3dab9a0f3379cb849b` | `0002/artifacts/cardano-ledger/eras/babbage/impl/testlib/Test/Cardano/Ledger/Babbage/TxInfoSpec.hs` / `1f84cd90dc752051eb4ca7cb2788d6e4db0fa10ad9abde9b8a6d5559a7a2ea32` |
| `cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Rules/Utxos.hs` | changed | `0001/artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Rules/Utxos.hs` / `05b05bab4e821e32b96ed66767bbedafb5d83b82aa8539d09c8da5b2fce7e56a` | `0002/artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Rules/Utxos.hs` / `af32f7740cef782e64722b2895015b7594b156eb00d03e88e74dd35e87b11c3f` |
| `cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Scripts.hs` | changed | `0001/artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Scripts.hs` / `09c3ea360bffd0c3a52e473bb0bdb765b62b50c6f9d9b92c9bcdfdad5636f1c0` | `0002/artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/Scripts.hs` / `fe6bf56c3e4d1fb66567b74ff979a885d721bc3fb6b98f237c66f6dce5c6ab88` |
| `cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/TxInfo.hs` | changed | `0001/artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/TxInfo.hs` / `127509935f8dc336e136619ea5d3f11612a34f07e497944ccae9edb39991b4a6` | `0002/artifacts/cardano-ledger/eras/conway/impl/src/Cardano/Ledger/Conway/TxInfo.hs` / `ad44dd79cfb4598bf34f9706fc01e29ef397a24f80288ff3ef8dbe02297642c9` |
| `cardano-ledger/eras/conway/impl/testlib/Test/Cardano/Ledger/Conway/TxInfoSpec.hs` | unchanged | `0001/artifacts/cardano-ledger/eras/conway/impl/testlib/Test/Cardano/Ledger/Conway/TxInfoSpec.hs` / `a4526cec991c15f9a492db4ef5d185aa0aeff7c11eaabb6deeab04458038806d` | `0001/artifacts/cardano-ledger/eras/conway/impl/testlib/Test/Cardano/Ledger/Conway/TxInfoSpec.hs` / `a4526cec991c15f9a492db4ef5d185aa0aeff7c11eaabb6deeab04458038806d` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus.hs` | unchanged | `0001/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus.hs` / `009292938ecd818883aacd9aed07fa51abd553a04f1d26d0b9d6895eb5423654` | `0001/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus.hs` / `009292938ecd818883aacd9aed07fa51abd553a04f1d26d0b9d6895eb5423654` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/CostModels.hs` | unchanged | `0001/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/CostModels.hs` / `5fd2ab01dabc4a7998b46f1bce8a8db2ddd89912fc6d680416d5f9d5a1becf47` | `0001/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/CostModels.hs` / `5fd2ab01dabc4a7998b46f1bce8a8db2ddd89912fc6d680416d5f9d5a1becf47` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Data.hs` | changed | `0001/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Data.hs` / `6cb38292ff6db086ba97b771df7b3480383ff986720c7dce1059d4f3864e7301` | `0002/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Data.hs` / `55355d7d12508d5bec88d6046fb5a2856247fc02a9f7b23c6b40f837105e125f` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Evaluate.hs` | changed | `0001/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Evaluate.hs` / `733bd37b4f534f3c8a70d53b007addd2393ccb4b6e1a6a3e780cc0682ed8d966` | `0002/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Evaluate.hs` / `f82bd7d74515d1bd92e107185655f750772a774c1ff0a12e0e532b6df59ff867` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ExUnits.hs` | changed | `0001/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ExUnits.hs` / `618964660c3e7445367563440c4c1994b8c50f6e129d7185cd05fb87a2a9be4c` | `0002/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ExUnits.hs` / `b969c078d9fe8a71fc2dadeb05fddaa892411266c03f458fd65b5a86aaeb1a3e` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Language.hs` | changed | `0001/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Language.hs` / `4810809113145806885f54154bb3fbdb2f553001a9fdc35730b348391d92ea06` | `0002/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/Language.hs` / `233660eff9726f07c0c4eec6db851f011aa00d6dd4b42d67584df18ff9c0f8f4` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ToPlutusData.hs` | changed | `0001/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ToPlutusData.hs` / `509183c7b0ae1b1fea4b4c9ae0fa44b1575a93a675882fc17a84004079079e67` | `0002/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/ToPlutusData.hs` / `13f1fa98c7d64177c90eee3d5813a900d7648edcd821c527d5e5fddd38890709` |
| `cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/TxInfo.hs` | changed | `0001/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/TxInfo.hs` / `0f358b95a13e726318ec1dd8677a296fb5378416143250ac882a6384b30a47b7` | `0002/artifacts/cardano-ledger/libs/cardano-ledger-core/src/Cardano/Ledger/Plutus/TxInfo.hs` / `e37ab30f595ae4aee1a71a494a1700530f0fc2606fd70ddcf3c894be79504a9c` |
| `conformance/README.md` | changed | `0001/artifacts/conformance/README.md` / `8b60dbbce1c2ee21f88eec91ed9e4948e9f00e2aa27d297177d2774ed4f79a88` | `0002/artifacts/conformance/README.md` / `e78b63737bfe74dd37c4bb46b512699fdcd15f508df235e5ff7bec15b7a1062c` |
| `conformance/corpus.json` | changed | `0001/artifacts/conformance/corpus.json` / `d1fb60fcd1f680667fe7133c292c24a8b484d1ba0585622d855f0aacb31d1ed5` | `0002/artifacts/conformance/corpus.json` / `974aeeb28b16ab8d0232b20535f382122f7329222399bc7839ae800c8e1df759` |
| `plutus/LICENSE.md` | unchanged | `0001/artifacts/plutus/LICENSE.md` / `43070e2d4e532684de521b885f385d0841030efa2b1a20bafb76133a5e1379c1` | `0001/artifacts/plutus/LICENSE.md` / `43070e2d4e532684de521b885f385d0841030efa2b1a20bafb76133a5e1379c1` |
| `plutus/NOTICE.md` | unchanged | `0001/artifacts/plutus/NOTICE.md` / `9e1fe670abe06d2cbdffa2d724765b5e86aa7f65d8eccd34b391a37fbf7bab89` | `0001/artifacts/plutus/NOTICE.md` / `9e1fe670abe06d2cbdffa2d724765b5e86aa7f65d8eccd34b391a37fbf7bab89` |
| `plutus/README.adoc` | unchanged | `0001/artifacts/plutus/README.adoc` / `47811f8c530b2a96db0b347be916f6a4063948b2e4e834148cdf0fdb1ca112ed` | `0001/artifacts/plutus/README.adoc` / `47811f8c530b2a96db0b347be916f6a4063948b2e4e834148cdf0fdb1ca112ed` |
| `plutus/plutus-conformance/LICENSE` | unchanged | `0001/artifacts/plutus/plutus-conformance/LICENSE` / `69ce94606a661fa3290eeaca21f3f9ad7d73dc985ffa346a1104704548f72d93` | `0001/artifacts/plutus/plutus-conformance/LICENSE` / `69ce94606a661fa3290eeaca21f3f9ad7d73dc985ffa346a1104704548f72d93` |
| `plutus/plutus-conformance/NOTICE` | unchanged | `0001/artifacts/plutus/plutus-conformance/NOTICE` / `9f5112659daf50eec7866133b2c11432547d132ec8253121ffc8503970197b34` | `0001/artifacts/plutus/plutus-conformance/NOTICE` / `9f5112659daf50eec7866133b2c11432547d132ec8253121ffc8503970197b34` |
| `plutus/plutus-conformance/README.md` | changed | `0001/artifacts/plutus/plutus-conformance/README.md` / `8bfe738336b8fc58745b18818082215d3b866e7a0651186af1b75b0d8c20f2ec` | `0002/artifacts/plutus/plutus-conformance/README.md` / `852f4bdb7afba613b9cb4199a22821b208cb991950594529b1d3241771df72c9` |
| `plutus/plutus-conformance/src/PlutusConformance/Common.hs` | changed | `0001/artifacts/plutus/plutus-conformance/src/PlutusConformance/Common.hs` / `aea3b958aca12484855a12814ccdb6b43825bc56f77d24ccb5e9cea257574acf` | `0002/artifacts/plutus/plutus-conformance/src/PlutusConformance/Common.hs` / `8eb319dd9b6fbe838513b4f5ba789d1efa15332b924bb180ea2575f290cc4b2a` |
| `plutus/plutus-core/cost-model/data/builtinCostModelA.json` | changed | `0001/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelA.json` / `0f6faf14e0d54a05bbcc3a3bd697b3bbe19e8b002f5f675f665c33e61522859a` | `0002/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelA.json` / `76e166858b763cc3ef815c4bcc903bef4b36dbd2d09411bb00c7dce9db5371a4` |
| `plutus/plutus-core/cost-model/data/builtinCostModelB.json` | changed | `0001/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelB.json` / `c78c4490e04959f5979a5c0e3b81cf29f9dfe352705d21da7fab7e1a2a6e9c96` | `0002/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelB.json` / `2faa8ff8d81f4f7c07fb5f13369a8c0367b721e21a152e2c5cb2cd5437398656` |
| `plutus/plutus-core/cost-model/data/builtinCostModelC.json` | changed | `0001/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelC.json` / `1556b0d8ee1652758a35fed8f894cef2335fab248575101aa0d1226fb2c996d7` | `0002/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelC.json` / `b35dd8a6c2755f6241b1fcbacb24d501d1d38b976da58903f2cb60032f0889ea` |
| `plutus/plutus-core/cost-model/data/builtinCostModelD.json` | changed | `0001/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelD.json` / `d019c35e677d27124f8ff05e3835f9e2f538a50f1139676d9678c1779b30f0d0` | `0002/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelD.json` / `428d796d5656731d44b763279ae146d663d7e39c96858255aec1d57fe89b5a9b` |
| `plutus/plutus-core/cost-model/data/builtinCostModelE.json` | changed | `0001/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelE.json` / `56abe89beca140e9e9bbb7f573829397a3021d6340e22f868406dc4c10df83f9` | `0002/artifacts/plutus/plutus-core/cost-model/data/builtinCostModelE.json` / `e3ba8e7095b04e3a9d468cf64389f2ebcd94a894cda40e3a5c85e477ac8472d6` |
| `plutus/plutus-core/cost-model/data/cekMachineCostsA.json` | unchanged | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsA.json` / `5457650b1e138f59888f2f7c486d8137fa0aa3c119b25931d130bbfd73e4a56f` | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsA.json` / `5457650b1e138f59888f2f7c486d8137fa0aa3c119b25931d130bbfd73e4a56f` |
| `plutus/plutus-core/cost-model/data/cekMachineCostsB.json` | unchanged | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsB.json` / `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsB.json` / `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` |
| `plutus/plutus-core/cost-model/data/cekMachineCostsC.json` | unchanged | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsC.json` / `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsC.json` / `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` |
| `plutus/plutus-core/cost-model/data/cekMachineCostsD.json` | unchanged | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsD.json` / `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsD.json` / `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` |
| `plutus/plutus-core/cost-model/data/cekMachineCostsE.json` | unchanged | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsE.json` / `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` | `0001/artifacts/plutus/plutus-core/cost-model/data/cekMachineCostsE.json` / `37b37d3c544b9fad73a112af2c8d8a9116f8639ea6b2e879f20bfafa0529a051` |
| `plutus/plutus-core/plutus-core/src/Codec/Extras/SerialiseViaFlat.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/Codec/Extras/SerialiseViaFlat.hs` / `0ed6622daa30737f5691f78056e3ff25538eab7e9c249273df180ca297b37d0c` | `0001/artifacts/plutus/plutus-core/plutus-core/src/Codec/Extras/SerialiseViaFlat.hs` / `0ed6622daa30737f5691f78056e3ff25538eab7e9c249273df180ca297b37d0c` |
| `plutus/plutus-core/plutus-core/src/Data/Vector/Orphans.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/Data/Vector/Orphans.hs` / `2d017a10136c1b516dfe0acdf501b67662495100540da982e532a32c59cf0381` | `0001/artifacts/plutus/plutus-core/plutus-core/src/Data/Vector/Orphans.hs` / `2d017a10136c1b516dfe0acdf501b67662495100540da982e532a32c59cf0381` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Bitwise.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Bitwise.hs` / `a18717fb9199d9489191372baa5974114a3b4ec90d15b6d8c9fe60e77f5d8ea9` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Bitwise.hs` / `a18717fb9199d9489191372baa5974114a3b4ec90d15b6d8c9fe60e77f5d8ea9` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin.hs` / `8b20f7142c91dd12521e14f896ad9b9319813031e1a8271ecb06be3af8b1b2a6` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin.hs` / `8b20f7142c91dd12521e14f896ad9b9319813031e1a8271ecb06be3af8b1b2a6` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/KnownType.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/KnownType.hs` / `d5c08abc0be1a33663c708799a3aaba74d41348774696567f09b59b9ce64860a` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/KnownType.hs` / `d5c08abc0be1a33663c708799a3aaba74d41348774696567f09b59b9ce64860a` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Meaning.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Meaning.hs` / `e5ad4d68a2a8ab3aa71427ddfb37ad649f3f6a8fabd4b20e334aed816a05e5d9` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Meaning.hs` / `e5ad4d68a2a8ab3aa71427ddfb37ad649f3f6a8fabd4b20e334aed816a05e5d9` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Result.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Result.hs` / `79f8202a565a883bfc7e5aa033228d0243b214cbbe982c0450202d32b97a663e` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Result.hs` / `79f8202a565a883bfc7e5aa033228d0243b214cbbe982c0450202d32b97a663e` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Runtime.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Runtime.hs` / `29f6241a275645f5c339c96edebbfbfbaaeb3fc47e832e1165b63c234d8cf290` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/Runtime.hs` / `29f6241a275645f5c339c96edebbfbfbaaeb3fc47e832e1165b63c234d8cf290` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/TypeScheme.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/TypeScheme.hs` / `b81f6fb30ae2b12f36e1fc74d7f9058a6e0386d5e8787f2509ee235e89370cf3` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Builtin/TypeScheme.hs` / `b81f6fb30ae2b12f36e1fc74d7f9058a6e0386d5e8787f2509ee235e89370cf3` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Bounds.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Bounds.hs` / `8255a49552e7d28781c21c238370dff4b01d8fcb9571f514c3889bccb11eac21` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Bounds.hs` / `8255a49552e7d28781c21c238370dff4b01d8fcb9571f514c3889bccb11eac21` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Error.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Error.hs` / `b2c02ed3ce7646902885f1727cf2654bd18ec0a428392b1938f3914652208a7e` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Error.hs` / `b2c02ed3ce7646902885f1727cf2654bd18ec0a428392b1938f3914652208a7e` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G1.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G1.hs` / `4c497eb561d8485759069c1c719d56c1f879ef890a3dedb62b6c63f8694bc152` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G1.hs` / `4c497eb561d8485759069c1c719d56c1f879ef890a3dedb62b6c63f8694bc152` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G2.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G2.hs` / `1ea930741576a7703515719edc4d92f5746a3e639ca1a306dd0d3c7330da8d18` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/G2.hs` / `1ea930741576a7703515719edc4d92f5746a3e639ca1a306dd0d3c7330da8d18` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Pairing.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Pairing.hs` / `8403c461db988da5f303b35adfd13c099798681ab23ed88cf30fbb9d842dfd4a` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/BLS12_381/Pairing.hs` / `8403c461db988da5f303b35adfd13c099798681ab23ed88cf30fbb9d842dfd4a` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Ed25519.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Ed25519.hs` / `9a6fa8a721e28d90483a6f23347e0f0aaf4cec7028e1717e7b586c7fcf028259` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Ed25519.hs` / `9a6fa8a721e28d90483a6f23347e0f0aaf4cec7028e1717e7b586c7fcf028259` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/ExpMod.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/ExpMod.hs` / `697727fe1e2aaa7c37f1abb3cc8d71d5333163fdd218d383fcce3f2b0eacaf08` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/ExpMod.hs` / `697727fe1e2aaa7c37f1abb3cc8d71d5333163fdd218d383fcce3f2b0eacaf08` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Hash.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Hash.hs` / `40d8f6e118f4d0e1f0f72714d19d3322e095691ec69c09a4118544f8acbd3113` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Hash.hs` / `40d8f6e118f4d0e1f0f72714d19d3322e095691ec69c09a4118544f8acbd3113` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Secp256k1.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Secp256k1.hs` / `e25b37a1056a14342b9f0e5591c9f9c0362b8c0edf1f4c5354bd83bbfcb7e644` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Secp256k1.hs` / `e25b37a1056a14342b9f0e5591c9f9c0362b8c0edf1f4c5354bd83bbfcb7e644` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Utils.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Utils.hs` / `06a2dae624cc3df9e02e64ad24d599c5485b12810a41381c2cd8cbe58bc463cf` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Crypto/Utils.hs` / `06a2dae624cc3df9e02e64ad24d599c5485b12810a41381c2cd8cbe58bc463cf` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Data.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Data.hs` / `e4633b7b293b4078387b107eb36d1f16864b93b7a129a7a2531493791fb5628c` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Data.hs` / `e4633b7b293b4078387b107eb36d1f16864b93b7a129a7a2531493791fb5628c` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/DeBruijn.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/DeBruijn.hs` / `ae7d44eaabdc5b58fbb01898c3472972ba69df9d46de0c785b515392082c48b2` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/DeBruijn.hs` / `ae7d44eaabdc5b58fbb01898c3472972ba69df9d46de0c785b515392082c48b2` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/DeBruijn/Internal.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/DeBruijn/Internal.hs` / `55b2928e0d2d0af4a246bfa21a790270af243f3b9d9336490ef1468d223053ec` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/DeBruijn/Internal.hs` / `55b2928e0d2d0af4a246bfa21a790270af243f3b9d9336490ef1468d223053ec` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Default.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default.hs` / `e11984b4e05701e101283742639fae96bb3d0c67122f79eb6ac771c0d74ee76d` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default.hs` / `e11984b4e05701e101283742639fae96bb3d0c67122f79eb6ac771c0d74ee76d` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Default/Builtins.hs` | changed | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default/Builtins.hs` / `5ee08d1f385a3372438d7fd2575ddcd32a2add779b9bb81fc45cd0ed4e5d0393` | `0002/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default/Builtins.hs` / `396226f91c048eee40b9bc3288e3441a46e737d6eb4a84d386ed9d2493da2218` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Default/Universe.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default/Universe.hs` / `88d257b863f3f5a70971a19b431725610cceec623ec132e2b8344a9efa2cc517` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default/Universe.hs` / `88d257b863f3f5a70971a19b431725610cceec623ec132e2b8344a9efa2cc517` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Default/Universe/Cardano.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default/Universe/Cardano.hs` / `353c896bb289591bb6e6ca323b63fa55bf7563fc77cd63f462aa61b74c031f3d` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Default/Universe/Cardano.hs` / `353c896bb289591bb6e6ca323b63fa55bf7563fc77cd63f462aa61b74c031f3d` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Error.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Error.hs` / `6b5d8035b535264fc133af41782ec5d03ea7baa40ebd0736db9367dc0e14c456` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Error.hs` / `6b5d8035b535264fc133af41782ec5d03ea7baa40ebd0736db9367dc0e14c456` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/BuiltinCostModel.hs` | changed | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/BuiltinCostModel.hs` / `49d80fe1636117d57268cf997c1abe9a8a23efa9981574246045a6d088101610` | `0002/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/BuiltinCostModel.hs` / `e4589f78ccebc3683f290b67d842cc759d436fe16e13b71ce2d5639986dfb28c` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostModelInterface.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostModelInterface.hs` / `f3286eacee23ab160a285195af0caf0c215f7395e0e6c88fcffe564bc96f925d` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostModelInterface.hs` / `f3286eacee23ab160a285195af0caf0c215f7395e0e6c88fcffe564bc96f925d` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostStream.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostStream.hs` / `46b33a311e77e1335dd4688e5e94cb966a48416ecdca3fdf32ea605af8116c3b` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostStream.hs` / `46b33a311e77e1335dd4688e5e94cb966a48416ecdca3fdf32ea605af8116c3b` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostingFun/Core.hs` | changed | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostingFun/Core.hs` / `acf2d2dd373d8763416d04125a8cc9a85a2bf59aebdf67859079eedc4b77e61b` | `0002/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/CostingFun/Core.hs` / `2b4a422186ebfd3bff4b889567cf6d1c3e2f5e5c06e8ba4ae6767a7913bc7178` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudget.hs` | changed | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudget.hs` / `45947e92230654f900799ff1c3d437e3bc287dc75f36c55a0d4567e45823c54e` | `0002/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudget.hs` / `747b74d7920b3be825e796a60e9699ea56a55cc2993bb84fb7ec36cab311d02a` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetStream.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetStream.hs` / `0f21b09e0252825c53ef83eeb19e23a8dd85fc269da495d172390b5ec4333da8` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetStream.hs` / `0f21b09e0252825c53ef83eeb19e23a8dd85fc269da495d172390b5ec4333da8` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetingDefaults.hs` | changed | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetingDefaults.hs` / `03fd2656f8c8aa82c5bedda46350bac03aed37b48d272795dd130e97c71a91f5` | `0002/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExBudgetingDefaults.hs` / `4d24c16963cdd46a7b6669c2d0d93845ac263dfe201c9415787fc7eac70ad8b6` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemory.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemory.hs` / `d6da66e49a1e38abd7a745999eb884433afc3a2306a73808e7016a5238ba7dac` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemory.hs` / `d6da66e49a1e38abd7a745999eb884433afc3a2306a73808e7016a5238ba7dac` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemoryUsage.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemoryUsage.hs` / `63b72128955389b0fc4134f5559b6521a69020d87d186df25790b57822866027` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/ExMemoryUsage.hs` / `63b72128955389b0fc4134f5559b6521a69020d87d186df25790b57822866027` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/Exception.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/Exception.hs` / `58140a97be5898cbe0cb6057a9eb55f4c48c0b4d09cbe69bcc1737111f917cc1` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/Exception.hs` / `58140a97be5898cbe0cb6057a9eb55f4c48c0b4d09cbe69bcc1737111f917cc1` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters.hs` / `e63180de6672fb5e16f974a00f4678f9e89ca07220284ff62460d1dcf724cf34` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters.hs` / `e63180de6672fb5e16f974a00f4678f9e89ca07220284ff62460d1dcf724cf34` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters/Default.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters/Default.hs` / `a8fff1b96f9b014b4a2f9b459ba89659da090d7a796215ef382bb899bb3ce92a` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/MachineParameters/Default.hs` / `a8fff1b96f9b014b4a2f9b459ba89659da090d7a796215ef382bb899bb3ce92a` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/SimpleBuiltinCostModel.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/SimpleBuiltinCostModel.hs` / `8b0dd3fc51ba9ef980a6a5f2fbe2ddc08aedb4d96070362b8d4a54be912ab1ef` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Evaluation/Machine/SimpleBuiltinCostModel.hs` / `8b0dd3fc51ba9ef980a6a5f2fbe2ddc08aedb4d96070362b8d4a54be912ab1ef` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/FlatInstances.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/FlatInstances.hs` / `693e1fa70b419e73de486bafec3f01d6e574ed0896a345d5688bc1925ed5107d` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/FlatInstances.hs` / `693e1fa70b419e73de486bafec3f01d6e574ed0896a345d5688bc1925ed5107d` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/MkPlc.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/MkPlc.hs` / `64ddf39f5a2a46114ead6113d95a76abcf24fc62a368315de5e06b4748a6aa50` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/MkPlc.hs` / `64ddf39f5a2a46114ead6113d95a76abcf24fc62a368315de5e06b4748a6aa50` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Value.hs` | changed | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Value.hs` / `325cd37de6c967f985c740e25fac30f9b68b9b4528af541d1294a63dd6883fd1` | `0002/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Value.hs` / `b1e15d2bf4fdeae9cdb79d1866426ba44200819ac9d5606f538d6b6cab25b1f0` |
| `plutus/plutus-core/plutus-core/src/PlutusCore/Version.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Version.hs` / `f0a48a92483a323fab9caa502de11850362fb4c7cc7cc0fd4e102486dd137e7c` | `0001/artifacts/plutus/plutus-core/plutus-core/src/PlutusCore/Version.hs` / `f0a48a92483a323fab9caa502de11850362fb4c7cc7cc0fd4e102486dd137e7c` |
| `plutus/plutus-core/plutus-core/test/CostModelInterface/Spec.hs` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/test/CostModelInterface/Spec.hs` / `fa0ea6df8addab0b586b24c407322ce16013899537097639cc2c863954328687` | `0001/artifacts/plutus/plutus-core/plutus-core/test/CostModelInterface/Spec.hs` / `fa0ea6df8addab0b586b24c407322ce16013899537097639cc2c863954328687` |
| `plutus/plutus-core/plutus-core/test/CostModelInterface/defaultCostModelParams.json` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/test/CostModelInterface/defaultCostModelParams.json` / `a47727bcdf68efeceac8c49f032d19682a374c14fede64392203d92279d3534f` | `0001/artifacts/plutus/plutus-core/plutus-core/test/CostModelInterface/defaultCostModelParams.json` / `a47727bcdf68efeceac8c49f032d19682a374c14fede64392203d92279d3534f` |
| `plutus/plutus-core/plutus-core/test/Flat/golden/encoding-stability.golden` | unchanged | `0001/artifacts/plutus/plutus-core/plutus-core/test/Flat/golden/encoding-stability.golden` / `794d5bdf0baa30dbe74478d3a414dc015e591456c7c99ca0859bff496325302d` | `0001/artifacts/plutus/plutus-core/plutus-core/test/Flat/golden/encoding-stability.golden` / `794d5bdf0baa30dbe74478d3a414dc015e591456c7c99ca0859bff496325302d` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Check/Scope.hs` | unchanged | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Check/Scope.hs` / `60442f821052511b5ccd73bca1d382570f17f8d4b719e8ff8c6b9cec24b0497f` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Check/Scope.hs` / `60442f821052511b5ccd73bca1d382570f17f8d4b719e8ff8c6b9cec24b0497f` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Instance/Flat.hs` | unchanged | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Instance/Flat.hs` / `ee762bda65323a0c6f270cad06b198b39728530980703ae235af26f695f32188` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Instance/Flat.hs` / `ee762bda65323a0c6f270cad06b198b39728530980703ae235af26f695f32188` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Type.hs` | unchanged | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Type.hs` / `902bb51a5ba193a5f7947b08c9b61d1632b358ddb9ac0bb7996841d51576f275` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Core/Type.hs` / `902bb51a5ba193a5f7947b08c9b61d1632b358ddb9ac0bb7996841d51576f275` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/DeBruijn.hs` | unchanged | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/DeBruijn.hs` / `22f6cb8f0da939345c3affd221ecea005f3fa676ccf7bd388770831a2066ac56` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/DeBruijn.hs` / `22f6cb8f0da939345c3affd221ecea005f3fa676ccf7bd388770831a2066ac56` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek.hs` | unchanged | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek.hs` / `567caff202fe0813c929c1af12e475fdaaf586cc75b89679c8630f96c827331c` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek.hs` / `567caff202fe0813c929c1af12e475fdaaf586cc75b89679c8630f96c827331c` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/CekMachineCosts.hs` | changed | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/CekMachineCosts.hs` / `96a1fa30b7736a1ed3674ddc200298033edc6ded4ba34c9f4b516b69526a32f7` | `0002/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/CekMachineCosts.hs` / `b3bb420a2bb58ee650512984d565c004b88485dd5d26f5b1e4b2082619f3c8b1` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/EmitterMode.hs` | unchanged | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/EmitterMode.hs` / `52e453ee3b1d1378c570fe613b5a81ee25d81b9532ca9f1cf8ab7258614a62d6` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/EmitterMode.hs` / `52e453ee3b1d1378c570fe613b5a81ee25d81b9532ca9f1cf8ab7258614a62d6` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/ExBudgetMode.hs` | unchanged | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/ExBudgetMode.hs` / `dc79de42f831b3c197f4fb6d51f5f7f9b2381f9da06022bf8e359311ed35aa81` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/ExBudgetMode.hs` / `dc79de42f831b3c197f4fb6d51f5f7f9b2381f9da06022bf8e359311ed35aa81` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/Internal.hs` | unchanged | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/Internal.hs` / `f7483a9661dc5ff5291140dc4c6f4609911e6f6d5f54b043bf98a0cb88090527` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/Cek/Internal.hs` / `f7483a9661dc5ff5291140dc4c6f4609911e6f6d5f54b043bf98a0cb88090527` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/CommonAPI.hs` | unchanged | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/CommonAPI.hs` / `20fd7fdccd4fb00c86ea158341f4802acffc03aeb03cb53a21083970fbdf2804` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/Evaluation/Machine/CommonAPI.hs` / `20fd7fdccd4fb00c86ea158341f4802acffc03aeb03cb53a21083970fbdf2804` |
| `plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/MkUPlc.hs` | unchanged | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/MkUPlc.hs` / `f1eec0e2237a8e1d8065011a3cf51b159255124c63d0937037d9609cc1bfb7e2` | `0001/artifacts/plutus/plutus-core/untyped-plutus-core/src/UntypedPlutusCore/MkUPlc.hs` / `f1eec0e2237a8e1d8065011a3cf51b159255124c63d0937037d9609cc1bfb7e2` |
| `plutus/plutus-ledger-api/CostModel/Params/CostModelParams/costModelParamNames.golden.txt` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/CostModel/Params/CostModelParams/costModelParamNames.golden.txt` / `93898d8fd636685fae59dbdb861d2e64e8d45700e0fcc9f5eda584c65f6387f8` | `0001/artifacts/plutus/plutus-ledger-api/CostModel/Params/CostModelParams/costModelParamNames.golden.txt` / `93898d8fd636685fae59dbdb861d2e64e8d45700e0fcc9f5eda584c65f6387f8` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Eval.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Eval.hs` / `9844d259cff614a988a042fff430ede2f9670f1757a06c5c0953c3900f17935b` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Eval.hs` / `9844d259cff614a988a042fff430ede2f9670f1757a06c5c0953c3900f17935b` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ParamName.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ParamName.hs` / `cb63e7b54aafb67b3e69ef113845613db2eef40733b6a385d90d3a53ef3dcf28` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ParamName.hs` / `cb63e7b54aafb67b3e69ef113845613db2eef40733b6a385d90d3a53ef3dcf28` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ProtocolVersions.hs` | changed | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ProtocolVersions.hs` / `ce06ce9bf577415903fa8241293a6a94f7b1f1452a4f9c570303e8f91b841f2b` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/ProtocolVersions.hs` / `db59d72270205b84d06c861301cf71c720659eea3eff1d5c009002ccf90e2927` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/SerialisedScript.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/SerialisedScript.hs` / `7feb2ddd0001d48e81db2246a2fda592f2b6cae5f98422cfcbb744b7befdd412` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/SerialisedScript.hs` / `7feb2ddd0001d48e81db2246a2fda592f2b6cae5f98422cfcbb744b7befdd412` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs` | changed | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs` / `137a302960f24aeb8b5ded13fd8431194876bde123bc27c5d030dc4e038c87c9` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs` / `c9ac5701e9d8554fc40ecaed7a92d705b7096d553502aa39a54a5f5d141d5a8a` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V1.hs` | changed | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V1.hs` / `f0edecd396506e2e1aa38ff32e0ab9065a804cad76b79275cb721d43c652a231` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V1.hs` / `da8f638e2bcf537dbc0640e96e61ffded579abd51f9657ce8aedff9fc4677595` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V2.hs` | changed | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V2.hs` / `d65557b5ca35ec395d785a6e9dcd337d39d19869c8a68f4d132dc2619fcbcced` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V2.hs` / `de749c8f1b2d5f6d81b6d5760935c748f60fc0e7346e3e41b9c13aa9f5536974` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V3.hs` | changed | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V3.hs` / `47f85dfe0013df3adc94e032f7df591cc5f06c9d0451add0fb0162d2e2f0fe65` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/Data/V3.hs` / `2766e0a170470dad2b7aea34b4ab196f4d4ec9f33e055dc0b05b05b1708fdb17` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/MachineParameters.hs` | changed | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/MachineParameters.hs` / `7f76270784a1482ab1e38210ffa57510a542aee662cb614ac4dd05191c56d462` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/MachineParameters.hs` / `39749ecc7d171a033622265393604ef4bf6722f0da8e4dc225588b604bca26ce` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Contexts.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Contexts.hs` / `6b4d6c913959ff70e38b6fa6b0053d4bab957e7b7d7b1443c2d54c97092b332f` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Contexts.hs` / `6b4d6c913959ff70e38b6fa6b0053d4bab957e7b7d7b1443c2d54c97092b332f` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Address.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Address.hs` / `eb68fad824b70953c5b107bf5ed883d2475d915c8c0c176fd8f18d53a708161c` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Address.hs` / `eb68fad824b70953c5b107bf5ed883d2475d915c8c0c176fd8f18d53a708161c` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Contexts.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Contexts.hs` / `501df38b935fda1039d543bb341a1b2c720f833f6a98bbe5948d1e71a0eff6b8` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Contexts.hs` / `501df38b935fda1039d543bb341a1b2c720f833f6a98bbe5948d1e71a0eff6b8` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Credential.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Credential.hs` / `9930b6b0097aa83646defc958b82e71f47c14cb62fe4b2d7f9ced26235eefa81` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Credential.hs` / `9930b6b0097aa83646defc958b82e71f47c14cb62fe4b2d7f9ced26235eefa81` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/DCert.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/DCert.hs` / `858c7a0b2e58b6309b860be8c78b0f18fffe211ec1e258e91307f9593d48ec1d` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/DCert.hs` / `858c7a0b2e58b6309b860be8c78b0f18fffe211ec1e258e91307f9593d48ec1d` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Interval.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Interval.hs` / `243952f5d415c106f4db54b477d0788ea7aec9c99014057e3e888e6576a78b8f` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Interval.hs` / `243952f5d415c106f4db54b477d0788ea7aec9c99014057e3e888e6576a78b8f` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Time.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Time.hs` / `7286d160c1de7c69c00212c23176891229ed0286b3556b55ce0d3ddd5689b116` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Time.hs` / `7286d160c1de7c69c00212c23176891229ed0286b3556b55ce0d3ddd5689b116` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Tx.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Tx.hs` / `782d3a6f7ee4c2f88d5c30165332b428c77f4bd80f8e4d2f272438c6fcf5a84e` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Tx.hs` / `782d3a6f7ee4c2f88d5c30165332b428c77f4bd80f8e4d2f272438c6fcf5a84e` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Value.hs` | changed | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Value.hs` / `5baed5f10b100d64622520ff62b905deebbce5207a4416a4841d11c255c7bb0b` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Data/Value.hs` / `b6015c7acfbde9edc772bf7c48c756fea1df991300a84a492ccd80d7b0201eed` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/EvaluationContext.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/EvaluationContext.hs` / `cc3f68cfa670dea838715512597d32c1bce1c77515f8fcef6632fc4e640bfb63` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/EvaluationContext.hs` / `cc3f68cfa670dea838715512597d32c1bce1c77515f8fcef6632fc4e640bfb63` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/ParamName.hs` | changed | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/ParamName.hs` / `3a39cec1e520a42d860ad1ca9f2a9213e878a6e97dd00b35acedeccd938914db` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/ParamName.hs` / `f6a74dfe889fcaa4061fbd32b63fdeb022e57dbb5e8bd38385bf96df8ea29126` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Scripts.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Scripts.hs` / `562806a9b75d48187d8a1c4ac66778c0c7e71001a766d37061075504e32618dc` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Scripts.hs` / `562806a9b75d48187d8a1c4ac66778c0c7e71001a766d37061075504e32618dc` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Tx.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Tx.hs` / `95e69701c55f3762ff6d6c07127b7a78ad30e6b12fdc572bdbc6727e9d6a311f` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Tx.hs` / `95e69701c55f3762ff6d6c07127b7a78ad30e6b12fdc572bdbc6727e9d6a311f` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Value.hs` | changed | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Value.hs` / `046b52475911fa325ad909a06d87fcbdea47ace874a4280400f6e9e7c118b565` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V1/Value.hs` / `06168acf83fd8f00580a988376decde17216a767c118f34f0b75c78d076bba1a` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Contexts.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Contexts.hs` / `ca07745f836b291009a7df9986ceb1ae12f9605938222d8b370243194cc34767` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Contexts.hs` / `ca07745f836b291009a7df9986ceb1ae12f9605938222d8b370243194cc34767` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Contexts.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Contexts.hs` / `81ecd549083deb3b436b96468545463fe9fff6b95bdbe8d8f3e7e809654ac31a` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Contexts.hs` / `81ecd549083deb3b436b96468545463fe9fff6b95bdbe8d8f3e7e809654ac31a` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Tx.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Tx.hs` / `d01f63d4d292795350cfdc5432ae418638c9057ae0fd8f8abe5f8f17d05a975c` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Data/Tx.hs` / `d01f63d4d292795350cfdc5432ae418638c9057ae0fd8f8abe5f8f17d05a975c` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/EvaluationContext.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/EvaluationContext.hs` / `5636a676507259548d9214d2493b9b442966350e30ed38dc8390d502a9dded1d` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/EvaluationContext.hs` / `5636a676507259548d9214d2493b9b442966350e30ed38dc8390d502a9dded1d` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/ParamName.hs` | changed | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/ParamName.hs` / `5d5a8e3a1f4623a081b3812aef9f30c66c73bb3d39807abe1b3262a2f77402bc` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/ParamName.hs` / `b6ae8d3524dce04ede0f320c7958c4bafc1ac75b18da4a5280c5ef6dd946c712` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Tx.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Tx.hs` / `484e75d3c04874a034c5c7c07f1c741f8179a519f624e5f1ea02f3bac49c773e` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V2/Tx.hs` / `484e75d3c04874a034c5c7c07f1c741f8179a519f624e5f1ea02f3bac49c773e` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Contexts.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Contexts.hs` / `206fe185a66c79df8f095b350b4d23796220001a254ab687a9893ce21b301af4` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Contexts.hs` / `206fe185a66c79df8f095b350b4d23796220001a254ab687a9893ce21b301af4` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Contexts.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Contexts.hs` / `d0d049d095d1e91480ecfaa9e83e18afd19b49ebe3d2880638efa1d16c9b0261` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Contexts.hs` / `d0d049d095d1e91480ecfaa9e83e18afd19b49ebe3d2880638efa1d16c9b0261` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/MintValue.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/MintValue.hs` / `dd6674980a22a4e9d01a535652b03bef434b6c2ddd9f0f85d234a0d244786f02` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/MintValue.hs` / `dd6674980a22a4e9d01a535652b03bef434b6c2ddd9f0f85d234a0d244786f02` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Tx.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Tx.hs` / `e6e942aba7f44cc03088455208e858638191a0ca0e41dbc6e1edf98aa5daa268` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Data/Tx.hs` / `e6e942aba7f44cc03088455208e858638191a0ca0e41dbc6e1edf98aa5daa268` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/EvaluationContext.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/EvaluationContext.hs` / `efc693f86c5b803f694d7a683f1fe53851f2a16e690fb1d2573a369decb23268` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/EvaluationContext.hs` / `efc693f86c5b803f694d7a683f1fe53851f2a16e690fb1d2573a369decb23268` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/MintValue.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/MintValue.hs` / `0278e5fa319658f50ffad9ae856908ad3c483094a1a29b1ce1483c0e3c10aeea` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/MintValue.hs` / `0278e5fa319658f50ffad9ae856908ad3c483094a1a29b1ce1483c0e3c10aeea` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/ParamName.hs` | changed | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/ParamName.hs` / `ebf3d1c938af1168f8e79f816e638366a89d2c6651e11e13bce5d2c039052369` | `0002/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/ParamName.hs` / `1b02b46b25a51a90259a2947efa4df1affc60af498895be93d1a58b0a26f4d87` |
| `plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Tx.hs` | unchanged | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Tx.hs` / `e312e1cb5ab8e2b4bd48402235f2daf6e999d6c205a9a5c823f65fe954239313` | `0001/artifacts/plutus/plutus-ledger-api/src/PlutusLedgerApi/V3/Tx.hs` / `e312e1cb5ab8e2b4bd48402235f2daf6e999d6c205a9a5c823f65fe954239313` |

Corpus source-path comparison from the original capture: 1,636 unchanged, 170 changed, 3,016 added and 1,207 removed; these are transport-path counts, not inferred semantic additions/removals.

## Development reconciliation provenance

- Explicit human request: update installed SPECTRE and reconcile the existing 0002 captures with numbered incremental updates.
- Reconciled at: 20260909T170023Z.
- Previous descriptor path: `0002-uplc/SNAPSHOT.md`.
- Previous descriptor SHA-256: `f4bfd924768f084931a8a032cb659848d0d8860022652c0c6da02b9b9cf99b54`.
- Previous complete effective-inventory SHA-256: `45b0acf6076152a8ca5100c5b9442e43fe5ed0fb5e4469b910820f34e25fb049` (canonical compact JSON mapping logical path to bytes/sha256).
- Retained original Created value and upstream identities. Renamed this untracked capture to 0002,
  separated summary from specification, replaced duplicate files with references, and rebased
  generated controls. This is an explicitly requested development reconciliation, not a new
  capture, implicit exception for future edits, or claim that upstream validation was rerun.
- Rebased controls: `SHA256SUMS`, `conformance/README.md`.
- All original upstream source bytes and corpus content are preserved. The 0001 snapshots and all
  implementation instructions, results, ledger states and archived decisions are unchanged.
- Compare the frozen source table, resolved inventory and checksums before consumption. Use
  [CAPTURE.md](CAPTURE.md) for advisory findings, not verification rules.
