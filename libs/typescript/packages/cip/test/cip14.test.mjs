import assert from "node:assert/strict";
import test from "node:test";

import { encodeBech32 } from "@xray-network/xray-cardano-lib-core/bech32";
import { AssetName } from "@xray-network/xray-cardano-lib-chain/conway";
import { ScriptHash } from "@xray-network/xray-cardano-lib-crypto";
import { AssetFingerprint } from "@xray-network/xray-cardano-lib-cip/cip14";

const vectors = [
  ["7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373", "", "asset1rjklcrnsdzqp65wjgrg55sy9723kw09mlgvlc3"],
  ["7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc37e", "", "asset1nl0puwxmhas8fawxp8nx4e2q3wekg969n2auw3"],
  ["1e349c9bdea19fd6c147626a5260bc44b71635f398b67c59881df209", "", "asset1uyuxku60yqe57nusqzjx38aan3f2wq6s93f6ea"],
  ["7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373", "504154415445", "asset13n25uv0yaf5kus35fm2k86cqy60z58d9xmde92"],
  ["1e349c9bdea19fd6c147626a5260bc44b71635f398b67c59881df209", "504154415445", "asset1hv4p5tv2a837mzqrst04d0dcptdjmluqvdx9k3"],
  ["1e349c9bdea19fd6c147626a5260bc44b71635f398b67c59881df209", "7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373", "asset1aqrdypg669jgazruv5ah07nuyqe0wxjhe2el6f"],
  ["7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373", "1e349c9bdea19fd6c147626a5260bc44b71635f398b67c59881df209", "asset17jd78wukhtrnmjh3fngzasxm8rck0l2r4hhyyt"],
  ["7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373", "00".repeat(32), "asset1pkpwyknlvul7az0xx8czhl60pyel45rpje4z8w"],
];

const bech32Alphabet = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
function bech32Polymod(values) {
  const generators = [0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3];
  let checksum = 1;
  for (const value of values) {
    const top = checksum >>> 25;
    checksum = ((checksum & 0x1ff_ffff) << 5) ^ value;
    for (let index = 0; index < generators.length; index += 1) if (((top >>> index) & 1) !== 0) checksum ^= generators[index];
  }
  return checksum >>> 0;
}
function asBech32m(value) {
  const separator = value.lastIndexOf("1");
  const prefix = value.slice(0, separator);
  const data = [...value.slice(separator + 1, -6)].map((character) => bech32Alphabet.indexOf(character));
  const expanded = [...prefix].map((character) => character.charCodeAt(0) >>> 5)
    .concat([0], [...prefix].map((character) => character.charCodeAt(0) & 31));
  const checksum = bech32Polymod(expanded.concat(data, [0, 0, 0, 0, 0, 0])) ^ 0x2bc8_30a3;
  const suffix = Array.from({ length: 6 }, (_, index) => (checksum >>> (5 * (5 - index))) & 31);
  return `${prefix}1${data.concat(suffix).map((word) => bech32Alphabet[word]).join("")}`;
}

test("CIP-14 matches all eight captured official vectors", () => {
  for (const [policyHex, nameHex, expected] of vectors) {
    const fingerprint = AssetFingerprint.from_parts(
      ScriptHash.from_hex(policyHex),
      AssetName.from_hex(nameHex),
    );
    assert.equal(fingerprint.to_bech32(), expected);
    assert.equal(AssetFingerprint.from_bech32(expected).to_bech32(), expected);
    assert.equal(AssetFingerprint.from_bech32(expected.toUpperCase()).to_bech32(), expected);
    assert.equal(fingerprint.to_raw_bytes().length, 20);
    assert.equal(fingerprint.equals(AssetFingerprint.from_bech32(expected)), true);
  }
});

test("CIP-14 parsing is strict and fingerprint bytes are defensively owned", () => {
  const value = AssetFingerprint.from_bech32(vectors[0][2]);
  const raw = value.to_raw_bytes();
  const expectedFirst = raw[0];
  raw[0] ^= 0xff;
  assert.equal(value.to_raw_bytes()[0], expectedFirst);
  assert.equal(value.equals(AssetFingerprint.from_bech32(vectors[1][2])), false);

  assert.throws(() => AssetFingerprint.from_bech32(encodeBech32("token", new Uint8Array(20))));
  assert.throws(() => AssetFingerprint.from_bech32(encodeBech32("asset", new Uint8Array(19))));
  assert.throws(() => AssetFingerprint.from_bech32(encodeBech32("asset", new Uint8Array(21))));
  const valid = encodeBech32("asset", new Uint8Array(20));
  assert.throws(() => AssetFingerprint.from_bech32(`${valid.slice(0, -1)}${valid.endsWith("q") ? "p" : "q"}`));
  assert.throws(() => AssetFingerprint.from_bech32(`A${valid.slice(1)}`));
  assert.throws(() => AssetFingerprint.from_bech32(asBech32m(valid)));
  assert.throws(() => ScriptHash.from_raw_bytes(new Uint8Array(27)));
  assert.throws(() => AssetName.from_raw_bytes(new Uint8Array(33)));

  const policyBytes = new Uint8Array(28);
  const nameBytes = Uint8Array.of(1, 2, 3);
  const policy = ScriptHash.from_raw_bytes(policyBytes);
  const name = AssetName.from_raw_bytes(nameBytes);
  const fingerprint = AssetFingerprint.from_parts(policy, name).to_bech32();
  policyBytes.fill(9);
  nameBytes.fill(9);
  assert.equal(AssetFingerprint.from_parts(policy, name).to_bech32(), fingerprint);
});
