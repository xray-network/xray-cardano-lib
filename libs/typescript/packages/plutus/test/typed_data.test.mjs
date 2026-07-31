import assert from "node:assert/strict";
import test from "node:test";

import {
  Constr,
  Data,
} from "@xray-network/cardano-plutus/data";

test("native Data codec round-trips every Plutus data variant", () => {
  const value = new Constr(3, [
    42n,
    "deadbeef",
    [1n, "00"],
    new Map([
      ["aa", 7n],
      [8n, new Constr(0, [])],
    ]),
  ]);

  const encoded = Data.to(value);
  assert.deepEqual(Data.from(encoded), value);
  assert.equal(Data.void(), "d87980");
});

test("native Data schemas cast objects, enums, nullable values, and collections", () => {
  const person = Data.Object({
    name: Data.Bytes({ minLength: 1 }),
    age: Data.Integer({ minimum: 0, maximum: 150 }),
    tags: Data.Array(Data.Bytes(), { maxItems: 2, uniqueItems: true }),
  });
  const personValue = { name: "616c696365", age: 42n, tags: ["01", "02"] };
  assert.deepEqual(Data.from(Data.to(personValue, person), person), personValue);
  assert.throws(() => Data.to({ ...personValue, age: 151n }, person), /above maximum/u);
  assert.throws(() => Data.to({ ...personValue, tags: ["01", "01"] }, person), /duplicate/u);

  const action = Data.Enum([
    Data.Literal("Stop"),
    Data.Object({ Transfer: Data.Tuple([Data.Bytes(), Data.Integer()]) }),
  ]);
  assert.equal(Data.from(Data.to("Stop", action), action), "Stop");
  const transfer = { Transfer: ["abcd", 5n] };
  assert.deepEqual(Data.from(Data.to(transfer, action), action), transfer);

  const result = Data.Enum([
    Data.Object({
      Complete: Data.Object({
        transactionId: Data.Bytes(),
        outputIndex: Data.Integer(),
      }),
    }),
  ]);
  const complete = { Complete: { transactionId: "abcd", outputIndex: 1n } };
  assert.deepEqual(Data.from(Data.to(complete, result), result), complete);

  const optional = Data.Nullable(Data.Boolean());
  assert.equal(Data.from(Data.to(true, optional), optional), true);
  assert.equal(Data.from(Data.to(null, optional), optional), null);

  const balances = Data.Map(Data.Bytes(), Data.Integer(), { minItems: 1 });
  const map = new Map([["aa", 1n]]);
  assert.deepEqual(Data.from(Data.to(map, balances), balances), map);
});

test("native Data JSON conversion is browser-safe and preserves large integers", () => {
  const value = Data.fromJson({
    owner: "alice",
    token: "0xdeadbeef",
    amount: 9_007_199_254_740_993n,
  });
  assert.deepEqual({ ...Data.toJson(value) }, {
    owner: "alice",
    token: "0xdeadbeef",
    amount: 9_007_199_254_740_993n,
  });
});
