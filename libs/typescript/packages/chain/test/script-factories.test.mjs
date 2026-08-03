import assert from "node:assert/strict";
import test from "node:test";
import { decodeCbor, encodeCbor } from "../../core/dist/esm/index.js";
import { Ed25519KeyHash } from "../../crypto/dist/esm/index.js";
import {
  NativeScript,
  PlutusV1Script,
  PlutusV2Script,
  PlutusV3Script,
  Script,
  ScriptKind,
  ScriptRef,
} from "../dist/esm/index.js";

const bytes = (value) => ({kind:"bytes",value,encoding:{kind:"definite",width:0}});

test("typed Script factories and accessors cover every ledger variant", () => {
  const native=NativeScript.new_script_pubkey(Ed25519KeyHash.from_raw_bytes(new Uint8Array(28).fill(1)));
  const v1=PlutusV1Script.new(Uint8Array.of(1));
  const v2=PlutusV2Script.new(Uint8Array.of(2));
  const v3=PlutusV3Script.new(Uint8Array.of(3));
  const scripts=[
    [Script.new_native(native),ScriptKind.Native,"as_native",native.to_cbor_bytes()],
    [Script.new_plutus_v1(v1),ScriptKind.PlutusV1,"as_plutus_v1",v1.to_cbor_bytes()],
    [Script.new_plutus_v2(v2),ScriptKind.PlutusV2,"as_plutus_v2",v2.to_cbor_bytes()],
    [Script.new_plutus_v3(v3),ScriptKind.PlutusV3,"as_plutus_v3",v3.to_cbor_bytes()],
  ];
  for(const [script,kind,accessor,expected] of scripts){
    assert.equal(script.kind(),kind);
    assert.deepEqual(script[accessor]().to_cbor_bytes(),expected);
  }
  assert.equal(scripts[0][0].as_plutus_v1(),undefined);
  assert.equal(scripts[1][0].as_native(),undefined);
});

test("ScriptRef.new owns an exact tag-24 embedded Script and exposes it defensively", () => {
  const source=Uint8Array.of(1,2,3);
  const script=Script.new_plutus_v1(PlutusV1Script.new(source));
  source[0]=9;
  const reference=ScriptRef.new(script);
  const decoded=decodeCbor(reference.to_cbor_bytes());
  assert.equal(decoded.kind,"tag");assert.equal(decoded.tag,24n);assert.equal(decoded.value.kind,"bytes");
  assert.deepEqual(decoded.value.value,script.to_cbor_bytes());
  const first=reference.script().as_plutus_v1().to_raw_bytes();first[0]=8;
  assert.deepEqual(reference.script().as_plutus_v1().to_raw_bytes(),Uint8Array.of(1,2,3));
  assert.deepEqual(ScriptRef.new_script(script).script().to_cbor_bytes(),script.to_cbor_bytes());
});

test("typed scripts preserve accepted wire bytes and reject malformed kinds and references", () => {
  const preserved=Uint8Array.of(0x82,0x18,0x01,0x41,0xaa);
  const script=Script.from_cbor_bytes(preserved);
  assert.deepEqual(script.to_cbor_bytes(),preserved);
  assert.equal(script.kind(),ScriptKind.PlutusV1);
  assert.equal(script.to_canonical_cbor_hex(),"820141aa");
  assert.throws(()=>Script.from_cbor_bytes(encodeCbor({kind:"array",values:[{kind:"unsigned",value:4n,encoding:{width:0}},bytes(Uint8Array.of())],encoding:{kind:"definite",width:0}})),/Script/);
  assert.throws(()=>ScriptRef.from_cbor_bytes(encodeCbor({kind:"tag",tag:23n,value:bytes(script.to_cbor_bytes()),encoding:{width:0}})),/ScriptRef/);
  assert.throws(()=>ScriptRef.from_cbor_bytes(encodeCbor({kind:"tag",tag:24n,value:bytes(Uint8Array.of(0)),encoding:{width:0}})).script(),/CBOR|Script/);
});
