import assert from "node:assert/strict";
import test from "node:test";
import { encodeCbor } from "../../core/dist/esm/index.js";
import * as root from "../dist/esm/index.js";
import * as focused from "../dist/esm/uplc/index.js";
import * as aggregate from "../../runtime/dist/esm/index.js";

const bytesNode=(value)=>({kind:"bytes",value,encoding:{kind:"definite",width:0}});
const encodeBytes=(value)=>encodeCbor(bytesNode(value),{mode:"canonical"});

test("SerializedPlutusScript explicitly converts raw, single-CBOR, and double-CBOR forms", () => {
  const raw=root.encodeFlatProgram(root.parseUplcText("(program 1.0.0 (con unit ()))"));
  const single=encodeBytes(raw),double=encodeBytes(single);
  const values=[
    root.SerializedPlutusScript.from_raw_flat(raw),
    root.SerializedPlutusScript.from_single_cbor(single),
    root.SerializedPlutusScript.from_double_cbor(double),
  ];
  assert.deepEqual(values.map((value)=>value.kind()),[
    root.SerializedPlutusScriptKind.RawFlat,
    root.SerializedPlutusScriptKind.SingleCbor,
    root.SerializedPlutusScriptKind.DoubleCbor,
  ]);
  for(const value of values){
    assert.deepEqual(value.to_raw_flat(),raw);
    assert.deepEqual(value.to_single_cbor(),single);
    assert.deepEqual(value.to_double_cbor(),double);
  }
  assert.deepEqual(values.map((value)=>value.bytes()),[raw,single,double]);
});

test("serialized envelopes preserve exact input form and own all byte boundaries", () => {
  const raw=root.encodeFlatProgram(root.parseUplcText("(program 1.0.0 (con unit ()))"));
  assert.ok(raw.length<256);
  const noncanonical=Uint8Array.of(0x58,raw.length,...raw);
  const source=Uint8Array.from(noncanonical);
  const script=root.SerializedPlutusScript.from_single_cbor(source);
  source.fill(0);
  assert.deepEqual(script.bytes(),noncanonical);
  assert.deepEqual(script.to_single_cbor(),noncanonical);
  const exposed=script.to_raw_flat();exposed.fill(0);
  assert.deepEqual(script.to_raw_flat(),raw);
  assert.deepEqual(script.to_double_cbor(),encodeBytes(noncanonical));
});

test("serialized envelopes reject malformed, trailing, invalid, and ambiguous forms", () => {
  const raw=root.encodeFlatProgram(root.parseUplcText("(program 1.0.0 (con unit ()))"));
  const single=encodeBytes(raw),double=encodeBytes(single),triple=encodeBytes(double);
  assert.throws(()=>root.SerializedPlutusScript.from_raw_flat(single));
  assert.throws(()=>root.SerializedPlutusScript.from_single_cbor(double));
  assert.throws(()=>root.SerializedPlutusScript.from_double_cbor(triple));
  assert.throws(()=>root.SerializedPlutusScript.from_raw_flat(Uint8Array.of(0)));
  assert.throws(()=>root.SerializedPlutusScript.from_single_cbor(Uint8Array.of(0)));
  assert.throws(()=>root.SerializedPlutusScript.from_single_cbor(Uint8Array.of(...single,0)),/trailing/i);
  assert.throws(()=>root.SerializedPlutusScript.from_double_cbor(encodeBytes(Uint8Array.of(0))));
});

test("serialized script ownership is identical across focused, root, and aggregate exports", () => {
  assert.strictEqual(focused.SerializedPlutusScript,root.SerializedPlutusScript);
  assert.strictEqual(aggregate.plutus.SerializedPlutusScript,root.SerializedPlutusScript);
  assert.strictEqual(focused.SerializedPlutusScriptKind,root.SerializedPlutusScriptKind);
  assert.strictEqual(aggregate.plutus.SerializedPlutusScriptKind,root.SerializedPlutusScriptKind);
});
