import assert from "node:assert/strict";
import test from "node:test";
import {
  DeserializeError,
  Int,
  ProtocolMagic,
  bytesToHex,
  decodeCbor,
  decodeEmbeddedCbor,
  decodeProtocolMagic,
  encodeCbor,
  encodeProtocolMagic,
  hexToBytes,
} from "../dist/esm/index.js";

const hex = (value) => hexToBytes(value.replaceAll(" ", ""));
const roundTrip = (source) => assert.deepEqual(encodeCbor(decodeCbor(source)), source);

test("Int handles all four active boundary cases", () => {
  const cases = [
    ["00", "0"],
    ["1bffffffffffffffff", "18446744073709551615"],
    ["3bffffffffffffffff", "-18446744073709551616"],
    ["20", "-1"],
  ];
  for (const [encoded, decimal] of cases) {
    const value = Int.from_cbor_bytes(hex(encoded));
    assert.equal(value.to_str(), decimal);
    assert.deepEqual(value.to_cbor_bytes(), hex(encoded));
    assert.equal(Int.from_json(value.to_json()).to_str(), decimal);
  }
});

test("Int validates its full CBOR range and preserves non-minimal heads", () => {
  assert.deepEqual(Int.from_cbor_bytes(hex("1800")).to_cbor_bytes(), hex("1800"));
  assert.deepEqual(Int.from_cbor_bytes(hex("390000")).to_cbor_bytes(), hex("390000"));
  assert.deepEqual(
    encodeCbor(decodeCbor(Int.from_cbor_bytes(hex("1800")).to_cbor_bytes()), { mode: "canonical" }),
    hex("00"),
  );
  assert.throws(() => Int.new(18446744073709551616n), { name: "CardanoBoundsError" });
  assert.throws(() => Int.new(-18446744073709551617n), { name: "CardanoBoundsError" });
});

test("integer and tag heads preserve every legal encoded width", () => {
  for (const source of [
    "00", "1818", "190018", "1a00000018", "1b0000000000000018",
    "20", "3818", "390018", "3a00000018", "3b0000000000000018",
    "c0f6", "d818f6", "d90018f6", "da00000018f6", "db0000000000000018f6",
  ]) roundTrip(hex(source));
});

test("strings and containers preserve definite widths and indefinite chunks", () => {
  for (const source of [
    "40", "5800", "590000", "5a00000000", "5b0000000000000000",
    "60", "7800", "790000", "7a00000000", "7b0000000000000000",
    "80", "9800", "990000", "9a00000000", "9b0000000000000000",
    "a0", "b800", "b90000", "ba00000000", "bb0000000000000000",
    "5f4101410240ff", "7f6161616260ff", "9f011802ff", "bf0102616103ff",
  ]) roundTrip(hex(source));
});

test("common Cardano tags and embedded CBOR remain structurally available", () => {
  for (const source of ["c249010000000000000000", "c349010000000000000000", "d81e820102", "d90102820102"]) roundTrip(hex(source));
  const embedded = decodeCbor(hex("d8184101"));
  assert.deepEqual(decodeEmbeddedCbor(embedded), {
    kind: "unsigned",
    value: 1n,
    encoding: { width: 0 },
    span: { start: 0, end: 1 },
  });
  assert.throws(() => decodeEmbeddedCbor(decodeCbor(hex("01"))), { failure: "TAG_MISMATCH" });
});

test("preservation metadata falls back only for a changed subtree", () => {
  const original = decodeCbor(hex("9f5f41014102ff1800ff"));
  assert.equal(original.kind, "array");
  const changedBytes = { ...original.values[0], value: hex("010203") };
  const changed = { ...original, values: [changedBytes, original.values[1]] };
  assert.equal(bytesToHex(encodeCbor(changed)), "9f430102031800ff");
  assert.equal(bytesToHex(encodeCbor(changed, { mode: "canonical" })), "824301020300");
});

test("randomized values losslessly re-encode and canonical encoding converges", () => {
  let state = 0x9e3779b9;
  const next = () => { state = (Math.imul(state, 1664525) + 1013904223) >>> 0; return state; };
  for (let index = 0; index < 500; index += 1) {
    const n = BigInt(next());
    const width = [0, 1, 2, 4, 8][next() % 5];
    const value = n % 2n === 0n
      ? { kind: "unsigned", value: n, encoding: { width } }
      : { kind: "negative", value: -1n - n, encoding: { width } };
    const preserved = encodeCbor(value);
    assert.deepEqual(encodeCbor(decodeCbor(preserved)), preserved);
    const canonical = encodeCbor(value, { mode: "canonical" });
    assert.deepEqual(encodeCbor(decodeCbor(canonical), { mode: "canonical" }), canonical);
  }
});

test("canonical maps use encoded-key ordering and floats choose shortest exact width", () => {
  const map = decodeCbor(hex("bf616202616101ff"));
  assert.equal(bytesToHex(encodeCbor(map, { mode: "canonical" })), "a2616101616202");
  assert.equal(bytesToHex(encodeCbor(decodeCbor(hex("fb3ff8000000000000")), { mode: "canonical" })), "f93e00");
  assert.equal(bytesToHex(encodeCbor(decodeCbor(hex("fa3f8ccccd")), { mode: "canonical" })), "fa3f8ccccd");
});

test("strict decoder safely rejects trailing, truncated, invalid-break, depth, and allocation inputs", () => {
  const checks = [
    [() => decodeCbor(hex("0001")), "TRAILING_DATA"],
    [() => decodeCbor(hex("1a0000")), "TRUNCATED_INPUT"],
    [() => decodeCbor(hex("ff")), "BREAK_IN_DEFINITE_LENGTH"],
    [() => decodeCbor(hex("9f01")), "ENDING_BREAK_MISSING"],
    [() => decodeCbor(hex("bf01ff")), "INVALID_STRUCTURE"],
    [() => decodeCbor(hex("81818100"), { limits: { maxDepth: 1 } }), "DEPTH_LIMIT_EXCEEDED"],
    [() => decodeCbor(hex("5a00010000"), { limits: { maxStringBytes: 32 } }), "OUT_OF_RANGE"],
    [() => decodeCbor(hex("9a00010000"), { limits: { maxCollectionLength: 32 } }), "OUT_OF_RANGE"],
    [() => decodeCbor(hex("820001"), { limits: { maxTokens: 2 } }), "OUT_OF_RANGE"],
  ];
  for (const [operation, failure] of checks) {
    assert.throws(operation, (error) => error instanceof DeserializeError && error.failure === failure);
  }
});

test("nested decoding failures report the enclosing structural path", () => {
  assert.throws(
    () => decodeCbor(hex("82 01 81 1a0000")),
    (error) => error instanceof DeserializeError
      && error.failure === "TRUNCATED_INPUT"
      && assert.deepEqual(error.path, [1, 0]) === undefined,
  );
});

test("ProtocolMagic is a bounded uint32 with a core CBOR codec", () => {
  assert.equal(ProtocolMagic.new(764824073).to_int(), 764824073);
  assert.equal(decodeProtocolMagic(hex("1a00000001")).to_int(), 1);
  assert.deepEqual(encodeProtocolMagic(decodeProtocolMagic(hex("1a00000001"))), hex("01"));
  assert.throws(() => ProtocolMagic.new(0x1_0000_0000), { name: "CardanoBoundsError" });
});
