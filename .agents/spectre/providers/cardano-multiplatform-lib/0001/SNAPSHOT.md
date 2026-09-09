# Cardano Multiplatform Lib provider snapshot

Provider-Snapshot-Version: v1
Snapshot: 0001
Provider: cardano-multiplatform-lib
Created: 20260723T122735Z
Previous-Snapshot: NONE
Source-Type: git
Source-Repository: https://github.com/dcSpark/cardano-multiplatform-lib.git
Source-Commit: 39681e0d435a71f7c47a2601507ab16e691abb9e
Source-Ref: NONE
Source-Tag: NONE
Source-URL: NONE
Source-SHA256: NONE

## Evidence objective

Preserve the frozen Cardano Multiplatform Lib CDDL comparison baseline and reusable historical
test vectors selected by the snapshot specification.

## Frozen capture specification

The rules below preserve this capture’s original source selection, mappings, transformations,
completeness, licensing and consumer boundaries. They apply only to the immutable identities
recorded above. Discovery/ref and future-planning statements describe the original capture context;
they cannot retarget this snapshot. Repository-root paths remain repository-root-relative;
`artifacts/` paths resolve from this directory. No mutable provider guide supplies normative rules.

### Purpose

This capture preserves the historical CDDL comparison baseline and test-vector artifacts required
by XRAY Cardano Lib's compatibility tests. Its exact source identity and rules remain frozen.

### Source

| Field | Value |
| --- | --- |
| Repository | `https://github.com/dcSpark/cardano-multiplatform-lib.git` |
| Exact commit | `39681e0d435a71f7c47a2601507ab16e691abb9e` |
| Git tree | `172d2a1d1b47968592ec408ea0411ee108ae47fe` |
| Revision policy | Exact commit above for this capture |
| Source mode | Immutable captured evidence |
| Submodules | Not part of the source |
| License | MIT |

The three CML license artifacts are `LICENSE`, `LICENSE-EMURGO`, and `LICENSE-IOHK`.

### CDDL and legal artifact selection

Preserve specification paths exactly:

| Upstream selection | Expected files | Snapshot destination |
| --- | ---: | --- |
| `specs/cip25.cddl` | 1 | `artifacts/specs/cip25.cddl` |
| the three selected files below `specs/cip36/` | 3 | `artifacts/specs/cip36/` |
| the selected CDDL files below `specs/conway/` | 10 | `artifacts/specs/conway/` |
| the selected CDDL files below `specs/multiera/` | 16 | `artifacts/specs/multiera/` |
| the selected CDDL files below `specs/multiera-byron/` | 8 | `artifacts/specs/multiera-byron/` |
| `LICENSE`, `LICENSE-EMURGO`, `LICENSE-IOHK` | 3 | `artifacts/legal/` |

The selected CDDL inventory is exactly:

```text
specs/cip25.cddl
specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl
specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_crypto/mod.cddl
specs/cip36/lib.cddl
specs/conway/address.cddl
specs/conway/assets.cddl
specs/conway/auxdata.cddl
specs/conway/block.cddl
specs/conway/certs.cddl
specs/conway/crypto.cddl
specs/conway/governance.cddl
specs/conway/lib.cddl
specs/conway/plutus.cddl
specs/conway/transaction.cddl
specs/multiera-byron/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl
specs/multiera-byron/byron/block.cddl
specs/multiera-byron/byron/delegation.cddl
specs/multiera-byron/byron/mod.cddl
specs/multiera-byron/byron/mpc.cddl
specs/multiera-byron/byron/transaction.cddl
specs/multiera-byron/byron/update.cddl
specs/multiera-byron/lib.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/assets.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/auxdata.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/block.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/certs.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/crypto.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/mod.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/plutus.cddl
specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/transaction.cddl
specs/multiera/allegra/mod.cddl
specs/multiera/alonzo/mod.cddl
specs/multiera/babbage/mod.cddl
specs/multiera/lib.cddl
specs/multiera/mary/mod.cddl
specs/multiera/shelley/mod.cddl
```

Reject missing, additional, renamed, symlinked, or non-CDDL files within the selected scopes. The
reference-only `specs/byron.cddl`, `specs/byron_minimal.cddl`, `specs/shelley.cddl`, and
`specs/README.md` are excluded from the reviewed baseline.

### Test-vector artifact selection

Copy these regular files byte-for-byte from the exact CML commit, using this deterministic mapping:

| Upstream selection | Files | Snapshot destination |
| --- | ---: | --- |
| `chain/rust/src/genesis/byron/test_data/*.json` | 4 | `artifacts/test-vectors/genesis/byron/` |
| `chain/rust/src/genesis/shelley/test_data/{test.json,test-yaci.json}` | 2 | `artifacts/test-vectors/genesis/shelley/` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/*.cbor` | 34 | `artifacts/test-vectors/blocks/mainnet/` |
| `multi-era/rust/tests/golden_vectors/pallas/*.block` | 52 | `artifacts/test-vectors/blocks/pallas/` |
| `multi-era/rust/tests/golden_vectors/PROVENANCE.md` | 1 | `artifacts/test-vectors/PROVENANCE.md` |

Do not select additional files from either `test_data/` tree. Pallas `u5c*` vectors are excluded.
Preserve all 92 vector bytes and the upstream provenance document exactly; only the deterministic
directory mapping changes.

Create these snapshot-local control artifacts:

- `artifacts/test-vectors/manifest.json`: authoritative deterministic inventory of all 92 vectors,
  including logical path, tracked snapshot path, stored size, SHA-256, storage form, era metadata
  where applicable, and expected result.
- `artifacts/test-vectors/PROVENANCE.json`: CML source identity, manifest location, supplemental
  checksums, and the pinned Dolos/Pallas source and license mappings carried by CML.
- `artifacts/test-vectors/README.md`: corpus layout, path mapping, refresh policy, and consumer
  guidance.
- `artifacts/test-vectors/.gitattributes`: disable text conversion for block, CBOR, and JSON bytes.
- `artifacts/test-vectors/LICENSE-APACHE-2.0.txt`: Apache-2.0 text for the Dolos/Pallas vectors,
  with SHA-256 `4541e95aa81113643b71a96d7ff673c4a83ede3d2e8f0df2ad676e7970e2b1fa`.

These five files are deterministic snapshot metadata rather than byte-exact CML source. Package
tests must validate the manifest, provenance, licenses, and local Git attributes.

### Consumption and planning requirements

- Preserve the stable logical `specs/` prefix and the
  `_CDDL_CODEGEN_EXTERN_DEPS_DIR_` specification scopes.
- Preserve deterministic provenance, exact inventory, current owned-source behavior, and public
  API.
- Package tests consume vectors directly from `artifacts/test-vectors/`; do not duplicate them
  below packages, a root fixture directory, or another provider.
- Later snapshots may reuse this corpus only by its full immutable snapshot path and must name it
  under `Comparison sources` and in the artifact/change map.
- Replacing the frozen CML baseline requires an explicit new snapshot plan.

The `plutus.cddl` files describe ledger data and transaction wire grammar. They do not define or
authorize UPLC language or evaluator work.

### Excluded source material

- Reference-only CDDL outside the selected 38-file baseline
- Rust source, Rust test code, unrelated test data, build configuration, and Git metadata
- Pallas `u5c*` vectors
- Message-signing functionality
- UPLC language parsing, evaluation, Flat encoding, or builtin semantics

### Verified artifact inventory

This table freezes all 139 regular artifacts, including existing control files. Paths are
relative to this snapshot. Hashes and sizes were verified during the metadata conversion on
2026-09-09; this local verification does not claim a new source-tree capture or upstream audit.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `artifacts/legal/LICENSE` | 1064 | `456c5929a8238677b7ef27be10ae3fcc9f98e7799f8f27ce1e10a4f21824f2b0` |
| `artifacts/legal/LICENSE-EMURGO` | 1191 | `9725d2d36969b002ecf53e7834d7fc1f357264fa1f7414809862b7a2a60af1c8` |
| `artifacts/legal/LICENSE-IOHK` | 1063 | `d38373e47d0203e1338c85b9ca50a7e6497b48ca42334c2b920d3f359d1bd3b6` |
| `artifacts/specs/cip25.cddl` | 1589 | `7dd5c249e598d7b28ce9279ba937e9cbe538191d5c7002e765188c0e998c5794` |
| `artifacts/specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl` | 66 | `0ad27753bd73c39f4c25c3caf5e6e69b658c8af2f29eb1e9fddcb5448fc494b2` |
| `artifacts/specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_crypto/mod.cddl` | 93 | `c3d4a5d4343287a097450394346c3715bb43013326c972c3b702076d2ffb54ad` |
| `artifacts/specs/cip36/lib.cddl` | 1854 | `d16f45ba77813b5895337e5a2660c94d7b161db9601bd3738ca896ac101ab849` |
| `artifacts/specs/conway/address.cddl` | 1133 | `5c54dd0b87a8d40769692b2ef0393829e8b04932b6e5d777c75d1fe2c4c466de` |
| `artifacts/specs/conway/assets.cddl` | 1124 | `7f010bd3895812459ec1ce1b499ab99d12c18fd75b14b3aa6895b06ec78e57b6` |
| `artifacts/specs/conway/auxdata.cddl` | 1065 | `38fe486e8267d52ae446282607de9819b223dead4186a7712293f4704259cb85` |
| `artifacts/specs/conway/block.cddl` | 1210 | `a6bb0ee868346a57d9472c21a57191b0db2007759cf4c1827b015084f957a00f` |
| `artifacts/specs/conway/certs.cddl` | 4325 | `b900b298cb648cba94143f359a8ac7234b43018531f03296545678db690a8863` |
| `artifacts/specs/conway/crypto.cddl` | 2388 | `7f368d7b703a3e18cdcbcff85fa7dc2b1e135681b9d3723c7746025fc1821b5f` |
| `artifacts/specs/conway/governance.cddl` | 1732 | `bd37f836db61a756eed5377d1172edcc2fc12c2fcff86707806908fe2bda51ff` |
| `artifacts/specs/conway/lib.cddl` | 3768 | `c9f1cab4068bb5cf42e953da9f0fd3b932f934b59ba798818de232c1ae1717a7` |
| `artifacts/specs/conway/plutus.cddl` | 2590 | `e42786e438dbc1530d9ca00b11480320193b72a82ca28d1ffe5f14a400e7a66e` |
| `artifacts/specs/conway/transaction.cddl` | 7593 | `f79055a97a1e8ded3b19cc2eefca5c3cf16b630c6a206f887372d83c8cf7aafb` |
| `artifacts/specs/multiera-byron/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl` | 1109 | `22aaa8f996981e1dc74807a8954191fa922664d4b3a1484eeb831b860061803a` |
| `artifacts/specs/multiera-byron/byron/block.cddl` | 2563 | `a70e75c91a91f01c0f193b7554392ce0314bdfd15b0b9ffd9d0f199a621b229f` |
| `artifacts/specs/multiera-byron/byron/delegation.cddl` | 576 | `645839665e734498f97b5dbfe97a23a508189528812710b9af96214fbfc30876` |
| `artifacts/specs/multiera-byron/byron/mod.cddl` | 793 | `8a83ab9181d4f3d93c020f2bf8794ed40ca8522f210c80e1808f11601948e12c` |
| `artifacts/specs/multiera-byron/byron/mpc.cddl` | 3092 | `05a00b631ee8b62648b0002dd42a957452aa22de8f2ec8d32fdd54d7eac67501` |
| `artifacts/specs/multiera-byron/byron/transaction.cddl` | 2094 | `7afd0131c3a13040cba3ef29708590ac86616f811f591aa230b84e002fa061dc` |
| `artifacts/specs/multiera-byron/byron/update.cddl` | 2586 | `f3993321f4e870bdd0f2b874778bb1393ee81e0cc183e8f08d03b214216889ad` |
| `artifacts/specs/multiera-byron/lib.cddl` | 266 | `ea5cbfaa05fd96855d2477ab74dbfb8cbfac9ad11e0ff3f43975ddfea15d42c4` |
| `artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl` | 80 | `265b63c66f750d6d5bf3cae5f30f3e021173f69c772c3b793de9d418d246f33d` |
| `artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/assets.cddl` | 255 | `3c9829a602425689817aca0865a9204500570d2373b8ef482140b2a1e45b9e84` |
| `artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/auxdata.cddl` | 146 | `4dfde4f81edabdfd92a2d276f79a2fbbc209d88c27ba5ab3077fc66231cb07b9` |
| `artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/block.cddl` | 243 | `143579223d49d8e83814d439991d7128b151cba43cb60651841bbe25a31186a4` |
| `artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl` | 970 | `d6147183d31e337b58ac792da3935b983ed8b888a977b37952edbc952ffbe082` |
| `artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/certs.cddl` | 653 | `612076245460d3804290f603382b64bce2df4333ecc6e1bcdaa310e721d2642b` |
| `artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/crypto.cddl` | 1100 | `c3ccb293a93c38530e68e38838c09e6c0fee03795c64345f0f9fb2515f5d741d` |
| `artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/mod.cddl` | 1072 | `93a95689175ecf1322a9ae8e0d004cfd6029ec19175eee5ef8b5ba47f4002e66` |
| `artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/plutus.cddl` | 255 | `37577835deeccdd70854174f78de212d39fa11317bface84bc973c4553ea8996` |
| `artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/transaction.cddl` | 227 | `10cfe0ef7533a9b72b02ef8f6d41f5bf545e8e3cc71c5170acdbbfa476b404c4` |
| `artifacts/specs/multiera/allegra/mod.cddl` | 2274 | `1cfd5f0bab1d882bab448352a2ad1ed2fbe453855a64bdc8f802d4f53210347c` |
| `artifacts/specs/multiera/alonzo/mod.cddl` | 4405 | `50c341fefbb7fd0422e1d55b3eb028283c7dd0201f2ba3f25807f10818056d4d` |
| `artifacts/specs/multiera/babbage/mod.cddl` | 4957 | `dc29a2c2fe0a2b96b54242c918c9d627447a805acc34e72cf84899b53e7ad313` |
| `artifacts/specs/multiera/lib.cddl` | 790 | `9ad13611cf3287026dd56105dbafab683740b8cbd4a9f7b3c454d742687df2e5` |
| `artifacts/specs/multiera/mary/mod.cddl` | 1303 | `069869c505505948a5a4799ccd6dc836a2ed07640dcb22d976ff6536edeb0eaf` |
| `artifacts/specs/multiera/shelley/mod.cddl` | 5089 | `39f4564a3bb9f85780b07c74df9092611bb1c9c01464da4275813b48a0fe690a` |
| `artifacts/test-vectors/.gitattributes` | 49 | `31b52b337518c738389224eba47ae83952d346d49c2766d5dd2101af90c7c0be` |
| `artifacts/test-vectors/LICENSE-APACHE-2.0.txt` | 11347 | `4541e95aa81113643b71a96d7ff673c4a83ede3d2e8f0df2ad676e7970e2b1fa` |
| `artifacts/test-vectors/PROVENANCE.json` | 986 | `798da8585e837153850d06cdb437b4b1ad4768033d3a2ce24ac79e7d6259cedd` |
| `artifacts/test-vectors/PROVENANCE.md` | 2547 | `f36364497bd77fa917b2fc239e72fa87b0e7c31405bcee4e385ff8191b53db01` |
| `artifacts/test-vectors/README.md` | 1307 | `1e12b5ba2618dbc6f8273a7c2b9902dcc35e223cbfb2efa859f1b80074d8cf89` |
| `artifacts/test-vectors/blocks/mainnet/0822e72ec531fd72b74af75bbce83876547f538ca7ca9bc854d043aa888c478c.cbor` | 55683 | `c7c3dc18f68bfc66a67a4f9442a75bc13a6504ddfb3372b45fa1a1304a2552df` |
| `artifacts/test-vectors/blocks/mainnet/0ade775289bbf05b0ed77052bac35cc1ef728751fc00081d179d9f6f7728f56d.cbor` | 2122 | `aa3933a5a6e8a30182d74c468e847cf29fcf55a9e6f919c2509ae81ad78f4bdc` |
| `artifacts/test-vectors/blocks/mainnet/11c08542cf8da1ab1d1686c259dfc922e492c8ae893bccb588059f62717e573a.cbor` | 80134 | `685d59f5b3461c4bada700a35244e0e013135dea847bea03eef50b81da250239` |
| `artifacts/test-vectors/blocks/mainnet/18d49cdcae701d93ec69efd1f8140de6e5337a735a162f5e509e91b8f9ee1c18.cbor` | 1948 | `b61cb6ceabf11a3168f45bf98def6c75f1f8d0e6ea042905180e49f0d6b06f30` |
| `artifacts/test-vectors/blocks/mainnet/28532f081779a6c435c31ecc64f95a39417c6edce001fd9a964c622e18de95f4.cbor` | 29618 | `98f8d506ea4fba4ba70d6b422c82a2b0ff082961dc30b12c406d7d2935009c74` |
| `artifacts/test-vectors/blocks/mainnet/36cb7c4cb0053e112f57cde99204c785a4375fb7c6ac3eabe3a02fdc2d4a2867.cbor` | 66289 | `80555c27072199dbb07cc847669d3ff84910715ac6d404e20fb6a4f89220fe91` |
| `artifacts/test-vectors/blocks/mainnet/3b234ca0d5ac7c52a5407b63b358391b1b920c17bb40fe50c9fbbd76f77e6f67.cbor` | 2126 | `bb473e7d2d5d9ea8ff05bf3c62b1495c28be58096e19df2f6d89c8d62425c74a` |
| `artifacts/test-vectors/blocks/mainnet/3ba7cdc89bf74fc1915da5bd2a593231b70734d30c8a4c61e40e01ad345ca092.cbor` | 1354 | `7f7cf88255852249332b8a9f7e5cdbb053dfbf18403704b70cdb0b19c6615ab7` |
| `artifacts/test-vectors/blocks/mainnet/44b156deb04b7f923e6716fbc8dbda23ba8d748e6dcc08ded4914b5985524ce3.cbor` | 2157 | `b6e4780cf4847fc7382247c03f33383670bd1663a15c89ef178d49b51bacc907` |
| `artifacts/test-vectors/blocks/mainnet/55e828f124ef484d24b9ab60d25549f3ffb4999a9b9816904038e53aac2a5cc9.cbor` | 9226 | `db1454be87b95c3e9803109ef920e3ab9dcfc19658dffecd7243f13556cf8917` |
| `artifacts/test-vectors/blocks/mainnet/56f45594dbd995ae19a892d94db94962ecfab2d016ad387bd634f54ba750c85d.cbor` | 1765 | `0c4325480c1fbb9b8a7031fc784c93cf9b6beff637e5b085328492136d8dc140` |
| `artifacts/test-vectors/blocks/mainnet/5740eb9b6bb6207b7b1cc663e83532fa51fe826cf61714e747339d134b201681.cbor` | 3009 | `f1c822448d15ced664145f333c6ad7f24be366523e1ada6301adff7c5a84c427` |
| `artifacts/test-vectors/blocks/mainnet/586525732f41fa76f2c3b6d97831c2b68158b3429901e5c3c4f39506d1029f55.cbor` | 944 | `d00b1723df729a9e868099c9eda2f5900e3b967ddbbe7bd76ed1e00c34533bc9` |
| `artifacts/test-vectors/blocks/mainnet/61cc265848410b4fb43daeef29240bae841dfa4389d201bcf2d376922fe14a83.cbor` | 1945 | `01c7fffc4c606c89df54de729dda8e5bb2224c8488ab54bdb149158839ca71c9` |
| `artifacts/test-vectors/blocks/mainnet/62e8b3028c204e9712e9df5412ac47fe44c2683409c2ee0abbb3f07c7c26fbd3.cbor` | 1945 | `c0a7295a4b41732e6cf307da06794e8d6fe2f29088749476bac3d5c9cefcb412` |
| `artifacts/test-vectors/blocks/mainnet/698344b7f8b07c3ede9da2a0b8e758e89ffcfdd27fdbced449092f4417247291.cbor` | 2157 | `c269bb705ecb48b0fc6c767736d39f92d9329c84cd74393b6e0330d2282147ca` |
| `artifacts/test-vectors/blocks/mainnet/6e1d7a1964693e6d410596c3574fe6e2121be2021218310ea6b5dce067a02625.cbor` | 2118 | `9ded68313f717beae52faec349788be718052cc33607201f9f25efac14e00182` |
| `artifacts/test-vectors/blocks/mainnet/6f473b27b600ebf575a43623b2e13b6478aa2f2806cd32e1bd9971b4c85a3e72.cbor` | 41313 | `23bfb8934fde2ca292f9661f95dd2c9184f05cc3028ed76236dd5004d8f6d892` |
| `artifacts/test-vectors/blocks/mainnet/844f76e7576a1ae73ce7a50754084f3d30fbd7e696ea8c20cff1858066ca8912.cbor` | 79260 | `c8c97b2b3f521c3cd2b8dfc9388ee5b76f7482c8bc0f93a73afec6fa3eb8b430` |
| `artifacts/test-vectors/blocks/mainnet/850805044e0df6c13ced2190db7b11489672b0225d478a35a6db71fbfb33afc0.cbor` | 2166 | `acae18d2ea40d50108f881581dab722222119453987b12d936b612269cc15980` |
| `artifacts/test-vectors/blocks/mainnet/8d378effcbbe98c9142371b934c32a3946628a2dfb05eee4472a40ec9d1ca862.cbor` | 80005 | `08361ab2aacf4425ee8f8acc783cd5306c5e5b81adeb3c9cde426304f67b3cac` |
| `artifacts/test-vectors/blocks/mainnet/8d4eb9c1e090f3ed4f23ff3690a4ace2fb474cbfec15c420c88d9c3fe8fe3823.cbor` | 89342 | `b2e87edcb2748841e2a6d8056ab2434a46cf41f3b3578cf21087196006464d54` |
| `artifacts/test-vectors/blocks/mainnet/9cefb8bced44c596f5e635ea0a589b89164eaee6a1928cf2ea8e0c13ef4cf700.cbor` | 2303 | `a431d6728adbce53b7c3df0f09fb201b0e4ab8ecf49c26aa48508635d32f3c12` |
| `artifacts/test-vectors/blocks/mainnet/9f2e53fb897d2546ecdfe8b70ce38a545cc64cabe996f4418135557c87d5f78e.cbor` | 1800 | `b8d03de20940916060f78532b8cd49cf442517f4fe851521212ad5feaf891ffd` |
| `artifacts/test-vectors/blocks/mainnet/9f63162b78765332c1632f46960fb386da7a2916090e2ef155ab97941ee1bbb9.cbor` | 1711 | `618acf25fcfa8c0e83b641e5d5704c57c9b7f2dcf13d947d600232c310deabc9` |
| `artifacts/test-vectors/blocks/mainnet/a84fcbe2447bfc2ed53ba10abee56e81e54f6de72d053faf87bd701a9440b5fb.cbor` | 50169 | `303a3ac27099a4b981be8121f114b9e344448be2b2217491aaf79f589ea92d71` |
| `artifacts/test-vectors/blocks/mainnet/a91e6a056eb03adf3ae5a03787caa00892e617b970cd2422d75773dd4ab6e04e.cbor` | 1883 | `d15a9a3d32ba41c4164fc7e19f6ed28df7141f61c6d0fba230694aa3f6fcfcc2` |
| `artifacts/test-vectors/blocks/mainnet/bcb8b595c14d85fa278f2d68ddaa1cce758b940a4b9fe76453d6879715b50d90.cbor` | 42760 | `611b6e9fafccc4f24a2d7f150e2676f555c6c38e7aefeccd974d2642c7af66f2` |
| `artifacts/test-vectors/blocks/mainnet/bfcfa5a3cc8980d14749084e1357f199f4ae68fecbd790b9870934ea5d4fdca6.cbor` | 41283 | `655b374dde6a4868439607d40d276c286e2ab6c708a7cddb0a80e2ab5c2a9727` |
| `artifacts/test-vectors/blocks/mainnet/c8950f90432bff030d84b0ffd3dab4cc023f03fcb723f0db3420992558b3d99d.cbor` | 1910 | `c61507f09f655c42f1181c34425f2687580253efe1ef265a7559af98a438a841` |
| `artifacts/test-vectors/blocks/mainnet/d798a8d617b25fc6456ffe2d90895a2c15a7271b671dab2d18d46f3d0e4ef495.cbor` | 1481 | `93a71179259adb15c763b66a2e13337394076ddbc73f1256e03d0a0cc3b29d89` |
| `artifacts/test-vectors/blocks/mainnet/da8dd783e7383951dd563aa75c308d1a109ecb4787df7e1ec980710613fddfa8.cbor` | 57917 | `7717b2462e475a29462796113f5ebca226bc10115e0cc724c4d3885941fec7d3` |
| `artifacts/test-vectors/blocks/mainnet/fc74c07951c30750d586c4b95f63a7baa6ef51805955dcc7f5dadced530c40f5.cbor` | 3060 | `01d8322c16426064317669d5cfc06d8ae9907ca98c95e13dd0b86407db2e0e48` |
| `artifacts/test-vectors/blocks/mainnet/ff02499ddeb542a740fbebd58730c8ebdd09cf7235471f06d45398c8a9c0c4b3.cbor` | 68068 | `b400658ec9bfb429391bca269ba4e536fe64d12456816b7f187961a2d4a59156` |
| `artifacts/test-vectors/blocks/pallas/allegra1.block` | 6460 | `ad58d2f26fc044ff8d05790449422a1394a885479fad84c027332b3319eb7455` |
| `artifacts/test-vectors/blocks/pallas/alonzo1.block` | 5390 | `39d81d148438c1dd56848bbbd644e7224e55c31218e502e215637486e5ef4490` |
| `artifacts/test-vectors/blocks/pallas/alonzo10.block` | 2016 | `8aac2a8c70520564bece55dc0dc2f18707dd3054850a2bde7ca8ed61055b8b5c` |
| `artifacts/test-vectors/blocks/pallas/alonzo11.block` | 11928 | `dc23fa436f216ad0bd3042941be3162a1e8da7922545e6458ceba23e7ca2b377` |
| `artifacts/test-vectors/blocks/pallas/alonzo12.block` | 133212 | `8817a54f214720d4a8d6d765701f2235ff7496f7e94cfabe7f5183d7e02a1a1f` |
| `artifacts/test-vectors/blocks/pallas/alonzo13.block` | 20848 | `4138fc0e730335dc7df355dea95c77149ddba40e97bbfe2cf085c30421113a76` |
| `artifacts/test-vectors/blocks/pallas/alonzo14.block` | 142734 | `72374a2ff03c6a7b268bb510a6bbf3d5b5d7bc3df82a8cbc04b09d88e31c10d6` |
| `artifacts/test-vectors/blocks/pallas/alonzo15.block` | 34410 | `8cac9a7e52db9b205c265abb7f7a4487e75139aaafec44f3b648c32a83e93f30` |
| `artifacts/test-vectors/blocks/pallas/alonzo16.block` | 7508 | `7da29f2579469d95e7c1906e345a1df205e79d45bb413024998d0801b06ee282` |
| `artifacts/test-vectors/blocks/pallas/alonzo17.block` | 7412 | `743d9eb770f0448a8b8dba8c0b2490c8bdffd9e82c7609667e0aff864c2899f4` |
| `artifacts/test-vectors/blocks/pallas/alonzo18.block` | 34724 | `e65bfad1aec382c10feeb0a1f06207fc583db1ca8ab17e81c34b17eae2e9fd0f` |
| `artifacts/test-vectors/blocks/pallas/alonzo19.block` | 3256 | `37455c9af3371ea0bd934bfe215dc2c20f831205ac6b09619582f02bbd5957d3` |
| `artifacts/test-vectors/blocks/pallas/alonzo2.block` | 7922 | `d0d4da10c2f20ddc7bbdb01910fdfeb03278b464e75a740347974dba5522d9a9` |
| `artifacts/test-vectors/blocks/pallas/alonzo20.block` | 3492 | `31c303deafeb92899f3452a22d9deeb236ac491bcc9d3035e70a138cfdc0c924` |
| `artifacts/test-vectors/blocks/pallas/alonzo21.block` | 129418 | `23a96ce7b3b333c7d14edb1eb290499469c39aa52ade1feeeac4554fed5cd8e6` |
| `artifacts/test-vectors/blocks/pallas/alonzo22.block` | 2744 | `996bc37ac3c134470f555730c918736cf44b8d08393c766dcd3a22e13915cf29` |
| `artifacts/test-vectors/blocks/pallas/alonzo23.block` | 15002 | `f30b2a3510f87cb32e91e3a5e8ca13e5a63ee929b120f7e63a4cd682d89ffa62` |
| `artifacts/test-vectors/blocks/pallas/alonzo24.block` | 4456 | `d397feeddab6beb7a6712622ff3c5c396d7ecd240c760e28ef7aa8a7cf0aeb66` |
| `artifacts/test-vectors/blocks/pallas/alonzo27.block` | 85426 | `14e613b1e8511204ec6d9de8181a7801ec025d39b53ced91945e2ef1887ca122` |
| `artifacts/test-vectors/blocks/pallas/alonzo3.block` | 10506 | `c17a869a79231b943ee62eaea7a0ad187130aff132716637f16b14c3e9d26b15` |
| `artifacts/test-vectors/blocks/pallas/alonzo4.block` | 7758 | `da63de894f00e58c99122690830e3cf2f5c26676408c3f541cca27fdad5eb7c2` |
| `artifacts/test-vectors/blocks/pallas/alonzo5.block` | 8220 | `3e0eb798b48663343b9d2f30369fe7c9a0eea6ab732ff1149ebe2c0d773820a2` |
| `artifacts/test-vectors/blocks/pallas/alonzo6.block` | 36356 | `e0a24f6d1b948e12e50176bc30f75fe0227b4ea62c8740dd8b8a35cf243c228b` |
| `artifacts/test-vectors/blocks/pallas/alonzo7.block` | 4146 | `433a1271806920b0e98a953089d2be54e533ae1a3f14cf071d98b80227e21ed6` |
| `artifacts/test-vectors/blocks/pallas/alonzo8.block` | 37120 | `894350ec6feeaabb65abbf48d413aa7d8126fb4bed86cc4ee309e9c62d8cf6ea` |
| `artifacts/test-vectors/blocks/pallas/alonzo9.block` | 80292 | `57749c3664f893885c5cb0fec3b7a8504342f3307e8688542f6ef408358d9ace` |
| `artifacts/test-vectors/blocks/pallas/babbage1.block` | 3364 | `415aa9f34214cb04926745a3c18600dfd04fd61a6ce4e7fa988a89235da7f8a1` |
| `artifacts/test-vectors/blocks/pallas/babbage10.block` | 3174 | `10c9afeef22c180dda998527415da360cc83b36b40bb5610d076998830758713` |
| `artifacts/test-vectors/blocks/pallas/babbage2.block` | 3352 | `916ccc461c16203f9558929d796081a93d3cbf8161df53f2b63c75058c6e04a6` |
| `artifacts/test-vectors/blocks/pallas/babbage3.block` | 3330 | `7a6d4dbfa7b987ce10d6f26d501d349ff59db8503f17428e16f6753bccb63be2` |
| `artifacts/test-vectors/blocks/pallas/babbage4.block` | 39750 | `e8d716175c4e23e7232ccbfc6eef8477181381d08c2f48841a61532207d44355` |
| `artifacts/test-vectors/blocks/pallas/babbage5.block` | 17690 | `b3b36cb83d637a23c411791370ebde726954e4cc2bb69d150b94dde3b408b8f9` |
| `artifacts/test-vectors/blocks/pallas/babbage6.block` | 8700 | `08f55d5febbb48106cbcd62a2c0d2b370ceb7485650c54391e093f3c75eb275c` |
| `artifacts/test-vectors/blocks/pallas/babbage7.block` | 7590 | `d8ba5f4ae18bd982e2be7cee700086763b56de13de4e0c5edd5ad277a77049d3` |
| `artifacts/test-vectors/blocks/pallas/babbage8.block` | 8952 | `c4a9596ec569d416dd1d564427de7e3ff18c69df9b0c2761431362eb56b2423d` |
| `artifacts/test-vectors/blocks/pallas/babbage9.block` | 163946 | `d3f786eaa7ca5e6d94880c9bd80c2112b8d53649de3cccd618761cb882b182fe` |
| `artifacts/test-vectors/blocks/pallas/byron1.block` | 1272 | `77a34d9e007e64ac2cb024a3d47615e7a6474ba1ae114ae2a277dc2a8211a14b` |
| `artifacts/test-vectors/blocks/pallas/byron2.block` | 5150 | `be8d3ca8245dacd7544c0748b425f9869117263a245374ac6acc41d11ee99547` |
| `artifacts/test-vectors/blocks/pallas/byron3.block` | 6318 | `2a03348561dd73fe8903d341f068161beaa49e36bac52287e00d39de2e689b4a` |
| `artifacts/test-vectors/blocks/pallas/byron4.block` | 2050 | `9997e1d78c0286764abf8f499bd5bc816f7054fc2bab152f43878b0c50f3e6fc` |
| `artifacts/test-vectors/blocks/pallas/byron5.block` | 3422 | `36fb6290f2fa77880f8d47b0fe14d260b26b955853becf51fa1744ea76c32b86` |
| `artifacts/test-vectors/blocks/pallas/byron6.block` | 2212 | `4bdb8afda26b442e1798d7b4fc32ce2fe8451a5b5e2b261e88b8008e8235c374` |
| `artifacts/test-vectors/blocks/pallas/byron7.block` | 19478 | `ac161f1440a874d966969875cea393eed8085f0b5a2c1a7acc438636a1bb4601` |
| `artifacts/test-vectors/blocks/pallas/byron8.block` | 4222 | `c3748dccbcd694eec80d301d4a17233a407eb688313e4d3ef6b95427ef08cd09` |
| `artifacts/test-vectors/blocks/pallas/conway1.block` | 3482 | `29405309714608b328071a26f1fe0f1b778eb7d0e3ae568627749ba68e4ca8c8` |
| `artifacts/test-vectors/blocks/pallas/conway2.block` | 2394 | `0425168fd5aefdeadc9fc2dca60aa50452cdd142ff7539489788a4cf16cc66ca` |
| `artifacts/test-vectors/blocks/pallas/conway3.block` | 2534 | `c043303fdb58a6decee03a108e45828eb8201e685ae26a2695b0cf21b32d9096` |
| `artifacts/test-vectors/blocks/pallas/conway4.block` | 3750 | `b0106388ff177993e865e33cb8bacb8b22e8109150da25feb25f024d36f24df3` |
| `artifacts/test-vectors/blocks/pallas/conway8.block` | 5106 | `b846ab008159f0df2b9df903deb5dc5a5f6be8b49bc2a41467b9bf6fff048937` |
| `artifacts/test-vectors/blocks/pallas/genesis.block` | 1296174 | `18976e16f654435df817a4918a3880e33b091a8e312ec4f6fb67fce162c114ce` |
| `artifacts/test-vectors/blocks/pallas/mary1.block` | 41074 | `64ca50616aa1b93d4254c41f5aa9fec89a7b436472c5df2402b7c3eab954a462` |
| `artifacts/test-vectors/blocks/pallas/shelley1.block` | 4876 | `87bb16b8624f3ae0212b57231de58c6576b7b30f29eb482c540e1da78b0f4f21` |
| `artifacts/test-vectors/genesis/byron/5f20df933584822601f9e3f8c024eb5eb252fe8cefb24d1317dc3d432e940ebb.json` | 910756 | `40d865ea0af837ded73c1d4288faaa4647817296b1eb59c7823e4ebc95824ea8` |
| `artifacts/test-vectors/genesis/byron/96fceff972c2c06bd3bb5243c39215333be6d56aaf4823073dca31afe5038471.json` | 31112 | `dca3139e907bb943bfeff5f186f3fc71217ff12cabde93a926277c6ddd647d87` |
| `artifacts/test-vectors/genesis/byron/b7f76950bc4866423538ab7764fc1c7020b24a5f717a5bee3109ff2796567214.json` | 27272 | `6b62660df9f2cf27192cc071616ffb0617018c5a42c753200e6f6bac9471793e` |
| `artifacts/test-vectors/genesis/byron/c6a004d3d178f600cd8caa10abbebe1549bef878f0665aea2903472d5abf7323.json` | 905398 | `f2ef79f67f39cc9eec0bff3549b1fdb31d3e454281ba9d9a84807908f5b52ad7` |
| `artifacts/test-vectors/genesis/shelley/test-yaci.json` | 5022 | `0d7293404873954247b1b938d9c3636781e983a23ea10656ad96f1d4cd225e63` |
| `artifacts/test-vectors/genesis/shelley/test.json` | 6401 | `555530d611a5884c234be72915a6f11839d89378fe12d13ace328681fbd448c7` |
| `artifacts/test-vectors/manifest.json` | 54387 | `83f12c4af07b1d2217725a5a0475f96b5a1abd8cd0efce9eb8300e3977bc32f1` |

## Captured scope

- 38 selected CDDL files below `artifacts/specs/`.
- Three CML license files below `artifacts/legal/`.
- Six genesis JSON vectors and 86 historical block vectors below
  `artifacts/test-vectors/`.
- Byte-exact upstream provenance plus deterministic manifest, provenance, license, README, and
  Git-attributes control files.

The 92 vectors retain their original bytes and expected outcomes. The manifest records source
paths, repository paths, sizes, SHA-256 values, storage forms, era metadata, and expected results.
Dolos is pinned to `ea7960a1c2e56c523fec7c4bab75f390ee443514`; Pallas is pinned to
`a7b5a86e3922ea46723e7959118293232db7bf3a`.

## Integrity and licensing

The exact inventory and deterministic transformations are defined by
[this frozen capture specification](#frozen-capture-specification). Test-vector integrity and license mappings are recorded in
`artifacts/test-vectors/manifest.json` and `artifacts/test-vectors/PROVENANCE.json`.

## Semantic evidence

The captured CDDL is a frozen comparison source. The vectors provide historical byte-preservation
and decoding evidence. They are not generated source and do not prescribe a library's public API
or language representation.

## Exclusions

Reference-only CDDL outside the selected baseline, Rust source, Rust tests, unrelated test data,
build configuration, Git metadata, Pallas `u5c*` vectors, message signing, and UPLC evaluation are
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
| CIP-36 metadata and delegation behavior; [semantic evidence](#semantic-evidence) | TypeScript: `libs/typescript/packages/cip/src/cip36/metadata.ts` | `libs/typescript/packages/cip/test/cip36.test.mjs` | No implementation change from this migration. Compare a separately captured update before planning behavior changes. |
| Transactions, collections, witnesses and redeemer positions; [semantic evidence](#semantic-evidence) | TypeScript: `libs/typescript/packages/chain/src/builder/transaction.ts` | `libs/typescript/packages/chain/test/builders.test.mjs` | No implementation change from this migration. Compare a separately captured update before planning behavior changes. |

Behavioral evidence is language-neutral. C++ is an unmaintained, opt-in consumer.

## Unresolved questions

No fresh upstream completeness or delta audit was performed for this metadata migration.
The inventory verifies stored bytes; original capture claims are retained as historical claims.
Any future update must independently enumerate its pinned sources and resolve semantic mappings.

## Migration provenance

- Metadata conversion date: 2026-09-09, explicitly requested by the human while SPECTRE 1.0.0 is in development.
- Previous snapshot descriptor SHA-256: `1fb3ec912beeafae6abb32d32d7603f6f05ed51c854add8ff8407360f7950959`.
- Original applicable capture-rules SHA-256: `a23d22a6ba833cc9169a784a0251990ed5b07599713088efa8431d3aff7797b0` (historical label v1).
- Embedded the original applicable rules, with snapshot-relative scope and obsolete provider-version
  routing removed. Added this exact artifact inventory and clearly labeled migration-time guidance.
- Corrected the initial snapshot’s self-referential Previous-Snapshot to NONE; filled inapplicable
  URL source fields with NONE. Preserved Created, snapshot ID, source identities, all artifact paths
  and every artifact byte. No new snapshot or upstream capture was created.
- The converted descriptor and its artifacts are frozen after this migration. Future changes
  require a new numbered snapshot with its own complete specification.
