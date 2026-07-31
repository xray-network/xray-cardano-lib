import assert from "node:assert/strict";
import test from "node:test";
import {
  Address,
  AddressContent,
  AddressHeaderKind,
  AlonzoFormatTxOut,
  AssetName,
  BaseAddress,
  ByronAddress,
  CardanoNodePlutusDatumSchema,
  Credential,
  Crc32,
  Ed25519KeyHash,
  Ed25519KeyHashList,
  EnterpriseAddress,
  ExUnitPrices,
  Ipv4,
  Ipv6,
  LinearFee,
  MapAssetNameToCoin,
  MetadataJsonSchema,
  NativeScript,
  NativeScriptList,
  NetworkInfo,
  MultiAsset,
  PlutusData,
  Pointer,
  PointerAddress,
  PrivateKey,
  PublicKey,
  Rational,
  RewardAddress,
  ScriptHash,
  TransactionMetadatum,
  Transaction,
  TransactionBody,
  TransactionHash,
  TransactionOutput,
  Value,
  compute_total_ex_units,
  decode_arbitrary_bytes_from_metadatum,
  decode_metadatum_to_json_str,
  decode_plutus_datum_to_json_str,
  encode_arbitrary_bytes_as_metadatum,
  encode_json_str_to_metadatum,
  encode_json_str_to_plutus_datum,
  genesis_txid_byron,
  genesis_txid_shelley,
  get_deposit,
  get_implicit_input,
  hash_plutus_data,
  make_icarus_bootstrap_witness,
  make_vkey_witness,
  min_ada_required,
  min_fee,
  min_no_script_fee,
  min_script_fee,
} from "../../runtime/dist/esm/index.js";
import { decodeCbor, encodeCbor } from "../../core/dist/esm/index.js";
import {
  crc32,
  decodeBase58,
  encodeBase58,
} from "../dist/esm/era/byron/encoding.js";
import { min_ref_script_fee } from "../dist/esm/ledger/operations.js";
import { Bip32PrivateKey, Bip32PublicKey } from "../../crypto/dist/esm/index.js";

const bytes = (hex) => Uint8Array.from(Buffer.from(hex, "hex"));
const harden = (index) => (index | 0x8000_0000) >>> 0;

test("variable_nat_encoding and Shelley address construction match known vectors", () => {
  const entropy = bytes("df9ed25ed146bf43336a5d7cf7395994");
  const root = Bip32PrivateKey.from_bip39_entropy(entropy, new Uint8Array());
  const spend = root.derive(harden(1852)).derive(harden(1815)).derive(harden(0)).derive(0).derive(0).to_public();
  const stake = root.derive(harden(1852)).derive(harden(1815)).derive(harden(0)).derive(2).derive(0).to_public();
  const payment = Credential.new_pub_key(spend.to_raw_key().hash());
  const staking = Credential.new_pub_key(stake.to_raw_key().hash());
  assert.equal(
    BaseAddress.new(0, payment, staking).to_address().to_bech32(),
    "addr_test1qz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3jcu5d8ps7zex2k2xt3uqxgjqnnj83ws8lhrn648jjxtwq2ytjqp",
  );
  assert.equal(
    EnterpriseAddress.new(1, payment).to_address().to_bech32(),
    "addr1vx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzers66hrl8",
  );
  assert.equal(
    PointerAddress.new(1, payment, Pointer.new(24157n, 177n, 42n)).to_address().to_bech32(),
    "addr1gx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer5ph3wczvf2w8lunk",
  );
  assert.equal(
    RewardAddress.new(0, staking).to_address().to_bech32(),
    "stake_test1uqevw2xnsc0pvn9t9r9c7qryfqfeerchgrlm3ea2nefr9hqp8n5xl",
  );
});

test("bip32_15, bip32_24, Icarus, and multisig address vectors match", () => {
  const deriveCredentials = (entropyHex, purpose = 1852) => {
    const root = Bip32PrivateKey.from_bip39_entropy(bytes(entropyHex), new Uint8Array());
    const account = root.derive(harden(purpose)).derive(harden(1815)).derive(harden(0));
    return {
      root,
      payment: Credential.new_pub_key(account.derive(0).derive(0).to_public().to_raw_key().hash()),
      stake: Credential.new_pub_key(account.derive(2).derive(0).to_public().to_raw_key().hash()),
    };
  };
  const fifteen = deriveCredentials("0ccb74f36b7da1649a8144675522d4d8097c6412");
  assert.equal(BaseAddress.new(0, fifteen.payment, fifteen.stake).to_address().to_bech32(), "addr_test1qpu5vlrf4xkxv2qpwngf6cjhtw542ayty80v8dyr49rf5ewvxwdrt70qlcpeeagscasafhffqsxy36t90ldv06wqrk2qum8x5w");
  assert.equal(EnterpriseAddress.new(1, fifteen.payment).to_address().to_bech32(), "addr1v9u5vlrf4xkxv2qpwngf6cjhtw542ayty80v8dyr49rf5eg0kvk0f");
  assert.equal(PointerAddress.new(1, fifteen.payment, Pointer.new(24157n, 177n, 42n)).to_address().to_bech32(), "addr1g9u5vlrf4xkxv2qpwngf6cjhtw542ayty80v8dyr49rf5evph3wczvf2kd5vam");
  const byronKey = fifteen.root.derive(harden(44)).derive(harden(1815)).derive(harden(0)).derive(0).derive(0).to_public();
  assert.equal(AddressContent.icarus_from_key(byronKey, NetworkInfo.mainnet().protocol_magic()).to_address().to_base58(), "Ae2tdPwUPEZHtBmjZBF4YpMkK9tMSPTE2ADEZTPN97saNkhG78TvXdp3GDk");

  const twentyFourEntropy = "4e828f9a67ddcff0e6391ad4f26ddb7579f59ba14b6dd4baf63dcfdb9d2420da";
  const twentyFour = deriveCredentials(twentyFourEntropy);
  assert.equal(BaseAddress.new(1, twentyFour.payment, twentyFour.stake).to_address().to_bech32(), "addr1qyy6nhfyks7wdu3dudslys37v252w2nwhv0fw2nfawemmn8k8ttq8f3gag0h89aepvx3xf69g0l9pf80tqv7cve0l33sdn8p3d");
  assert.equal(EnterpriseAddress.new(1, twentyFour.payment).to_address().to_bech32(), "addr1vyy6nhfyks7wdu3dudslys37v252w2nwhv0fw2nfawemmnqs6l44z");
  assert.equal(PointerAddress.new(1, twentyFour.payment, Pointer.new(24157n, 177n, 42n)).to_address().to_bech32(), "addr1gyy6nhfyks7wdu3dudslys37v252w2nwhv0fw2nfawemmnyph3wczvf2dqflgt");

  const multisig = deriveCredentials(twentyFourEntropy, 1854);
  assert.equal(BaseAddress.new(1, multisig.payment, multisig.stake).to_address().to_bech32(), "addr1qx8fg2e9yn0ga6sav0760cxmx0antql96mfuhqgzcc5swugw2jqqlugnx9qjep9xvcx40z0zfyep55r2t3lav5smyjrsxv9uuh");
  const spendingHash = twentyFour.payment.as_pub_key();
  const scripts = NativeScriptList.new(); scripts.add(NativeScript.new_script_pubkey(spendingHash));
  const scriptCredential = Credential.new_script(NativeScript.new_script_n_of_k(1n, scripts).hash());
  assert.equal(BaseAddress.new(1, scriptCredential, scriptCredential).to_address().to_bech32(), "addr1x80de0mz3m9xmgtlmqqzu06s0uvfsczskdec8k7v4jhr7077mjlk9rk2dkshlkqq9cl4qlccnps9pvmns0duet9w8ulsylzv28");
});

test("address_header_matching, bech32_parsing, pointer_address_big, and long_address", () => {
  const parsed = Address.from_bech32("addr1u8pcjgmx7962w6hey5hhsd502araxp26kdtgagakhaqtq8sxy9w7g");
  assert.equal(parsed.to_bech32("foobar"), "foobar1u8pcjgmx7962w6hey5hhsd502araxp26kdtgagakhaqtq8s92n4tm");
  const reward = RewardAddress.new(9, Credential.new_script(ScriptHash.from_hex("7f".repeat(28)))).to_address();
  assert.equal(reward.header(), 0xf9);
  assert.equal(Address.header_matches_kind(reward.header(), AddressHeaderKind.RewardScript), true);
  const pointer = PointerAddress.from_address(Address.from_bech32("addr_test1grqe6lg9ay8wkcu5k5e38lne63c80h3nq6xxhqfmhewf645pllllllllllll7lupllllllllllll7lupllllllllllll7lc9wayvj")).stake();
  assert.deepEqual([pointer.slot(), pointer.transaction_index(), pointer.certificate_index()], [0xffff_ffff_ffff_ffffn, 0xffff_ffff_ffff_ffffn, 0xffff_ffff_ffff_ffffn]);
  const long = "addr1q9d66zzs27kppmx8qc8h43q7m4hkxp5d39377lvxefvxd8j7eukjsdqc5c97t2zg5guqadepqqx6rc9m7wtnxy6tajjvk4a0kze4ljyuvvrpexg5up2sqxj33363v35gtew";
  assert.equal(Address.from_bech32(long).to_bech32(), long);
  assert.throws(() => Address.from_bech32("addr_test1vqt3w9chzut3w9chzut3w9chzut3w9chzut3w9chzut3w9cqqspqvqcqsmxqdssg97"));
  const huge = Pointer.new((1n << 1400n) - 1n, 0xffff_ffff_ffff_ffffn, 0xffff_ffff_ffff_ffffn);
  const hugeAddress = PointerAddress.new(0, Credential.new_pub_key(Ed25519KeyHash.from_hex("01".repeat(28))), huge).to_address();
  assert.equal(Address.from_raw_bytes(hugeAddress.to_raw_bytes()).to_hex(), hugeAddress.to_hex());
});

test("base58 vectors, crc32, Icarus, and Byron envelope round trips", () => {
  const vectors = [
    [new Uint8Array(4), "11111"],
    [new TextEncoder().encode("This is awesome!"), "BRY7dK2V98Sgi7CFWiZbap"],
    [new TextEncoder().encode("Hello World..."), "TcgsE5dzphUWfjcb9i5"],
    [bytes("00616263"), "1ZiCa"],
    [bytes("0000616263"), "11ZiCa"],
    [bytes("000000616263"), "111ZiCa"],
    [bytes("00000000616263"), "1111ZiCa"],
    [new TextEncoder().encode("abcdefghijklmnopqrstuvwxyz"), "3yxU3u1igY8WkgtjK92fbJQCd4BZiiT1v25f"],
  ];
  for (const [raw, encoded] of vectors) {
    assert.equal(encodeBase58(raw), encoded);
    assert.deepEqual(decodeBase58(encoded), raw);
  }
  assert.equal(crc32(new TextEncoder().encode("The quick brown fox jumps over the lazy dog")), 0x414fa339);
  const incremental = Crc32.new();
  incremental.update(new TextEncoder().encode("The quick brown "));
  incremental.update(new TextEncoder().encode("fox jumps over the lazy dog"));
  assert.equal(incremental.finalize(), 0x414fa339);
  const known = "Ae2tdPwUPEZHtBmjZBF4YpMkK9tMSPTE2ADEZTPN97saNkhG78TvXdp3GDk";
  assert.equal(ByronAddress.from_base58(known).to_base58(), known);
  assert.equal(Address.is_valid(known), true);
  assert.equal(ByronAddress.from_address(ByronAddress.from_base58(known).to_address()).to_base58(), known);
  const historical = [
    "DdzFFzCqrhsrcTVhLygT24QwTnNqQqQ8mZrq5jykUzMveU26sxaH529kMpo7VhPrt5pwW3dXeB2k3EEvKcNBRmzCfcQ7dTkyGzTs658C",
    "DdzFFzCqrht4it4GYgBp4J39FNnKBsPFejSppARXHCf2gGiTJcwXzpRvgDmxPvKQ8aZZmVqcLUz5L66a8Ja46pfKVtFRaKyn9eKdvpaC",
    "DdzFFzCqrhsvNQtyViTvEdGxfdc5T1E5RorzFWjYodqjhFDy8fQxfDPccmTc4ePbvkiwvRkR8dtqQ1SHpH53fDSoxD17fo9f6WkRjjAA",
    "DdzFFzCqrhsn7ZAhKy8mxkzW6G3wryM7K6bH38VAjE2FesJMxia3UviivMvGz146TP1FpDharxTE6nUgCCnZx2fmtKpmxAosg9Tf5b8y",
    "DdzFFzCqrhssTCJf4sv664bdQURovAwzx1hNKkMkNLwMNyaxZFuPSDdZTTRMcoDyXHuCiZhbD4umvMJcWGkvFMMzBoBUW5UBdBbDqXGX",
    "DdzFFzCqrhsfi5fFjJUHYPSnfTYrnMohzh3PrrtrVQgwua33HWPKUdTJXo3o77pSGCmDNrjYaAiZmJddaPW9iHyUDatvU2WhX7MgnNMy",
    "DdzFFzCqrhsy2zYMDQRCF4Nw34C3P7aT5B7JwHFQ6gLAeoHgVXurCLPCm3AeV1nTa1Nd46uDoNt16cnsPFkb4fpLi1J17AmvphCtGFz2",
    "DdzFFzCqrht8ygB5pLM4uVbS2x4ek2NTDx6R3DJqP7fUaWEkx8RA9UFR8CHitp2R74XLDP876Pe3KLUByHnrWrKWnffpqPpm14rPCxeP",
    "DdzFFzCqrhssTywqjv3dw3EakpEydWQcc3phQzR3YF9NPgQN9Ftkx68FfLLnpJ4vhWo9mAjx5EcpM1wNvorSySrpARZGfk5QugHkVs58",
    "DdzFFzCqrhsqTG4t3uq5UBqFrxhxGVM6bvF4q1QcZXqUpizFddEEip7dx5rbife2s9o2fRU3hVKhRp4higog7As8z42s4AMw6Pcu8vL4",
    "Ae2tdPwUPEZ4YjgvykNpoFeYUxoyhNj2kg8KfKWN2FizsSpLUPv68MpTVDo",
    "2cWKMJemoBaipzQe9BArYdo2iPUfJQdZAjm4iCzDA1AfNxJSTgm9FZQTmFCYhKkeYrede",
    "37btjrVyb4KEg6anTcJ9E4EAvYtNV9xXL6LNpA15YLhgvm9zJ1D2jwme574HikZ36rKdTwaUmpEicCoL1bDw4CtH5PNcFnTRGQNaFd5ai6Wvo6CZsi",
    "Ae2tdPwUPEZ3MHKkpT5Bpj549vrRH7nBqYjNXnCV8G2Bc2YxNcGHEa8ykDp",
  ];
  for (const value of historical) assert.equal(ByronAddress.from_base58(value).content().to_address().to_base58(), value);
  const firstPublicKey = Bip32PublicKey.from_raw_bytes(bytes("6a509689c653175865985ad1e0eb5ff9ada6997aa403e648614b3b78fcba9c27308228d9872af8b65b987ff23e1a20cd90d8346c31f0edb8998952dc67665580"));
  assert.equal(ByronAddress.from_base58(historical[0]).content().identical_with_pubkey(firstPublicKey), true);
  assert.equal(ByronAddress.from_base58(historical[10]).content().byron_protocol_magic().to_int(), 764824073);
  assert.equal(ByronAddress.from_base58(historical[11]).content().byron_protocol_magic().to_int(), 1097911063);
});

test("calc_redeem_txid and Shelley genesis transaction IDs match source vectors", () => {
  const publicKey = PublicKey.from_bytes(Uint8Array.from(Buffer.from("AAG3vJwTzCcL0zp2-1yfI-mn_7haYvSYJln2xR_aBS8=", "base64url")));
  const redeem = genesis_txid_byron(publicKey);
  assert.equal(redeem.txid().to_hex(), "927edb96f3386ab91b5f5d85d84cb4253c65b1c2f65fa7df25f81fab1d62987a");
  assert.equal(redeem.address().to_base58(), "Ae2tdPwUPEZ9vtyppa1FdJzvqJZkEcXgdHxVYAzTWcPaoNycVq5rc36LC1S");
  const shelley = Address.from_bech32("addr1u8pcjgmx7962w6hey5hhsd502araxp26kdtgagakhaqtq8sxy9w7g");
  assert.equal(genesis_txid_shelley(shelley).to_hex(), "30753180ac456e3e045a2c1f6a7bb367b9b0bbb02126754fb2455a953e3076a5");
});

test("Icarus bootstrap and vkey witnesses sign the exact transaction hash", () => {
  const root = Bip32PrivateKey.from_bip39_entropy(bytes("0ccb74f36b7da1649a8144675522d4d8097c6412"), new Uint8Array());
  const key = root.derive(harden(44)).derive(harden(1815)).derive(harden(0)).derive(0).derive(0);
  const address = AddressContent.icarus_from_key(key.to_public(), NetworkInfo.mainnet().protocol_magic()).to_address();
  const hash = TransactionHash.from_hex("42".repeat(32));
  const witness = make_icarus_bootstrap_witness(hash, address, key);
  assert.equal(witness.public_key().verify(hash.to_raw_bytes(), witness.signature()), true);
  assert.deepEqual(witness.chain_code(), key.chaincode());
  assert.equal(witness.to_address().to_address().to_base58(), address.to_base58());
  const privateKey = PrivateKey.from_normal_bytes(bytes("11".repeat(32)));
  const vkey = make_vkey_witness(hash, privateKey);
  const node = decodeCbor(vkey.to_cbor_bytes());
  assert.equal(node.kind, "array");
  assert.equal(PublicKey.from_bytes(node.values[0].value).verify(hash.to_raw_bytes(), { to_raw_bytes: () => node.values[1].value }), true);
});

test("ipv4_json and ipv6_json use node-compatible canonical text", () => {
  assert.equal(Ipv4.from_json('"255.255.255.255"').to_json(), '"255.255.255.255"');
  const cases = [
    ["2001:0db8:0000:0000:0000:ff00:0042:8329", "2001:db8::ff00:42:8329"],
    ["2001:0db8:0000:0000:1111:0000:0000:8329", "2001:db8::1111:0:0:8329"],
    ["0001:0000:0002:0000:0000:0000:0003:0000", "1:0:2::3:0"],
    ["000a:000b:0000:0000:0000:0000:0000:0000", "a:b::"],
    ["0000:0000:0000:0000:0000:0000:abcd:0000", "::abcd:0"],
    ["0000:000a:0000:000b:0000:000c:0000:000d", "0:a:0:b:0:c:0:d"],
  ];
  for (const [input, expected] of cases) assert.equal(Ipv6.from_str(input).to_js_value(), expected);
});

test("metadata JSON schemas and arbitrary-byte chunking round-trip", () => {
  const samples = [
    [MetadataJsonSchema.NoConversions, '{"name":"x","items":[1,2]}'],
    [MetadataJsonSchema.BasicConversions, '{"1":"0x0011","name":"x"}'],
    [MetadataJsonSchema.DetailedSchema, '{"map":[{"k":{"string":"x"},"v":{"int":42}}]}'],
  ];
  for (const [schema, json] of samples) {
    const native = encode_json_str_to_metadatum(json, schema);
    assert.deepEqual(JSON.parse(decode_metadatum_to_json_str(native, schema)), JSON.parse(json));
  }
  const raw = Uint8Array.from({ length: 150 }, (_, index) => index);
  assert.deepEqual(decode_arbitrary_bytes_from_metadatum(encode_arbitrary_bytes_as_metadatum(raw)), raw);
  assert.equal(decode_arbitrary_bytes_from_metadatum(TransactionMetadatum.new_text("x")), undefined);
});

test("Plutus JSON and hashes preserve source bytes instead of silently canonicalizing", () => {
  const detailed = '{"constructor":0,"fields":[{"int":42},{"bytes":"aabb"}]}';
  const native = encode_json_str_to_plutus_datum(detailed, CardanoNodePlutusDatumSchema.DetailedSchema);
  assert.deepEqual(JSON.parse(decode_plutus_datum_to_json_str(native, CardanoNodePlutusDatumSchema.DetailedSchema)), JSON.parse(detailed));
  const preserved = PlutusData.from_cbor_hex("1817");
  const canonical = PlutusData.from_cbor_hex("17");
  assert.notEqual(hash_plutus_data(preserved).to_hex(), hash_plutus_data(canonical).to_hex());
});

test("native script verification, signer discovery, and hash namespace", () => {
  const first = Ed25519KeyHash.from_hex("01".repeat(28));
  const second = Ed25519KeyHash.from_hex("02".repeat(28));
  const children = NativeScriptList.new();
  children.add(NativeScript.new_script_pubkey(first));
  children.add(NativeScript.new_script_invalid_before(10n));
  const script = NativeScript.new_script_all(children);
  const keys = Ed25519KeyHashList.new(); keys.add(first);
  assert.equal(script.verify(10n, undefined, keys), true);
  assert.equal(script.verify(9n, undefined, keys), false);
  assert.equal(script.get_required_signers().len(), 1);
  assert.equal(script.hash().to_raw_bytes().length, 28);
  assert.equal(NativeScript.new_script_pubkey(second).verify(undefined, undefined, keys), false);
});

test("network constants, linear fees, reference tiers, and minimum ADA are checked", () => {
  assert.deepEqual(
    [NetworkInfo.mainnet().network_id(), NetworkInfo.mainnet().protocol_magic().to_int()],
    [1, 764824073],
  );
  assert.deepEqual(
    [NetworkInfo.preview().network_id(), NetworkInfo.preview().protocol_magic().to_int()],
    [0, 2],
  );
  const fee = LinearFee.new(2n, 5n, 10n);
  assert.equal(min_no_script_fee({ to_cbor_bytes: () => new Uint8Array(100) }, fee), 205n);
  assert.equal(min_ref_script_fee(fee, 25_600n), 256_000n);
  assert.equal(min_ref_script_fee(fee, 25_601n), 256_012n);
  assert.equal(min_ada_required({ to_cbor_bytes: () => new Uint8Array(29) }, 4_310n, 0n), 831_830n);
  assert.throws(() => LinearFee.new(-1n, 0n, 0n));
});

test("all minimum-ADA source vectors match exact output sizes", () => {
  const entropy = bytes("0ccb74f36b7da1649a8144675522d4d8097c6412");
  const root = Bip32PrivateKey.from_bip39_entropy(entropy, new Uint8Array());
  const spend = root.derive(harden(1852)).derive(harden(1815)).derive(harden(0)).derive(0).derive(0).to_public();
  const stake = root.derive(harden(1852)).derive(harden(1815)).derive(harden(0)).derive(2).derive(0).to_public();
  const address = BaseAddress.new(0, Credential.new_pub_key(spend.to_raw_key().hash()), Credential.new_pub_key(stake.to_raw_key().hash())).to_address();
  const output = (value) => TransactionOutput.from_cbor_bytes(AlonzoFormatTxOut.new(address.to_raw_bytes(), value).to_cbor_bytes());
  const bundle = (specification) => {
    const multiAsset = MultiAsset.new();
    for (const [policyByte, assets] of specification) {
      const policy = ScriptHash.from_hex(policyByte.toString(16).padStart(2, "0").repeat(28));
      const values = MapAssetNameToCoin.new();
      for (const asset of assets) values.insert(AssetName.new(Uint8Array.from(asset)), 1n);
      multiAsset.insert_assets(policy, values);
    }
    return multiAsset;
  };
  const cases = [
    [Value.from_coin(0n), 969_750n],
    [Value.new(0n, bundle([[0, [[]]]])), 1_120_600n],
    [Value.new(1_407_406n, bundle([[0, [[1]]]])), 1_124_910n],
    [Value.new(1_555_554n, bundle([[0, [[1], [2], [3]]]])), 1_150_770n],
    [Value.new(1_592_591n, bundle([[0, [[]]], [1, [[]]]])), 1_262_830n],
    [Value.new(1_592_591n, bundle([[0, [[1]]], [1, [[1]]]])), 1_271_450n],
    [Value.new(7_592_585n, bundle([[1, Array.from({ length: 32 }, (_, index) => [32 + index])], [2, Array.from({ length: 32 }, (_, index) => [64 + index])], [3, Array.from({ length: 32 }, (_, index) => [96 + index])]])), 2_633_410n],
    [Value.new(1_555_554n, bundle([[0, [new Array(32).fill(1), new Array(32).fill(2), new Array(32).fill(3)]]])), 1_564_530n],
  ];
  for (const [value, expected] of cases) assert.equal(min_ada_required(output(value), 4_310n), expected);
});

test("script fees and total execution units support both redeemer wire formats", () => {
  const uint = (value) => ({ kind: "unsigned", value, encoding: { width: 0 } });
  const array = (values) => ({ kind: "array", values, encoding: { kind: "definite", width: 0 } });
  const legacy = array([
    array([uint(0n), uint(0n), uint(0n), array([uint(3n), uint(4n)])]),
    array([uint(1n), uint(0n), uint(0n), array([uint(5n), uint(6n)])]),
  ]);
  const redeemers = { to_cbor_bytes: () => encodeCbor(legacy) };
  assert.deepEqual(compute_total_ex_units(redeemers).to_js_value(), { mem: 8, steps: 10 });
  const witness = { kind: "map", entries: [[uint(5n), legacy]], encoding: { kind: "definite", width: 0 } };
  const txNode = array([{
    kind: "map",
    entries: [
      [uint(0n), array([])],
      [uint(1n), array([])],
      [uint(2n), uint(0n)],
    ],
    encoding: { kind: "definite", width: 0 },
  }, witness, { kind: "boolean", value: true }, { kind: "null" }]);
  const tx = Transaction.from_cbor_bytes(encodeCbor(txNode));
  const prices = ExUnitPrices.new(Rational.new(1n, 2n), Rational.new(1n, 4n));
  assert.equal(min_script_fee(tx, prices), 7n);
  const linear = LinearFee.new(2n, 5n, 10n);
  assert.equal(min_fee(tx, linear, prices, 0n), BigInt(tx.to_cbor_bytes().length) * 2n + 12n);
});

test("certificate deposits, refunds, proposals, and withdrawals are derived from body CBOR", () => {
  const uint = (value) => ({ kind: "unsigned", value, encoding: { width: 0 } });
  const array = (values) => ({ kind: "array", values, encoding: { kind: "definite", width: 0 } });
  const bytesNode = (value) => ({ kind: "bytes", value, encoding: { kind: "definite", width: 0 } });
  const tag = (number, value) => ({ kind: "tag", tag: number, value, encoding: { width: 0 } });
  const credential = array([uint(0n), bytesNode(new Uint8Array(28))]);
  const certificates = array([
    array([uint(0n), credential]),
    array([
      uint(3n),
      bytesNode(new Uint8Array(28)),
      bytesNode(new Uint8Array(32)),
      uint(0n),
      uint(0n),
      tag(30n, array([uint(0n), uint(1n)])),
      bytesNode(new Uint8Array()),
      array([]),
      array([]),
      { kind: "null" },
    ]),
    array([uint(7n), credential, uint(9n)]),
  ]);
  const proposals = array([array([
    uint(11n),
    bytesNode(new Uint8Array()),
    array([uint(6n)]),
    array([
      { kind: "text", value: "", encoding: { kind: "definite", width: 0 } },
      bytesNode(new Uint8Array(32)),
    ]),
  ])]);
  const mandatory = [
    [uint(0n), array([])],
    [uint(1n), array([])],
    [uint(2n), uint(0n)],
  ];
  const depositBody = TransactionBody.from_cbor_bytes(encodeCbor({ kind: "map", entries: [...mandatory, [uint(4n), certificates], [uint(20n), proposals]], encoding: { kind: "definite", width: 0 } }));
  assert.equal(get_deposit(depositBody, 100n, 20n), 140n);

  const refunds = array([
    array([uint(1n), credential]),
    array([uint(4n), bytesNode(new Uint8Array(28)), uint(0n)]),
    array([uint(8n), credential, uint(7n)]),
    array([uint(17n), credential, uint(8n)]),
    array([uint(15n), credential, { kind: "null" }]),
  ]);
  const withdrawals = { kind: "map", entries: [[bytesNode(Uint8Array.of(1)), uint(3n)], [bytesNode(Uint8Array.of(2)), uint(4n)]], encoding: { kind: "definite", width: 0 } };
  const refundBody = TransactionBody.from_cbor_bytes(encodeCbor({ kind: "map", entries: [...mandatory, [uint(4n), refunds], [uint(5n), withdrawals]], encoding: { kind: "definite", width: 0 } }));
  assert.equal(get_implicit_input(refundBody, 100n, 20n).coin(), 162n);
});
