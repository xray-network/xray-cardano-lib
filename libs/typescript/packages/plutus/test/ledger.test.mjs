import assert from "node:assert/strict";
import test from "node:test";

import { encodeCbor } from "@xray-network/xray-cardano-lib-core";
import { blake2b256 } from "@xray-network/xray-cardano-lib-crypto";

const definite = { kind: "definite", width: 0 };
const unsigned = (value) => ({ kind: "unsigned", value, encoding: { width: 0 } });
const bytes = (value) => ({ kind: "bytes", value: Uint8Array.from(value), encoding: definite });
const array = (values) => ({ kind: "array", values, encoding: definite });
const map = (entries) => ({ kind: "map", entries, encoding: definite });
const fields = (value) => {
  assert.equal(value.kind, "tag");
  assert.equal(value.value.kind, "array");
  return value.value.values;
};

test("ledger context translation uses official V1, V2, and V3 Data layouts", async () => {
  const { makeScriptContext } = await import("../dist/esm/ledger/context.js");
  const transactionId = new Uint8Array(32).fill(1);
  const input = array([bytes(transactionId), unsigned(2n)]);
  const address = Uint8Array.of(0x60, ...new Uint8Array(28).fill(3));
  const alonzoOutput = array([bytes(address), unsigned(4_000_000n)]);
  const babbageOutput = map([
    [unsigned(0n), bytes(address)],
    [unsigned(1n), unsigned(4_000_000n)],
  ]);
  const body = map([
    [unsigned(0n), array([input])],
    [unsigned(1n), array([babbageOutput])],
    [unsigned(2n), unsigned(170_000n)],
    [unsigned(3n), unsigned(20n)],
    [unsigned(8n), unsigned(10n)],
  ]);
  const witnesses = map([]);
  const redeemer = { tag: 0, index: 0n, data: unsigned(7n) };

  const v1 = makeScriptContext(
    redeemer,
    [redeemer],
    body,
    witnesses,
    [{ input, output: alonzoOutput }],
    [1_000n, 0n, 100n],
    5,
    0,
  );
  const [v1Info, v1Purpose] = fields(v1);
  assert.equal(fields(v1Info).length, 10);
  assert.equal(v1Purpose.tag, 122n);
  const v1InfoFields = fields(v1Info);
  const inputInfo = v1InfoFields[0].values[0];
  const outputReference = fields(inputInfo)[0];
  const wrappedTransactionId = fields(outputReference)[0];
  assert.deepEqual(fields(wrappedTransactionId)[0].value, transactionId);
  assert.deepEqual(
    fields(v1InfoFields[9])[0].value,
    blake2b256(encodeCbor(body, { mode: "preserve" })),
  );

  const v2 = makeScriptContext(
    redeemer,
    [redeemer],
    body,
    witnesses,
    [{ input, output: babbageOutput }],
    [1_000n, 0n, 100n],
    7,
    1,
  );
  assert.equal(fields(fields(v2)[0]).length, 12);
  assert.equal(fields(v2)[1].tag, 122n);

  const v3 = makeScriptContext(
    redeemer,
    [redeemer],
    body,
    witnesses,
    [{ input, output: babbageOutput }],
    [1_000n, 0n, 100n],
    9,
    2,
  );
  const [v3Info, v3Redeemer, v3ScriptInfo] = fields(v3);
  assert.equal(fields(v3Info).length, 16);
  assert.equal(v3Redeemer.value, 7n);
  assert.equal(v3ScriptInfo.tag, 122n);
  assert.equal(fields(v3ScriptInfo).length, 2);

  const lower = fields(fields(v1InfoFields[6])[0])[0];
  const upper = fields(fields(v1InfoFields[6])[1])[0];
  assert.equal(fields(lower)[0].value, 2_000n);
  assert.equal(fields(upper)[0].value, 3_000n);
});
