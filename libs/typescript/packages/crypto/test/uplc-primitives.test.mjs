import assert from "node:assert/strict";
import test from "node:test";

import {
  bls12_381_compress,
  bls12_381_scalar_mul,
  keccak_256,
  ripemd_160,
  sha2_256,
  verifySecp256k1Ecdsa,
  verifySecp256k1Schnorr,
} from "@xray-network/cardano-crypto";
import {
  schnorr,
  secp256k1,
} from "@noble/curves/secp256k1.js";

const hex = (bytes) => Array.from(bytes, (value) => value.toString(16).padStart(2, "0")).join("");

test("UPLC hash primitives match authoritative empty-message vectors", () => {
  const empty = new Uint8Array();
  assert.equal(hex(sha2_256(empty)), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  assert.equal(hex(keccak_256(empty)), "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
  assert.equal(hex(ripemd_160(empty)), "9c1185a5c5e9fc54612808977ee8f548b2258d31");
});

test("UPLC secp256k1 verification accepts compact authoritative encodings", () => {
  const secret = Uint8Array.from({ length: 32 }, (_, index) => index + 1);
  const digest = sha2_256(new TextEncoder().encode("cardano-lib UPLC"));
  const publicKey = secp256k1.getPublicKey(secret, true);
  const ecdsa = secp256k1.sign(digest, secret, { prehash: false, format: "compact" });
  assert.equal(verifySecp256k1Ecdsa(publicKey, ecdsa, digest), true);
  assert.equal(verifySecp256k1Ecdsa(publicKey, ecdsa, Uint8Array.from(digest, (value) => value ^ 1)), false);

  const message = new TextEncoder().encode("BIP-340 permits arbitrary message lengths");
  const schnorrPublicKey = schnorr.getPublicKey(secret);
  const signature = schnorr.sign(message, secret, new Uint8Array(32));
  assert.equal(verifySecp256k1Schnorr(schnorrPublicKey, signature, message), true);
});

test("UPLC BLS12-381 wrappers retain the standard compressed G1 generator", () => {
  const parsed = fromHex(
    "97f1d3a73197d7942695638c4fa9ac0f" +
    "c3688c4f9774b905a14e3a3f171bac58" +
    "6c55e83ff97a1aeffb3af00adb22c6bb",
  );
  assert.deepEqual(bls12_381_compress("g1", parsed), parsed);
  assert.deepEqual(bls12_381_scalar_mul("g1", 1n, parsed), parsed);
});

function fromHex(value) {
  return Uint8Array.from(value.match(/../gu)?.map((byte) => Number.parseInt(byte, 16)) ?? []);
}
