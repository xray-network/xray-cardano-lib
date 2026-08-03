import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";

import { decodeBech32, encodeBech32 } from "@xray-network/xray-cardano-lib-core/bech32";
import {
  CardanoKeyRole,
  Cip1852Path,
  Cip1852Role,
  PrivateKey,
  cip1852AccountPublic,
  cip1852RootFromIcarusEntropy,
  decodeCardanoBip32PrivateKey,
  decodeCardanoBip32PublicKey,
  decodeCardanoPrivateKey,
  decodeCardanoPublicKey,
  deriveCip1852AccountPrivate,
  deriveCip1852Private,
  deriveCip1852Public,
  encodeCardanoBip32PrivateKey,
  encodeCardanoBip32PublicKey,
  encodeCardanoPrivateKey,
  encodeCardanoPublicKey,
} from "../dist/esm/index.js";

const hex = (value) => Uint8Array.from(Buffer.from(value, "hex"));
const toHex = (value) => Buffer.from(value).toString("hex");
const vectorUrls = [1, 2, 3, 4].map((number) => new URL(
  `../../../../../.xray/updates/providers/cardano-cips/0001-cardano-cips/artifacts/upstream/CIP-0105/test-vectors/test-vector-${number}.md`,
  import.meta.url,
));

function encodedPairs(source) {
  const pairs = new Map();
  for (const match of source.matchAll(/Hex: `([0-9a-f]+)`\s+Bech32: `([^`]+)`/gu)) {
    const raw = match[1];
    const encoded = match[2];
    pairs.set(decodeBech32(encoded).prefix, { raw, encoded });
  }
  return pairs;
}

const roleForPrefix = {
  drep: CardanoKeyRole.DRep,
  cc_cold: CardanoKeyRole.ConstitutionalCommitteeCold,
  cc_hot: CardanoKeyRole.ConstitutionalCommitteeHot,
};

test("role-aware Cardano key codecs match every key in all four CIP-0105 vector documents", async () => {
  for (const url of vectorUrls) {
    const pairs = encodedPairs(await readFile(url, "utf8"));
    for (const [base, role] of Object.entries(roleForPrefix)) {
      const cases = [
        ["sk", decodeCardanoPrivateKey, encodeCardanoPrivateKey],
        ["vk", decodeCardanoPublicKey, encodeCardanoPublicKey],
        ["xsk", decodeCardanoBip32PrivateKey, encodeCardanoBip32PrivateKey],
        ["xvk", decodeCardanoBip32PublicKey, encodeCardanoBip32PublicKey],
      ];
      for (const [suffix, decode, encode] of cases) {
        const expected = pairs.get(`${base}_${suffix}`);
        assert.ok(expected, `${base}_${suffix} is absent from ${url.pathname}`);
        const key = decode(role, expected.encoded);
        assert.equal(toHex(key.to_raw_bytes()), expected.raw);
        assert.equal(encode(role, key), expected.encoded);
      }
    }
  }
});

test("CIP-1852 private and account-public derivation match all four CIP-0105 documents", async () => {
  const entropies = [
    "df9ed25ed146bf43336a5d7cf7395994",
    "df9ed25ed146bf43336a5d7cf7395994",
    "4e828f9a67ddcff0e6391ad4f26ddb7579f59ba14b6dd4baf63dcfdb9d2420da",
    "4e828f9a67ddcff0e6391ad4f26ddb7579f59ba14b6dd4baf63dcfdb9d2420da",
  ];
  const accounts = [0, 256, 0, 256];
  const roles = [
    ["drep", Cip1852Role.DRep],
    ["cc_cold", Cip1852Role.ConstitutionalCommitteeCold],
    ["cc_hot", Cip1852Role.ConstitutionalCommitteeHot],
  ];
  for (let vector = 0; vector < vectorUrls.length; vector += 1) {
    const pairs = encodedPairs(await readFile(vectorUrls[vector], "utf8"));
    const root = cip1852RootFromIcarusEntropy(hex(entropies[vector]));
    const accountPrivate = deriveCip1852AccountPrivate(root, accounts[vector]);
    const accountPublic = cip1852AccountPublic(accountPrivate);
    for (const [base, role] of roles) {
      const path = Cip1852Path.new(accounts[vector], role, 0);
      const privateChild = deriveCip1852Private(root, path);
      const publicChild = deriveCip1852Public(accountPublic, role, 0);
      assert.equal(toHex(privateChild.to_raw_bytes()), pairs.get(`${base}_xsk`).raw);
      assert.equal(toHex(privateChild.to_raw_key().to_raw_bytes()), pairs.get(`${base}_sk`).raw);
      assert.equal(toHex(privateChild.to_public().to_raw_bytes()), pairs.get(`${base}_xvk`).raw);
      assert.equal(toHex(privateChild.to_public().to_raw_key().to_raw_bytes()), pairs.get(`${base}_vk`).raw);
      assert.deepEqual(publicChild.to_raw_bytes(), privateChild.to_public().to_raw_bytes());
    }
  }
});

test("CIP-1852 paths and Cardano key shapes enforce every hardened boundary", () => {
  const path = Cip1852Path.new(0x7fff_ffff, Cip1852Role.ConstitutionalCommitteeHot, 0x7fff_ffff);
  assert.deepEqual(path.indices(), [0x8000_073c, 0x8000_0717, 0xffff_ffff, 5, 0x7fff_ffff]);
  assert.equal(path.toString(), "m/1852'/1815'/2147483647'/5/2147483647");
  assert.throws(() => Cip1852Path.new(-1, Cip1852Role.External, 0));
  assert.throws(() => Cip1852Path.new(0x8000_0000, Cip1852Role.External, 0));
  assert.throws(() => Cip1852Path.new(0, 6, 0));
  assert.throws(() => Cip1852Path.new(0, Cip1852Role.External, 0x8000_0000));

  const root = cip1852RootFromIcarusEntropy(hex("df9ed25ed146bf43336a5d7cf7395994"));
  const account = deriveCip1852AccountPrivate(root, 0);
  const accountPublic = cip1852AccountPublic(account);
  for (const role of [0, 1, 2, 3, 4, 5]) {
    assert.deepEqual(deriveCip1852Public(accountPublic, role, 0).to_raw_bytes(), account.derive(role).derive(0).to_public().to_raw_bytes());
  }
  assert.throws(() => accountPublic.derive(0x8000_0000));
  assert.throws(() => deriveCip1852Public(accountPublic, Cip1852Role.External, 0x8000_0000));
  assert.doesNotThrow(() => deriveCip1852Private(root, path));

  const normal = PrivateKey.from_normal_bytes(new Uint8Array(32));
  const extended = account.derive(Cip1852Role.DRep).derive(0).to_raw_key();
  assert.equal(decodeCardanoPrivateKey(CardanoKeyRole.Payment, encodeCardanoPrivateKey(CardanoKeyRole.Payment, normal)).to_raw_bytes().length, 32);
  assert.equal(decodeCardanoPrivateKey(CardanoKeyRole.DRep, encodeCardanoPrivateKey(CardanoKeyRole.DRep, extended)).to_raw_bytes().length, 64);
  assert.throws(() => encodeCardanoPrivateKey(CardanoKeyRole.Payment, extended));
  assert.throws(() => decodeCardanoPrivateKey(CardanoKeyRole.Stake, encodeCardanoPrivateKey(CardanoKeyRole.DRep, extended)));
  assert.throws(() => decodeCardanoBip32PrivateKey(CardanoKeyRole.DRep, encodeBech32("drep_xsk", new Uint8Array(128))));
  const disposable = decodeCardanoPrivateKey(CardanoKeyRole.DRep, encodeCardanoPrivateKey(CardanoKeyRole.DRep, extended));
  disposable.dispose();
  assert.throws(() => encodeCardanoPrivateKey(CardanoKeyRole.DRep, disposable));
});
