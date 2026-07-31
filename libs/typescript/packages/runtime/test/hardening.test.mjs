import assert from "node:assert/strict";
import { performance } from "node:perf_hooks";
import test from "node:test";

import {
  Int,
  OrderedMap,
  PairMap,
  bytesToHex,
  decodeCbor,
  encodeCbor,
} from "../../core/dist/esm/index.js";
import {
  Address,
  CardanoNodePlutusDatumSchema,
  ChangeSelectionAlgo,
  Credential,
  EnterpriseAddress,
  ExUnitPrices,
  LinearFee,
  MetadataJsonSchema,
  PlutusData,
  Rational,
  SingleInputBuilder,
  TransactionBuilder,
  TransactionBuilderConfigBuilder,
  TransactionInput,
  TransactionOutputBuilder,
  Value,
  decode_metadatum_to_json_str,
  encode_json_str_to_metadatum,
  encode_json_str_to_plutus_datum,
} from "../../chain/dist/esm/index.js";
import { PrivateKey, TransactionHash } from "../../crypto/dist/esm/index.js";
import { BabbageTransactionBody } from "../../chain/dist/esm/era/babbage/index.js";

const campaignCases = Number.parseInt(process.env.XRAY_CARDANO_LIB_HARDENING_CASES ?? "6000", 10);
if (!Number.isSafeInteger(campaignCases) || campaignCases < 100) {
  throw new RangeError("XRAY_CARDANO_LIB_HARDENING_CASES must be an integer of at least 100");
}

function random(seed) {
  let state = seed >>> 0;
  return () => {
    state ^= state << 13;
    state ^= state >>> 17;
    state ^= state << 5;
    return state >>> 0;
  };
}

function canonicalConverges(bytes) {
  const first = encodeCbor(decodeCbor(bytes), { mode: "canonical" });
  const second = encodeCbor(decodeCbor(first), { mode: "canonical" });
  assert.deepEqual(second, first);
}

test(`bounded malformed-CBOR campaign (${campaignCases} deterministic cases)`, () => {
  const next = random(0xc0b0_12f0);
  const targeted = [
    [], [0xff], [0x1a, 0, 0], [0xd9], [0x9f, 1], [0xbf, 1, 0xff],
    [0x5b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff],
    [0x9b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff],
    [...Array.from({ length: 80 }, () => 0x81), 0],
    [0xa2, 1, 2, 1, 3],
  ].map((value) => Uint8Array.from(value));
  let accepted = 0;
  let rejected = 0;
  let totalBytes = 0;
  const started = performance.now();

  for (let index = 0; index < campaignCases; index += 1) {
    let bytes;
    if (index < targeted.length) bytes = targeted[index];
    else {
      const length = next() % 129;
      bytes = Uint8Array.from({ length }, () => next() & 0xff);
      if (index % 7 === 0 && bytes.length > 0) bytes[next() % bytes.length] ^= 1 << (next() % 8);
    }
    totalBytes += bytes.length;
    try {
      const value = decodeCbor(bytes, {
        limits: { maxDepth: 48, maxCollectionLength: 512, maxStringBytes: 4096, maxTokens: 2048 },
      });
      const preserved = encodeCbor(value);
      assert.ok(preserved.length <= 16_384);
      canonicalConverges(preserved);
      accepted += 1;
    } catch (error) {
      assert.ok(error instanceof Error);
      rejected += 1;
    }
  }

  assert.equal(accepted + rejected, campaignCases);
  assert.ok(totalBytes <= campaignCases * 128);
  assert.ok(performance.now() - started < 15_000, "bounded malformed input campaign exceeded 15s");
});

function randomCbor(next, depth = 0) {
  const kind = depth >= 5 ? next() % 4 : next() % 8;
  if (kind === 0) return { kind: "unsigned", value: BigInt(next()), encoding: { width: 8 } };
  if (kind === 1) return { kind: "negative", value: -1n - BigInt(next()), encoding: { width: 8 } };
  if (kind === 2) return { kind: "bytes", value: Uint8Array.from({ length: next() % 24 }, () => next() & 0xff), encoding: { kind: "definite", width: 1 } };
  if (kind === 3) return { kind: "text", value: `v${next().toString(16)}`, encoding: { kind: "definite", width: 1 } };
  if (kind === 4) return { kind: "array", values: Array.from({ length: next() % 5 }, () => randomCbor(next, depth + 1)), encoding: next() % 2 === 0 ? { kind: "definite", width: 1 } : { kind: "indefinite" } };
  if (kind === 5) return { kind: "map", entries: Array.from({ length: next() % 5 }, () => [randomCbor(next, depth + 1), randomCbor(next, depth + 1)]), encoding: next() % 2 === 0 ? { kind: "definite", width: 1 } : { kind: "indefinite" } };
  if (kind === 6) return { kind: "tag", tag: BigInt(next() % 1024), value: randomCbor(next, depth + 1), encoding: { width: 2 } };
  return { kind: "boolean", value: next() % 2 === 0 };
}

test("nested CBOR preservation and canonical convergence properties", () => {
  const next = random(0x51a7_e123);
  for (let index = 0; index < 1500; index += 1) {
    const wire = encodeCbor(randomCbor(next));
    assert.deepEqual(encodeCbor(decodeCbor(wire)), wire);
    canonicalConverges(wire);
  }
});

function randomMetadata(next, depth = 0) {
  if (depth >= 4 || next() % 3 === 0) return next() % 2 === 0 ? next() % 10_000 : `text-${next() % 10_000}`;
  if (next() % 2 === 0) return Array.from({ length: next() % 5 }, () => randomMetadata(next, depth + 1));
  return Object.fromEntries(Array.from({ length: next() % 5 }, (_, index) => [`key-${depth}-${index}`, randomMetadata(next, depth + 1)]));
}

function randomPlutus(next, depth = 0) {
  if (depth >= 4) return next() % 2 === 0 ? { int: next() % 100_000 } : { bytes: (next() >>> 0).toString(16).padStart(8, "0") };
  const kind = next() % 5;
  if (kind === 0) return { int: next() % 100_000 };
  if (kind === 1) return { bytes: (next() >>> 0).toString(16).padStart(8, "0") };
  if (kind === 2) return { list: Array.from({ length: next() % 4 }, () => randomPlutus(next, depth + 1)) };
  if (kind === 3) return { map: Array.from({ length: next() % 4 }, () => ({ k: randomPlutus(next, depth + 1), v: randomPlutus(next, depth + 1) })) };
  return { constructor: next() % 128, fields: Array.from({ length: next() % 4 }, () => randomPlutus(next, depth + 1)) };
}

test("nested metadata and Plutus JSON properties", () => {
  const next = random(0x7a11_da7a);
  for (let index = 0; index < 500; index += 1) {
    const metadata = randomMetadata(next);
    const encoded = encode_json_str_to_metadatum(JSON.stringify(metadata), MetadataJsonSchema.NoConversions);
    assert.deepEqual(JSON.parse(decode_metadatum_to_json_str(encoded, MetadataJsonSchema.NoConversions)), metadata);

    const plutusJson = randomPlutus(next);
    const plutus = PlutusData.from_json(JSON.stringify(plutusJson));
    const reparsed = PlutusData.from_json(plutus.to_json());
    assert.equal(reparsed.to_canonical_cbor_hex(), plutus.to_canonical_cbor_hex());
  }
});

test("ordered maps, duplicate maps, and integer boundaries retain their invariants", () => {
  const next = random(0x0dd3_12ab);
  const ordered = new OrderedMap();
  const model = new Map();
  const pairs = new PairMap();
  for (let index = 0; index < 1000; index += 1) {
    const key = next() % 32;
    const value = next();
    ordered.set(key, value);
    model.set(key, value);
    pairs.append(key, value);
  }
  assert.deepEqual([...ordered], [...model]);
  for (let key = 0; key < 32; key += 1) assert.equal(pairs.getAll(key).length, [...pairs].filter(([item]) => item === key).length);

  for (const edge of [-18446744073709551616n, -1n, 0n, 23n, 24n, 18446744073709551615n]) {
    const value = Int.new(edge);
    assert.equal(Int.from_cbor_bytes(value.to_cbor_bytes()).to_str(), edge.toString());
  }
});

test("deterministic signatures verify and reject every changed message", () => {
  const next = random(0x5eec_0de5);
  for (let index = 0; index < 128; index += 1) {
    const secret = Uint8Array.from({ length: 32 }, () => next() & 0xff);
    const message = Uint8Array.from({ length: next() % 257 }, () => next() & 0xff);
    const key = PrivateKey.from_normal_bytes(secret);
    const signature = key.sign(message);
    assert.equal(key.to_public().verify(message, signature), true);
    const changed = message.length === 0 ? Uint8Array.of(1) : Uint8Array.from(message);
    if (message.length > 0) changed[next() % changed.length] ^= 1;
    assert.equal(key.to_public().verify(changed, signature), false);
    key.dispose();
  }
});

function builderConfig() {
  return TransactionBuilderConfigBuilder.new()
    .fee_algo(LinearFee.new(1n, 10n, 0n))
    .pool_deposit(500n).key_deposit(100n)
    .max_value_size(5000).max_tx_size(16_384).coins_per_utxo_byte(1n)
    .ex_unit_prices(ExUnitPrices.new(Rational.new(1n, 10n), Rational.new(1n, 10n)))
    .collateral_percentage(150).max_collateral_inputs(3).build();
}

test("transaction builders preserve the coin balance invariant across recorded seeds", () => {
  const key = PrivateKey.from_normal_bytes(Uint8Array.from({ length: 32 }, (_, index) => index + 1));
  const address = EnterpriseAddress.new(0, Credential.new_pub_key(key.to_public().hash())).to_address();
  const next = random(0xba1a_0ced);
  for (let index = 0; index < 96; index += 1) {
    const inputCoin = 3_000_000n + BigInt(next() % 5_000_000);
    const outputCoin = 1_000_000n + BigInt(next() % 1_000_000);
    const hash = new Uint8Array(32); hash[31] = index;
    const input = TransactionInput.new(TransactionHash.from_raw_bytes(hash), BigInt(index));
    const source = TransactionOutputBuilder.new().with_address(address).next().with_value(Value.from_coin(inputCoin)).build().output();
    const output = TransactionOutputBuilder.new().with_address(address).next().with_value(Value.from_coin(outputCoin)).build();
    const builder = TransactionBuilder.new(builderConfig());
    builder.add_input(SingleInputBuilder.new(input, source).payment_key());
    builder.add_output(output);
    builder.build(ChangeSelectionAlgo.Default, address);
    assert.equal(
      builder.get_total_input().coin(),
      builder.get_total_output().coin() + (builder.get_fee_if_set() ?? 0n),
    );
  }
  key.dispose();
});

test("hostile JSON cannot pollute prototypes and excessive nesting is bounded", () => {
  const hostile = '{"__proto__":{"polluted":"yes"},"constructor":{"prototype":{"polluted":"yes"}}}';
  const metadata = encode_json_str_to_metadatum(hostile, MetadataJsonSchema.NoConversions);
  const decoded = JSON.parse(decode_metadatum_to_json_str(metadata, MetadataJsonSchema.NoConversions));
  assert.equal(Object.hasOwn(decoded, "__proto__"), true);
  assert.equal(Object.hasOwn(decoded, "constructor"), true);
  assert.equal(Object.prototype.polluted, undefined);
  assert.equal({}.polluted, undefined);

  assert.throws(() => PlutusData.from_json(hostile), /Invalid Plutus/u);
  const deepMetadata = `${"[".repeat(140)}0${"]".repeat(140)}`;
  const deepPlutus = `${'{"list":['.repeat(140)}{"int":0}${"]}".repeat(140)}`;
  assert.throws(() => encode_json_str_to_metadatum(deepMetadata, MetadataJsonSchema.NoConversions), /nesting/u);
  assert.throws(() => encode_json_str_to_plutus_datum(deepMetadata, CardanoNodePlutusDatumSchema.BasicConversions), /nesting/u);
  assert.throws(() => PlutusData.from_json(deepPlutus), /nesting/u);
  assert.throws(() => BabbageTransactionBody.from_json(deepMetadata), /nesting/u);
});

test("fuzz campaign seed is stable", () => {
  const next = random(0xc0b0_12f0);
  assert.equal(bytesToHex(Uint8Array.from({ length: 8 }, () => next() & 0xff)), "67e1a39816f2bd5e");
  assert.equal(Address.from_raw_bytes(Uint8Array.from([0x60, ...new Uint8Array(28)])).network_id(), 0);
});
