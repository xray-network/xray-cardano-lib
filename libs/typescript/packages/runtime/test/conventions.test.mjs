import assert from "node:assert/strict";
import test from "node:test";

import {
  CardanoBoundsError,
  bytesEqual,
  copyBytes,
} from "@xray-network/cardano-core";
import { secureRandomBytes } from "@xray-network/cardano-crypto";

test("byte helpers use Uint8Array and defensive copies", () => {
  const original = Uint8Array.of(1, 2, 3);
  const copy = copyBytes(original);
  assert.ok(copy instanceof Uint8Array);
  assert.notStrictEqual(copy, original);
  assert.ok(bytesEqual(copy, original));
  copy[0] = 9;
  assert.equal(original[0], 1);
});

test("secure randomness is browser-compatible and injectable", () => {
  const bytes = secureRandomBytes(4, {
    fill(target) {
      target.fill(7);
    },
  });
  assert.deepEqual(bytes, Uint8Array.of(7, 7, 7, 7));
  assert.throws(() => secureRandomBytes(-1), CardanoBoundsError);
});
