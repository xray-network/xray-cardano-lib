import assert from "node:assert/strict";
import test from "node:test";

import * as cardano from "@xray-network/cardano-lib";
import * as multiEra from "@xray-network/cardano-chain/multi-era";
import * as cip8 from "@xray-network/cardano-cip/cip8";

test("native public values do not expose WASM memory lifecycle methods", () => {
  for (const [facade, module] of [["main", cardano], ["multi-era", multiEra], ["cip8", cip8]]) {
    for (const [name, value] of Object.entries(module)) {
      if (typeof value !== "function" || value.prototype === undefined) continue;
      let prototype = value.prototype;
      while (prototype !== null && prototype !== Object.prototype) {
        assert.equal(Object.hasOwn(prototype, "free"), false, `${facade}.${name}.free`);
        assert.equal(Object.hasOwn(prototype, Symbol.dispose), false, `${facade}.${name}[Symbol.dispose]`);
        prototype = Object.getPrototypeOf(prototype);
      }
    }
  }
  assert.equal(typeof cardano.PrivateKey.from_normal_bytes(new Uint8Array(32)).dispose, "function");
});

test("collection getters clone, mutations stay local, and bounds throw", () => {
  const input = cardano.TransactionInput.new(
    cardano.TransactionHash.from_raw_bytes(new Uint8Array(32).fill(0x11)),
    3n,
  );
  const list = cardano.TransactionInputList.new();
  list.add(input);
  const first = list.get(0);
  const second = list.get(0);
  assert.notStrictEqual(first, second);
  assert.deepEqual(first.to_cbor_bytes(), second.to_cbor_bytes());
  assert.throws(() => list.get(1), RangeError);

  const detached = first.transaction_id().to_raw_bytes();
  detached.fill(0xff);
  assert.deepEqual(first.transaction_id().to_raw_bytes(), new Uint8Array(32).fill(0x11));
});

test("recursive JSON shapes retain runtime-compatible number and byte conventions", () => {
  const value = cardano.PlutusData.from_json(JSON.stringify({
    constructor: 0,
    fields: [{ list: [{ int: 42 }, { bytes: "aabb" }] }],
  }));
  assert.deepEqual(JSON.parse(value.to_json()), {
    constructor: 0,
    fields: [{ list: [{ int: 42 }, { bytes: "aabb" }] }],
  });
  assert.throws(() => cardano.PlutusData.from_json('{"bytes":"zz"}'), /hex/u);
});
