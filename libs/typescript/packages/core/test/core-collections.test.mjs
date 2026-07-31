import assert from "node:assert/strict";
import test from "node:test";
import {
  NonEmptyMap,
  NonEmptyVec,
  OrderedMap,
  PairMap,
} from "../dist/esm/index.js";

test("NonEmptyVec preserves its minimum-one invariant", () => {
  assert.throws(() => NonEmptyVec.from([]), { code: "INVARIANT" });
  const values = NonEmptyVec.from([1, 2]);
  assert.equal(values.remove(1), 2);
  assert.throws(() => values.pop(), { code: "INVARIANT" });
  values.push(3);
  assert.deepEqual(values.toArray(), [1, 3]);
});

test("ordered and non-empty maps have stable replacement and removal semantics", () => {
  const ordered = new OrderedMap([["a", 1], ["b", 2]]);
  ordered.set("a", 3);
  assert.deepEqual([...ordered], [["a", 3], ["b", 2]]);
  const nonEmpty = NonEmptyMap.from([["a", 1], ["b", 2]]);
  assert.equal(nonEmpty.delete("missing"), false);
  assert.equal(nonEmpty.delete("b"), true);
  assert.throws(() => nonEmpty.delete("missing"), { code: "INVARIANT" });
});

test("PairMap retains duplicate keys and their insertion order", () => {
  const pairs = new PairMap([[1, "first"], [1, "second"], [2, "third"]]);
  assert.equal(pairs.get(1), "first");
  assert.deepEqual(pairs.getAll(1), ["first", "second"]);
  assert.deepEqual(pairs.toArray(), [[1, "first"], [1, "second"], [2, "third"]]);
});

test("map keys can use domain structural equality without losing insertion order", () => {
  const key = (value) => ({ value, equals(other) { return this.value === other?.value; } });
  const ordered = new OrderedMap([[key(1), "first"], [key(2), "second"]]);
  ordered.set(key(1), "replacement");
  assert.equal(ordered.size, 2);
  assert.deepEqual([...ordered].map(([entryKey, value]) => [entryKey.value, value]), [
    [1, "replacement"],
    [2, "second"],
  ]);
});
