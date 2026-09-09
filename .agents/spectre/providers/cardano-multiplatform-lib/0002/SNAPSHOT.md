# Cardano Multiplatform Lib provider snapshot

Provider-Snapshot-Version: v1
Snapshot: 0002
Provider: cardano-multiplatform-lib
Created: 20260909T134705Z
Previous-Snapshot: 0001/SNAPSHOT.md
Previous-Snapshot-SHA256: 2f4c1c955486d1d16a308a5ec7ccd595dafa741ab47415c872efa9e5d8906182
Source-Type: git
Source-Repository: https://github.com/dcSpark/cardano-multiplatform-lib.git
Source-Commit: 69b8664c84931cd6632e0d951c2795019597527f
Source-Ref: refs/heads/develop
Source-Tag: NONE
Source-URL: NONE
Source-SHA256: NONE

Capture-Summary: CAPTURE.md
Capture-Summary-SHA256: 869a0360fc7256cc83a5397431dfd831ddfed6ca8ab9ed15151c68de6750fa03

## Evidence objective

Capture the reviewed CML CDDL, CIP-36 and collection/builder behavior selection from live develop.
Compare it with 0001 and the current maintained TypeScript implementation, including the recent
LOCAL fixes, so a later consumer plan can distinguish completed behavior from remaining differences.

## Comparison sources

- [Previous CML snapshot](../0001/SNAPSHOT.md), commit
  `39681e0d435a71f7c47a2601507ab16e691abb9e`, is the only previous same-provider capture.
- The 92 vectors and their upstream PROVENANCE.md are byte-identical to 0001. Original expected
  outcomes and era metadata are reused only after exact membership and SHA-256 verification.
- Supplemental Apache-2.0 text is reused byte-exactly from 0001’s
  [license artifact](../0001/artifacts/test-vectors/LICENSE-APACHE-2.0.txt), SHA-256
  `4541e95aa81113643b71a96d7ff673c4a83ede3d2e8f0df2ad676e7970e2b1fa` (11,347 bytes).
  It supplies legal text, not another semantic upstream source.
- Captured CML provenance attributes Dolos vectors to `ea7960a1c2e56c523fec7c4bab75f390ee443514`
  and Pallas vectors to `a7b5a86e3922ea46723e7959118293232db7bf3a`. These are inherited attributions,
  not newly fetched Dolos/Pallas trees; see the frozen PROVENANCE.json and PROVENANCE.md.

## Complete capture specification

### Source selection and mapping

Resolve develop once to the full commit above. Its root Git tree is `cf0bb93a8eb235adecb3350875f6cc63dea22298`.
The complete non-truncated recursive tree was enumerated independently before downloading artifacts;
all 138 selected source paths were regular mode-100644 blobs. Download each selected file by immutable
commit and verify both byte length and Git blob SHA-1 against that independent tree, then SHA-256.
The exact source-to-artifact inventory below is authoritative for this snapshot only.

Selection comprises the explicitly listed 30 CDDL and 12 Rust paths; the three CML license files;
immediate-child Byron genesis *.json; exactly Shelley test.json/test-yaci.json; immediate-child
mainnet *.cbor and Pallas *.block except u5c*; and upstream golden_vectors/PROVENANCE.md.
No recursive glob expansion beyond those immediate-child vector directories is permitted.
Compared with 0001, ten obsolete multi-era extern CDDL paths are excluded and two CIP-36 dependency
paths are added. This selection change is explicit, not a claim that arbitrary extern types resolve.

| Upstream path | Logical artifact path | Bytes | Git blob SHA-1 |
| --- | --- | ---: | --- |
| `LICENSE` | `legal/LICENSE` | 1064 | `b317603799800e5cadf5f8657068d43773dd3346` |
| `LICENSE-EMURGO` | `legal/LICENSE-EMURGO` | 1191 | `0540392ac47065448094db0408506e9c1fcb5c5c` |
| `LICENSE-IOHK` | `legal/LICENSE-IOHK` | 1063 | `c03cf18031ab5f8034199641f86c32b797fb9758` |
| `chain/rust/src/builders/redeemer_builder.rs` | `upstream/chain/rust/src/builders/redeemer_builder.rs` | 15705 | `a7719e67bf22cd93e23f2e9fa30e1c3614d34098` |
| `chain/rust/src/builders/tx_builder.rs` | `upstream/chain/rust/src/builders/tx_builder.rs` | 326170 | `39a946e0bd6499b7f44004c25dda59c99d5eb433` |
| `chain/rust/src/builders/witness_builder.rs` | `upstream/chain/rust/src/builders/witness_builder.rs` | 31319 | `69592e523e4566d421d627ab1e69f293164a458d` |
| `chain/rust/src/genesis/byron/test_data/5f20df933584822601f9e3f8c024eb5eb252fe8cefb24d1317dc3d432e940ebb.json` | `test-vectors/genesis/byron/5f20df933584822601f9e3f8c024eb5eb252fe8cefb24d1317dc3d432e940ebb.json` | 910756 | `1f0e6a85f91fc6b032bb68d385dcf49db1c76fd7` |
| `chain/rust/src/genesis/byron/test_data/96fceff972c2c06bd3bb5243c39215333be6d56aaf4823073dca31afe5038471.json` | `test-vectors/genesis/byron/96fceff972c2c06bd3bb5243c39215333be6d56aaf4823073dca31afe5038471.json` | 31112 | `79fde9c20198f9d017af3bd297504f75d5791e70` |
| `chain/rust/src/genesis/byron/test_data/b7f76950bc4866423538ab7764fc1c7020b24a5f717a5bee3109ff2796567214.json` | `test-vectors/genesis/byron/b7f76950bc4866423538ab7764fc1c7020b24a5f717a5bee3109ff2796567214.json` | 27272 | `c41ffb6cbeb8e8079964a046a6493aec9c442a19` |
| `chain/rust/src/genesis/byron/test_data/c6a004d3d178f600cd8caa10abbebe1549bef878f0665aea2903472d5abf7323.json` | `test-vectors/genesis/byron/c6a004d3d178f600cd8caa10abbebe1549bef878f0665aea2903472d5abf7323.json` | 905398 | `8fc09de054d265b9eb640c53563487fb97c3e23f` |
| `chain/rust/src/genesis/shelley/test_data/test-yaci.json` | `test-vectors/genesis/shelley/test-yaci.json` | 5022 | `6c2585e1b8305d1dd45e2c5c6b386ef190bab37a` |
| `chain/rust/src/genesis/shelley/test_data/test.json` | `test-vectors/genesis/shelley/test.json` | 6401 | `09f234efe06273605a8c8cc2a7953f4308997373` |
| `cip36/rust/src/generated/cbor_encodings.rs` | `upstream/cip36/rust/src/generated/cbor_encodings.rs` | 2927 | `ac7902d864362f6a3e9c6fb9361fc78d047a93c6` |
| `cip36/rust/src/generated/mod.rs` | `upstream/cip36/rust/src/generated/mod.rs` | 11145 | `e9d850ed0d57f84cfff6d51fb1ca13c8a1efdfcd` |
| `cip36/rust/src/generated/serialization.rs` | `upstream/cip36/rust/src/generated/serialization.rs` | 68841 | `15656cd5a955f3cf2e9aefd6ef49038f95deb343` |
| `cip36/rust/src/utils.rs` | `upstream/cip36/rust/src/utils.rs` | 9279 | `8f87e4f725d6de637595e42a8240a59390d34cb8` |
| `cip36/rust/tests/cbor_roundtrip.rs` | `upstream/cip36/rust/tests/cbor_roundtrip.rs` | 30895 | `7d43c120f2210a04e58cbc034f558054825fff6f` |
| `cip36/rust/tests/invariants.rs` | `upstream/cip36/rust/tests/invariants.rs` | 22912 | `00372288e057430708437da044e81568a1cbdecf` |
| `cip36/rust/tests/json.rs` | `upstream/cip36/rust/tests/json.rs` | 16894 | `d34fb61252f813c104795ecf31aff15e55df5db6` |
| `cip36/rust/tests/unknown_keys.rs` | `upstream/cip36/rust/tests/unknown_keys.rs` | 26039 | `8afaddd93dd70e28811769bb490cfc80533201ed` |
| `core/rust/src/ordered_set.rs` | `upstream/core/rust/src/ordered_set.rs` | 20895 | `0b148fd634d3defd93ebcca1238bc9c92eff2ebb` |
| `multi-era/rust/tests/golden_vectors/PROVENANCE.md` | `test-vectors/PROVENANCE.md` | 2547 | `dc5507760b5d5cbe0b316e61c779ccc12fd7fae6` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/0822e72ec531fd72b74af75bbce83876547f538ca7ca9bc854d043aa888c478c.cbor` | `test-vectors/blocks/mainnet/0822e72ec531fd72b74af75bbce83876547f538ca7ca9bc854d043aa888c478c.cbor` | 55683 | `a7abe115aa30eab7463aa2bbf4a8071de2999429` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/0ade775289bbf05b0ed77052bac35cc1ef728751fc00081d179d9f6f7728f56d.cbor` | `test-vectors/blocks/mainnet/0ade775289bbf05b0ed77052bac35cc1ef728751fc00081d179d9f6f7728f56d.cbor` | 2122 | `3b72c8377eb88aeae3744aed3207d063242d929c` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/11c08542cf8da1ab1d1686c259dfc922e492c8ae893bccb588059f62717e573a.cbor` | `test-vectors/blocks/mainnet/11c08542cf8da1ab1d1686c259dfc922e492c8ae893bccb588059f62717e573a.cbor` | 80134 | `5f8420a55c5ad0e026cbf635050e398975a9b28b` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/18d49cdcae701d93ec69efd1f8140de6e5337a735a162f5e509e91b8f9ee1c18.cbor` | `test-vectors/blocks/mainnet/18d49cdcae701d93ec69efd1f8140de6e5337a735a162f5e509e91b8f9ee1c18.cbor` | 1948 | `c8811cb625a78d8c59c82af48f7297880111689b` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/28532f081779a6c435c31ecc64f95a39417c6edce001fd9a964c622e18de95f4.cbor` | `test-vectors/blocks/mainnet/28532f081779a6c435c31ecc64f95a39417c6edce001fd9a964c622e18de95f4.cbor` | 29618 | `08ed43cc6f7621dc56164b72813297126b9987bf` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/36cb7c4cb0053e112f57cde99204c785a4375fb7c6ac3eabe3a02fdc2d4a2867.cbor` | `test-vectors/blocks/mainnet/36cb7c4cb0053e112f57cde99204c785a4375fb7c6ac3eabe3a02fdc2d4a2867.cbor` | 66289 | `de3d20179ddb00f800f18be15fbc5634ca1b08a3` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/3b234ca0d5ac7c52a5407b63b358391b1b920c17bb40fe50c9fbbd76f77e6f67.cbor` | `test-vectors/blocks/mainnet/3b234ca0d5ac7c52a5407b63b358391b1b920c17bb40fe50c9fbbd76f77e6f67.cbor` | 2126 | `ad7ae3c65a24327edec9fc49e579fe013cf1a3f1` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/3ba7cdc89bf74fc1915da5bd2a593231b70734d30c8a4c61e40e01ad345ca092.cbor` | `test-vectors/blocks/mainnet/3ba7cdc89bf74fc1915da5bd2a593231b70734d30c8a4c61e40e01ad345ca092.cbor` | 1354 | `bd9976090b83255f9382fd2d069a396f4cd12f2e` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/44b156deb04b7f923e6716fbc8dbda23ba8d748e6dcc08ded4914b5985524ce3.cbor` | `test-vectors/blocks/mainnet/44b156deb04b7f923e6716fbc8dbda23ba8d748e6dcc08ded4914b5985524ce3.cbor` | 2157 | `82799d7a18c62d9f0d4fa0d8ae46503f7a087c7a` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/55e828f124ef484d24b9ab60d25549f3ffb4999a9b9816904038e53aac2a5cc9.cbor` | `test-vectors/blocks/mainnet/55e828f124ef484d24b9ab60d25549f3ffb4999a9b9816904038e53aac2a5cc9.cbor` | 9226 | `3c0f33d4a237f8fbefa252a88fb6eaf485d9b8b6` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/56f45594dbd995ae19a892d94db94962ecfab2d016ad387bd634f54ba750c85d.cbor` | `test-vectors/blocks/mainnet/56f45594dbd995ae19a892d94db94962ecfab2d016ad387bd634f54ba750c85d.cbor` | 1765 | `e46c7771f3545eb489ce334653a95309ab555d44` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/5740eb9b6bb6207b7b1cc663e83532fa51fe826cf61714e747339d134b201681.cbor` | `test-vectors/blocks/mainnet/5740eb9b6bb6207b7b1cc663e83532fa51fe826cf61714e747339d134b201681.cbor` | 3009 | `10bb8b60a2f75ab3ed42483893688c6acb510001` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/586525732f41fa76f2c3b6d97831c2b68158b3429901e5c3c4f39506d1029f55.cbor` | `test-vectors/blocks/mainnet/586525732f41fa76f2c3b6d97831c2b68158b3429901e5c3c4f39506d1029f55.cbor` | 944 | `939dd9a607fc51ef663014260cb57b355aaf1012` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/61cc265848410b4fb43daeef29240bae841dfa4389d201bcf2d376922fe14a83.cbor` | `test-vectors/blocks/mainnet/61cc265848410b4fb43daeef29240bae841dfa4389d201bcf2d376922fe14a83.cbor` | 1945 | `b21e89eab410cb2f3bddf9e8c9b3adf726c6fde7` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/62e8b3028c204e9712e9df5412ac47fe44c2683409c2ee0abbb3f07c7c26fbd3.cbor` | `test-vectors/blocks/mainnet/62e8b3028c204e9712e9df5412ac47fe44c2683409c2ee0abbb3f07c7c26fbd3.cbor` | 1945 | `e052cc8f0b5820fa7184cc0890144fd66441c058` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/698344b7f8b07c3ede9da2a0b8e758e89ffcfdd27fdbced449092f4417247291.cbor` | `test-vectors/blocks/mainnet/698344b7f8b07c3ede9da2a0b8e758e89ffcfdd27fdbced449092f4417247291.cbor` | 2157 | `f54865d3d659e7f9094eddf445a4462282f94548` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/6e1d7a1964693e6d410596c3574fe6e2121be2021218310ea6b5dce067a02625.cbor` | `test-vectors/blocks/mainnet/6e1d7a1964693e6d410596c3574fe6e2121be2021218310ea6b5dce067a02625.cbor` | 2118 | `c1e8ffd1bd8e0fc8895a358a61f27cada497a2ba` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/6f473b27b600ebf575a43623b2e13b6478aa2f2806cd32e1bd9971b4c85a3e72.cbor` | `test-vectors/blocks/mainnet/6f473b27b600ebf575a43623b2e13b6478aa2f2806cd32e1bd9971b4c85a3e72.cbor` | 41313 | `fbeac6a596ba277b69ff56325fc6e34c05751c1f` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/844f76e7576a1ae73ce7a50754084f3d30fbd7e696ea8c20cff1858066ca8912.cbor` | `test-vectors/blocks/mainnet/844f76e7576a1ae73ce7a50754084f3d30fbd7e696ea8c20cff1858066ca8912.cbor` | 79260 | `aad5756262e4c19e6b726adfdb867898f51ed634` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/850805044e0df6c13ced2190db7b11489672b0225d478a35a6db71fbfb33afc0.cbor` | `test-vectors/blocks/mainnet/850805044e0df6c13ced2190db7b11489672b0225d478a35a6db71fbfb33afc0.cbor` | 2166 | `151f393b46d3a8197c9053c826445c3b7f33494d` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/8d378effcbbe98c9142371b934c32a3946628a2dfb05eee4472a40ec9d1ca862.cbor` | `test-vectors/blocks/mainnet/8d378effcbbe98c9142371b934c32a3946628a2dfb05eee4472a40ec9d1ca862.cbor` | 80005 | `69475cd5fd4837e02f99eb9f346f6ac2bd43702e` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/8d4eb9c1e090f3ed4f23ff3690a4ace2fb474cbfec15c420c88d9c3fe8fe3823.cbor` | `test-vectors/blocks/mainnet/8d4eb9c1e090f3ed4f23ff3690a4ace2fb474cbfec15c420c88d9c3fe8fe3823.cbor` | 89342 | `c732ffdfbe2632df100286842f4cf0856a4e6909` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/9cefb8bced44c596f5e635ea0a589b89164eaee6a1928cf2ea8e0c13ef4cf700.cbor` | `test-vectors/blocks/mainnet/9cefb8bced44c596f5e635ea0a589b89164eaee6a1928cf2ea8e0c13ef4cf700.cbor` | 2303 | `074b9db595af52ef6dd11a6b8501b6d9a3bfece5` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/9f2e53fb897d2546ecdfe8b70ce38a545cc64cabe996f4418135557c87d5f78e.cbor` | `test-vectors/blocks/mainnet/9f2e53fb897d2546ecdfe8b70ce38a545cc64cabe996f4418135557c87d5f78e.cbor` | 1800 | `cd6cfae30975ac808ecba93b98bbb511333fa1ff` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/9f63162b78765332c1632f46960fb386da7a2916090e2ef155ab97941ee1bbb9.cbor` | `test-vectors/blocks/mainnet/9f63162b78765332c1632f46960fb386da7a2916090e2ef155ab97941ee1bbb9.cbor` | 1711 | `bd1e1ca4ccc517d73e7b99f45dc2965c16c5de77` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/a84fcbe2447bfc2ed53ba10abee56e81e54f6de72d053faf87bd701a9440b5fb.cbor` | `test-vectors/blocks/mainnet/a84fcbe2447bfc2ed53ba10abee56e81e54f6de72d053faf87bd701a9440b5fb.cbor` | 50169 | `411e2a4b7b878fd3435434ad0825c46f20cbe8e7` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/a91e6a056eb03adf3ae5a03787caa00892e617b970cd2422d75773dd4ab6e04e.cbor` | `test-vectors/blocks/mainnet/a91e6a056eb03adf3ae5a03787caa00892e617b970cd2422d75773dd4ab6e04e.cbor` | 1883 | `30260f1a49493a28bb89a0f2fae489e8e4a5108e` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/bcb8b595c14d85fa278f2d68ddaa1cce758b940a4b9fe76453d6879715b50d90.cbor` | `test-vectors/blocks/mainnet/bcb8b595c14d85fa278f2d68ddaa1cce758b940a4b9fe76453d6879715b50d90.cbor` | 42760 | `3175fe950378b926f79bc0aaccd39ef7f8dd3e83` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/bfcfa5a3cc8980d14749084e1357f199f4ae68fecbd790b9870934ea5d4fdca6.cbor` | `test-vectors/blocks/mainnet/bfcfa5a3cc8980d14749084e1357f199f4ae68fecbd790b9870934ea5d4fdca6.cbor` | 41283 | `2506d0a9a5b80d2723df8320dd6357937bff38a0` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/c8950f90432bff030d84b0ffd3dab4cc023f03fcb723f0db3420992558b3d99d.cbor` | `test-vectors/blocks/mainnet/c8950f90432bff030d84b0ffd3dab4cc023f03fcb723f0db3420992558b3d99d.cbor` | 1910 | `7a60a550da44072a2ede5a0ee43e07942c98fc11` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/d798a8d617b25fc6456ffe2d90895a2c15a7271b671dab2d18d46f3d0e4ef495.cbor` | `test-vectors/blocks/mainnet/d798a8d617b25fc6456ffe2d90895a2c15a7271b671dab2d18d46f3d0e4ef495.cbor` | 1481 | `ce82a2456d02c0e2a60b84a81862ee199fda0f0b` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/da8dd783e7383951dd563aa75c308d1a109ecb4787df7e1ec980710613fddfa8.cbor` | `test-vectors/blocks/mainnet/da8dd783e7383951dd563aa75c308d1a109ecb4787df7e1ec980710613fddfa8.cbor` | 57917 | `ad55f8e768ef8d4f01109935dc8d9dd570d19638` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/fc74c07951c30750d586c4b95f63a7baa6ef51805955dcc7f5dadced530c40f5.cbor` | `test-vectors/blocks/mainnet/fc74c07951c30750d586c4b95f63a7baa6ef51805955dcc7f5dadced530c40f5.cbor` | 3060 | `eeea6746e1a05ae1477e789ca0e964c417d7baaa` |
| `multi-era/rust/tests/golden_vectors/mainnet_blocks/ff02499ddeb542a740fbebd58730c8ebdd09cf7235471f06d45398c8a9c0c4b3.cbor` | `test-vectors/blocks/mainnet/ff02499ddeb542a740fbebd58730c8ebdd09cf7235471f06d45398c8a9c0c4b3.cbor` | 68068 | `78b02933a48e8e55693383fce8fe6454bd78e6b6` |
| `multi-era/rust/tests/golden_vectors/pallas/allegra1.block` | `test-vectors/blocks/pallas/allegra1.block` | 6460 | `a276b1f3af34b07fe49ffead83d7e8a86d1a900c` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo1.block` | `test-vectors/blocks/pallas/alonzo1.block` | 5390 | `bc635929264938b5cb5d212ed7c66fbd9acc87bc` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo10.block` | `test-vectors/blocks/pallas/alonzo10.block` | 2016 | `fbb5f3150d4aa1421785d5c8a80d1084b931d880` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo11.block` | `test-vectors/blocks/pallas/alonzo11.block` | 11928 | `6d95b58cd0c5578a3dbc14869e3282e454688fc2` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo12.block` | `test-vectors/blocks/pallas/alonzo12.block` | 133212 | `b960f22530bca9085c51ec06a74dde5cae163007` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo13.block` | `test-vectors/blocks/pallas/alonzo13.block` | 20848 | `e7ee1317440314b33641e287be4b5cf75764d067` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo14.block` | `test-vectors/blocks/pallas/alonzo14.block` | 142734 | `69a195fd54622622395b2905281cbf1510923f2c` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo15.block` | `test-vectors/blocks/pallas/alonzo15.block` | 34410 | `da2c6363de91e6692d98c186b92904edfe17f483` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo16.block` | `test-vectors/blocks/pallas/alonzo16.block` | 7508 | `b1bd744ae118664ac467d0cedef9e3f30fd154ef` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo17.block` | `test-vectors/blocks/pallas/alonzo17.block` | 7412 | `9085e53d175b4520acb3972a8a7b0e8bab235bf9` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo18.block` | `test-vectors/blocks/pallas/alonzo18.block` | 34724 | `8bb6bf7c8178ad8f4e55e1639fb21b2003aba7e6` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo19.block` | `test-vectors/blocks/pallas/alonzo19.block` | 3256 | `11d3a5455e6a68ce5014075afe7e3ecd1ffd6ac5` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo2.block` | `test-vectors/blocks/pallas/alonzo2.block` | 7922 | `32f50833c3ab87c1eb594760a171226329e661ce` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo20.block` | `test-vectors/blocks/pallas/alonzo20.block` | 3492 | `3bba2f21fbb5bf836304334664c5295f217a0f37` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo21.block` | `test-vectors/blocks/pallas/alonzo21.block` | 129418 | `cb7d22c9f2af276454afce448cb0c3f9eaea4ee5` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo22.block` | `test-vectors/blocks/pallas/alonzo22.block` | 2744 | `8f4e95df1aca0e0472b20eba9c2214e4e06aa7a8` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo23.block` | `test-vectors/blocks/pallas/alonzo23.block` | 15002 | `a096d95ea188c344b531700eebb53fa59bf6eb77` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo24.block` | `test-vectors/blocks/pallas/alonzo24.block` | 4456 | `298135b3f33f04e6e0411c461d3f802b7c41a744` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo27.block` | `test-vectors/blocks/pallas/alonzo27.block` | 85426 | `d7cbcbfde6c4da8e3849dd7cd51e2080cf5a196d` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo3.block` | `test-vectors/blocks/pallas/alonzo3.block` | 10506 | `36b471c77513719f5377e81bd2de6983d22f1b22` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo4.block` | `test-vectors/blocks/pallas/alonzo4.block` | 7758 | `14de68ea92662fbb1cafc0c8056f8c557d695dde` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo5.block` | `test-vectors/blocks/pallas/alonzo5.block` | 8220 | `585bbf6fba3c3659ba2222b3e88e5a96077d0341` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo6.block` | `test-vectors/blocks/pallas/alonzo6.block` | 36356 | `04cfa9ff547e280b7017f99f3230f4950af2e86f` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo7.block` | `test-vectors/blocks/pallas/alonzo7.block` | 4146 | `34eb53dbd5d11aa0833a7b4b6d442de6633c39a3` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo8.block` | `test-vectors/blocks/pallas/alonzo8.block` | 37120 | `fbcc060fc1f73473cec9948231e268e5c2bf29eb` |
| `multi-era/rust/tests/golden_vectors/pallas/alonzo9.block` | `test-vectors/blocks/pallas/alonzo9.block` | 80292 | `df0ae625b6152265ab8b1798cb3e9729d751110b` |
| `multi-era/rust/tests/golden_vectors/pallas/babbage1.block` | `test-vectors/blocks/pallas/babbage1.block` | 3364 | `61896d2b6fe2823603fab58f2aec3ee9741bab90` |
| `multi-era/rust/tests/golden_vectors/pallas/babbage10.block` | `test-vectors/blocks/pallas/babbage10.block` | 3174 | `462ff86c685e678bca63592067446e786d98ab1d` |
| `multi-era/rust/tests/golden_vectors/pallas/babbage2.block` | `test-vectors/blocks/pallas/babbage2.block` | 3352 | `293203769f70e3235aeebfa315aca9b0c72e174b` |
| `multi-era/rust/tests/golden_vectors/pallas/babbage3.block` | `test-vectors/blocks/pallas/babbage3.block` | 3330 | `ad396c9e6825c27889cb46b4eee6ec93e9954b6e` |
| `multi-era/rust/tests/golden_vectors/pallas/babbage4.block` | `test-vectors/blocks/pallas/babbage4.block` | 39750 | `2985946d15db4ceae2ba5606e24b4e96113f1e15` |
| `multi-era/rust/tests/golden_vectors/pallas/babbage5.block` | `test-vectors/blocks/pallas/babbage5.block` | 17690 | `68ae3abe9e6acc215d8cec74760d9e373342f107` |
| `multi-era/rust/tests/golden_vectors/pallas/babbage6.block` | `test-vectors/blocks/pallas/babbage6.block` | 8700 | `96d2d78118ebd8576384c7154551e1c6198a9226` |
| `multi-era/rust/tests/golden_vectors/pallas/babbage7.block` | `test-vectors/blocks/pallas/babbage7.block` | 7590 | `067019db3a6bcb05237975e83ef228653b60b949` |
| `multi-era/rust/tests/golden_vectors/pallas/babbage8.block` | `test-vectors/blocks/pallas/babbage8.block` | 8952 | `23da9a764fd315df0ad043f6760910e997b76fb1` |
| `multi-era/rust/tests/golden_vectors/pallas/babbage9.block` | `test-vectors/blocks/pallas/babbage9.block` | 163946 | `0e9c9b93819199815751a02484afc94cd2a3fed5` |
| `multi-era/rust/tests/golden_vectors/pallas/byron1.block` | `test-vectors/blocks/pallas/byron1.block` | 1272 | `8a82df746fc5f97fe5e4b753143f1ab715aa9ce7` |
| `multi-era/rust/tests/golden_vectors/pallas/byron2.block` | `test-vectors/blocks/pallas/byron2.block` | 5150 | `2c26a9a3ac80d01e257d0341f50b919793ab6a91` |
| `multi-era/rust/tests/golden_vectors/pallas/byron3.block` | `test-vectors/blocks/pallas/byron3.block` | 6318 | `cf14fdc1dace12e11af7b4247af1331b469f799d` |
| `multi-era/rust/tests/golden_vectors/pallas/byron4.block` | `test-vectors/blocks/pallas/byron4.block` | 2050 | `4e4831aa2d656f4912cb2b79a59211b795f4e6c1` |
| `multi-era/rust/tests/golden_vectors/pallas/byron5.block` | `test-vectors/blocks/pallas/byron5.block` | 3422 | `5016be61d986e70c0d9daded0da5a33678299fd3` |
| `multi-era/rust/tests/golden_vectors/pallas/byron6.block` | `test-vectors/blocks/pallas/byron6.block` | 2212 | `0d068fd20cebdf900961a65343c5745076650b3e` |
| `multi-era/rust/tests/golden_vectors/pallas/byron7.block` | `test-vectors/blocks/pallas/byron7.block` | 19478 | `9c44ecfd764217ab108d9fcfa6e973239967ff6b` |
| `multi-era/rust/tests/golden_vectors/pallas/byron8.block` | `test-vectors/blocks/pallas/byron8.block` | 4222 | `38d06a5cda80926bfb10643592fecd20f9af8166` |
| `multi-era/rust/tests/golden_vectors/pallas/conway1.block` | `test-vectors/blocks/pallas/conway1.block` | 3482 | `df783ddd3198900870ef9e9afce2a61bf471ef22` |
| `multi-era/rust/tests/golden_vectors/pallas/conway2.block` | `test-vectors/blocks/pallas/conway2.block` | 2394 | `974f8d1e4f179893ef577f8ed266a725360a75f8` |
| `multi-era/rust/tests/golden_vectors/pallas/conway3.block` | `test-vectors/blocks/pallas/conway3.block` | 2534 | `e86d2239304627e9a8a89895167845371192d583` |
| `multi-era/rust/tests/golden_vectors/pallas/conway4.block` | `test-vectors/blocks/pallas/conway4.block` | 3750 | `db91113a1e5c6c631e2cf4208ae774a9daf6f723` |
| `multi-era/rust/tests/golden_vectors/pallas/conway8.block` | `test-vectors/blocks/pallas/conway8.block` | 5106 | `a3272d6fc86f942f48a97f0dbfa71491b2f3b91a` |
| `multi-era/rust/tests/golden_vectors/pallas/genesis.block` | `test-vectors/blocks/pallas/genesis.block` | 1296174 | `aaf713f75c3e20af780357d5e3ccf9f61ed80a81` |
| `multi-era/rust/tests/golden_vectors/pallas/mary1.block` | `test-vectors/blocks/pallas/mary1.block` | 41074 | `7d71682e34a6576e62aa3c6fb456653a3934cc4f` |
| `multi-era/rust/tests/golden_vectors/pallas/shelley1.block` | `test-vectors/blocks/pallas/shelley1.block` | 4876 | `66da341e9d4adf668334c9cdc65a28f218b4bee6` |
| `specs/cip25.cddl` | `specs/cip25.cddl` | 1589 | `aa61058c7b7d031408d886938a7aab277e4d4f3c` |
| `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl` | `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl` | 66 | `b5eb9ec087c90d23e902e1d92108d615207ea0f1` |
| `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/auxdata.cddl` | `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/auxdata.cddl` | 51 | `a8089ba4fcc629a0df30511e805d034c10d5295b` |
| `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/mod.cddl` | `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/mod.cddl` | 35 | `90765882c0cced77ccf19adf08388bc3b514d1af` |
| `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_crypto/mod.cddl` | `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_crypto/mod.cddl` | 93 | `a9e58a522c44aafe31c0091844f1e3294a35fd91` |
| `specs/cip36/lib.cddl` | `specs/cip36/lib.cddl` | 2525 | `fefaf277df60fd72fb22321b5c4ce3837fc12362` |
| `specs/conway/address.cddl` | `specs/conway/address.cddl` | 1133 | `12310db8b3d0e8fb887ebd73299c0990a2603790` |
| `specs/conway/assets.cddl` | `specs/conway/assets.cddl` | 1124 | `56f0bc41e4189cb68196c285e820cabfdafa1e61` |
| `specs/conway/auxdata.cddl` | `specs/conway/auxdata.cddl` | 1065 | `d9a08dda114f9345bea25eadff206591e884dfe6` |
| `specs/conway/block.cddl` | `specs/conway/block.cddl` | 1210 | `5dc7127f29c8d23d4958986d6755f4279147e403` |
| `specs/conway/certs.cddl` | `specs/conway/certs.cddl` | 4325 | `cfda29683f28a2e77baf27a33ff494f6854b3c2f` |
| `specs/conway/crypto.cddl` | `specs/conway/crypto.cddl` | 2388 | `7406c4f1b2d1a4af95c02ecb05692f6d07ae75cb` |
| `specs/conway/governance.cddl` | `specs/conway/governance.cddl` | 1732 | `52291c49e181873e9de549fc990e76f9861ddda9` |
| `specs/conway/lib.cddl` | `specs/conway/lib.cddl` | 3768 | `f275467b19838bc8d1204794a0bbf27498f318b9` |
| `specs/conway/plutus.cddl` | `specs/conway/plutus.cddl` | 3399 | `5f57f5f8803564040b01f30c84381b8e5ca973b6` |
| `specs/conway/transaction.cddl` | `specs/conway/transaction.cddl` | 7593 | `38a8f081b5879f2670a3174b304ab4ba338a0dc3` |
| `specs/multiera-byron/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl` | `specs/multiera-byron/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl` | 1109 | `47a9e1769b013467bc44d7166d9409ebc6c53ef3` |
| `specs/multiera-byron/byron/block.cddl` | `specs/multiera-byron/byron/block.cddl` | 2563 | `cba9b10a1bf4afd75ce2aa6bb473e0e4406ef1f0` |
| `specs/multiera-byron/byron/delegation.cddl` | `specs/multiera-byron/byron/delegation.cddl` | 576 | `c82037bdad0ef7752f00be9444fb4eddbb6eb15f` |
| `specs/multiera-byron/byron/mod.cddl` | `specs/multiera-byron/byron/mod.cddl` | 793 | `10ff904ba21ad9d5b07f0bbba490347f7dd364a4` |
| `specs/multiera-byron/byron/mpc.cddl` | `specs/multiera-byron/byron/mpc.cddl` | 3092 | `114a1fb91172571410171615a758da7d02d1ec16` |
| `specs/multiera-byron/byron/transaction.cddl` | `specs/multiera-byron/byron/transaction.cddl` | 2069 | `5bf057dc0e0af8f23f4de6cb50516740883e8262` |
| `specs/multiera-byron/byron/update.cddl` | `specs/multiera-byron/byron/update.cddl` | 2586 | `5a159e38d5825cf57583455ea3e5a701cbabacf1` |
| `specs/multiera-byron/lib.cddl` | `specs/multiera-byron/lib.cddl` | 266 | `760303164eda745484d00c13ce098db0ecfcb382` |
| `specs/multiera/allegra/mod.cddl` | `specs/multiera/allegra/mod.cddl` | 2274 | `267cfbd82abf9fe8102c80095260b7e7eec6ee60` |
| `specs/multiera/alonzo/mod.cddl` | `specs/multiera/alonzo/mod.cddl` | 4405 | `737cafa515b615c6ac4bae35c8792d7f8533b4ae` |
| `specs/multiera/babbage/mod.cddl` | `specs/multiera/babbage/mod.cddl` | 4957 | `2b2237fcfb533eb801ee0848b378eba43b3050c2` |
| `specs/multiera/lib.cddl` | `specs/multiera/lib.cddl` | 790 | `c6274ab2cee972ed7a44c45e8623c3f2f363508a` |
| `specs/multiera/mary/mod.cddl` | `specs/multiera/mary/mod.cddl` | 1303 | `b015889550efa873cdd71fd49031106e96686cd9` |
| `specs/multiera/shelley/mod.cddl` | `specs/multiera/shelley/mod.cddl` | 5089 | `67e94f54cdfc149f064ffca117f7a9ace06c688c` |

### Completeness and format rules

Reject missing/additional selected paths, truncated source trees, duplicates, unsafe paths, symlinks,
gitlinks, submodules, special/empty files and files over 16 MiB. Preserve upstream bytes exactly;
never execute Rust, upstream hooks, code generators or build/test tooling. Generated Rust source
is captured read-only evidence, not a source generator for this repository.

The five logical vector controls retain schemaVersion 1. The rebased manifest records vector
fixturePath values resolving to 0001 files with unchanged size/hash, storage form, expected result
and era metadata. PROVENANCE.json records this capture's CML commit and inherited origin/license
mappings. Its fixtureInventory and licenseFile strings resolve relative to its physical directory;
supplementalSha256 keys identify logical test-vector artifacts resolved through the inventory.
README documents direct reuse; .gitattributes and Apache license text are inherited unchanged.
Generated JSON is UTF-8, two-space-indented with a trailing newline and preserved key order.

SHA256SUMS contains lowercase SHA-256, two spaces and provider-root-relative physical file paths,
LF, ordered by UTF-8 physical path bytes. It covers every effective artifact except itself; its
own hash is frozen below. Verify it from the provider root. Metadata is excluded from artifact counts.

### Inventory and counts

| Property | Exact value |
| --- | --- |
| Selected CML source blobs | 138: 30 CDDL + 12 Rust + 3 licenses + 92 vectors + 1 upstream provenance |
| Supplemental/generated control files | 6: five vector controls plus SHA256SUMS |
| Effective logical artifacts | 144 |
| Vector cases | 92: 86 historical blocks and 6 genesis JSON fixtures |
| Golden block outcomes | Inherited exact 0001 manifest outcomes, including malformed conway8 rejection |
| Source membership | Complete source-to-artifact table above, verified against the independent Git tree |
| Artifact membership and integrity | Complete resolved inventory below, plus SHA256SUMS and vector manifest/provenance |

### Integrity and licensing rules

CML licenses are captured byte-exact under legal/. CDDL, selected Rust and genesis source use CML’s
MIT attribution; Dolos/Pallas block vectors carry Apache-2.0 provenance and the supplemental text.
Preserve every license and notice; verify SHA256SUMS before use, then every vector’s size/hash and
manifest membership. All Git source blobs passed independent size and Git-hash validation.

### Consumption boundaries and exclusions

CML is comparison evidence; official Ledger defines ledger validity. The CDDL extern placeholders
are not standalone closed grammars. UPLC language/evaluation and message signing are outside this
provider. Keep lossless historical decoding separate from stricter construction policy. Captured
Rust is not a runtime dependency and is never executed. Library source, tests, public API and all
implementation lifecycle decisions remain owned by separately authorized consumer work.

## Captured scope

The artifact inventory contains the complete selected CDDL, behavior sources, vectors and controls.
No additional Rust API coverage, third-party implementation execution, or ledger-validity proof is
implied. No TypeScript source or test was changed during this capture.

## Original capture validation evidence

The following records the original capture checks, not new execution during this reconciliation.
Current migration checks verify local bytes, mappings and counts; no fresh upstream audit is claimed.


All 138 upstream files passed Git-blob/size checks; all 144 final artifacts passed SHA-256 inventory
checks. All 92 vectors, three CML legal files and upstream provenance match 0001 byte-for-byte.
Current local CIP36 and builder tests passed: 29 tests, zero failures. Those tests check the existing
implementation and synthetic/local cases; they do not establish full parity with the new sources.

## Exclusions

Unselected source, reference-only CDDL, arbitrary Rust API parity, Pallas u5c* vectors, upstream
execution, message signing, UPLC evaluator semantics, schedules and automatic implementation.

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
| `0001/SNAPSHOT.md` | `08cd6c6fd13e0ccab4b5726eadf51d21be0794d2778e4f7ef8315571b523cd80` | Immediate predecessor and owner of every reused artifact |

### Physical storage counts

- Effective logical artifacts: 144.
- Newly stored physical artifacts in 0002: 21.
- Logical entries reusing 0001 physical files: 123.
- Logical comparison: 15 added, 6 changed, 10 removed, 123 unchanged.
- Removed entries are absent here; their 0001 files remain available.

### Complete resolved artifact inventory

| Logical path | Physical path from provider root | Bytes | SHA-256 |
| --- | --- | --- | --- |
| `SHA256SUMS` | `0002/artifacts/SHA256SUMS` | 19358 | `cd7043d45e9b604cc6169c61328600ed1c8d84314e4cfe94049af23c6ed9e7d9` |
| `legal/LICENSE` | `0001/artifacts/legal/LICENSE` | 1064 | `456c5929a8238677b7ef27be10ae3fcc9f98e7799f8f27ce1e10a4f21824f2b0` |
| `legal/LICENSE-EMURGO` | `0001/artifacts/legal/LICENSE-EMURGO` | 1191 | `9725d2d36969b002ecf53e7834d7fc1f357264fa1f7414809862b7a2a60af1c8` |
| `legal/LICENSE-IOHK` | `0001/artifacts/legal/LICENSE-IOHK` | 1063 | `d38373e47d0203e1338c85b9ca50a7e6497b48ca42334c2b920d3f359d1bd3b6` |
| `specs/cip25.cddl` | `0001/artifacts/specs/cip25.cddl` | 1589 | `7dd5c249e598d7b28ce9279ba937e9cbe538191d5c7002e765188c0e998c5794` |
| `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl` | `0001/artifacts/specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl` | 66 | `0ad27753bd73c39f4c25c3caf5e6e69b658c8af2f29eb1e9fddcb5448fc494b2` |
| `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/auxdata.cddl` | `0002/artifacts/specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/auxdata.cddl` | 51 | `77f792b9097962d54ee79ec36ceddfda62116f5e76b0bb24cb3820682a46a099` |
| `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/mod.cddl` | `0002/artifacts/specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/mod.cddl` | 35 | `514407168373e5260c7f95b81f546d2c41fa90648304cab80eaedfac4d399077` |
| `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_crypto/mod.cddl` | `0001/artifacts/specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_crypto/mod.cddl` | 93 | `c3d4a5d4343287a097450394346c3715bb43013326c972c3b702076d2ffb54ad` |
| `specs/cip36/lib.cddl` | `0002/artifacts/specs/cip36/lib.cddl` | 2525 | `d7cdd57013236664c7ee320641bf0ed7f5fea4db2aed7b2fe437da08e8abe19b` |
| `specs/conway/address.cddl` | `0001/artifacts/specs/conway/address.cddl` | 1133 | `5c54dd0b87a8d40769692b2ef0393829e8b04932b6e5d777c75d1fe2c4c466de` |
| `specs/conway/assets.cddl` | `0001/artifacts/specs/conway/assets.cddl` | 1124 | `7f010bd3895812459ec1ce1b499ab99d12c18fd75b14b3aa6895b06ec78e57b6` |
| `specs/conway/auxdata.cddl` | `0001/artifacts/specs/conway/auxdata.cddl` | 1065 | `38fe486e8267d52ae446282607de9819b223dead4186a7712293f4704259cb85` |
| `specs/conway/block.cddl` | `0001/artifacts/specs/conway/block.cddl` | 1210 | `a6bb0ee868346a57d9472c21a57191b0db2007759cf4c1827b015084f957a00f` |
| `specs/conway/certs.cddl` | `0001/artifacts/specs/conway/certs.cddl` | 4325 | `b900b298cb648cba94143f359a8ac7234b43018531f03296545678db690a8863` |
| `specs/conway/crypto.cddl` | `0001/artifacts/specs/conway/crypto.cddl` | 2388 | `7f368d7b703a3e18cdcbcff85fa7dc2b1e135681b9d3723c7746025fc1821b5f` |
| `specs/conway/governance.cddl` | `0001/artifacts/specs/conway/governance.cddl` | 1732 | `bd37f836db61a756eed5377d1172edcc2fc12c2fcff86707806908fe2bda51ff` |
| `specs/conway/lib.cddl` | `0001/artifacts/specs/conway/lib.cddl` | 3768 | `c9f1cab4068bb5cf42e953da9f0fd3b932f934b59ba798818de232c1ae1717a7` |
| `specs/conway/plutus.cddl` | `0002/artifacts/specs/conway/plutus.cddl` | 3399 | `3a7112910702fdb4754030ee351a2e3017e12161030cf1f9041f2f5eb630af38` |
| `specs/conway/transaction.cddl` | `0001/artifacts/specs/conway/transaction.cddl` | 7593 | `f79055a97a1e8ded3b19cc2eefca5c3cf16b630c6a206f887372d83c8cf7aafb` |
| `specs/multiera-byron/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl` | `0001/artifacts/specs/multiera-byron/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl` | 1109 | `22aaa8f996981e1dc74807a8954191fa922664d4b3a1484eeb831b860061803a` |
| `specs/multiera-byron/byron/block.cddl` | `0001/artifacts/specs/multiera-byron/byron/block.cddl` | 2563 | `a70e75c91a91f01c0f193b7554392ce0314bdfd15b0b9ffd9d0f199a621b229f` |
| `specs/multiera-byron/byron/delegation.cddl` | `0001/artifacts/specs/multiera-byron/byron/delegation.cddl` | 576 | `645839665e734498f97b5dbfe97a23a508189528812710b9af96214fbfc30876` |
| `specs/multiera-byron/byron/mod.cddl` | `0001/artifacts/specs/multiera-byron/byron/mod.cddl` | 793 | `8a83ab9181d4f3d93c020f2bf8794ed40ca8522f210c80e1808f11601948e12c` |
| `specs/multiera-byron/byron/mpc.cddl` | `0001/artifacts/specs/multiera-byron/byron/mpc.cddl` | 3092 | `05a00b631ee8b62648b0002dd42a957452aa22de8f2ec8d32fdd54d7eac67501` |
| `specs/multiera-byron/byron/transaction.cddl` | `0002/artifacts/specs/multiera-byron/byron/transaction.cddl` | 2069 | `65884fe094184cae480f9ac206a213c0ae368039c2e1e0964b51017e09e57340` |
| `specs/multiera-byron/byron/update.cddl` | `0001/artifacts/specs/multiera-byron/byron/update.cddl` | 2586 | `f3993321f4e870bdd0f2b874778bb1393ee81e0cc183e8f08d03b214216889ad` |
| `specs/multiera-byron/lib.cddl` | `0001/artifacts/specs/multiera-byron/lib.cddl` | 266 | `ea5cbfaa05fd96855d2477ab74dbfb8cbfac9ad11e0ff3f43975ddfea15d42c4` |
| `specs/multiera/allegra/mod.cddl` | `0001/artifacts/specs/multiera/allegra/mod.cddl` | 2274 | `1cfd5f0bab1d882bab448352a2ad1ed2fbe453855a64bdc8f802d4f53210347c` |
| `specs/multiera/alonzo/mod.cddl` | `0001/artifacts/specs/multiera/alonzo/mod.cddl` | 4405 | `50c341fefbb7fd0422e1d55b3eb028283c7dd0201f2ba3f25807f10818056d4d` |
| `specs/multiera/babbage/mod.cddl` | `0001/artifacts/specs/multiera/babbage/mod.cddl` | 4957 | `dc29a2c2fe0a2b96b54242c918c9d627447a805acc34e72cf84899b53e7ad313` |
| `specs/multiera/lib.cddl` | `0001/artifacts/specs/multiera/lib.cddl` | 790 | `9ad13611cf3287026dd56105dbafab683740b8cbd4a9f7b3c454d742687df2e5` |
| `specs/multiera/mary/mod.cddl` | `0001/artifacts/specs/multiera/mary/mod.cddl` | 1303 | `069869c505505948a5a4799ccd6dc836a2ed07640dcb22d976ff6536edeb0eaf` |
| `specs/multiera/shelley/mod.cddl` | `0001/artifacts/specs/multiera/shelley/mod.cddl` | 5089 | `39f4564a3bb9f85780b07c74df9092611bb1c9c01464da4275813b48a0fe690a` |
| `test-vectors/.gitattributes` | `0001/artifacts/test-vectors/.gitattributes` | 49 | `31b52b337518c738389224eba47ae83952d346d49c2766d5dd2101af90c7c0be` |
| `test-vectors/LICENSE-APACHE-2.0.txt` | `0001/artifacts/test-vectors/LICENSE-APACHE-2.0.txt` | 11347 | `4541e95aa81113643b71a96d7ff673c4a83ede3d2e8f0df2ad676e7970e2b1fa` |
| `test-vectors/PROVENANCE.json` | `0002/artifacts/test-vectors/PROVENANCE.json` | 1060 | `d4a8729ac9cc1819fc043084753c41778be46c3d917e86872d24ee3f2e062c3c` |
| `test-vectors/PROVENANCE.md` | `0001/artifacts/test-vectors/PROVENANCE.md` | 2547 | `f36364497bd77fa917b2fc239e72fa87b0e7c31405bcee4e385ff8191b53db01` |
| `test-vectors/README.md` | `0002/artifacts/test-vectors/README.md` | 1158 | `0e0cd8a644b13eafcebf38380b354db3ef87e9b4e897715588868d20fdc6a245` |
| `test-vectors/blocks/mainnet/0822e72ec531fd72b74af75bbce83876547f538ca7ca9bc854d043aa888c478c.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/0822e72ec531fd72b74af75bbce83876547f538ca7ca9bc854d043aa888c478c.cbor` | 55683 | `c7c3dc18f68bfc66a67a4f9442a75bc13a6504ddfb3372b45fa1a1304a2552df` |
| `test-vectors/blocks/mainnet/0ade775289bbf05b0ed77052bac35cc1ef728751fc00081d179d9f6f7728f56d.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/0ade775289bbf05b0ed77052bac35cc1ef728751fc00081d179d9f6f7728f56d.cbor` | 2122 | `aa3933a5a6e8a30182d74c468e847cf29fcf55a9e6f919c2509ae81ad78f4bdc` |
| `test-vectors/blocks/mainnet/11c08542cf8da1ab1d1686c259dfc922e492c8ae893bccb588059f62717e573a.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/11c08542cf8da1ab1d1686c259dfc922e492c8ae893bccb588059f62717e573a.cbor` | 80134 | `685d59f5b3461c4bada700a35244e0e013135dea847bea03eef50b81da250239` |
| `test-vectors/blocks/mainnet/18d49cdcae701d93ec69efd1f8140de6e5337a735a162f5e509e91b8f9ee1c18.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/18d49cdcae701d93ec69efd1f8140de6e5337a735a162f5e509e91b8f9ee1c18.cbor` | 1948 | `b61cb6ceabf11a3168f45bf98def6c75f1f8d0e6ea042905180e49f0d6b06f30` |
| `test-vectors/blocks/mainnet/28532f081779a6c435c31ecc64f95a39417c6edce001fd9a964c622e18de95f4.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/28532f081779a6c435c31ecc64f95a39417c6edce001fd9a964c622e18de95f4.cbor` | 29618 | `98f8d506ea4fba4ba70d6b422c82a2b0ff082961dc30b12c406d7d2935009c74` |
| `test-vectors/blocks/mainnet/36cb7c4cb0053e112f57cde99204c785a4375fb7c6ac3eabe3a02fdc2d4a2867.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/36cb7c4cb0053e112f57cde99204c785a4375fb7c6ac3eabe3a02fdc2d4a2867.cbor` | 66289 | `80555c27072199dbb07cc847669d3ff84910715ac6d404e20fb6a4f89220fe91` |
| `test-vectors/blocks/mainnet/3b234ca0d5ac7c52a5407b63b358391b1b920c17bb40fe50c9fbbd76f77e6f67.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/3b234ca0d5ac7c52a5407b63b358391b1b920c17bb40fe50c9fbbd76f77e6f67.cbor` | 2126 | `bb473e7d2d5d9ea8ff05bf3c62b1495c28be58096e19df2f6d89c8d62425c74a` |
| `test-vectors/blocks/mainnet/3ba7cdc89bf74fc1915da5bd2a593231b70734d30c8a4c61e40e01ad345ca092.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/3ba7cdc89bf74fc1915da5bd2a593231b70734d30c8a4c61e40e01ad345ca092.cbor` | 1354 | `7f7cf88255852249332b8a9f7e5cdbb053dfbf18403704b70cdb0b19c6615ab7` |
| `test-vectors/blocks/mainnet/44b156deb04b7f923e6716fbc8dbda23ba8d748e6dcc08ded4914b5985524ce3.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/44b156deb04b7f923e6716fbc8dbda23ba8d748e6dcc08ded4914b5985524ce3.cbor` | 2157 | `b6e4780cf4847fc7382247c03f33383670bd1663a15c89ef178d49b51bacc907` |
| `test-vectors/blocks/mainnet/55e828f124ef484d24b9ab60d25549f3ffb4999a9b9816904038e53aac2a5cc9.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/55e828f124ef484d24b9ab60d25549f3ffb4999a9b9816904038e53aac2a5cc9.cbor` | 9226 | `db1454be87b95c3e9803109ef920e3ab9dcfc19658dffecd7243f13556cf8917` |
| `test-vectors/blocks/mainnet/56f45594dbd995ae19a892d94db94962ecfab2d016ad387bd634f54ba750c85d.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/56f45594dbd995ae19a892d94db94962ecfab2d016ad387bd634f54ba750c85d.cbor` | 1765 | `0c4325480c1fbb9b8a7031fc784c93cf9b6beff637e5b085328492136d8dc140` |
| `test-vectors/blocks/mainnet/5740eb9b6bb6207b7b1cc663e83532fa51fe826cf61714e747339d134b201681.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/5740eb9b6bb6207b7b1cc663e83532fa51fe826cf61714e747339d134b201681.cbor` | 3009 | `f1c822448d15ced664145f333c6ad7f24be366523e1ada6301adff7c5a84c427` |
| `test-vectors/blocks/mainnet/586525732f41fa76f2c3b6d97831c2b68158b3429901e5c3c4f39506d1029f55.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/586525732f41fa76f2c3b6d97831c2b68158b3429901e5c3c4f39506d1029f55.cbor` | 944 | `d00b1723df729a9e868099c9eda2f5900e3b967ddbbe7bd76ed1e00c34533bc9` |
| `test-vectors/blocks/mainnet/61cc265848410b4fb43daeef29240bae841dfa4389d201bcf2d376922fe14a83.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/61cc265848410b4fb43daeef29240bae841dfa4389d201bcf2d376922fe14a83.cbor` | 1945 | `01c7fffc4c606c89df54de729dda8e5bb2224c8488ab54bdb149158839ca71c9` |
| `test-vectors/blocks/mainnet/62e8b3028c204e9712e9df5412ac47fe44c2683409c2ee0abbb3f07c7c26fbd3.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/62e8b3028c204e9712e9df5412ac47fe44c2683409c2ee0abbb3f07c7c26fbd3.cbor` | 1945 | `c0a7295a4b41732e6cf307da06794e8d6fe2f29088749476bac3d5c9cefcb412` |
| `test-vectors/blocks/mainnet/698344b7f8b07c3ede9da2a0b8e758e89ffcfdd27fdbced449092f4417247291.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/698344b7f8b07c3ede9da2a0b8e758e89ffcfdd27fdbced449092f4417247291.cbor` | 2157 | `c269bb705ecb48b0fc6c767736d39f92d9329c84cd74393b6e0330d2282147ca` |
| `test-vectors/blocks/mainnet/6e1d7a1964693e6d410596c3574fe6e2121be2021218310ea6b5dce067a02625.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/6e1d7a1964693e6d410596c3574fe6e2121be2021218310ea6b5dce067a02625.cbor` | 2118 | `9ded68313f717beae52faec349788be718052cc33607201f9f25efac14e00182` |
| `test-vectors/blocks/mainnet/6f473b27b600ebf575a43623b2e13b6478aa2f2806cd32e1bd9971b4c85a3e72.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/6f473b27b600ebf575a43623b2e13b6478aa2f2806cd32e1bd9971b4c85a3e72.cbor` | 41313 | `23bfb8934fde2ca292f9661f95dd2c9184f05cc3028ed76236dd5004d8f6d892` |
| `test-vectors/blocks/mainnet/844f76e7576a1ae73ce7a50754084f3d30fbd7e696ea8c20cff1858066ca8912.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/844f76e7576a1ae73ce7a50754084f3d30fbd7e696ea8c20cff1858066ca8912.cbor` | 79260 | `c8c97b2b3f521c3cd2b8dfc9388ee5b76f7482c8bc0f93a73afec6fa3eb8b430` |
| `test-vectors/blocks/mainnet/850805044e0df6c13ced2190db7b11489672b0225d478a35a6db71fbfb33afc0.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/850805044e0df6c13ced2190db7b11489672b0225d478a35a6db71fbfb33afc0.cbor` | 2166 | `acae18d2ea40d50108f881581dab722222119453987b12d936b612269cc15980` |
| `test-vectors/blocks/mainnet/8d378effcbbe98c9142371b934c32a3946628a2dfb05eee4472a40ec9d1ca862.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/8d378effcbbe98c9142371b934c32a3946628a2dfb05eee4472a40ec9d1ca862.cbor` | 80005 | `08361ab2aacf4425ee8f8acc783cd5306c5e5b81adeb3c9cde426304f67b3cac` |
| `test-vectors/blocks/mainnet/8d4eb9c1e090f3ed4f23ff3690a4ace2fb474cbfec15c420c88d9c3fe8fe3823.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/8d4eb9c1e090f3ed4f23ff3690a4ace2fb474cbfec15c420c88d9c3fe8fe3823.cbor` | 89342 | `b2e87edcb2748841e2a6d8056ab2434a46cf41f3b3578cf21087196006464d54` |
| `test-vectors/blocks/mainnet/9cefb8bced44c596f5e635ea0a589b89164eaee6a1928cf2ea8e0c13ef4cf700.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/9cefb8bced44c596f5e635ea0a589b89164eaee6a1928cf2ea8e0c13ef4cf700.cbor` | 2303 | `a431d6728adbce53b7c3df0f09fb201b0e4ab8ecf49c26aa48508635d32f3c12` |
| `test-vectors/blocks/mainnet/9f2e53fb897d2546ecdfe8b70ce38a545cc64cabe996f4418135557c87d5f78e.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/9f2e53fb897d2546ecdfe8b70ce38a545cc64cabe996f4418135557c87d5f78e.cbor` | 1800 | `b8d03de20940916060f78532b8cd49cf442517f4fe851521212ad5feaf891ffd` |
| `test-vectors/blocks/mainnet/9f63162b78765332c1632f46960fb386da7a2916090e2ef155ab97941ee1bbb9.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/9f63162b78765332c1632f46960fb386da7a2916090e2ef155ab97941ee1bbb9.cbor` | 1711 | `618acf25fcfa8c0e83b641e5d5704c57c9b7f2dcf13d947d600232c310deabc9` |
| `test-vectors/blocks/mainnet/a84fcbe2447bfc2ed53ba10abee56e81e54f6de72d053faf87bd701a9440b5fb.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/a84fcbe2447bfc2ed53ba10abee56e81e54f6de72d053faf87bd701a9440b5fb.cbor` | 50169 | `303a3ac27099a4b981be8121f114b9e344448be2b2217491aaf79f589ea92d71` |
| `test-vectors/blocks/mainnet/a91e6a056eb03adf3ae5a03787caa00892e617b970cd2422d75773dd4ab6e04e.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/a91e6a056eb03adf3ae5a03787caa00892e617b970cd2422d75773dd4ab6e04e.cbor` | 1883 | `d15a9a3d32ba41c4164fc7e19f6ed28df7141f61c6d0fba230694aa3f6fcfcc2` |
| `test-vectors/blocks/mainnet/bcb8b595c14d85fa278f2d68ddaa1cce758b940a4b9fe76453d6879715b50d90.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/bcb8b595c14d85fa278f2d68ddaa1cce758b940a4b9fe76453d6879715b50d90.cbor` | 42760 | `611b6e9fafccc4f24a2d7f150e2676f555c6c38e7aefeccd974d2642c7af66f2` |
| `test-vectors/blocks/mainnet/bfcfa5a3cc8980d14749084e1357f199f4ae68fecbd790b9870934ea5d4fdca6.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/bfcfa5a3cc8980d14749084e1357f199f4ae68fecbd790b9870934ea5d4fdca6.cbor` | 41283 | `655b374dde6a4868439607d40d276c286e2ab6c708a7cddb0a80e2ab5c2a9727` |
| `test-vectors/blocks/mainnet/c8950f90432bff030d84b0ffd3dab4cc023f03fcb723f0db3420992558b3d99d.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/c8950f90432bff030d84b0ffd3dab4cc023f03fcb723f0db3420992558b3d99d.cbor` | 1910 | `c61507f09f655c42f1181c34425f2687580253efe1ef265a7559af98a438a841` |
| `test-vectors/blocks/mainnet/d798a8d617b25fc6456ffe2d90895a2c15a7271b671dab2d18d46f3d0e4ef495.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/d798a8d617b25fc6456ffe2d90895a2c15a7271b671dab2d18d46f3d0e4ef495.cbor` | 1481 | `93a71179259adb15c763b66a2e13337394076ddbc73f1256e03d0a0cc3b29d89` |
| `test-vectors/blocks/mainnet/da8dd783e7383951dd563aa75c308d1a109ecb4787df7e1ec980710613fddfa8.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/da8dd783e7383951dd563aa75c308d1a109ecb4787df7e1ec980710613fddfa8.cbor` | 57917 | `7717b2462e475a29462796113f5ebca226bc10115e0cc724c4d3885941fec7d3` |
| `test-vectors/blocks/mainnet/fc74c07951c30750d586c4b95f63a7baa6ef51805955dcc7f5dadced530c40f5.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/fc74c07951c30750d586c4b95f63a7baa6ef51805955dcc7f5dadced530c40f5.cbor` | 3060 | `01d8322c16426064317669d5cfc06d8ae9907ca98c95e13dd0b86407db2e0e48` |
| `test-vectors/blocks/mainnet/ff02499ddeb542a740fbebd58730c8ebdd09cf7235471f06d45398c8a9c0c4b3.cbor` | `0001/artifacts/test-vectors/blocks/mainnet/ff02499ddeb542a740fbebd58730c8ebdd09cf7235471f06d45398c8a9c0c4b3.cbor` | 68068 | `b400658ec9bfb429391bca269ba4e536fe64d12456816b7f187961a2d4a59156` |
| `test-vectors/blocks/pallas/allegra1.block` | `0001/artifacts/test-vectors/blocks/pallas/allegra1.block` | 6460 | `ad58d2f26fc044ff8d05790449422a1394a885479fad84c027332b3319eb7455` |
| `test-vectors/blocks/pallas/alonzo1.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo1.block` | 5390 | `39d81d148438c1dd56848bbbd644e7224e55c31218e502e215637486e5ef4490` |
| `test-vectors/blocks/pallas/alonzo10.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo10.block` | 2016 | `8aac2a8c70520564bece55dc0dc2f18707dd3054850a2bde7ca8ed61055b8b5c` |
| `test-vectors/blocks/pallas/alonzo11.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo11.block` | 11928 | `dc23fa436f216ad0bd3042941be3162a1e8da7922545e6458ceba23e7ca2b377` |
| `test-vectors/blocks/pallas/alonzo12.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo12.block` | 133212 | `8817a54f214720d4a8d6d765701f2235ff7496f7e94cfabe7f5183d7e02a1a1f` |
| `test-vectors/blocks/pallas/alonzo13.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo13.block` | 20848 | `4138fc0e730335dc7df355dea95c77149ddba40e97bbfe2cf085c30421113a76` |
| `test-vectors/blocks/pallas/alonzo14.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo14.block` | 142734 | `72374a2ff03c6a7b268bb510a6bbf3d5b5d7bc3df82a8cbc04b09d88e31c10d6` |
| `test-vectors/blocks/pallas/alonzo15.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo15.block` | 34410 | `8cac9a7e52db9b205c265abb7f7a4487e75139aaafec44f3b648c32a83e93f30` |
| `test-vectors/blocks/pallas/alonzo16.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo16.block` | 7508 | `7da29f2579469d95e7c1906e345a1df205e79d45bb413024998d0801b06ee282` |
| `test-vectors/blocks/pallas/alonzo17.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo17.block` | 7412 | `743d9eb770f0448a8b8dba8c0b2490c8bdffd9e82c7609667e0aff864c2899f4` |
| `test-vectors/blocks/pallas/alonzo18.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo18.block` | 34724 | `e65bfad1aec382c10feeb0a1f06207fc583db1ca8ab17e81c34b17eae2e9fd0f` |
| `test-vectors/blocks/pallas/alonzo19.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo19.block` | 3256 | `37455c9af3371ea0bd934bfe215dc2c20f831205ac6b09619582f02bbd5957d3` |
| `test-vectors/blocks/pallas/alonzo2.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo2.block` | 7922 | `d0d4da10c2f20ddc7bbdb01910fdfeb03278b464e75a740347974dba5522d9a9` |
| `test-vectors/blocks/pallas/alonzo20.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo20.block` | 3492 | `31c303deafeb92899f3452a22d9deeb236ac491bcc9d3035e70a138cfdc0c924` |
| `test-vectors/blocks/pallas/alonzo21.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo21.block` | 129418 | `23a96ce7b3b333c7d14edb1eb290499469c39aa52ade1feeeac4554fed5cd8e6` |
| `test-vectors/blocks/pallas/alonzo22.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo22.block` | 2744 | `996bc37ac3c134470f555730c918736cf44b8d08393c766dcd3a22e13915cf29` |
| `test-vectors/blocks/pallas/alonzo23.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo23.block` | 15002 | `f30b2a3510f87cb32e91e3a5e8ca13e5a63ee929b120f7e63a4cd682d89ffa62` |
| `test-vectors/blocks/pallas/alonzo24.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo24.block` | 4456 | `d397feeddab6beb7a6712622ff3c5c396d7ecd240c760e28ef7aa8a7cf0aeb66` |
| `test-vectors/blocks/pallas/alonzo27.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo27.block` | 85426 | `14e613b1e8511204ec6d9de8181a7801ec025d39b53ced91945e2ef1887ca122` |
| `test-vectors/blocks/pallas/alonzo3.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo3.block` | 10506 | `c17a869a79231b943ee62eaea7a0ad187130aff132716637f16b14c3e9d26b15` |
| `test-vectors/blocks/pallas/alonzo4.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo4.block` | 7758 | `da63de894f00e58c99122690830e3cf2f5c26676408c3f541cca27fdad5eb7c2` |
| `test-vectors/blocks/pallas/alonzo5.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo5.block` | 8220 | `3e0eb798b48663343b9d2f30369fe7c9a0eea6ab732ff1149ebe2c0d773820a2` |
| `test-vectors/blocks/pallas/alonzo6.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo6.block` | 36356 | `e0a24f6d1b948e12e50176bc30f75fe0227b4ea62c8740dd8b8a35cf243c228b` |
| `test-vectors/blocks/pallas/alonzo7.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo7.block` | 4146 | `433a1271806920b0e98a953089d2be54e533ae1a3f14cf071d98b80227e21ed6` |
| `test-vectors/blocks/pallas/alonzo8.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo8.block` | 37120 | `894350ec6feeaabb65abbf48d413aa7d8126fb4bed86cc4ee309e9c62d8cf6ea` |
| `test-vectors/blocks/pallas/alonzo9.block` | `0001/artifacts/test-vectors/blocks/pallas/alonzo9.block` | 80292 | `57749c3664f893885c5cb0fec3b7a8504342f3307e8688542f6ef408358d9ace` |
| `test-vectors/blocks/pallas/babbage1.block` | `0001/artifacts/test-vectors/blocks/pallas/babbage1.block` | 3364 | `415aa9f34214cb04926745a3c18600dfd04fd61a6ce4e7fa988a89235da7f8a1` |
| `test-vectors/blocks/pallas/babbage10.block` | `0001/artifacts/test-vectors/blocks/pallas/babbage10.block` | 3174 | `10c9afeef22c180dda998527415da360cc83b36b40bb5610d076998830758713` |
| `test-vectors/blocks/pallas/babbage2.block` | `0001/artifacts/test-vectors/blocks/pallas/babbage2.block` | 3352 | `916ccc461c16203f9558929d796081a93d3cbf8161df53f2b63c75058c6e04a6` |
| `test-vectors/blocks/pallas/babbage3.block` | `0001/artifacts/test-vectors/blocks/pallas/babbage3.block` | 3330 | `7a6d4dbfa7b987ce10d6f26d501d349ff59db8503f17428e16f6753bccb63be2` |
| `test-vectors/blocks/pallas/babbage4.block` | `0001/artifacts/test-vectors/blocks/pallas/babbage4.block` | 39750 | `e8d716175c4e23e7232ccbfc6eef8477181381d08c2f48841a61532207d44355` |
| `test-vectors/blocks/pallas/babbage5.block` | `0001/artifacts/test-vectors/blocks/pallas/babbage5.block` | 17690 | `b3b36cb83d637a23c411791370ebde726954e4cc2bb69d150b94dde3b408b8f9` |
| `test-vectors/blocks/pallas/babbage6.block` | `0001/artifacts/test-vectors/blocks/pallas/babbage6.block` | 8700 | `08f55d5febbb48106cbcd62a2c0d2b370ceb7485650c54391e093f3c75eb275c` |
| `test-vectors/blocks/pallas/babbage7.block` | `0001/artifacts/test-vectors/blocks/pallas/babbage7.block` | 7590 | `d8ba5f4ae18bd982e2be7cee700086763b56de13de4e0c5edd5ad277a77049d3` |
| `test-vectors/blocks/pallas/babbage8.block` | `0001/artifacts/test-vectors/blocks/pallas/babbage8.block` | 8952 | `c4a9596ec569d416dd1d564427de7e3ff18c69df9b0c2761431362eb56b2423d` |
| `test-vectors/blocks/pallas/babbage9.block` | `0001/artifacts/test-vectors/blocks/pallas/babbage9.block` | 163946 | `d3f786eaa7ca5e6d94880c9bd80c2112b8d53649de3cccd618761cb882b182fe` |
| `test-vectors/blocks/pallas/byron1.block` | `0001/artifacts/test-vectors/blocks/pallas/byron1.block` | 1272 | `77a34d9e007e64ac2cb024a3d47615e7a6474ba1ae114ae2a277dc2a8211a14b` |
| `test-vectors/blocks/pallas/byron2.block` | `0001/artifacts/test-vectors/blocks/pallas/byron2.block` | 5150 | `be8d3ca8245dacd7544c0748b425f9869117263a245374ac6acc41d11ee99547` |
| `test-vectors/blocks/pallas/byron3.block` | `0001/artifacts/test-vectors/blocks/pallas/byron3.block` | 6318 | `2a03348561dd73fe8903d341f068161beaa49e36bac52287e00d39de2e689b4a` |
| `test-vectors/blocks/pallas/byron4.block` | `0001/artifacts/test-vectors/blocks/pallas/byron4.block` | 2050 | `9997e1d78c0286764abf8f499bd5bc816f7054fc2bab152f43878b0c50f3e6fc` |
| `test-vectors/blocks/pallas/byron5.block` | `0001/artifacts/test-vectors/blocks/pallas/byron5.block` | 3422 | `36fb6290f2fa77880f8d47b0fe14d260b26b955853becf51fa1744ea76c32b86` |
| `test-vectors/blocks/pallas/byron6.block` | `0001/artifacts/test-vectors/blocks/pallas/byron6.block` | 2212 | `4bdb8afda26b442e1798d7b4fc32ce2fe8451a5b5e2b261e88b8008e8235c374` |
| `test-vectors/blocks/pallas/byron7.block` | `0001/artifacts/test-vectors/blocks/pallas/byron7.block` | 19478 | `ac161f1440a874d966969875cea393eed8085f0b5a2c1a7acc438636a1bb4601` |
| `test-vectors/blocks/pallas/byron8.block` | `0001/artifacts/test-vectors/blocks/pallas/byron8.block` | 4222 | `c3748dccbcd694eec80d301d4a17233a407eb688313e4d3ef6b95427ef08cd09` |
| `test-vectors/blocks/pallas/conway1.block` | `0001/artifacts/test-vectors/blocks/pallas/conway1.block` | 3482 | `29405309714608b328071a26f1fe0f1b778eb7d0e3ae568627749ba68e4ca8c8` |
| `test-vectors/blocks/pallas/conway2.block` | `0001/artifacts/test-vectors/blocks/pallas/conway2.block` | 2394 | `0425168fd5aefdeadc9fc2dca60aa50452cdd142ff7539489788a4cf16cc66ca` |
| `test-vectors/blocks/pallas/conway3.block` | `0001/artifacts/test-vectors/blocks/pallas/conway3.block` | 2534 | `c043303fdb58a6decee03a108e45828eb8201e685ae26a2695b0cf21b32d9096` |
| `test-vectors/blocks/pallas/conway4.block` | `0001/artifacts/test-vectors/blocks/pallas/conway4.block` | 3750 | `b0106388ff177993e865e33cb8bacb8b22e8109150da25feb25f024d36f24df3` |
| `test-vectors/blocks/pallas/conway8.block` | `0001/artifacts/test-vectors/blocks/pallas/conway8.block` | 5106 | `b846ab008159f0df2b9df903deb5dc5a5f6be8b49bc2a41467b9bf6fff048937` |
| `test-vectors/blocks/pallas/genesis.block` | `0001/artifacts/test-vectors/blocks/pallas/genesis.block` | 1296174 | `18976e16f654435df817a4918a3880e33b091a8e312ec4f6fb67fce162c114ce` |
| `test-vectors/blocks/pallas/mary1.block` | `0001/artifacts/test-vectors/blocks/pallas/mary1.block` | 41074 | `64ca50616aa1b93d4254c41f5aa9fec89a7b436472c5df2402b7c3eab954a462` |
| `test-vectors/blocks/pallas/shelley1.block` | `0001/artifacts/test-vectors/blocks/pallas/shelley1.block` | 4876 | `87bb16b8624f3ae0212b57231de58c6576b7b30f29eb482c540e1da78b0f4f21` |
| `test-vectors/genesis/byron/5f20df933584822601f9e3f8c024eb5eb252fe8cefb24d1317dc3d432e940ebb.json` | `0001/artifacts/test-vectors/genesis/byron/5f20df933584822601f9e3f8c024eb5eb252fe8cefb24d1317dc3d432e940ebb.json` | 910756 | `40d865ea0af837ded73c1d4288faaa4647817296b1eb59c7823e4ebc95824ea8` |
| `test-vectors/genesis/byron/96fceff972c2c06bd3bb5243c39215333be6d56aaf4823073dca31afe5038471.json` | `0001/artifacts/test-vectors/genesis/byron/96fceff972c2c06bd3bb5243c39215333be6d56aaf4823073dca31afe5038471.json` | 31112 | `dca3139e907bb943bfeff5f186f3fc71217ff12cabde93a926277c6ddd647d87` |
| `test-vectors/genesis/byron/b7f76950bc4866423538ab7764fc1c7020b24a5f717a5bee3109ff2796567214.json` | `0001/artifacts/test-vectors/genesis/byron/b7f76950bc4866423538ab7764fc1c7020b24a5f717a5bee3109ff2796567214.json` | 27272 | `6b62660df9f2cf27192cc071616ffb0617018c5a42c753200e6f6bac9471793e` |
| `test-vectors/genesis/byron/c6a004d3d178f600cd8caa10abbebe1549bef878f0665aea2903472d5abf7323.json` | `0001/artifacts/test-vectors/genesis/byron/c6a004d3d178f600cd8caa10abbebe1549bef878f0665aea2903472d5abf7323.json` | 905398 | `f2ef79f67f39cc9eec0bff3549b1fdb31d3e454281ba9d9a84807908f5b52ad7` |
| `test-vectors/genesis/shelley/test-yaci.json` | `0001/artifacts/test-vectors/genesis/shelley/test-yaci.json` | 5022 | `0d7293404873954247b1b938d9c3636781e983a23ea10656ad96f1d4cd225e63` |
| `test-vectors/genesis/shelley/test.json` | `0001/artifacts/test-vectors/genesis/shelley/test.json` | 6401 | `555530d611a5884c234be72915a6f11839d89378fe12d13ace328681fbd448c7` |
| `test-vectors/manifest.json` | `0002/artifacts/test-vectors/manifest.json` | 51719 | `6130eea16c6196b07b20694e67e4aac68c26a06df85267b5273abb092f167962` |
| `upstream/chain/rust/src/builders/redeemer_builder.rs` | `0002/artifacts/upstream/chain/rust/src/builders/redeemer_builder.rs` | 15705 | `dbb39470f28363faba28186bacf6a93ccbf979d421f25bf1fa145822192235b4` |
| `upstream/chain/rust/src/builders/tx_builder.rs` | `0002/artifacts/upstream/chain/rust/src/builders/tx_builder.rs` | 326170 | `ba8d067641cc3aae7bea4275192ce63ec985c968509f8118358a8d934a9efb12` |
| `upstream/chain/rust/src/builders/witness_builder.rs` | `0002/artifacts/upstream/chain/rust/src/builders/witness_builder.rs` | 31319 | `e375c9230d1cd1057b6cb9fd051b3fe6e37fc4c1d61a88f3917c3fea3b51a89d` |
| `upstream/cip36/rust/src/generated/cbor_encodings.rs` | `0002/artifacts/upstream/cip36/rust/src/generated/cbor_encodings.rs` | 2927 | `f18f3fc4f827dc1cdb9efa9071046ff9a0a6805edfa740806ed21a9a77fe9550` |
| `upstream/cip36/rust/src/generated/mod.rs` | `0002/artifacts/upstream/cip36/rust/src/generated/mod.rs` | 11145 | `096e4af72ea75f8b5e56a4875958454302b995076210730bd41946691a6e7d2c` |
| `upstream/cip36/rust/src/generated/serialization.rs` | `0002/artifacts/upstream/cip36/rust/src/generated/serialization.rs` | 68841 | `5dfa96d6294417b025a15d529744daf93dd9c3ef8ca2fca4265ad4978ceaf859` |
| `upstream/cip36/rust/src/utils.rs` | `0002/artifacts/upstream/cip36/rust/src/utils.rs` | 9279 | `4c0edb2a640688a6e91faee08a0cfdf33d0f53cee8e6472fb53e1da88d71d89e` |
| `upstream/cip36/rust/tests/cbor_roundtrip.rs` | `0002/artifacts/upstream/cip36/rust/tests/cbor_roundtrip.rs` | 30895 | `660761a34cfae7c81aeee1f87afe2eb2e85323e3fa150c9ea3def7efda582159` |
| `upstream/cip36/rust/tests/invariants.rs` | `0002/artifacts/upstream/cip36/rust/tests/invariants.rs` | 22912 | `3b1eb13bf953292b7431a86666a7205fba664680ab919c4749fab31b9fad6f0f` |
| `upstream/cip36/rust/tests/json.rs` | `0002/artifacts/upstream/cip36/rust/tests/json.rs` | 16894 | `735cf94b775bed1bccf398a39e07e1bfc80c5e680cafb573b65e71d4d10cc740` |
| `upstream/cip36/rust/tests/unknown_keys.rs` | `0002/artifacts/upstream/cip36/rust/tests/unknown_keys.rs` | 26039 | `501d14e4280e7706ddb4f936247bf4f63254abb2693dd5d72a07cdd94560eda0` |
| `upstream/core/rust/src/ordered_set.rs` | `0002/artifacts/upstream/core/rust/src/ordered_set.rs` | 20895 | `c964ddcc2204800dbc0d04dc86e51208a34866f5dbe555f91561de9344615e1f` |

### Changes from predecessor

| Logical path | Change | Previous physical path / SHA-256 | Current physical path / SHA-256 |
| --- | --- | --- | --- |
| `SHA256SUMS` | added | NONE | `0002/artifacts/SHA256SUMS` / `cd7043d45e9b604cc6169c61328600ed1c8d84314e4cfe94049af23c6ed9e7d9` |
| `legal/LICENSE` | unchanged | `0001/artifacts/legal/LICENSE` / `456c5929a8238677b7ef27be10ae3fcc9f98e7799f8f27ce1e10a4f21824f2b0` | `0001/artifacts/legal/LICENSE` / `456c5929a8238677b7ef27be10ae3fcc9f98e7799f8f27ce1e10a4f21824f2b0` |
| `legal/LICENSE-EMURGO` | unchanged | `0001/artifacts/legal/LICENSE-EMURGO` / `9725d2d36969b002ecf53e7834d7fc1f357264fa1f7414809862b7a2a60af1c8` | `0001/artifacts/legal/LICENSE-EMURGO` / `9725d2d36969b002ecf53e7834d7fc1f357264fa1f7414809862b7a2a60af1c8` |
| `legal/LICENSE-IOHK` | unchanged | `0001/artifacts/legal/LICENSE-IOHK` / `d38373e47d0203e1338c85b9ca50a7e6497b48ca42334c2b920d3f359d1bd3b6` | `0001/artifacts/legal/LICENSE-IOHK` / `d38373e47d0203e1338c85b9ca50a7e6497b48ca42334c2b920d3f359d1bd3b6` |
| `specs/cip25.cddl` | unchanged | `0001/artifacts/specs/cip25.cddl` / `7dd5c249e598d7b28ce9279ba937e9cbe538191d5c7002e765188c0e998c5794` | `0001/artifacts/specs/cip25.cddl` / `7dd5c249e598d7b28ce9279ba937e9cbe538191d5c7002e765188c0e998c5794` |
| `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl` | unchanged | `0001/artifacts/specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl` / `0ad27753bd73c39f4c25c3caf5e6e69b658c8af2f29eb1e9fddcb5448fc494b2` | `0001/artifacts/specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl` / `0ad27753bd73c39f4c25c3caf5e6e69b658c8af2f29eb1e9fddcb5448fc494b2` |
| `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/auxdata.cddl` | added | NONE | `0002/artifacts/specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/auxdata.cddl` / `77f792b9097962d54ee79ec36ceddfda62116f5e76b0bb24cb3820682a46a099` |
| `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/mod.cddl` | added | NONE | `0002/artifacts/specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/mod.cddl` / `514407168373e5260c7f95b81f546d2c41fa90648304cab80eaedfac4d399077` |
| `specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_crypto/mod.cddl` | unchanged | `0001/artifacts/specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_crypto/mod.cddl` / `c3d4a5d4343287a097450394346c3715bb43013326c972c3b702076d2ffb54ad` | `0001/artifacts/specs/cip36/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_crypto/mod.cddl` / `c3d4a5d4343287a097450394346c3715bb43013326c972c3b702076d2ffb54ad` |
| `specs/cip36/lib.cddl` | changed | `0001/artifacts/specs/cip36/lib.cddl` / `d16f45ba77813b5895337e5a2660c94d7b161db9601bd3738ca896ac101ab849` | `0002/artifacts/specs/cip36/lib.cddl` / `d7cdd57013236664c7ee320641bf0ed7f5fea4db2aed7b2fe437da08e8abe19b` |
| `specs/conway/address.cddl` | unchanged | `0001/artifacts/specs/conway/address.cddl` / `5c54dd0b87a8d40769692b2ef0393829e8b04932b6e5d777c75d1fe2c4c466de` | `0001/artifacts/specs/conway/address.cddl` / `5c54dd0b87a8d40769692b2ef0393829e8b04932b6e5d777c75d1fe2c4c466de` |
| `specs/conway/assets.cddl` | unchanged | `0001/artifacts/specs/conway/assets.cddl` / `7f010bd3895812459ec1ce1b499ab99d12c18fd75b14b3aa6895b06ec78e57b6` | `0001/artifacts/specs/conway/assets.cddl` / `7f010bd3895812459ec1ce1b499ab99d12c18fd75b14b3aa6895b06ec78e57b6` |
| `specs/conway/auxdata.cddl` | unchanged | `0001/artifacts/specs/conway/auxdata.cddl` / `38fe486e8267d52ae446282607de9819b223dead4186a7712293f4704259cb85` | `0001/artifacts/specs/conway/auxdata.cddl` / `38fe486e8267d52ae446282607de9819b223dead4186a7712293f4704259cb85` |
| `specs/conway/block.cddl` | unchanged | `0001/artifacts/specs/conway/block.cddl` / `a6bb0ee868346a57d9472c21a57191b0db2007759cf4c1827b015084f957a00f` | `0001/artifacts/specs/conway/block.cddl` / `a6bb0ee868346a57d9472c21a57191b0db2007759cf4c1827b015084f957a00f` |
| `specs/conway/certs.cddl` | unchanged | `0001/artifacts/specs/conway/certs.cddl` / `b900b298cb648cba94143f359a8ac7234b43018531f03296545678db690a8863` | `0001/artifacts/specs/conway/certs.cddl` / `b900b298cb648cba94143f359a8ac7234b43018531f03296545678db690a8863` |
| `specs/conway/crypto.cddl` | unchanged | `0001/artifacts/specs/conway/crypto.cddl` / `7f368d7b703a3e18cdcbcff85fa7dc2b1e135681b9d3723c7746025fc1821b5f` | `0001/artifacts/specs/conway/crypto.cddl` / `7f368d7b703a3e18cdcbcff85fa7dc2b1e135681b9d3723c7746025fc1821b5f` |
| `specs/conway/governance.cddl` | unchanged | `0001/artifacts/specs/conway/governance.cddl` / `bd37f836db61a756eed5377d1172edcc2fc12c2fcff86707806908fe2bda51ff` | `0001/artifacts/specs/conway/governance.cddl` / `bd37f836db61a756eed5377d1172edcc2fc12c2fcff86707806908fe2bda51ff` |
| `specs/conway/lib.cddl` | unchanged | `0001/artifacts/specs/conway/lib.cddl` / `c9f1cab4068bb5cf42e953da9f0fd3b932f934b59ba798818de232c1ae1717a7` | `0001/artifacts/specs/conway/lib.cddl` / `c9f1cab4068bb5cf42e953da9f0fd3b932f934b59ba798818de232c1ae1717a7` |
| `specs/conway/plutus.cddl` | changed | `0001/artifacts/specs/conway/plutus.cddl` / `e42786e438dbc1530d9ca00b11480320193b72a82ca28d1ffe5f14a400e7a66e` | `0002/artifacts/specs/conway/plutus.cddl` / `3a7112910702fdb4754030ee351a2e3017e12161030cf1f9041f2f5eb630af38` |
| `specs/conway/transaction.cddl` | unchanged | `0001/artifacts/specs/conway/transaction.cddl` / `f79055a97a1e8ded3b19cc2eefca5c3cf16b630c6a206f887372d83c8cf7aafb` | `0001/artifacts/specs/conway/transaction.cddl` / `f79055a97a1e8ded3b19cc2eefca5c3cf16b630c6a206f887372d83c8cf7aafb` |
| `specs/multiera-byron/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl` | unchanged | `0001/artifacts/specs/multiera-byron/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl` / `22aaa8f996981e1dc74807a8954191fa922664d4b3a1484eeb831b860061803a` | `0001/artifacts/specs/multiera-byron/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl` / `22aaa8f996981e1dc74807a8954191fa922664d4b3a1484eeb831b860061803a` |
| `specs/multiera-byron/byron/block.cddl` | unchanged | `0001/artifacts/specs/multiera-byron/byron/block.cddl` / `a70e75c91a91f01c0f193b7554392ce0314bdfd15b0b9ffd9d0f199a621b229f` | `0001/artifacts/specs/multiera-byron/byron/block.cddl` / `a70e75c91a91f01c0f193b7554392ce0314bdfd15b0b9ffd9d0f199a621b229f` |
| `specs/multiera-byron/byron/delegation.cddl` | unchanged | `0001/artifacts/specs/multiera-byron/byron/delegation.cddl` / `645839665e734498f97b5dbfe97a23a508189528812710b9af96214fbfc30876` | `0001/artifacts/specs/multiera-byron/byron/delegation.cddl` / `645839665e734498f97b5dbfe97a23a508189528812710b9af96214fbfc30876` |
| `specs/multiera-byron/byron/mod.cddl` | unchanged | `0001/artifacts/specs/multiera-byron/byron/mod.cddl` / `8a83ab9181d4f3d93c020f2bf8794ed40ca8522f210c80e1808f11601948e12c` | `0001/artifacts/specs/multiera-byron/byron/mod.cddl` / `8a83ab9181d4f3d93c020f2bf8794ed40ca8522f210c80e1808f11601948e12c` |
| `specs/multiera-byron/byron/mpc.cddl` | unchanged | `0001/artifacts/specs/multiera-byron/byron/mpc.cddl` / `05a00b631ee8b62648b0002dd42a957452aa22de8f2ec8d32fdd54d7eac67501` | `0001/artifacts/specs/multiera-byron/byron/mpc.cddl` / `05a00b631ee8b62648b0002dd42a957452aa22de8f2ec8d32fdd54d7eac67501` |
| `specs/multiera-byron/byron/transaction.cddl` | changed | `0001/artifacts/specs/multiera-byron/byron/transaction.cddl` / `7afd0131c3a13040cba3ef29708590ac86616f811f591aa230b84e002fa061dc` | `0002/artifacts/specs/multiera-byron/byron/transaction.cddl` / `65884fe094184cae480f9ac206a213c0ae368039c2e1e0964b51017e09e57340` |
| `specs/multiera-byron/byron/update.cddl` | unchanged | `0001/artifacts/specs/multiera-byron/byron/update.cddl` / `f3993321f4e870bdd0f2b874778bb1393ee81e0cc183e8f08d03b214216889ad` | `0001/artifacts/specs/multiera-byron/byron/update.cddl` / `f3993321f4e870bdd0f2b874778bb1393ee81e0cc183e8f08d03b214216889ad` |
| `specs/multiera-byron/lib.cddl` | unchanged | `0001/artifacts/specs/multiera-byron/lib.cddl` / `ea5cbfaa05fd96855d2477ab74dbfb8cbfac9ad11e0ff3f43975ddfea15d42c4` | `0001/artifacts/specs/multiera-byron/lib.cddl` / `ea5cbfaa05fd96855d2477ab74dbfb8cbfac9ad11e0ff3f43975ddfea15d42c4` |
| `specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl` | removed | `0001/artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/address.cddl` / `265b63c66f750d6d5bf3cae5f30f3e021173f69c772c3b793de9d418d246f33d` | NONE |
| `specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/assets.cddl` | removed | `0001/artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/assets.cddl` / `3c9829a602425689817aca0865a9204500570d2373b8ef482140b2a1e45b9e84` | NONE |
| `specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/auxdata.cddl` | removed | `0001/artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/auxdata.cddl` / `4dfde4f81edabdfd92a2d276f79a2fbbc209d88c27ba5ab3077fc66231cb07b9` | NONE |
| `specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/block.cddl` | removed | `0001/artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/block.cddl` / `143579223d49d8e83814d439991d7128b151cba43cb60651841bbe25a31186a4` | NONE |
| `specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl` | removed | `0001/artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/byron.cddl` / `d6147183d31e337b58ac792da3935b983ed8b888a977b37952edbc952ffbe082` | NONE |
| `specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/certs.cddl` | removed | `0001/artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/certs.cddl` / `612076245460d3804290f603382b64bce2df4333ecc6e1bcdaa310e721d2642b` | NONE |
| `specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/crypto.cddl` | removed | `0001/artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/crypto.cddl` / `c3ccb293a93c38530e68e38838c09e6c0fee03795c64345f0f9fb2515f5d741d` | NONE |
| `specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/mod.cddl` | removed | `0001/artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/mod.cddl` / `93a95689175ecf1322a9ae8e0d004cfd6029ec19175eee5ef8b5ba47f4002e66` | NONE |
| `specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/plutus.cddl` | removed | `0001/artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/plutus.cddl` / `37577835deeccdd70854174f78de212d39fa11317bface84bc973c4553ea8996` | NONE |
| `specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/transaction.cddl` | removed | `0001/artifacts/specs/multiera/_CDDL_CODEGEN_EXTERN_DEPS_DIR_/cml_chain/transaction.cddl` / `10cfe0ef7533a9b72b02ef8f6d41f5bf545e8e3cc71c5170acdbbfa476b404c4` | NONE |
| `specs/multiera/allegra/mod.cddl` | unchanged | `0001/artifacts/specs/multiera/allegra/mod.cddl` / `1cfd5f0bab1d882bab448352a2ad1ed2fbe453855a64bdc8f802d4f53210347c` | `0001/artifacts/specs/multiera/allegra/mod.cddl` / `1cfd5f0bab1d882bab448352a2ad1ed2fbe453855a64bdc8f802d4f53210347c` |
| `specs/multiera/alonzo/mod.cddl` | unchanged | `0001/artifacts/specs/multiera/alonzo/mod.cddl` / `50c341fefbb7fd0422e1d55b3eb028283c7dd0201f2ba3f25807f10818056d4d` | `0001/artifacts/specs/multiera/alonzo/mod.cddl` / `50c341fefbb7fd0422e1d55b3eb028283c7dd0201f2ba3f25807f10818056d4d` |
| `specs/multiera/babbage/mod.cddl` | unchanged | `0001/artifacts/specs/multiera/babbage/mod.cddl` / `dc29a2c2fe0a2b96b54242c918c9d627447a805acc34e72cf84899b53e7ad313` | `0001/artifacts/specs/multiera/babbage/mod.cddl` / `dc29a2c2fe0a2b96b54242c918c9d627447a805acc34e72cf84899b53e7ad313` |
| `specs/multiera/lib.cddl` | unchanged | `0001/artifacts/specs/multiera/lib.cddl` / `9ad13611cf3287026dd56105dbafab683740b8cbd4a9f7b3c454d742687df2e5` | `0001/artifacts/specs/multiera/lib.cddl` / `9ad13611cf3287026dd56105dbafab683740b8cbd4a9f7b3c454d742687df2e5` |
| `specs/multiera/mary/mod.cddl` | unchanged | `0001/artifacts/specs/multiera/mary/mod.cddl` / `069869c505505948a5a4799ccd6dc836a2ed07640dcb22d976ff6536edeb0eaf` | `0001/artifacts/specs/multiera/mary/mod.cddl` / `069869c505505948a5a4799ccd6dc836a2ed07640dcb22d976ff6536edeb0eaf` |
| `specs/multiera/shelley/mod.cddl` | unchanged | `0001/artifacts/specs/multiera/shelley/mod.cddl` / `39f4564a3bb9f85780b07c74df9092611bb1c9c01464da4275813b48a0fe690a` | `0001/artifacts/specs/multiera/shelley/mod.cddl` / `39f4564a3bb9f85780b07c74df9092611bb1c9c01464da4275813b48a0fe690a` |
| `test-vectors/.gitattributes` | unchanged | `0001/artifacts/test-vectors/.gitattributes` / `31b52b337518c738389224eba47ae83952d346d49c2766d5dd2101af90c7c0be` | `0001/artifacts/test-vectors/.gitattributes` / `31b52b337518c738389224eba47ae83952d346d49c2766d5dd2101af90c7c0be` |
| `test-vectors/LICENSE-APACHE-2.0.txt` | unchanged | `0001/artifacts/test-vectors/LICENSE-APACHE-2.0.txt` / `4541e95aa81113643b71a96d7ff673c4a83ede3d2e8f0df2ad676e7970e2b1fa` | `0001/artifacts/test-vectors/LICENSE-APACHE-2.0.txt` / `4541e95aa81113643b71a96d7ff673c4a83ede3d2e8f0df2ad676e7970e2b1fa` |
| `test-vectors/PROVENANCE.json` | changed | `0001/artifacts/test-vectors/PROVENANCE.json` / `798da8585e837153850d06cdb437b4b1ad4768033d3a2ce24ac79e7d6259cedd` | `0002/artifacts/test-vectors/PROVENANCE.json` / `d4a8729ac9cc1819fc043084753c41778be46c3d917e86872d24ee3f2e062c3c` |
| `test-vectors/PROVENANCE.md` | unchanged | `0001/artifacts/test-vectors/PROVENANCE.md` / `f36364497bd77fa917b2fc239e72fa87b0e7c31405bcee4e385ff8191b53db01` | `0001/artifacts/test-vectors/PROVENANCE.md` / `f36364497bd77fa917b2fc239e72fa87b0e7c31405bcee4e385ff8191b53db01` |
| `test-vectors/README.md` | changed | `0001/artifacts/test-vectors/README.md` / `1e12b5ba2618dbc6f8273a7c2b9902dcc35e223cbfb2efa859f1b80074d8cf89` | `0002/artifacts/test-vectors/README.md` / `0e0cd8a644b13eafcebf38380b354db3ef87e9b4e897715588868d20fdc6a245` |
| `test-vectors/blocks/mainnet/0822e72ec531fd72b74af75bbce83876547f538ca7ca9bc854d043aa888c478c.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/0822e72ec531fd72b74af75bbce83876547f538ca7ca9bc854d043aa888c478c.cbor` / `c7c3dc18f68bfc66a67a4f9442a75bc13a6504ddfb3372b45fa1a1304a2552df` | `0001/artifacts/test-vectors/blocks/mainnet/0822e72ec531fd72b74af75bbce83876547f538ca7ca9bc854d043aa888c478c.cbor` / `c7c3dc18f68bfc66a67a4f9442a75bc13a6504ddfb3372b45fa1a1304a2552df` |
| `test-vectors/blocks/mainnet/0ade775289bbf05b0ed77052bac35cc1ef728751fc00081d179d9f6f7728f56d.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/0ade775289bbf05b0ed77052bac35cc1ef728751fc00081d179d9f6f7728f56d.cbor` / `aa3933a5a6e8a30182d74c468e847cf29fcf55a9e6f919c2509ae81ad78f4bdc` | `0001/artifacts/test-vectors/blocks/mainnet/0ade775289bbf05b0ed77052bac35cc1ef728751fc00081d179d9f6f7728f56d.cbor` / `aa3933a5a6e8a30182d74c468e847cf29fcf55a9e6f919c2509ae81ad78f4bdc` |
| `test-vectors/blocks/mainnet/11c08542cf8da1ab1d1686c259dfc922e492c8ae893bccb588059f62717e573a.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/11c08542cf8da1ab1d1686c259dfc922e492c8ae893bccb588059f62717e573a.cbor` / `685d59f5b3461c4bada700a35244e0e013135dea847bea03eef50b81da250239` | `0001/artifacts/test-vectors/blocks/mainnet/11c08542cf8da1ab1d1686c259dfc922e492c8ae893bccb588059f62717e573a.cbor` / `685d59f5b3461c4bada700a35244e0e013135dea847bea03eef50b81da250239` |
| `test-vectors/blocks/mainnet/18d49cdcae701d93ec69efd1f8140de6e5337a735a162f5e509e91b8f9ee1c18.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/18d49cdcae701d93ec69efd1f8140de6e5337a735a162f5e509e91b8f9ee1c18.cbor` / `b61cb6ceabf11a3168f45bf98def6c75f1f8d0e6ea042905180e49f0d6b06f30` | `0001/artifacts/test-vectors/blocks/mainnet/18d49cdcae701d93ec69efd1f8140de6e5337a735a162f5e509e91b8f9ee1c18.cbor` / `b61cb6ceabf11a3168f45bf98def6c75f1f8d0e6ea042905180e49f0d6b06f30` |
| `test-vectors/blocks/mainnet/28532f081779a6c435c31ecc64f95a39417c6edce001fd9a964c622e18de95f4.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/28532f081779a6c435c31ecc64f95a39417c6edce001fd9a964c622e18de95f4.cbor` / `98f8d506ea4fba4ba70d6b422c82a2b0ff082961dc30b12c406d7d2935009c74` | `0001/artifacts/test-vectors/blocks/mainnet/28532f081779a6c435c31ecc64f95a39417c6edce001fd9a964c622e18de95f4.cbor` / `98f8d506ea4fba4ba70d6b422c82a2b0ff082961dc30b12c406d7d2935009c74` |
| `test-vectors/blocks/mainnet/36cb7c4cb0053e112f57cde99204c785a4375fb7c6ac3eabe3a02fdc2d4a2867.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/36cb7c4cb0053e112f57cde99204c785a4375fb7c6ac3eabe3a02fdc2d4a2867.cbor` / `80555c27072199dbb07cc847669d3ff84910715ac6d404e20fb6a4f89220fe91` | `0001/artifacts/test-vectors/blocks/mainnet/36cb7c4cb0053e112f57cde99204c785a4375fb7c6ac3eabe3a02fdc2d4a2867.cbor` / `80555c27072199dbb07cc847669d3ff84910715ac6d404e20fb6a4f89220fe91` |
| `test-vectors/blocks/mainnet/3b234ca0d5ac7c52a5407b63b358391b1b920c17bb40fe50c9fbbd76f77e6f67.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/3b234ca0d5ac7c52a5407b63b358391b1b920c17bb40fe50c9fbbd76f77e6f67.cbor` / `bb473e7d2d5d9ea8ff05bf3c62b1495c28be58096e19df2f6d89c8d62425c74a` | `0001/artifacts/test-vectors/blocks/mainnet/3b234ca0d5ac7c52a5407b63b358391b1b920c17bb40fe50c9fbbd76f77e6f67.cbor` / `bb473e7d2d5d9ea8ff05bf3c62b1495c28be58096e19df2f6d89c8d62425c74a` |
| `test-vectors/blocks/mainnet/3ba7cdc89bf74fc1915da5bd2a593231b70734d30c8a4c61e40e01ad345ca092.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/3ba7cdc89bf74fc1915da5bd2a593231b70734d30c8a4c61e40e01ad345ca092.cbor` / `7f7cf88255852249332b8a9f7e5cdbb053dfbf18403704b70cdb0b19c6615ab7` | `0001/artifacts/test-vectors/blocks/mainnet/3ba7cdc89bf74fc1915da5bd2a593231b70734d30c8a4c61e40e01ad345ca092.cbor` / `7f7cf88255852249332b8a9f7e5cdbb053dfbf18403704b70cdb0b19c6615ab7` |
| `test-vectors/blocks/mainnet/44b156deb04b7f923e6716fbc8dbda23ba8d748e6dcc08ded4914b5985524ce3.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/44b156deb04b7f923e6716fbc8dbda23ba8d748e6dcc08ded4914b5985524ce3.cbor` / `b6e4780cf4847fc7382247c03f33383670bd1663a15c89ef178d49b51bacc907` | `0001/artifacts/test-vectors/blocks/mainnet/44b156deb04b7f923e6716fbc8dbda23ba8d748e6dcc08ded4914b5985524ce3.cbor` / `b6e4780cf4847fc7382247c03f33383670bd1663a15c89ef178d49b51bacc907` |
| `test-vectors/blocks/mainnet/55e828f124ef484d24b9ab60d25549f3ffb4999a9b9816904038e53aac2a5cc9.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/55e828f124ef484d24b9ab60d25549f3ffb4999a9b9816904038e53aac2a5cc9.cbor` / `db1454be87b95c3e9803109ef920e3ab9dcfc19658dffecd7243f13556cf8917` | `0001/artifacts/test-vectors/blocks/mainnet/55e828f124ef484d24b9ab60d25549f3ffb4999a9b9816904038e53aac2a5cc9.cbor` / `db1454be87b95c3e9803109ef920e3ab9dcfc19658dffecd7243f13556cf8917` |
| `test-vectors/blocks/mainnet/56f45594dbd995ae19a892d94db94962ecfab2d016ad387bd634f54ba750c85d.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/56f45594dbd995ae19a892d94db94962ecfab2d016ad387bd634f54ba750c85d.cbor` / `0c4325480c1fbb9b8a7031fc784c93cf9b6beff637e5b085328492136d8dc140` | `0001/artifacts/test-vectors/blocks/mainnet/56f45594dbd995ae19a892d94db94962ecfab2d016ad387bd634f54ba750c85d.cbor` / `0c4325480c1fbb9b8a7031fc784c93cf9b6beff637e5b085328492136d8dc140` |
| `test-vectors/blocks/mainnet/5740eb9b6bb6207b7b1cc663e83532fa51fe826cf61714e747339d134b201681.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/5740eb9b6bb6207b7b1cc663e83532fa51fe826cf61714e747339d134b201681.cbor` / `f1c822448d15ced664145f333c6ad7f24be366523e1ada6301adff7c5a84c427` | `0001/artifacts/test-vectors/blocks/mainnet/5740eb9b6bb6207b7b1cc663e83532fa51fe826cf61714e747339d134b201681.cbor` / `f1c822448d15ced664145f333c6ad7f24be366523e1ada6301adff7c5a84c427` |
| `test-vectors/blocks/mainnet/586525732f41fa76f2c3b6d97831c2b68158b3429901e5c3c4f39506d1029f55.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/586525732f41fa76f2c3b6d97831c2b68158b3429901e5c3c4f39506d1029f55.cbor` / `d00b1723df729a9e868099c9eda2f5900e3b967ddbbe7bd76ed1e00c34533bc9` | `0001/artifacts/test-vectors/blocks/mainnet/586525732f41fa76f2c3b6d97831c2b68158b3429901e5c3c4f39506d1029f55.cbor` / `d00b1723df729a9e868099c9eda2f5900e3b967ddbbe7bd76ed1e00c34533bc9` |
| `test-vectors/blocks/mainnet/61cc265848410b4fb43daeef29240bae841dfa4389d201bcf2d376922fe14a83.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/61cc265848410b4fb43daeef29240bae841dfa4389d201bcf2d376922fe14a83.cbor` / `01c7fffc4c606c89df54de729dda8e5bb2224c8488ab54bdb149158839ca71c9` | `0001/artifacts/test-vectors/blocks/mainnet/61cc265848410b4fb43daeef29240bae841dfa4389d201bcf2d376922fe14a83.cbor` / `01c7fffc4c606c89df54de729dda8e5bb2224c8488ab54bdb149158839ca71c9` |
| `test-vectors/blocks/mainnet/62e8b3028c204e9712e9df5412ac47fe44c2683409c2ee0abbb3f07c7c26fbd3.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/62e8b3028c204e9712e9df5412ac47fe44c2683409c2ee0abbb3f07c7c26fbd3.cbor` / `c0a7295a4b41732e6cf307da06794e8d6fe2f29088749476bac3d5c9cefcb412` | `0001/artifacts/test-vectors/blocks/mainnet/62e8b3028c204e9712e9df5412ac47fe44c2683409c2ee0abbb3f07c7c26fbd3.cbor` / `c0a7295a4b41732e6cf307da06794e8d6fe2f29088749476bac3d5c9cefcb412` |
| `test-vectors/blocks/mainnet/698344b7f8b07c3ede9da2a0b8e758e89ffcfdd27fdbced449092f4417247291.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/698344b7f8b07c3ede9da2a0b8e758e89ffcfdd27fdbced449092f4417247291.cbor` / `c269bb705ecb48b0fc6c767736d39f92d9329c84cd74393b6e0330d2282147ca` | `0001/artifacts/test-vectors/blocks/mainnet/698344b7f8b07c3ede9da2a0b8e758e89ffcfdd27fdbced449092f4417247291.cbor` / `c269bb705ecb48b0fc6c767736d39f92d9329c84cd74393b6e0330d2282147ca` |
| `test-vectors/blocks/mainnet/6e1d7a1964693e6d410596c3574fe6e2121be2021218310ea6b5dce067a02625.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/6e1d7a1964693e6d410596c3574fe6e2121be2021218310ea6b5dce067a02625.cbor` / `9ded68313f717beae52faec349788be718052cc33607201f9f25efac14e00182` | `0001/artifacts/test-vectors/blocks/mainnet/6e1d7a1964693e6d410596c3574fe6e2121be2021218310ea6b5dce067a02625.cbor` / `9ded68313f717beae52faec349788be718052cc33607201f9f25efac14e00182` |
| `test-vectors/blocks/mainnet/6f473b27b600ebf575a43623b2e13b6478aa2f2806cd32e1bd9971b4c85a3e72.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/6f473b27b600ebf575a43623b2e13b6478aa2f2806cd32e1bd9971b4c85a3e72.cbor` / `23bfb8934fde2ca292f9661f95dd2c9184f05cc3028ed76236dd5004d8f6d892` | `0001/artifacts/test-vectors/blocks/mainnet/6f473b27b600ebf575a43623b2e13b6478aa2f2806cd32e1bd9971b4c85a3e72.cbor` / `23bfb8934fde2ca292f9661f95dd2c9184f05cc3028ed76236dd5004d8f6d892` |
| `test-vectors/blocks/mainnet/844f76e7576a1ae73ce7a50754084f3d30fbd7e696ea8c20cff1858066ca8912.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/844f76e7576a1ae73ce7a50754084f3d30fbd7e696ea8c20cff1858066ca8912.cbor` / `c8c97b2b3f521c3cd2b8dfc9388ee5b76f7482c8bc0f93a73afec6fa3eb8b430` | `0001/artifacts/test-vectors/blocks/mainnet/844f76e7576a1ae73ce7a50754084f3d30fbd7e696ea8c20cff1858066ca8912.cbor` / `c8c97b2b3f521c3cd2b8dfc9388ee5b76f7482c8bc0f93a73afec6fa3eb8b430` |
| `test-vectors/blocks/mainnet/850805044e0df6c13ced2190db7b11489672b0225d478a35a6db71fbfb33afc0.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/850805044e0df6c13ced2190db7b11489672b0225d478a35a6db71fbfb33afc0.cbor` / `acae18d2ea40d50108f881581dab722222119453987b12d936b612269cc15980` | `0001/artifacts/test-vectors/blocks/mainnet/850805044e0df6c13ced2190db7b11489672b0225d478a35a6db71fbfb33afc0.cbor` / `acae18d2ea40d50108f881581dab722222119453987b12d936b612269cc15980` |
| `test-vectors/blocks/mainnet/8d378effcbbe98c9142371b934c32a3946628a2dfb05eee4472a40ec9d1ca862.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/8d378effcbbe98c9142371b934c32a3946628a2dfb05eee4472a40ec9d1ca862.cbor` / `08361ab2aacf4425ee8f8acc783cd5306c5e5b81adeb3c9cde426304f67b3cac` | `0001/artifacts/test-vectors/blocks/mainnet/8d378effcbbe98c9142371b934c32a3946628a2dfb05eee4472a40ec9d1ca862.cbor` / `08361ab2aacf4425ee8f8acc783cd5306c5e5b81adeb3c9cde426304f67b3cac` |
| `test-vectors/blocks/mainnet/8d4eb9c1e090f3ed4f23ff3690a4ace2fb474cbfec15c420c88d9c3fe8fe3823.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/8d4eb9c1e090f3ed4f23ff3690a4ace2fb474cbfec15c420c88d9c3fe8fe3823.cbor` / `b2e87edcb2748841e2a6d8056ab2434a46cf41f3b3578cf21087196006464d54` | `0001/artifacts/test-vectors/blocks/mainnet/8d4eb9c1e090f3ed4f23ff3690a4ace2fb474cbfec15c420c88d9c3fe8fe3823.cbor` / `b2e87edcb2748841e2a6d8056ab2434a46cf41f3b3578cf21087196006464d54` |
| `test-vectors/blocks/mainnet/9cefb8bced44c596f5e635ea0a589b89164eaee6a1928cf2ea8e0c13ef4cf700.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/9cefb8bced44c596f5e635ea0a589b89164eaee6a1928cf2ea8e0c13ef4cf700.cbor` / `a431d6728adbce53b7c3df0f09fb201b0e4ab8ecf49c26aa48508635d32f3c12` | `0001/artifacts/test-vectors/blocks/mainnet/9cefb8bced44c596f5e635ea0a589b89164eaee6a1928cf2ea8e0c13ef4cf700.cbor` / `a431d6728adbce53b7c3df0f09fb201b0e4ab8ecf49c26aa48508635d32f3c12` |
| `test-vectors/blocks/mainnet/9f2e53fb897d2546ecdfe8b70ce38a545cc64cabe996f4418135557c87d5f78e.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/9f2e53fb897d2546ecdfe8b70ce38a545cc64cabe996f4418135557c87d5f78e.cbor` / `b8d03de20940916060f78532b8cd49cf442517f4fe851521212ad5feaf891ffd` | `0001/artifacts/test-vectors/blocks/mainnet/9f2e53fb897d2546ecdfe8b70ce38a545cc64cabe996f4418135557c87d5f78e.cbor` / `b8d03de20940916060f78532b8cd49cf442517f4fe851521212ad5feaf891ffd` |
| `test-vectors/blocks/mainnet/9f63162b78765332c1632f46960fb386da7a2916090e2ef155ab97941ee1bbb9.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/9f63162b78765332c1632f46960fb386da7a2916090e2ef155ab97941ee1bbb9.cbor` / `618acf25fcfa8c0e83b641e5d5704c57c9b7f2dcf13d947d600232c310deabc9` | `0001/artifacts/test-vectors/blocks/mainnet/9f63162b78765332c1632f46960fb386da7a2916090e2ef155ab97941ee1bbb9.cbor` / `618acf25fcfa8c0e83b641e5d5704c57c9b7f2dcf13d947d600232c310deabc9` |
| `test-vectors/blocks/mainnet/a84fcbe2447bfc2ed53ba10abee56e81e54f6de72d053faf87bd701a9440b5fb.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/a84fcbe2447bfc2ed53ba10abee56e81e54f6de72d053faf87bd701a9440b5fb.cbor` / `303a3ac27099a4b981be8121f114b9e344448be2b2217491aaf79f589ea92d71` | `0001/artifacts/test-vectors/blocks/mainnet/a84fcbe2447bfc2ed53ba10abee56e81e54f6de72d053faf87bd701a9440b5fb.cbor` / `303a3ac27099a4b981be8121f114b9e344448be2b2217491aaf79f589ea92d71` |
| `test-vectors/blocks/mainnet/a91e6a056eb03adf3ae5a03787caa00892e617b970cd2422d75773dd4ab6e04e.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/a91e6a056eb03adf3ae5a03787caa00892e617b970cd2422d75773dd4ab6e04e.cbor` / `d15a9a3d32ba41c4164fc7e19f6ed28df7141f61c6d0fba230694aa3f6fcfcc2` | `0001/artifacts/test-vectors/blocks/mainnet/a91e6a056eb03adf3ae5a03787caa00892e617b970cd2422d75773dd4ab6e04e.cbor` / `d15a9a3d32ba41c4164fc7e19f6ed28df7141f61c6d0fba230694aa3f6fcfcc2` |
| `test-vectors/blocks/mainnet/bcb8b595c14d85fa278f2d68ddaa1cce758b940a4b9fe76453d6879715b50d90.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/bcb8b595c14d85fa278f2d68ddaa1cce758b940a4b9fe76453d6879715b50d90.cbor` / `611b6e9fafccc4f24a2d7f150e2676f555c6c38e7aefeccd974d2642c7af66f2` | `0001/artifacts/test-vectors/blocks/mainnet/bcb8b595c14d85fa278f2d68ddaa1cce758b940a4b9fe76453d6879715b50d90.cbor` / `611b6e9fafccc4f24a2d7f150e2676f555c6c38e7aefeccd974d2642c7af66f2` |
| `test-vectors/blocks/mainnet/bfcfa5a3cc8980d14749084e1357f199f4ae68fecbd790b9870934ea5d4fdca6.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/bfcfa5a3cc8980d14749084e1357f199f4ae68fecbd790b9870934ea5d4fdca6.cbor` / `655b374dde6a4868439607d40d276c286e2ab6c708a7cddb0a80e2ab5c2a9727` | `0001/artifacts/test-vectors/blocks/mainnet/bfcfa5a3cc8980d14749084e1357f199f4ae68fecbd790b9870934ea5d4fdca6.cbor` / `655b374dde6a4868439607d40d276c286e2ab6c708a7cddb0a80e2ab5c2a9727` |
| `test-vectors/blocks/mainnet/c8950f90432bff030d84b0ffd3dab4cc023f03fcb723f0db3420992558b3d99d.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/c8950f90432bff030d84b0ffd3dab4cc023f03fcb723f0db3420992558b3d99d.cbor` / `c61507f09f655c42f1181c34425f2687580253efe1ef265a7559af98a438a841` | `0001/artifacts/test-vectors/blocks/mainnet/c8950f90432bff030d84b0ffd3dab4cc023f03fcb723f0db3420992558b3d99d.cbor` / `c61507f09f655c42f1181c34425f2687580253efe1ef265a7559af98a438a841` |
| `test-vectors/blocks/mainnet/d798a8d617b25fc6456ffe2d90895a2c15a7271b671dab2d18d46f3d0e4ef495.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/d798a8d617b25fc6456ffe2d90895a2c15a7271b671dab2d18d46f3d0e4ef495.cbor` / `93a71179259adb15c763b66a2e13337394076ddbc73f1256e03d0a0cc3b29d89` | `0001/artifacts/test-vectors/blocks/mainnet/d798a8d617b25fc6456ffe2d90895a2c15a7271b671dab2d18d46f3d0e4ef495.cbor` / `93a71179259adb15c763b66a2e13337394076ddbc73f1256e03d0a0cc3b29d89` |
| `test-vectors/blocks/mainnet/da8dd783e7383951dd563aa75c308d1a109ecb4787df7e1ec980710613fddfa8.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/da8dd783e7383951dd563aa75c308d1a109ecb4787df7e1ec980710613fddfa8.cbor` / `7717b2462e475a29462796113f5ebca226bc10115e0cc724c4d3885941fec7d3` | `0001/artifacts/test-vectors/blocks/mainnet/da8dd783e7383951dd563aa75c308d1a109ecb4787df7e1ec980710613fddfa8.cbor` / `7717b2462e475a29462796113f5ebca226bc10115e0cc724c4d3885941fec7d3` |
| `test-vectors/blocks/mainnet/fc74c07951c30750d586c4b95f63a7baa6ef51805955dcc7f5dadced530c40f5.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/fc74c07951c30750d586c4b95f63a7baa6ef51805955dcc7f5dadced530c40f5.cbor` / `01d8322c16426064317669d5cfc06d8ae9907ca98c95e13dd0b86407db2e0e48` | `0001/artifacts/test-vectors/blocks/mainnet/fc74c07951c30750d586c4b95f63a7baa6ef51805955dcc7f5dadced530c40f5.cbor` / `01d8322c16426064317669d5cfc06d8ae9907ca98c95e13dd0b86407db2e0e48` |
| `test-vectors/blocks/mainnet/ff02499ddeb542a740fbebd58730c8ebdd09cf7235471f06d45398c8a9c0c4b3.cbor` | unchanged | `0001/artifacts/test-vectors/blocks/mainnet/ff02499ddeb542a740fbebd58730c8ebdd09cf7235471f06d45398c8a9c0c4b3.cbor` / `b400658ec9bfb429391bca269ba4e536fe64d12456816b7f187961a2d4a59156` | `0001/artifacts/test-vectors/blocks/mainnet/ff02499ddeb542a740fbebd58730c8ebdd09cf7235471f06d45398c8a9c0c4b3.cbor` / `b400658ec9bfb429391bca269ba4e536fe64d12456816b7f187961a2d4a59156` |
| `test-vectors/blocks/pallas/allegra1.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/allegra1.block` / `ad58d2f26fc044ff8d05790449422a1394a885479fad84c027332b3319eb7455` | `0001/artifacts/test-vectors/blocks/pallas/allegra1.block` / `ad58d2f26fc044ff8d05790449422a1394a885479fad84c027332b3319eb7455` |
| `test-vectors/blocks/pallas/alonzo1.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo1.block` / `39d81d148438c1dd56848bbbd644e7224e55c31218e502e215637486e5ef4490` | `0001/artifacts/test-vectors/blocks/pallas/alonzo1.block` / `39d81d148438c1dd56848bbbd644e7224e55c31218e502e215637486e5ef4490` |
| `test-vectors/blocks/pallas/alonzo10.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo10.block` / `8aac2a8c70520564bece55dc0dc2f18707dd3054850a2bde7ca8ed61055b8b5c` | `0001/artifacts/test-vectors/blocks/pallas/alonzo10.block` / `8aac2a8c70520564bece55dc0dc2f18707dd3054850a2bde7ca8ed61055b8b5c` |
| `test-vectors/blocks/pallas/alonzo11.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo11.block` / `dc23fa436f216ad0bd3042941be3162a1e8da7922545e6458ceba23e7ca2b377` | `0001/artifacts/test-vectors/blocks/pallas/alonzo11.block` / `dc23fa436f216ad0bd3042941be3162a1e8da7922545e6458ceba23e7ca2b377` |
| `test-vectors/blocks/pallas/alonzo12.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo12.block` / `8817a54f214720d4a8d6d765701f2235ff7496f7e94cfabe7f5183d7e02a1a1f` | `0001/artifacts/test-vectors/blocks/pallas/alonzo12.block` / `8817a54f214720d4a8d6d765701f2235ff7496f7e94cfabe7f5183d7e02a1a1f` |
| `test-vectors/blocks/pallas/alonzo13.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo13.block` / `4138fc0e730335dc7df355dea95c77149ddba40e97bbfe2cf085c30421113a76` | `0001/artifacts/test-vectors/blocks/pallas/alonzo13.block` / `4138fc0e730335dc7df355dea95c77149ddba40e97bbfe2cf085c30421113a76` |
| `test-vectors/blocks/pallas/alonzo14.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo14.block` / `72374a2ff03c6a7b268bb510a6bbf3d5b5d7bc3df82a8cbc04b09d88e31c10d6` | `0001/artifacts/test-vectors/blocks/pallas/alonzo14.block` / `72374a2ff03c6a7b268bb510a6bbf3d5b5d7bc3df82a8cbc04b09d88e31c10d6` |
| `test-vectors/blocks/pallas/alonzo15.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo15.block` / `8cac9a7e52db9b205c265abb7f7a4487e75139aaafec44f3b648c32a83e93f30` | `0001/artifacts/test-vectors/blocks/pallas/alonzo15.block` / `8cac9a7e52db9b205c265abb7f7a4487e75139aaafec44f3b648c32a83e93f30` |
| `test-vectors/blocks/pallas/alonzo16.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo16.block` / `7da29f2579469d95e7c1906e345a1df205e79d45bb413024998d0801b06ee282` | `0001/artifacts/test-vectors/blocks/pallas/alonzo16.block` / `7da29f2579469d95e7c1906e345a1df205e79d45bb413024998d0801b06ee282` |
| `test-vectors/blocks/pallas/alonzo17.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo17.block` / `743d9eb770f0448a8b8dba8c0b2490c8bdffd9e82c7609667e0aff864c2899f4` | `0001/artifacts/test-vectors/blocks/pallas/alonzo17.block` / `743d9eb770f0448a8b8dba8c0b2490c8bdffd9e82c7609667e0aff864c2899f4` |
| `test-vectors/blocks/pallas/alonzo18.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo18.block` / `e65bfad1aec382c10feeb0a1f06207fc583db1ca8ab17e81c34b17eae2e9fd0f` | `0001/artifacts/test-vectors/blocks/pallas/alonzo18.block` / `e65bfad1aec382c10feeb0a1f06207fc583db1ca8ab17e81c34b17eae2e9fd0f` |
| `test-vectors/blocks/pallas/alonzo19.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo19.block` / `37455c9af3371ea0bd934bfe215dc2c20f831205ac6b09619582f02bbd5957d3` | `0001/artifacts/test-vectors/blocks/pallas/alonzo19.block` / `37455c9af3371ea0bd934bfe215dc2c20f831205ac6b09619582f02bbd5957d3` |
| `test-vectors/blocks/pallas/alonzo2.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo2.block` / `d0d4da10c2f20ddc7bbdb01910fdfeb03278b464e75a740347974dba5522d9a9` | `0001/artifacts/test-vectors/blocks/pallas/alonzo2.block` / `d0d4da10c2f20ddc7bbdb01910fdfeb03278b464e75a740347974dba5522d9a9` |
| `test-vectors/blocks/pallas/alonzo20.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo20.block` / `31c303deafeb92899f3452a22d9deeb236ac491bcc9d3035e70a138cfdc0c924` | `0001/artifacts/test-vectors/blocks/pallas/alonzo20.block` / `31c303deafeb92899f3452a22d9deeb236ac491bcc9d3035e70a138cfdc0c924` |
| `test-vectors/blocks/pallas/alonzo21.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo21.block` / `23a96ce7b3b333c7d14edb1eb290499469c39aa52ade1feeeac4554fed5cd8e6` | `0001/artifacts/test-vectors/blocks/pallas/alonzo21.block` / `23a96ce7b3b333c7d14edb1eb290499469c39aa52ade1feeeac4554fed5cd8e6` |
| `test-vectors/blocks/pallas/alonzo22.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo22.block` / `996bc37ac3c134470f555730c918736cf44b8d08393c766dcd3a22e13915cf29` | `0001/artifacts/test-vectors/blocks/pallas/alonzo22.block` / `996bc37ac3c134470f555730c918736cf44b8d08393c766dcd3a22e13915cf29` |
| `test-vectors/blocks/pallas/alonzo23.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo23.block` / `f30b2a3510f87cb32e91e3a5e8ca13e5a63ee929b120f7e63a4cd682d89ffa62` | `0001/artifacts/test-vectors/blocks/pallas/alonzo23.block` / `f30b2a3510f87cb32e91e3a5e8ca13e5a63ee929b120f7e63a4cd682d89ffa62` |
| `test-vectors/blocks/pallas/alonzo24.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo24.block` / `d397feeddab6beb7a6712622ff3c5c396d7ecd240c760e28ef7aa8a7cf0aeb66` | `0001/artifacts/test-vectors/blocks/pallas/alonzo24.block` / `d397feeddab6beb7a6712622ff3c5c396d7ecd240c760e28ef7aa8a7cf0aeb66` |
| `test-vectors/blocks/pallas/alonzo27.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo27.block` / `14e613b1e8511204ec6d9de8181a7801ec025d39b53ced91945e2ef1887ca122` | `0001/artifacts/test-vectors/blocks/pallas/alonzo27.block` / `14e613b1e8511204ec6d9de8181a7801ec025d39b53ced91945e2ef1887ca122` |
| `test-vectors/blocks/pallas/alonzo3.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo3.block` / `c17a869a79231b943ee62eaea7a0ad187130aff132716637f16b14c3e9d26b15` | `0001/artifacts/test-vectors/blocks/pallas/alonzo3.block` / `c17a869a79231b943ee62eaea7a0ad187130aff132716637f16b14c3e9d26b15` |
| `test-vectors/blocks/pallas/alonzo4.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo4.block` / `da63de894f00e58c99122690830e3cf2f5c26676408c3f541cca27fdad5eb7c2` | `0001/artifacts/test-vectors/blocks/pallas/alonzo4.block` / `da63de894f00e58c99122690830e3cf2f5c26676408c3f541cca27fdad5eb7c2` |
| `test-vectors/blocks/pallas/alonzo5.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo5.block` / `3e0eb798b48663343b9d2f30369fe7c9a0eea6ab732ff1149ebe2c0d773820a2` | `0001/artifacts/test-vectors/blocks/pallas/alonzo5.block` / `3e0eb798b48663343b9d2f30369fe7c9a0eea6ab732ff1149ebe2c0d773820a2` |
| `test-vectors/blocks/pallas/alonzo6.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo6.block` / `e0a24f6d1b948e12e50176bc30f75fe0227b4ea62c8740dd8b8a35cf243c228b` | `0001/artifacts/test-vectors/blocks/pallas/alonzo6.block` / `e0a24f6d1b948e12e50176bc30f75fe0227b4ea62c8740dd8b8a35cf243c228b` |
| `test-vectors/blocks/pallas/alonzo7.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo7.block` / `433a1271806920b0e98a953089d2be54e533ae1a3f14cf071d98b80227e21ed6` | `0001/artifacts/test-vectors/blocks/pallas/alonzo7.block` / `433a1271806920b0e98a953089d2be54e533ae1a3f14cf071d98b80227e21ed6` |
| `test-vectors/blocks/pallas/alonzo8.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo8.block` / `894350ec6feeaabb65abbf48d413aa7d8126fb4bed86cc4ee309e9c62d8cf6ea` | `0001/artifacts/test-vectors/blocks/pallas/alonzo8.block` / `894350ec6feeaabb65abbf48d413aa7d8126fb4bed86cc4ee309e9c62d8cf6ea` |
| `test-vectors/blocks/pallas/alonzo9.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/alonzo9.block` / `57749c3664f893885c5cb0fec3b7a8504342f3307e8688542f6ef408358d9ace` | `0001/artifacts/test-vectors/blocks/pallas/alonzo9.block` / `57749c3664f893885c5cb0fec3b7a8504342f3307e8688542f6ef408358d9ace` |
| `test-vectors/blocks/pallas/babbage1.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/babbage1.block` / `415aa9f34214cb04926745a3c18600dfd04fd61a6ce4e7fa988a89235da7f8a1` | `0001/artifacts/test-vectors/blocks/pallas/babbage1.block` / `415aa9f34214cb04926745a3c18600dfd04fd61a6ce4e7fa988a89235da7f8a1` |
| `test-vectors/blocks/pallas/babbage10.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/babbage10.block` / `10c9afeef22c180dda998527415da360cc83b36b40bb5610d076998830758713` | `0001/artifacts/test-vectors/blocks/pallas/babbage10.block` / `10c9afeef22c180dda998527415da360cc83b36b40bb5610d076998830758713` |
| `test-vectors/blocks/pallas/babbage2.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/babbage2.block` / `916ccc461c16203f9558929d796081a93d3cbf8161df53f2b63c75058c6e04a6` | `0001/artifacts/test-vectors/blocks/pallas/babbage2.block` / `916ccc461c16203f9558929d796081a93d3cbf8161df53f2b63c75058c6e04a6` |
| `test-vectors/blocks/pallas/babbage3.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/babbage3.block` / `7a6d4dbfa7b987ce10d6f26d501d349ff59db8503f17428e16f6753bccb63be2` | `0001/artifacts/test-vectors/blocks/pallas/babbage3.block` / `7a6d4dbfa7b987ce10d6f26d501d349ff59db8503f17428e16f6753bccb63be2` |
| `test-vectors/blocks/pallas/babbage4.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/babbage4.block` / `e8d716175c4e23e7232ccbfc6eef8477181381d08c2f48841a61532207d44355` | `0001/artifacts/test-vectors/blocks/pallas/babbage4.block` / `e8d716175c4e23e7232ccbfc6eef8477181381d08c2f48841a61532207d44355` |
| `test-vectors/blocks/pallas/babbage5.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/babbage5.block` / `b3b36cb83d637a23c411791370ebde726954e4cc2bb69d150b94dde3b408b8f9` | `0001/artifacts/test-vectors/blocks/pallas/babbage5.block` / `b3b36cb83d637a23c411791370ebde726954e4cc2bb69d150b94dde3b408b8f9` |
| `test-vectors/blocks/pallas/babbage6.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/babbage6.block` / `08f55d5febbb48106cbcd62a2c0d2b370ceb7485650c54391e093f3c75eb275c` | `0001/artifacts/test-vectors/blocks/pallas/babbage6.block` / `08f55d5febbb48106cbcd62a2c0d2b370ceb7485650c54391e093f3c75eb275c` |
| `test-vectors/blocks/pallas/babbage7.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/babbage7.block` / `d8ba5f4ae18bd982e2be7cee700086763b56de13de4e0c5edd5ad277a77049d3` | `0001/artifacts/test-vectors/blocks/pallas/babbage7.block` / `d8ba5f4ae18bd982e2be7cee700086763b56de13de4e0c5edd5ad277a77049d3` |
| `test-vectors/blocks/pallas/babbage8.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/babbage8.block` / `c4a9596ec569d416dd1d564427de7e3ff18c69df9b0c2761431362eb56b2423d` | `0001/artifacts/test-vectors/blocks/pallas/babbage8.block` / `c4a9596ec569d416dd1d564427de7e3ff18c69df9b0c2761431362eb56b2423d` |
| `test-vectors/blocks/pallas/babbage9.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/babbage9.block` / `d3f786eaa7ca5e6d94880c9bd80c2112b8d53649de3cccd618761cb882b182fe` | `0001/artifacts/test-vectors/blocks/pallas/babbage9.block` / `d3f786eaa7ca5e6d94880c9bd80c2112b8d53649de3cccd618761cb882b182fe` |
| `test-vectors/blocks/pallas/byron1.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/byron1.block` / `77a34d9e007e64ac2cb024a3d47615e7a6474ba1ae114ae2a277dc2a8211a14b` | `0001/artifacts/test-vectors/blocks/pallas/byron1.block` / `77a34d9e007e64ac2cb024a3d47615e7a6474ba1ae114ae2a277dc2a8211a14b` |
| `test-vectors/blocks/pallas/byron2.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/byron2.block` / `be8d3ca8245dacd7544c0748b425f9869117263a245374ac6acc41d11ee99547` | `0001/artifacts/test-vectors/blocks/pallas/byron2.block` / `be8d3ca8245dacd7544c0748b425f9869117263a245374ac6acc41d11ee99547` |
| `test-vectors/blocks/pallas/byron3.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/byron3.block` / `2a03348561dd73fe8903d341f068161beaa49e36bac52287e00d39de2e689b4a` | `0001/artifacts/test-vectors/blocks/pallas/byron3.block` / `2a03348561dd73fe8903d341f068161beaa49e36bac52287e00d39de2e689b4a` |
| `test-vectors/blocks/pallas/byron4.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/byron4.block` / `9997e1d78c0286764abf8f499bd5bc816f7054fc2bab152f43878b0c50f3e6fc` | `0001/artifacts/test-vectors/blocks/pallas/byron4.block` / `9997e1d78c0286764abf8f499bd5bc816f7054fc2bab152f43878b0c50f3e6fc` |
| `test-vectors/blocks/pallas/byron5.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/byron5.block` / `36fb6290f2fa77880f8d47b0fe14d260b26b955853becf51fa1744ea76c32b86` | `0001/artifacts/test-vectors/blocks/pallas/byron5.block` / `36fb6290f2fa77880f8d47b0fe14d260b26b955853becf51fa1744ea76c32b86` |
| `test-vectors/blocks/pallas/byron6.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/byron6.block` / `4bdb8afda26b442e1798d7b4fc32ce2fe8451a5b5e2b261e88b8008e8235c374` | `0001/artifacts/test-vectors/blocks/pallas/byron6.block` / `4bdb8afda26b442e1798d7b4fc32ce2fe8451a5b5e2b261e88b8008e8235c374` |
| `test-vectors/blocks/pallas/byron7.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/byron7.block` / `ac161f1440a874d966969875cea393eed8085f0b5a2c1a7acc438636a1bb4601` | `0001/artifacts/test-vectors/blocks/pallas/byron7.block` / `ac161f1440a874d966969875cea393eed8085f0b5a2c1a7acc438636a1bb4601` |
| `test-vectors/blocks/pallas/byron8.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/byron8.block` / `c3748dccbcd694eec80d301d4a17233a407eb688313e4d3ef6b95427ef08cd09` | `0001/artifacts/test-vectors/blocks/pallas/byron8.block` / `c3748dccbcd694eec80d301d4a17233a407eb688313e4d3ef6b95427ef08cd09` |
| `test-vectors/blocks/pallas/conway1.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/conway1.block` / `29405309714608b328071a26f1fe0f1b778eb7d0e3ae568627749ba68e4ca8c8` | `0001/artifacts/test-vectors/blocks/pallas/conway1.block` / `29405309714608b328071a26f1fe0f1b778eb7d0e3ae568627749ba68e4ca8c8` |
| `test-vectors/blocks/pallas/conway2.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/conway2.block` / `0425168fd5aefdeadc9fc2dca60aa50452cdd142ff7539489788a4cf16cc66ca` | `0001/artifacts/test-vectors/blocks/pallas/conway2.block` / `0425168fd5aefdeadc9fc2dca60aa50452cdd142ff7539489788a4cf16cc66ca` |
| `test-vectors/blocks/pallas/conway3.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/conway3.block` / `c043303fdb58a6decee03a108e45828eb8201e685ae26a2695b0cf21b32d9096` | `0001/artifacts/test-vectors/blocks/pallas/conway3.block` / `c043303fdb58a6decee03a108e45828eb8201e685ae26a2695b0cf21b32d9096` |
| `test-vectors/blocks/pallas/conway4.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/conway4.block` / `b0106388ff177993e865e33cb8bacb8b22e8109150da25feb25f024d36f24df3` | `0001/artifacts/test-vectors/blocks/pallas/conway4.block` / `b0106388ff177993e865e33cb8bacb8b22e8109150da25feb25f024d36f24df3` |
| `test-vectors/blocks/pallas/conway8.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/conway8.block` / `b846ab008159f0df2b9df903deb5dc5a5f6be8b49bc2a41467b9bf6fff048937` | `0001/artifacts/test-vectors/blocks/pallas/conway8.block` / `b846ab008159f0df2b9df903deb5dc5a5f6be8b49bc2a41467b9bf6fff048937` |
| `test-vectors/blocks/pallas/genesis.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/genesis.block` / `18976e16f654435df817a4918a3880e33b091a8e312ec4f6fb67fce162c114ce` | `0001/artifacts/test-vectors/blocks/pallas/genesis.block` / `18976e16f654435df817a4918a3880e33b091a8e312ec4f6fb67fce162c114ce` |
| `test-vectors/blocks/pallas/mary1.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/mary1.block` / `64ca50616aa1b93d4254c41f5aa9fec89a7b436472c5df2402b7c3eab954a462` | `0001/artifacts/test-vectors/blocks/pallas/mary1.block` / `64ca50616aa1b93d4254c41f5aa9fec89a7b436472c5df2402b7c3eab954a462` |
| `test-vectors/blocks/pallas/shelley1.block` | unchanged | `0001/artifacts/test-vectors/blocks/pallas/shelley1.block` / `87bb16b8624f3ae0212b57231de58c6576b7b30f29eb482c540e1da78b0f4f21` | `0001/artifacts/test-vectors/blocks/pallas/shelley1.block` / `87bb16b8624f3ae0212b57231de58c6576b7b30f29eb482c540e1da78b0f4f21` |
| `test-vectors/genesis/byron/5f20df933584822601f9e3f8c024eb5eb252fe8cefb24d1317dc3d432e940ebb.json` | unchanged | `0001/artifacts/test-vectors/genesis/byron/5f20df933584822601f9e3f8c024eb5eb252fe8cefb24d1317dc3d432e940ebb.json` / `40d865ea0af837ded73c1d4288faaa4647817296b1eb59c7823e4ebc95824ea8` | `0001/artifacts/test-vectors/genesis/byron/5f20df933584822601f9e3f8c024eb5eb252fe8cefb24d1317dc3d432e940ebb.json` / `40d865ea0af837ded73c1d4288faaa4647817296b1eb59c7823e4ebc95824ea8` |
| `test-vectors/genesis/byron/96fceff972c2c06bd3bb5243c39215333be6d56aaf4823073dca31afe5038471.json` | unchanged | `0001/artifacts/test-vectors/genesis/byron/96fceff972c2c06bd3bb5243c39215333be6d56aaf4823073dca31afe5038471.json` / `dca3139e907bb943bfeff5f186f3fc71217ff12cabde93a926277c6ddd647d87` | `0001/artifacts/test-vectors/genesis/byron/96fceff972c2c06bd3bb5243c39215333be6d56aaf4823073dca31afe5038471.json` / `dca3139e907bb943bfeff5f186f3fc71217ff12cabde93a926277c6ddd647d87` |
| `test-vectors/genesis/byron/b7f76950bc4866423538ab7764fc1c7020b24a5f717a5bee3109ff2796567214.json` | unchanged | `0001/artifacts/test-vectors/genesis/byron/b7f76950bc4866423538ab7764fc1c7020b24a5f717a5bee3109ff2796567214.json` / `6b62660df9f2cf27192cc071616ffb0617018c5a42c753200e6f6bac9471793e` | `0001/artifacts/test-vectors/genesis/byron/b7f76950bc4866423538ab7764fc1c7020b24a5f717a5bee3109ff2796567214.json` / `6b62660df9f2cf27192cc071616ffb0617018c5a42c753200e6f6bac9471793e` |
| `test-vectors/genesis/byron/c6a004d3d178f600cd8caa10abbebe1549bef878f0665aea2903472d5abf7323.json` | unchanged | `0001/artifacts/test-vectors/genesis/byron/c6a004d3d178f600cd8caa10abbebe1549bef878f0665aea2903472d5abf7323.json` / `f2ef79f67f39cc9eec0bff3549b1fdb31d3e454281ba9d9a84807908f5b52ad7` | `0001/artifacts/test-vectors/genesis/byron/c6a004d3d178f600cd8caa10abbebe1549bef878f0665aea2903472d5abf7323.json` / `f2ef79f67f39cc9eec0bff3549b1fdb31d3e454281ba9d9a84807908f5b52ad7` |
| `test-vectors/genesis/shelley/test-yaci.json` | unchanged | `0001/artifacts/test-vectors/genesis/shelley/test-yaci.json` / `0d7293404873954247b1b938d9c3636781e983a23ea10656ad96f1d4cd225e63` | `0001/artifacts/test-vectors/genesis/shelley/test-yaci.json` / `0d7293404873954247b1b938d9c3636781e983a23ea10656ad96f1d4cd225e63` |
| `test-vectors/genesis/shelley/test.json` | unchanged | `0001/artifacts/test-vectors/genesis/shelley/test.json` / `555530d611a5884c234be72915a6f11839d89378fe12d13ace328681fbd448c7` | `0001/artifacts/test-vectors/genesis/shelley/test.json` / `555530d611a5884c234be72915a6f11839d89378fe12d13ace328681fbd448c7` |
| `test-vectors/manifest.json` | changed | `0001/artifacts/test-vectors/manifest.json` / `83f12c4af07b1d2217725a5a0475f96b5a1abd8cd0efce9eb8300e3977bc32f1` | `0002/artifacts/test-vectors/manifest.json` / `6130eea16c6196b07b20694e67e4aac68c26a06df85267b5273abb092f167962` |
| `upstream/chain/rust/src/builders/redeemer_builder.rs` | added | NONE | `0002/artifacts/upstream/chain/rust/src/builders/redeemer_builder.rs` / `dbb39470f28363faba28186bacf6a93ccbf979d421f25bf1fa145822192235b4` |
| `upstream/chain/rust/src/builders/tx_builder.rs` | added | NONE | `0002/artifacts/upstream/chain/rust/src/builders/tx_builder.rs` / `ba8d067641cc3aae7bea4275192ce63ec985c968509f8118358a8d934a9efb12` |
| `upstream/chain/rust/src/builders/witness_builder.rs` | added | NONE | `0002/artifacts/upstream/chain/rust/src/builders/witness_builder.rs` / `e375c9230d1cd1057b6cb9fd051b3fe6e37fc4c1d61a88f3917c3fea3b51a89d` |
| `upstream/cip36/rust/src/generated/cbor_encodings.rs` | added | NONE | `0002/artifacts/upstream/cip36/rust/src/generated/cbor_encodings.rs` / `f18f3fc4f827dc1cdb9efa9071046ff9a0a6805edfa740806ed21a9a77fe9550` |
| `upstream/cip36/rust/src/generated/mod.rs` | added | NONE | `0002/artifacts/upstream/cip36/rust/src/generated/mod.rs` / `096e4af72ea75f8b5e56a4875958454302b995076210730bd41946691a6e7d2c` |
| `upstream/cip36/rust/src/generated/serialization.rs` | added | NONE | `0002/artifacts/upstream/cip36/rust/src/generated/serialization.rs` / `5dfa96d6294417b025a15d529744daf93dd9c3ef8ca2fca4265ad4978ceaf859` |
| `upstream/cip36/rust/src/utils.rs` | added | NONE | `0002/artifacts/upstream/cip36/rust/src/utils.rs` / `4c0edb2a640688a6e91faee08a0cfdf33d0f53cee8e6472fb53e1da88d71d89e` |
| `upstream/cip36/rust/tests/cbor_roundtrip.rs` | added | NONE | `0002/artifacts/upstream/cip36/rust/tests/cbor_roundtrip.rs` / `660761a34cfae7c81aeee1f87afe2eb2e85323e3fa150c9ea3def7efda582159` |
| `upstream/cip36/rust/tests/invariants.rs` | added | NONE | `0002/artifacts/upstream/cip36/rust/tests/invariants.rs` / `3b1eb13bf953292b7431a86666a7205fba664680ab919c4749fab31b9fad6f0f` |
| `upstream/cip36/rust/tests/json.rs` | added | NONE | `0002/artifacts/upstream/cip36/rust/tests/json.rs` / `735cf94b775bed1bccf398a39e07e1bfc80c5e680cafb573b65e71d4d10cc740` |
| `upstream/cip36/rust/tests/unknown_keys.rs` | added | NONE | `0002/artifacts/upstream/cip36/rust/tests/unknown_keys.rs` / `501d14e4280e7706ddb4f936247bf4f63254abb2693dd5d72a07cdd94560eda0` |
| `upstream/core/rust/src/ordered_set.rs` | added | NONE | `0002/artifacts/upstream/core/rust/src/ordered_set.rs` / `c964ddcc2204800dbc0d04dc86e51208a34866f5dbe555f91561de9344615e1f` |

## Development reconciliation provenance

- Explicit human request: update installed SPECTRE and reconcile the existing 0002 captures with numbered incremental updates.
- Reconciled at: 20260909T165928Z.
- Previous descriptor path: `0002-cardano-multiplatform-lib/SNAPSHOT.md`.
- Previous descriptor SHA-256: `ebf733742229e68a1ccdfa42e7130d4fdf65f25a3e8652d96f4e8964927623f4`.
- Previous complete effective-inventory SHA-256: `37a5f67d9378982ed3d9a1379c1109d2dd91b9b4ce114bd013d5a9ec109f1fab` (canonical compact JSON mapping logical path to bytes/sha256).
- Retained original Created value and upstream identities. Renamed this untracked capture to 0002,
  separated summary from specification, replaced duplicate files with references, and rebased
  generated controls. This is an explicitly requested development reconciliation, not a new
  capture, implicit exception for future edits, or claim that upstream validation was rerun.
- Rebased controls: `SHA256SUMS`, `test-vectors/PROVENANCE.json`, `test-vectors/README.md`, `test-vectors/manifest.json`.
- All original upstream source bytes and corpus content are preserved. The 0001 snapshots and all
  implementation instructions, results, ledger states and archived decisions are unchanged.
- Compare the frozen source table, resolved inventory and checksums before consumption. Use
  [CAPTURE.md](CAPTURE.md) for advisory findings, not verification rules.
