import assert from "node:assert/strict";
import test from "node:test";
import {
  BigInteger,
  ConstrPlutusData,
  NativeScript,
  NativeScriptList,
  PlutusData,
  PlutusDataList,
  PlutusMap,
  TransactionHash,
  TransactionInput,
} from "../../runtime/dist/esm/index.js";
import { TaggedTransactionInputSet } from "../dist/esm/era/shared/models.js";

const zeroHash = "00".repeat(32);

test("TransactionInput preserves noncanonical arrays and canonicalizes independently", () => {
  const value = TransactionInput.new(TransactionHash.from_hex(zeroHash), 5n);
  assert.equal(TransactionInput.from_json(value.to_json()).to_canonical_cbor_hex(), value.to_canonical_cbor_hex());
  const wire = `9f5820${zeroHash}1805ff`;
  const decoded = TransactionInput.from_cbor_hex(wire);
  assert.equal(decoded.to_cbor_hex(), wire);
  assert.equal(decoded.to_canonical_cbor_hex(), `825820${zeroHash}05`);
  assert.throws(() => TransactionInput.from_cbor_hex(`${wire}00`));
});

test("NativeScript nested choices preserve, canonicalize, and round-trip JSON", () => {
  const list = NativeScriptList.new();
  list.add(NativeScript.new_script_invalid_before(42n));
  const script = NativeScript.new_script_all(list);
  const wire = "9f019f9f04182affffff";
  const decoded = NativeScript.from_cbor_hex(wire);
  assert.equal(decoded.to_cbor_hex(), wire);
  assert.equal(decoded.to_canonical_cbor_hex(), script.to_canonical_cbor_hex());
  assert.equal(NativeScript.from_json(script.to_json()).to_cbor_hex(), script.to_cbor_hex());
});

test("PlutusData constructors and duplicate maps preserve wire and JSON behavior", () => {
  const integer = PlutusData.new_integer(BigInteger.from_str("42"));
  const fields = PlutusDataList.new(); fields.add(integer);
  const value = PlutusData.new_constr_plutus_data(ConstrPlutusData.new(0n, fields));
  assert.equal(PlutusData.from_json(value.to_json()).to_canonical_cbor_hex(), value.to_canonical_cbor_hex());
  const wire = "bf01020103ff";
  const map = PlutusMap.from_cbor_hex(wire);
  assert.equal(map.len(), 2);
  assert.equal(map.to_cbor_hex(), wire);
  const key = PlutusData.new_integer(BigInteger.from_str("1"));
  const replacement = PlutusData.new_integer(BigInteger.from_str("4"));
  assert.equal(map.get_all(key)?.len(), 2);
  map.set(key, replacement);
  assert.equal(map.len(), 1);
});

test("tag-258 sets preserve tag choice, head width, duplicates, and wire order", () => {
  const input = `9f5820${zeroHash}1805ff`;
  const wire = `da000001029f${input}${input}ff`;
  const set = TaggedTransactionInputSet.from_cbor_bytes(Uint8Array.from(Buffer.from(wire, "hex")));
  assert.equal(set.len(), 2);
  assert.equal(set.to_cbor_hex(), wire);
  assert.equal(set.to_canonical_cbor_hex(), `d9010282825820${zeroHash}05825820${zeroHash}05`);
  const untagged = TaggedTransactionInputSet.from_cbor_bytes(Uint8Array.from(Buffer.from(`81${input}`, "hex")));
  assert.equal(untagged.to_cbor_hex(), `81${input}`);
});
