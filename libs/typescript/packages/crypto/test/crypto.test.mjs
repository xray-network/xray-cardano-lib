import assert from "node:assert/strict";
import test from "node:test";
import {
  AnchorDocHash,
  AuxiliaryDataHash,
  Bip32PrivateKey,
  Bip32PublicKey,
  BlockBodyHash,
  BlockHeaderHash,
  DatumHash,
  Ed25519KeyHash,
  Ed25519Signature,
  emip3_decrypt_with_password,
  emip3_encrypt_with_password,
  GenesisDelegateHash,
  GenesisHash,
  KESVkey,
  NonceHash,
  PoolMetadataHash,
  PrivateKey,
  PublicKey,
  ScriptDataHash,
  ScriptHash,
  TransactionHash,
  VRFKeyHash,
  VRFVkey,
} from "../dist/esm/index.js";
import { AborDecoder, AborEncoder } from "../dist/esm/byron/abor.js";
import {
  signByronProxyCertificate,
  verifyByronProxyCertificate,
} from "../dist/esm/byron/proxy.js";
import {
  generateBip32PrivateKey,
  generateEd25519PrivateKey,
  generateExtendedPrivateKey,
} from "../dist/esm/keys/ed25519.js";
import {
  legacyPrivateKeyFromRawBytes,
  legacyPublicKey,
  legacySign,
} from "../dist/esm/byron/legacy.js";
import {
  blake2b160,
  blake2b224,
  blake2b256,
  sha3_256,
} from "../dist/esm/primitives/crypto.js";

const fromHex = (value) => Uint8Array.from(Buffer.from(value, "hex"));
const toHex = (value) => Buffer.from(value).toString("hex");
const message = new TextEncoder().encode("Cardano signing vector");

test("Blake2b and SHA3 match independent published vectors", () => {
  const abc = new TextEncoder().encode("abc");
  assert.equal(toHex(blake2b160(abc)), "384264f676f39536840523f284921cdc68b6846b");
  assert.equal(blake2b160(abc).length, 20);
  assert.notEqual(toHex(blake2b160(abc)), toHex(blake2b256(abc).slice(0, 20)));
  const digest = blake2b160(abc);
  digest.fill(0);
  assert.equal(toHex(blake2b160(abc)), "384264f676f39536840523f284921cdc68b6846b");
  assert.equal(toHex(blake2b224(abc)), "9bd237b02a29e43bdd6738afa5b53ff0eee178d6210b618e4511aec8");
  assert.equal(toHex(blake2b256(abc)), "bddd813c634239723171ef3fee98579b94964e3bb1cb3e427262c8c068d52319");
  assert.equal(
    toHex(sha3_256(new Uint8Array())),
    "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a",
  );
});

test("all fixed-size crypto wrappers enforce length and round-trip Bech32", () => {
  const wrappers = [
    [Ed25519KeyHash, 28], [ScriptHash, 28], [TransactionHash, 32],
    [GenesisDelegateHash, 28], [GenesisHash, 28], [AuxiliaryDataHash, 32],
    [PoolMetadataHash, 32], [VRFKeyHash, 32], [BlockBodyHash, 32],
    [BlockHeaderHash, 32], [DatumHash, 32], [ScriptDataHash, 32],
    [VRFVkey, 32], [KESVkey, 32], [NonceHash, 32], [AnchorDocHash, 32],
  ];
  for (const [Wrapper, length] of wrappers) {
    const bytes = Uint8Array.from({ length }, (_, index) => index);
    const value = Wrapper.from_raw_bytes(bytes);
    assert.equal(value.to_hex(), toHex(bytes));
    assert.equal(Wrapper.from_bech32(value.to_bech32("vector")).to_hex(), value.to_hex());
    assert.throws(() => Wrapper.from_raw_bytes(new Uint8Array(length - 1)));
  }
});

test("normal Ed25519 matches RFC 8032 test vector 1", () => {
  const key = PrivateKey.from_normal_bytes(
    fromHex("9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60"),
  );
  const publicKey = key.to_public();
  const signature = key.sign(new Uint8Array());
  assert.equal(publicKey.to_raw_bytes().length, 32);
  assert.equal(publicKey.to_bech32(), PublicKey.from_bech32(publicKey.to_bech32()).to_bech32());
  assert.equal(toHex(publicKey.to_raw_bytes()), "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a");
  assert.equal(
    signature.to_hex(),
    "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e06522490155" +
      "5fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b",
  );
  assert.equal(publicKey.verify(new Uint8Array(), signature), true);
  assert.equal(publicKey.verify(Uint8Array.of(0), signature), false);
});

test("normal and extended Ed25519 keys round-trip and produce verifiable signatures", () => {
  const encodedKeys = [
    "ed25519_sk1ahfetf02qwwg4dkq7mgp4a25lx5vh9920cr5wnxmpzz9906qvm8qwvlts0",
    "ed25519e_sk1gqwl4szuwwh6d0yk3nsqcc6xxc3fpvjlevgwvt60df59v8zd8f8prazt8ln3lmz096ux3xvhhvm3ca9wj2yctdh3pnw0szrma07rt5gl748fp",
  ];
  for (const encoded of encodedKeys) {
    const native = PrivateKey.from_bech32(encoded);
    assert.equal(native.to_bech32(), encoded);
    assert.equal(PrivateKey.from_bech32(native.to_bech32()).to_public().to_bech32(), native.to_public().to_bech32());
    assert.equal(native.to_public().verify(message, native.sign(message)), true);
    assert.equal(native.to_public().hash().to_raw_bytes().length, 28);
  }
  assert.throws(() => PrivateKey.from_normal_bytes(new Uint8Array(31)));
  assert.throws(() => PrivateKey.from_bech32("xpub1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqfr7f80"));
});

test("Cardano Ed25519-BIP32 root, hard/soft derivation, xpub, and 128-byte form are consistent", () => {
  const entropy = Uint8Array.from({ length: 32 }, (_, index) => index);
  const password = new TextEncoder().encode("cardano");
  const native = Bip32PrivateKey.from_bip39_entropy(entropy, password);
  assert.equal(Bip32PrivateKey.from_bech32(native.to_bech32()).to_bech32(), native.to_bech32());
  assert.equal(Bip32PublicKey.from_bech32(native.to_public().to_bech32()).to_bech32(), native.to_public().to_bech32());
  assert.equal(toHex(Bip32PrivateKey.from_128_xprv(native.to_128_xprv()).to_raw_bytes()), toHex(native.to_raw_bytes()));
  for (const index of [0, 1, 0x80000000, 0xffffffff]) {
    assert.equal(native.derive(index).to_raw_bytes().length, 96);
  }
  assert.equal(toHex(native.derive(17).to_public().to_raw_bytes()), toHex(native.to_public().derive(17).to_raw_bytes()));
  assert.throws(() => native.to_public().derive(0x80000000));
  assert.throws(() => native.derive(-1));
});

test("EMIP-3 PBKDF2 and ChaCha20-Poly1305 layout matches the published vector", () => {
  const args = [
    "70617373776f7264",
    "50515253c0c1c2c3c4c5c6c750515253c0c1c2c3c4c5c6c750515253c0c1c2c3",
    "50515253c0c1c2c3c4c5c6c7",
    "736f6d65206461746120746f20656e6372797074",
  ];
  const encrypted = emip3_encrypt_with_password(...args);
  assert.equal(
    encrypted,
    "50515253c0c1c2c3c4c5c6c750515253c0c1c2c3c4c5c6c750515253c0c1c2c3" +
      "50515253c0c1c2c3c4c5c6c7c266630887d216bf88cc4990f73bad7f" +
      "35bc7c0225b38fe24a7c28b5f9bda6283e3c5768",
  );
  assert.equal(emip3_decrypt_with_password(args[0], encrypted), args[3]);
  const tampered = `${encrypted.slice(0, -2)}00`;
  assert.throws(() => emip3_decrypt_with_password(args[0], tampered));
  assert.throws(() => emip3_encrypt_with_password("", args[1], args[2], args[3]));
});

test("key generation uses only the injected CSPRNG boundary and fails closed", () => {
  let next = 1;
  const source = { fill(target) { target.fill(next); next += 1; } };
  assert.equal(generateEd25519PrivateKey(source).to_raw_bytes()[0], 1);
  assert.equal(generateExtendedPrivateKey(source).to_raw_bytes()[32], 2);
  assert.equal(generateBip32PrivateKey(source).chaincode()[0], 3);
  const unavailable = { fill() { throw new Error("CSPRNG unavailable"); } };
  assert.throws(() => generateEd25519PrivateKey(unavailable), /CSPRNG unavailable/u);
  const first = PrivateKey.generate_ed25519().to_raw_bytes();
  const second = PrivateKey.generate_ed25519().to_raw_bytes();
  assert.notDeepEqual(first, second);
  assert.notDeepEqual(
    Bip32PrivateKey.generate_ed25519_bip32().to_raw_bytes(),
    Bip32PrivateKey.generate_ed25519_bip32().to_raw_bytes(),
  );
});

test("owned secret buffers have explicit best-effort disposal", () => {
  const key = PrivateKey.from_normal_bytes(new Uint8Array(32).fill(7));
  const detached = key.to_raw_bytes();
  key.dispose();
  assert.deepEqual(detached, new Uint8Array(32).fill(7));
  assert.throws(() => key.to_raw_bytes(), /disposed/u);
  assert.doesNotThrow(() => key.dispose());
});

test("legacy extended signing and Byron proxy certificates retain historical bytes", () => {
  const legacyRaw = fromHex(
    "28bcf7f6439e62f304f589619f6b1612f9a984978b445e4ec6f59e595c051150" +
      "fa47864365423db7ed7a117cd33c89aef1295322d385cad08eff39ede419e913" +
      "af586f2ce665c8bdddc4f470cdcea8b7a3a33e35730235f2c50fa08abbef2e48",
  );
  const legacy = legacyPrivateKeyFromRawBytes(legacyRaw);
  const legacySignature = Ed25519Signature.from_raw_bytes(legacySign(legacy, message));
  assert.equal(PublicKey.from_bytes(legacyPublicKey(legacy).subarray(0, 32)).verify(message, legacySignature), true);
  assert.equal(toHex(legacy.chaincode()), toHex(legacyRaw.subarray(64)));

  const issuer = Bip32PrivateKey.from_raw_bytes(fromHex(
    "b8b054ec1b92dd4542db35e2f813f013a8d7ee9f53255b26f3ef3dafb74e1146" +
      "2545bd9c85aa0a6f6719a933eba16909c1a2fa0bbb58e9cd98bf9ddbb79f7d50" +
      "fcfc22db8155f8d6ca0e3a975cb1b6aa5d6e7609b30c99877e469db06b5d5016",
  ));
  const delegate = Bip32PublicKey.from_raw_bytes(fromHex(
    "695b380fc72ae7d830d46f902a7c9d4057a4b9a7a0be235b87fdf51e698619e0" +
      "33aac8d93fd4cb82785973bb943f2047ddd1e664d4e185e7be634722e108389a",
  ));
  const certificate = signByronProxyCertificate(issuer, delegate, 0n, 328429219);
  assert.equal(certificate.to_hex(), "a72bf0119afd1ba5bed56b6521544105b6077c884609666296dbc59275477149a1b8230ce5b6c0fa81e1ec61c717164be57422e86a8f2f5773cdc66da99fcc0e");
  assert.equal(verifyByronProxyCertificate(issuer.to_public(), delegate, 0n, 328429219, certificate), true);
  assert.equal(verifyByronProxyCertificate(issuer.to_public(), delegate, 1n, 328429219, certificate), false);
});

test("legacy ABOR integer/byte serialization and typed byte access semantics are preserved", () => {
  const encoded = new AborEncoder()
    .u16(10).u32(0x12345).u64(0xffeeddcc00112233n)
    .u128(0xffeeddcc0011223321490219480912n).bytes(Uint8Array.of(1, 2, 3, 4, 5, 6, 7, 8, 9))
    .finalize();
  const decoder = new AborDecoder(encoded);
  assert.equal(decoder.u16(), 10);
  assert.equal(decoder.u32(), 0x12345);
  assert.equal(decoder.u64(), 0xffeeddcc00112233n);
  assert.equal(decoder.u128(), 0xffeeddcc0011223321490219480912n);
  assert.deepEqual(decoder.bytes(), Uint8Array.of(1, 2, 3, 4, 5, 6, 7, 8, 9));
  decoder.end();
  const typed = Uint8Array.from({ length: 10 }, (_, index) => index).subarray(2, 5);
  assert.deepEqual(typed, Uint8Array.of(2, 3, 4));
});
