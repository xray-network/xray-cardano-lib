import assert from "node:assert/strict";
import test from "node:test";

import {
  CardanoError,
  DeserializeError,
  decodeCbor,
  encodeCbor,
} from "@xray-network/xray-cardano-lib-core";
import {
  blake2b224,
  blake2b256,
} from "@xray-network/xray-cardano-lib-crypto";
import {
  applyParamsToScript,
  decodeFlatProgram,
  encodeFlatProgram,
  evaluatePhaseTwoRaw,
  evaluateProgram,
  parseUplcText,
} from "@xray-network/xray-cardano-lib-plutus";
import * as uplc from "@xray-network/xray-cardano-lib-plutus/uplc";

const fromHex = (hex) => Uint8Array.from(hex.match(/../g)?.map((value) => Number.parseInt(value, 16)) ?? []);
const toHex = (bytes) => Array.from(bytes, (value) => value.toString(16).padStart(2, "0")).join("");
const identityScript = fromHex("4d01000033222220051200120011");

test("camelCase phase-two APIs are available from the Plutus package", () => {
  assert.equal(toHex(applyParamsToScript(fromHex("80"), identityScript)), toHex(identityScript));
});

test("public UPLC APIs expose text parsing, Flat codecs, and budgeted evaluation", () => {
  assert.strictEqual(uplc.parseUplcText, parseUplcText);
  assert.strictEqual(uplc.decodeFlatProgram, decodeFlatProgram);
  assert.strictEqual(uplc.encodeFlatProgram, encodeFlatProgram);
  assert.strictEqual(uplc.evaluateProgram, evaluateProgram);

  const program = parseUplcText("(program 1.0.0 (con unit ()))");
  const flat = encodeFlatProgram(program);
  assert.deepEqual(decodeFlatProgram(flat), program);
  const result = evaluateProgram(
    program,
    [],
    { cpu: 1_000_000n, memory: 1_000_000n },
  );
  assert.equal(result.isUnit, true);
  assert.equal(result.budget.cpu > 0n, true);
  assert.equal(result.budget.memory > 0n, true);
});

test("applyParamsToScript preserves and canonicalizes an empty application", () => {
  const result = applyParamsToScript(fromHex("80"), identityScript);
  assert.equal(toHex(result), toHex(identityScript));
  assert.notEqual(result, identityScript);
});

test("applyParamsToScript applies Data constants left-to-right", () => {
  assert.equal(
    toHex(applyParamsToScript(fromHex("81182a"), identityScript)),
    "54010000333222220051200120014c0102182a0001",
  );
  assert.equal(
    toHex(applyParamsToScript(fromHex("820102"), identityScript)),
    "58180100003333222220051200120014c10101004c0101020001",
  );
});

test("applyParamsToScript rejects non-arrays, free variables, and double wrapping", () => {
  assert.throws(
    () => applyParamsToScript(fromHex("01"), identityScript),
    (error) => error instanceof DeserializeError && error.code === "DESERIALIZE",
  );
  assert.throws(
    () => applyParamsToScript(fromHex("80"), fromHex("4443010001")),
    (error) => error instanceof DeserializeError,
  );
  assert.throws(
    () => applyParamsToScript(fromHex("80"), fromHex("4e4d01000033222220051200120011")),
    (error) => error instanceof DeserializeError,
  );
});

test("evaluatePhaseTwoRaw accepts a structurally valid transaction without redeemers", () => {
  const tx = fromHex("84a0a0f5f6");
  assert.deepEqual(
    evaluatePhaseTwoRaw(tx, [], fromHex("a0"), [10_000_000n, 10_000_000n], [0n, 0n, 1_000n], 9, true),
    [],
  );
});

test("evaluatePhaseTwoRaw rejects unsupported protocol majors", () => {
  assert.throws(
    () => evaluatePhaseTwoRaw(
      fromHex("84a0a0f5f6"),
      [],
      fromHex("a0"),
      [10_000_000n, 10_000_000n],
      [0n, 0n, 1_000n],
      12,
      false,
    ),
    (error) => error instanceof CardanoError && error.code === "UNSUPPORTED",
  );
});

test("evaluatePhaseTwoRaw resolves an Alonzo spending script and rewrites ExUnits", async () => {
  const { encodeProgramEnvelope } = await import("../dist/esm/uplc/flat.js");
  const unit = { kind: "constant", constant: { type: { kind: "unit" }, value: null } };
  const script = encodeProgramEnvelope({
    version: [1n, 0n, 0n],
    term: { kind: "lambda", body: { kind: "lambda", body: { kind: "lambda", body: unit } } },
  });
  const scriptHash = blake2b224(Uint8Array.of(1, ...script));
  const input = array([bytes(new Uint8Array(32)), unsigned(0n)]);
  const inputBytes = encodeCbor(input);
  const datum = unsigned(42n);
  const address = Uint8Array.of(0x70, ...scriptHash);
  const output = array([bytes(address), unsigned(1_000_000n), bytes(blake2b256(encodeCbor(datum)))]);
  const body = map([
    [unsigned(0n), array([input])],
    [unsigned(1n), array([])],
    [unsigned(2n), unsigned(0n)],
  ]);
  const redeemer = array([
    unsigned(0n),
    unsigned(0n),
    unsigned(7n),
    array([unsigned(0n), unsigned(0n)]),
  ]);
  const witnesses = map([
    [unsigned(3n), array([bytes(script)])],
    [unsigned(4n), array([datum])],
    [unsigned(5n), array([redeemer])],
  ]);
  const transaction = array([body, witnesses, { kind: "boolean", value: true }, { kind: "null" }]);
  const parameters = Array.from({ length: 332 }, () => 1n);
  for (const [index, value] of [
    [17, 16_000n], [18, 100n], [19, 23_000n], [20, 100n],
    [21, 23_000n], [22, 100n], [23, 23_000n], [24, 100n],
    [25, 23_000n], [26, 100n], [27, 23_000n], [28, 100n],
    [29, 100n], [30, 100n], [31, 23_000n], [32, 100n],
  ]) parameters[index] = value;
  const costModels = map([[unsigned(0n), array(parameters.map(unsigned))]]);

  assert.throws(
    () => evaluatePhaseTwoRaw(
      encodeCbor(transaction),
      [[inputBytes, encodeCbor(output)]],
      encodeCbor(map([])),
      [10_000_000n, 10_000_000n],
      [0n, 0n, 1_000n],
      5,
      true,
    ),
    /missing Plutus V1 cost model/u,
  );

  const [result] = evaluatePhaseTwoRaw(
    encodeCbor(transaction),
    [[inputBytes, encodeCbor(output)]],
    encodeCbor(costModels),
    [10_000_000n, 10_000_000n],
    [0n, 0n, 1_000n],
    5,
    true,
  );
  assert.deepEqual(result[1], {
    cost: { cpu: 209_100n, memory: 1_100n },
    logs: [],
  });
  const rewritten = decodeCbor(result[0]);
  assert.equal(rewritten.kind, "array");
  assert.deepEqual(
    rewritten.values[3].values.map((value) => value.value),
    [1_100n, 209_100n],
  );
});

test("evaluatePhaseTwoRaw selects Babbage V2 and Conway V3 argument conventions", async () => {
  const { encodeProgramEnvelope } = await import("../dist/esm/uplc/flat.js");
  const unit = { kind: "constant", constant: { type: { kind: "unit" }, value: null } };
  const cases = [
    { language: 1, protocol: 7, lambdas: 2 },
    { language: 2, protocol: 9, lambdas: 1 },
  ];
  for (const { language, protocol, lambdas } of cases) {
    let term = unit;
    for (let index = 0; index < lambdas; index += 1) term = { kind: "lambda", body: term };
    const script = encodeProgramEnvelope({ version: [1n, 0n, 0n], term });
    const scriptHash = blake2b224(Uint8Array.of(language + 1, ...script));
    const input = array([bytes(new Uint8Array(32).fill(language)), unsigned(0n)]);
    const output = map([
      [unsigned(0n), bytes(Uint8Array.of(0x70, ...scriptHash))],
      [unsigned(1n), unsigned(1_000_000n)],
    ]);
    const body = map([
      [unsigned(0n), language === 2 ? tag(258n, array([input])) : array([input])],
      [unsigned(1n), array([])],
      [unsigned(2n), unsigned(0n)],
    ]);
    const redeemer = array([
      unsigned(0n),
      unsigned(0n),
      unsigned(7n),
      array([unsigned(999n), unsigned(999n)]),
    ]);
    const witnesses = map([
      [
        unsigned(language === 1 ? 6n : 7n),
        language === 2 ? tag(258n, array([bytes(script)])) : array([bytes(script)]),
      ],
      [unsigned(5n), array([redeemer])],
    ]);
    const parameters = Array.from({ length: language === 2 ? 350 : 332 }, () => 1n);
    const [result] = evaluatePhaseTwoRaw(
      encodeCbor(array([body, witnesses, { kind: "boolean", value: true }, { kind: "null" }])),
      [[encodeCbor(input), encodeCbor(output)]],
      encodeCbor(map([[unsigned(BigInt(language)), array(parameters.map(unsigned))]])),
      [10_000_000n, 10_000_000n],
      [0n, 0n, 1_000n],
      protocol,
      true,
    );
    assert.equal(result[1].cost.cpu > 0n, true);
    assert.equal(result[1].cost.memory > 0n, true);
    assert.deepEqual(decodeCbor(result[0]).values[3].values.map((value) => value.value), [
      result[1].cost.memory,
      result[1].cost.cpu,
    ]);

    if (language === 2) {
      const nonUnitScript = encodeProgramEnvelope({
        version: [1n, 0n, 0n],
        term: { kind: "lambda", body: {
          kind: "constant",
          constant: { type: { kind: "integer" }, value: 1n },
        } },
      });
      const nonUnitHash = blake2b224(Uint8Array.of(3, ...nonUnitScript));
      const nonUnitOutput = map([
        [unsigned(0n), bytes(Uint8Array.of(0x70, ...nonUnitHash))],
        [unsigned(1n), unsigned(1_000_000n)],
      ]);
      const nonUnitWitnesses = map([
        [unsigned(7n), tag(258n, array([bytes(nonUnitScript)]))],
        [unsigned(5n), array([redeemer])],
      ]);
      assert.throws(
        () => evaluatePhaseTwoRaw(
          encodeCbor(array([
            body,
            nonUnitWitnesses,
            { kind: "boolean", value: true },
            { kind: "null" },
          ])),
          [[encodeCbor(input), encodeCbor(nonUnitOutput)]],
          encodeCbor(map([[unsigned(2n), array(parameters.map(unsigned))]])),
          [10_000_000n, 10_000_000n],
          [0n, 0n, 1_000n],
          9,
          false,
        ),
        /non-Unit/u,
      );
    }
  }
});

test("evaluatePhaseTwoRaw collection checks reject extra pointers and missing UTxOs", () => {
  const input = array([bytes(new Uint8Array(32)), unsigned(0n)]);
  const body = map([
    [unsigned(0n), array([input])],
    [unsigned(1n), array([])],
    [unsigned(2n), unsigned(0n)],
  ]);
  const redeemer = array([
    unsigned(0n),
    unsigned(0n),
    unsigned(7n),
    array([unsigned(0n), unsigned(0n)]),
  ]);
  const transaction = encodeCbor(array([
    body,
    map([[unsigned(5n), array([redeemer])]]),
    { kind: "boolean", value: true },
    { kind: "null" },
  ]));

  assert.throws(
    () => evaluatePhaseTwoRaw(
      transaction,
      [],
      encodeCbor(map([])),
      [10_000_000n, 10_000_000n],
      [0n, 0n, 1_000n],
      5,
      true,
    ),
    /extra redeemer 0:0/u,
  );
  assert.throws(
    () => evaluatePhaseTwoRaw(
      transaction,
      [],
      encodeCbor(map([])),
      [10_000_000n, 10_000_000n],
      [0n, 0n, 1_000n],
      5,
      false,
    ),
    /missing spending UTxO/u,
  );
});

const definite = { kind: "definite", width: 0 };
const unsigned = (value) => ({ kind: "unsigned", value, encoding: { width: 0 } });
const bytes = (value) => ({ kind: "bytes", value: Uint8Array.from(value), encoding: definite });
const array = (values) => ({ kind: "array", values, encoding: definite });
const map = (entries) => ({ kind: "map", entries, encoding: definite });
const tag = (tagValue, value) => ({ kind: "tag", tag: tagValue, value, encoding: { width: 0 } });
