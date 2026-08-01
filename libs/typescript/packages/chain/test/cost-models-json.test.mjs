import assert from "node:assert/strict";
import test from "node:test";
import {
  CostModels,
  MapU64ToArrI64,
} from "../../runtime/dist/esm/index.js";
import { decodeCbor } from "../../core/dist/esm/index.js";

test("CostModels parses numeric JSON keys and signed integer parameters", () => {
  const models = CostModels.from_json('{"0":[1,-2,3],"1":[4],"2":[5]}');
  const values = models.get();
  const expectedValues = MapU64ToArrI64.new();
  expectedValues.insert(0n, BigInt64Array.from([1n, -2n, 3n]));
  expectedValues.insert(1n, BigInt64Array.from([4n]));
  expectedValues.insert(2n, BigInt64Array.from([5n]));

  assert.deepEqual([...values.get(0n)], [1n, -2n, 3n]);
  assert.deepEqual([...values.get(1n)], [4n]);
  assert.deepEqual([...values.get(2n)], [5n]);
  assert.equal(models.to_cbor_hex(), CostModels.new(expectedValues).to_cbor_hex());

  const node = decodeCbor(models.to_cbor_bytes());
  assert.equal(node.kind, "map");
  assert.ok(node.entries.every(([key]) => key.kind === "unsigned"));
});

test("CostModels JSON serialization has a symmetric object shape", () => {
  const models = CostModels.from_json('{"0":[1,-2,3],"1":[4],"2":[5]}');

  assert.deepEqual(JSON.parse(models.to_json()), {
    0: [1, -2, 3],
    1: [4],
    2: [5],
  });
  assert.equal(
    CostModels.from_json(models.to_json()).to_canonical_cbor_hex(),
    models.to_canonical_cbor_hex(),
  );
});

test("CostModels supports an empty JSON object", () => {
  const models = CostModels.from_json("{}");

  assert.deepEqual(models.to_js_value(), {});
  assert.equal(models.to_canonical_cbor_hex(), "a0");
});

test("CostModels rejects malformed JSON keys, values, and roots", () => {
  for (const json of [
    '{"PlutusV1":[1]}',
    '{"-1":[1]}',
    '{"256":[1]}',
    '{"0":"invalid"}',
    '{"0":[1.5]}',
    '{"0":[9007199254740992]}',
    "[]",
    "null",
  ]) {
    assert.throws(() => CostModels.from_json(json), json);
  }
});

test("CostModels.new applies the language-id bound beyond the uint64 map contract", () => {
  const inner = MapU64ToArrI64.new();
  inner.insert(256n, BigInt64Array.from([1n]));

  assert.throws(() => CostModels.new(inner), /language id in 0\.\.255/u);
});
