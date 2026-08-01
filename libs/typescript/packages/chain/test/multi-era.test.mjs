import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

import {
  Blake2b224,
  Blake2b256,
  MultiEraBlock,
  MultiEraBlockKind,
  MultiEraCertificate,
  MultiEraCertificateKind,
} from "@xray-network/xray-cardano-lib-chain/multi-era";
import {
  BabbageBlock,
  BabbageTransactionBody,
} from "@xray-network/xray-cardano-lib-chain/babbage";
import { ByronBlockKind } from "@xray-network/xray-cardano-lib-chain/byron";
import { encodeCbor } from "@xray-network/xray-cardano-lib-core";

const inventory = JSON.parse(
  await readFile(
    new URL(
      "../../../../../updates/providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/manifest.json",
      import.meta.url,
    ),
    "utf8",
  ),
);
const fixtureRoot = fileURLToPath(new URL(
  "../../../../../updates/providers/cardano-multiplatform-lib/0001-cardano-multiplatform-lib/artifacts/test-vectors/",
  import.meta.url,
));

function isAsciiHex(bytes) {
  return bytes.length > 0 && bytes.every((byte) =>
    (byte >= 48 && byte <= 57) ||
    (byte >= 65 && byte <= 70) ||
    (byte >= 97 && byte <= 102) ||
    byte === 9 || byte === 10 || byte === 13 || byte === 32
  );
}

async function fixtureBytes(fixture) {
  const stored = await readFile(path.join(fixtureRoot, fixture.path));
  return isAsciiHex(stored) ? Uint8Array.from(Buffer.from(stored.toString().trim(), "hex")) : stored;
}

function hex(bytes) {
  return Buffer.from(bytes).toString("hex");
}

test("all 86 historical-era golden blocks retain their recorded outcome", async () => {
  const successes = [];
  const rejections = [];
  const eraCounts = {};

  for (const fixture of inventory.fixtures.goldenBlocks) {
    const bytes = await fixtureBytes(fixture);
    try {
      const block = MultiEraBlock.from_explicit_network_cbor_bytes(bytes);
      assert.equal(fixture.expected, "byte-exact-round-trip", `${fixture.path} unexpectedly decoded`);
      assert.deepEqual(block.to_explicit_network_cbor_bytes(), bytes, fixture.path);
      assert.equal(block.kind(), fixture.eraTag <= 1 ? MultiEraBlockKind.Byron : fixture.eraTag - 1);
      assert.equal(typeof block.header().block_number(), "bigint");
      assert.equal(typeof block.header().slot(), "bigint");
      assert.equal(block.header().prev_hash()?.to_raw_bytes().length, 32);

      const filename = path.basename(fixture.path, path.extname(fixture.path));
      if (fixture.path.startsWith("blocks/mainnet/")) assert.equal(hex(block.hash()), filename);
      successes.push(fixture.path);
      eraCounts[fixture.era] = (eraCounts[fixture.era] ?? 0) + 1;
    } catch (error) {
      assert.equal(fixture.expected, "reject-invalid-hash-size", `${fixture.path}: ${String(error)}`);
      assert.equal(path.basename(fixture.path), "conway8.block");
      assert.match(String(error), /pool key hash size 56; expected 28/u);
      rejections.push(fixture.path);
    }
  }

  assert.equal(inventory.fixtures.goldenBlocks.length, 86);
  assert.equal(successes.length, 85);
  assert.equal(rejections.length, 1);
  assert.deepEqual(eraCounts, {
    allegra: 2,
    alonzo: 24,
    babbage: 15,
    "byron-ebb": 1,
    "byron-main": 28,
    conway: 4,
    mary: 6,
    shelley: 5,
  });
});

test("explicit envelopes dispatch exactly across all eight network era tags", async () => {
  const seenTags = new Set();
  for (const fixture of inventory.fixtures.goldenBlocks) {
    if (fixture.expected !== "byte-exact-round-trip" || seenTags.has(fixture.eraTag)) continue;
    const block = MultiEraBlock.from_explicit_network_cbor_bytes(await fixtureBytes(fixture));
    seenTags.add(fixture.eraTag);

    const variantNames = ["as_byron", "as_byron", "as_shelley", "as_allegra", "as_mary", "as_alonzo", "as_babbage", "as_conway"];
    assert.ok(block[variantNames[fixture.eraTag]]());
    if (fixture.eraTag <= 1) {
      assert.equal(block.as_byron().kind(), fixture.eraTag === 0 ? ByronBlockKind.EpochBoundary : ByronBlockKind.Main);
    }

    const bodies = block.transaction_bodies();
    for (let index = 0; index < bodies.len(); index += 1) {
      const body = bodies.get(index);
      assert.equal(body.kind(), block.kind());
      assert.equal(body.hash().to_raw_bytes().length, 32);
      assert.ok(body.inputs().len() >= 0);
      assert.ok(body.outputs().len() >= 0);
    }
  }
  assert.deepEqual([...seenTags].sort(), [0, 1, 2, 3, 4, 5, 6, 7]);
});

test("historical CBOR preserves duplicate maps and indefinite containers", () => {
  // Regression coverage for babbage_mint_duplicate and
  // babbage_tx_hash_mismatch_duplicate_mint: the lossless tree retains duplicate keys.
  const uint = (value) => ({ kind: "unsigned", value, encoding: { width: 0 } });
  const array = (values) => ({ kind: "array", values, encoding: { kind: "definite", width: 0 } });
  const bytes = (value) => ({ kind: "bytes", value, encoding: { kind: "definite", width: 0 } });
  const policy = bytes(new Uint8Array(28).fill(0x61));
  const asset = bytes(new Uint8Array());
  const bundle = (amount) => ({
    kind: "map",
    entries: [[asset, uint(amount)]],
    encoding: { kind: "definite", width: 0 },
  });
  const mint = {
    kind: "map",
    entries: [[policy, bundle(1n)], [policy, bundle(2n)]],
    encoding: { kind: "definite", width: 0 },
  };
  const duplicateMap = encodeCbor({
    kind: "map",
    entries: [
      [uint(0n), array([])],
      [uint(1n), array([])],
      [uint(2n), uint(0n)],
      [uint(9n), mint],
    ],
    encoding: { kind: "indefinite", width: 0 },
  });
  const body = BabbageTransactionBody.from_cbor_bytes(duplicateMap);
  assert.deepEqual(body.to_cbor_bytes(), duplicateMap);
  assert.equal(body.to_canonical_cbor_bytes()[0], 0xa4);
});

test("Byron, historical-era, certificate, and hash facade helpers are usable", async () => {
  const babbageFixture = inventory.fixtures.goldenBlocks.find((fixture) => fixture.era === "babbage");
  const block = MultiEraBlock.from_explicit_network_cbor_bytes(await fixtureBytes(babbageFixture));
  const historical = BabbageBlock.from_cbor_bytes(block.as_babbage().to_cbor_bytes());
  assert.deepEqual(historical.to_cbor_bytes(), block.as_babbage().to_cbor_bytes());

  const uint = (value) => ({ kind: "unsigned", value, encoding: { width: 0 } });
  const array = (values) => ({ kind: "array", values, encoding: { kind: "definite", width: 0 } });
  const certificate = MultiEraCertificate.from_cbor_bytes(encodeCbor(array([
    uint(0n),
    array([
      uint(0n),
      { kind: "bytes", value: new Uint8Array(28), encoding: { kind: "definite", width: 0 } },
    ]),
  ])));
  assert.equal(certificate.kind(), MultiEraCertificateKind.StakeRegistration);
  assert.ok(certificate.as_stake_registration());
  assert.equal(certificate.as_pool_retirement(), undefined);
  assert.equal(MultiEraCertificateKind.UnregDrepCert, 17);
  assert.equal(MultiEraCertificateKind.UpdateDrepCert, 18);

  const hash224 = Blake2b224.from_raw_bytes(new Uint8Array(28).fill(0x11));
  const hash256 = Blake2b256.from_raw_bytes(new Uint8Array(32).fill(0x22));
  assert.equal(Blake2b224.from_hex(hash224.to_hex()).to_bech32("hash").startsWith("hash1"), true);
  assert.deepEqual(Blake2b256.from_bech32(hash256.to_bech32("digest")).to_raw_bytes(), hash256.to_raw_bytes());
});
