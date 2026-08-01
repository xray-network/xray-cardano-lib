import {
  INT64_MAX,
  INT64_MIN,
  Int,
  UINT64_MAX,
  bytesEqual,
  bytesToHex,
  copyBytes,
  decodeCbor,
  encodeCbor,
  hexToBytes,
} from "@xray-network/xray-cardano-lib-core";
import type { CborValue } from "@xray-network/xray-cardano-lib-core";
import { AnchorDocHash, DatumHash, Ed25519KeyHash, ScriptHash } from "@xray-network/xray-cardano-lib-crypto";
import { Address } from "../../address/index.js";
import type { CostModelsJSON } from "../shared/json-types.js";
import { validateConwayModel } from "./validation.js";

export type ConwayWireShape = "alias" | "array" | "map" | "tag" | "choice" | "external" | "group";
export type ConwayInput = ConwayData | { to_cbor_bytes(): Uint8Array } | Uint8Array | bigint | number | string | boolean | null | readonly ConwayInput[];
type ConwayConstructor<T extends ConwayData> = { new(node: CborValue): T; readonly name: string; validateNode(node: CborValue): void };

function uintNode(value: bigint): CborValue {
  if (value < 0n || value > UINT64_MAX) throw new RangeError("value must fit uint64");
  return { kind: "unsigned", value, encoding: { width: 0 } };
}

function integerNode(value: bigint): CborValue {
  if (value >= 0n) return { kind: "unsigned", value, encoding: { width: 0 } };
  return { kind: "negative", value, encoding: { width: 0 } };
}

function inputNode(value: ConwayInput): CborValue {
  if (value instanceof ConwayData) return decodeCbor(value.to_cbor_bytes());
  if (value instanceof Uint8Array) return { kind: "bytes", value: copyBytes(value), encoding: { kind: "definite", width: 0 } };
  if (typeof value === "object" && value !== null && "to_cbor_bytes" in value) return decodeCbor(value.to_cbor_bytes());
  if (typeof value === "bigint") return integerNode(value);
  if (typeof value === "number") {
    if (!Number.isSafeInteger(value)) throw new RangeError("numeric CBOR inputs must be safe integers");
    return integerNode(BigInt(value));
  }
  if (typeof value === "string") return { kind: "text", value, encoding: { kind: "definite", width: 0 } };
  if (typeof value === "boolean") return { kind: "boolean", value };
  if (value === null) return { kind: "null" };
  return { kind: "array", values: value.map(inputNode), encoding: { kind: "definite", width: 0 } };
}

function modelNode(shape: ConwayWireShape, values: readonly ConwayInput[]): CborValue {
  if (shape === "map") {
    if (values.length % 2 !== 0) throw new TypeError("model map constructors require key/value pairs");
    const entries: Array<readonly [CborValue,CborValue]> = [];
    for (let index=0;index<values.length;index+=2) entries.push([inputNode(values[index] as ConwayInput),inputNode(values[index+1] as ConwayInput)]);
    return { kind:"map",entries,encoding:{kind:"definite",width:0} };
  }
  if ((shape === "alias" || shape === "external" || shape === "choice") && values.length === 1) return inputNode(values[0] as ConwayInput);
  return { kind:"array",values:values.map(inputNode),encoding:{kind:"definite",width:0} };
}

function jsonNode(value: unknown): CborValue {
  if (value === null) return { kind: "null" };
  if (typeof value === "boolean") return { kind: "boolean", value };
  if (typeof value === "number" && Number.isSafeInteger(value)) return integerNode(BigInt(value));
  if (typeof value === "string") return { kind: "text", value, encoding: { kind: "definite", width: 0 } };
  if (Array.isArray(value)) return { kind: "array", values: value.map(jsonNode), encoding: { kind: "definite", width: 0 } };
  if (typeof value === "object" && value !== null) {
    return {
      kind: "map",
      entries: Object.entries(value).map(([key, item]) => [
        { kind: "text", value: key, encoding: { kind: "definite", width: 0 } },
        jsonNode(item),
      ]),
      encoding: { kind: "definite", width: 0 },
    };
  }
  throw new TypeError("JSON value cannot be represented as Conway CBOR");
}

function nodeJson(node: CborValue): unknown {
  switch (node.kind) {
    case "unsigned": case "negative": return node.value <= BigInt(Number.MAX_SAFE_INTEGER) && node.value >= BigInt(Number.MIN_SAFE_INTEGER) ? Number(node.value) : node.value.toString();
    case "bytes": return bytesToHex(node.value);
    case "text": case "boolean": case "float": return node.value;
    case "null": return null;
    case "undefined": return undefined;
    case "simple": return node.value;
    case "array": return node.values.map(nodeJson);
    case "map": return node.entries.map(([key, value]) => ({ k: nodeJson(key), v: nodeJson(value) }));
    case "tag": return { tag: Number(node.tag), value: nodeJson(node.value) };
  }
}

function validateShape(node: CborValue, shape: ConwayWireShape, name: string): void {
  const valid = shape === "choice" || shape === "external" || shape === "alias"
    || (shape === "array" && node.kind === "array")
    || (shape === "group" && node.kind === "array")
    || (shape === "map" && node.kind === "map")
    || (shape === "tag" && node.kind === "tag");
  if (!valid) throw new TypeError(`${name} requires a ${shape} CBOR value`);
}

/** Shared lossless codec used by Conway record and choice classes. */
export class ConwayData {
  protected static readonly wireShape: ConwayWireShape = "choice";
  #node: CborValue;
  public constructor(node: CborValue) { this.#node = node; }
  public static validateNode(node: CborValue): void {
    if (!validateConwayModel(this.name, node)) validateShape(node, this.wireShape, this.name);
  }
  public static from_cbor_bytes<T extends ConwayData>(this: ConwayConstructor<T>, bytes: Uint8Array): T { const node=decodeCbor(bytes);this.validateNode(node);return new this(node); }
  public static from_cbor_hex<T extends ConwayData>(this: ConwayConstructor<T>, hex: string): T { const node=decodeCbor(hexToBytes(hex));this.validateNode(node);return new this(node); }
  public static from_json<T extends ConwayData>(this: ConwayConstructor<T>, json: string): T { const node=jsonNode(JSON.parse(json));this.validateNode(node);return new this(node); }
  protected cborNode(): CborValue { return this.#node; }
  protected replaceCborNode(node: CborValue): void { this.#node=node; }
  public to_cbor_bytes(): Uint8Array { return encodeCbor(this.#node); }
  public to_cbor_hex(): string { return bytesToHex(this.to_cbor_bytes()); }
  public to_canonical_cbor_bytes(): Uint8Array { return encodeCbor(this.#node, { mode: "canonical" }); }
  public to_canonical_cbor_hex(): string { return bytesToHex(this.to_canonical_cbor_bytes()); }
  public to_js_value(): unknown { return nodeJson(this.#node); }
  public to_json(): string { return JSON.stringify(this.to_js_value(), null, 2); }
}

abstract class RawBytesValue extends ConwayData {
  protected static readonly byteLength: number | readonly [number, number] | undefined = undefined;
  public constructor(node: CborValue) { super(node); }
  public static override validateNode(node: CborValue): void {
    if (node.kind !== "bytes") throw new TypeError(`${this.name} requires CBOR bytes`);
    const bound = this.byteLength;
    const valid = bound === undefined || (typeof bound === "number" ? node.value.length === bound : node.value.length >= bound[0] && node.value.length <= bound[1]);
    if (!valid) throw new RangeError(`${this.name} byte length is outside its CDDL bound`);
  }
  public static new<T extends RawBytesValue>(this: ConwayConstructor<T>, bytes: Uint8Array): T { const node:CborValue={ kind: "bytes", value: copyBytes(bytes), encoding: { kind: "definite", width: 0 } };this.validateNode(node);return new this(node); }
  public static from_raw_bytes<T extends RawBytesValue>(this: ConwayConstructor<T>, bytes: Uint8Array): T {
    const node: CborValue={ kind: "bytes", value: copyBytes(bytes), encoding: { kind: "definite", width: 0 } };this.validateNode(node);return new this(node);
  }
  public static from_hex<T extends RawBytesValue>(this: ConwayConstructor<T>, hex: string): T { const node:CborValue={kind:"bytes",value:hexToBytes(hex),encoding:{kind:"definite",width:0}};this.validateNode(node);return new this(node); }
  public get(): Uint8Array { return this.to_raw_bytes(); }
  public to_raw_bytes(): Uint8Array { const node = this.cborNode(); if (node.kind !== "bytes") throw new TypeError("invalid byte value"); return copyBytes(node.value); }
  public to_hex(): string { return bytesToHex(this.to_raw_bytes()); }
  public override to_js_value(): string { return this.to_hex(); }
}

export class AssetName extends RawBytesValue {
  protected static override readonly byteLength = [0, 32] as const;
  public static from_str(value: string): AssetName { return AssetName.from_raw_bytes(new TextEncoder().encode(value)); }
  public to_str(): string { return new TextDecoder("utf-8", { fatal: true }).decode(this.to_raw_bytes()); }
}
export class KESSignature extends RawBytesValue { protected static override readonly byteLength = 448; }
export class Ipv4 extends RawBytesValue {
  protected static override readonly byteLength = 4;
  public static from_str(value: string): Ipv4 { const parts=value.split(".");if(parts.length!==4)throw new TypeError("invalid IPv4 address");const bytes=parts.map((part)=>{if(!/^\d{1,3}$/u.test(part))throw new TypeError("invalid IPv4 address");const byte=Number(part);if(byte>255)throw new TypeError("invalid IPv4 address");return byte;});return Ipv4.new(Uint8Array.from(bytes)); }
  public static override from_json<T extends ConwayData>(this: ConwayConstructor<T>,json: string): T { const value:unknown=JSON.parse(json);if(typeof value!=="string")throw new TypeError("IPv4 JSON must be a string");const node=decodeCbor(Ipv4.from_str(value).to_cbor_bytes());this.validateNode(node);return new this(node); }
  public override to_js_value(): string { return [...this.to_raw_bytes()].join("."); }
}
export class Ipv6 extends RawBytesValue {
  protected static override readonly byteLength = 16;
  public static from_str(value: string): Ipv6 { if(value.indexOf("::")!==value.lastIndexOf("::"))throw new TypeError("invalid IPv6 address");const parse=(part:string):number[]=>part===""?[]:part.split(":").map((item)=>{if(!/^[0-9a-f]{1,4}$/iu.test(item))throw new TypeError("invalid IPv6 address");return Number.parseInt(item,16);});const [leftText,rightText]=value.includes("::")?value.split("::"):[value,undefined];const left=parse(leftText??"");const right=rightText===undefined?[]:parse(rightText);if((rightText===undefined&&left.length!==8)||(rightText!==undefined&&left.length+right.length>=8))throw new TypeError("invalid IPv6 address");const words=rightText===undefined?left:[...left,...Array.from({length:8-left.length-right.length},()=>0),...right];return Ipv6.new(Uint8Array.from(words.flatMap((word)=>[word>>>8,word&255]))); }
  public static override from_json<T extends ConwayData>(this: ConwayConstructor<T>,json: string): T { const value:unknown=JSON.parse(json);if(typeof value!=="string")throw new TypeError("IPv6 JSON must be a string");const node=decodeCbor(Ipv6.from_str(value).to_cbor_bytes());this.validateNode(node);return new this(node); }
  public override to_js_value(): string { const bytes=this.to_raw_bytes();const words=Array.from({length:8},(_,index)=>((bytes[index*2]??0)<<8)|(bytes[index*2+1]??0));let bestStart=-1,bestLength=0,currentStart=-1;for(let index=0;index<=words.length;index+=1){if(index<words.length&&words[index]===0){if(currentStart<0)currentStart=index;}else if(currentStart>=0){const length=index-currentStart;if(length>bestLength){bestStart=currentStart;bestLength=length;}currentStart=-1;}}const text=words.map((word)=>word.toString(16));if(bestLength<2)return text.join(":");const left=text.slice(0,bestStart).join(":");const right=text.slice(bestStart+bestLength).join(":");return `${left}::${right}`; }
}
export class PlutusV1Script extends RawBytesValue {}
export class PlutusV2Script extends RawBytesValue {}
export class PlutusV3Script extends RawBytesValue {}

abstract class TextValue extends ConwayData {
  protected static readonly maxBytes = 128;
  public constructor(node: CborValue) { super(node); }
  public static override validateNode(node: CborValue): void {
    if (node.kind !== "text") throw new TypeError(`${this.name} requires CBOR text`);
    if (new TextEncoder().encode(node.value).length > this.maxBytes) throw new RangeError(`${this.name} exceeds its CDDL UTF-8 bound`);
  }
  public static new<T extends TextValue>(this: ConwayConstructor<T>, value: string): T { const node:CborValue={ kind: "text", value, encoding: { kind: "definite", width: 0 } };this.validateNode(node);return new this(node); }
  public get(): string { const node = this.cborNode(); if (node.kind !== "text") throw new TypeError("invalid text value"); return node.value; }
  public override to_js_value(): string { return this.get(); }
}
export class DNSName extends TextValue {}
export class Url extends TextValue {}

export class Ed25519KeyHashList {
  readonly #values:Ed25519KeyHash[]=[];
  public static new():Ed25519KeyHashList{return new Ed25519KeyHashList();}
  public len():number{return this.#values.length;}
  public get(index:number):Ed25519KeyHash{const value=this.#values[index];if(value===undefined)throw new RangeError("Ed25519KeyHashList index is out of range");return Ed25519KeyHash.from_raw_bytes(value.to_raw_bytes());}
  public add(value:Ed25519KeyHash):void{this.#values.push(Ed25519KeyHash.from_raw_bytes(value.to_raw_bytes()));}
}

export class NetworkId extends ConwayData {
  public constructor(node: CborValue) { super(node); }
  public static override validateNode(node: CborValue): void {
    if (node.kind !== "unsigned" || node.value > 1n) throw new RangeError("NetworkId must be 0 or 1");
  }
  public static new(value: bigint): NetworkId { const node=uintNode(value);NetworkId.validateNode(node);return new NetworkId(node); }
  public static mainnet(): NetworkId { return NetworkId.new(1n); }
  public static testnet(): NetworkId { return NetworkId.new(0n); }
  public get(): bigint { const node = this.cborNode(); if (node.kind !== "unsigned") throw new TypeError("invalid network id"); return node.value; }
  public override to_js_value(): number | string { const value = this.get(); return value <= BigInt(Number.MAX_SAFE_INTEGER) ? Number(value) : value.toString(); }
}

abstract class TaggedRational extends ConwayData {
  public constructor(node: CborValue) { super(node); }
  protected static pair<T extends TaggedRational>(this: ConwayConstructor<T>, first: bigint, second: bigint): T {
    if (second === 0n) throw new RangeError("denominator must not be zero");
    const node:CborValue={ kind: "tag", tag: 30n, value: { kind: "array", values: [uintNode(first), uintNode(second)], encoding: { kind: "definite", width: 0 } }, encoding: { width: 0 } };this.validateNode(node);return new this(node);
  }
  public static override validateNode(node: CborValue): void {
    if (node.kind !== "tag" || node.tag !== 30n || node.value.kind !== "array" || node.value.values.length !== 2) throw new TypeError(`${this.name} requires tag 30 and a two-item array`);
    for (const item of node.value.values) if (item.kind !== "unsigned") throw new TypeError(`${this.name} components must be unsigned`);
    if (node.value.values[1]?.kind === "unsigned" && node.value.values[1].value === 0n) throw new RangeError("denominator must not be zero");
  }
  protected components(): readonly [bigint, bigint] { const node = this.cborNode(); if (node.kind !== "tag" || node.value.kind !== "array") throw new TypeError("invalid rational"); const [a,b]=node.value.values; if(a?.kind!=="unsigned"||b?.kind!=="unsigned") throw new TypeError("invalid rational"); return [a.value,b.value]; }
}
export class Rational extends TaggedRational {
  public static new(numerator: bigint, denominator: bigint): Rational { return Rational.pair(numerator, denominator); }
  public static from_base10_f32(value: number): Rational {
    if (!Number.isFinite(value) || value < 0) throw new RangeError("rational must be finite and non-negative");
    const scale = 1_000_000_000n; return Rational.new(BigInt(Math.round(value * Number(scale))), scale);
  }
  public numerator(): bigint { return this.components()[0]; }
  public denominator(): bigint { return this.components()[1]; }
  public override to_js_value(): { numerator: number | string; denominator: number | string } { const [a,b]=this.components(); const cv=(v:bigint)=>v<=BigInt(Number.MAX_SAFE_INTEGER)?Number(v):v.toString(); return { numerator:cv(a),denominator:cv(b) }; }
}
export class UnitInterval extends TaggedRational {
  public static new(start: bigint, end: bigint): UnitInterval { if (start > end) throw new RangeError("unit interval numerator exceeds denominator"); return UnitInterval.pair(start, end); }
  public static override validateNode(node: CborValue): void {
    super.validateNode(node);
    if (
      node.kind === "tag" &&
      node.value.kind === "array" &&
      node.value.values[0]?.kind === "unsigned" &&
      node.value.values[1]?.kind === "unsigned" &&
      node.value.values[0].value > node.value.values[1].value
    ) {
      throw new RangeError("unit interval numerator exceeds denominator");
    }
  }
  public start(): bigint { return this.components()[0]; }
  public end(): bigint { return this.components()[1]; }
  public override to_js_value(): { start: number | string; end: number | string } { const [a,b]=this.components(); const cv=(v:bigint)=>v<=BigInt(Number.MAX_SAFE_INTEGER)?Number(v):v.toString(); return { start:cv(a),end:cv(b) }; }
}

export class ExUnitPrices extends ConwayData {
  public static override validateNode(node:CborValue):void{if(node.kind!=="array"||node.values.length!==2)throw new TypeError("ExUnitPrices requires two rationals");for(const item of node.values)Rational.validateNode(item);}
  public static new(memPrice:Rational,stepPrice:Rational):ExUnitPrices{return new ExUnitPrices({kind:"array",values:[decodeCbor(memPrice.to_cbor_bytes()),decodeCbor(stepPrice.to_cbor_bytes())],encoding:{kind:"definite",width:0}});}
  public mem_price():Rational{const node=this.cborNode();if(node.kind!=="array"||node.values[0]===undefined)throw new TypeError("invalid ExUnitPrices");return Rational.from_cbor_bytes(encodeCbor(node.values[0]));}
  public step_price():Rational{const node=this.cborNode();if(node.kind!=="array"||node.values[1]===undefined)throw new TypeError("invalid ExUnitPrices");return Rational.from_cbor_bytes(encodeCbor(node.values[1]));}
}

export class ExUnits extends ConwayData {
  public constructor(node: CborValue) { super(node); }
  public static override validateNode(node: CborValue): void {
    if (
      node.kind !== "array" ||
      node.values.length !== 2 ||
      node.values.some((value) => value.kind !== "unsigned" || value.value > INT64_MAX)
    ) throw new TypeError("ExUnits requires two non-negative int64 values");
  }
  public static new(mem: bigint, steps: bigint): ExUnits { const node:CborValue={ kind: "array", values: [uintNode(mem), uintNode(steps)], encoding: { kind: "definite", width: 0 } };ExUnits.validateNode(node);return new ExUnits(node); }
  public mem(): bigint { const node=this.cborNode(); if(node.kind!=="array"||node.values[0]?.kind!=="unsigned")throw new TypeError("invalid ExUnits"); return node.values[0].value; }
  public steps(): bigint { const node=this.cborNode(); if(node.kind!=="array"||node.values[1]?.kind!=="unsigned")throw new TypeError("invalid ExUnits"); return node.values[1].value; }
  public checked_add(other: ExUnits): ExUnits | undefined { const mem=this.mem()+other.mem(),steps=this.steps()+other.steps(); return mem>INT64_MAX||steps>INT64_MAX?undefined:ExUnits.new(mem,steps); }
  public override to_js_value(): { mem: number | string; steps: number | string } { const cv=(v:bigint)=>v<=BigInt(Number.MAX_SAFE_INTEGER)?Number(v):v.toString(); return { mem:cv(this.mem()),steps:cv(this.steps()) }; }
}

export class Credential extends ConwayData {
  public static override validateNode(node: CborValue): void { if(node.kind!=="array"||node.values.length!==2||node.values[0]?.kind!=="unsigned"||node.values[0].value>1n||node.values[1]?.kind!=="bytes"||node.values[1].value.length!==28)throw new TypeError("Credential requires [0|1, hash28]"); }
  public static new_pub_key(hash: Ed25519KeyHash): Credential { return new Credential({kind:"array",values:[uintNode(0n),{kind:"bytes",value:hash.to_raw_bytes(),encoding:{kind:"definite",width:0}}],encoding:{kind:"definite",width:0}}); }
  public static new_script(hash: ScriptHash): Credential { return new Credential({kind:"array",values:[uintNode(1n),{kind:"bytes",value:hash.to_raw_bytes(),encoding:{kind:"definite",width:0}}],encoding:{kind:"definite",width:0}}); }
  public kind(): number { const node=this.cborNode();if(node.kind!=="array"||node.values[0]?.kind!=="unsigned")throw new TypeError("invalid Credential");return Number(node.values[0].value); }
  public as_pub_key(): Ed25519KeyHash | undefined { const node=this.cborNode();return this.kind()===0&&node.kind==="array"&&node.values[1]?.kind==="bytes"?Ed25519KeyHash.from_raw_bytes(node.values[1].value):undefined; }
  public as_script(): ScriptHash | undefined { const node=this.cborNode();return this.kind()===1&&node.kind==="array"&&node.values[1]?.kind==="bytes"?ScriptHash.from_raw_bytes(node.values[1].value):undefined; }
  public override to_js_value(): unknown { const hash=this.as_pub_key()??this.as_script();return this.kind()===0?{PubKey:{hash:hash?.to_hex()}}:{Script:{hash:hash?.to_hex()}}; }
}

export class Anchor extends ConwayData {
  public static override validateNode(node:CborValue):void { if(node.kind!=="array"||node.values.length!==2||node.values[0]?.kind!=="text"||new TextEncoder().encode(node.values[0].value).length>128||node.values[1]?.kind!=="bytes"||node.values[1].value.length!==32)throw new TypeError("Anchor requires [url<=128 UTF-8 bytes, hash32]"); }
  public static new(anchorUrl: Url,hash:AnchorDocHash):Anchor { const node:CborValue={kind:"array",values:[decodeCbor(anchorUrl.to_cbor_bytes()),{kind:"bytes",value:hash.to_raw_bytes(),encoding:{kind:"definite",width:0}}],encoding:{kind:"definite",width:0}};Anchor.validateNode(node);return new Anchor(node); }
  public anchor_url():Url { const node=this.cborNode();if(node.kind!=="array"||node.values[0]===undefined)throw new TypeError("invalid Anchor");return Url.from_cbor_bytes(encodeCbor(node.values[0])); }
  public anchor_doc_hash():AnchorDocHash { const node=this.cborNode();if(node.kind!=="array"||node.values[1]?.kind!=="bytes")throw new TypeError("invalid Anchor");return AnchorDocHash.from_raw_bytes(node.values[1].value); }
  public override to_js_value():unknown { return {anchor_url:this.anchor_url().get(),anchor_doc_hash:this.anchor_doc_hash().to_hex()}; }
}

export class ProtocolVersion extends ConwayData {
  public static override validateNode(node:CborValue):void { if(node.kind!=="array"||node.values.length!==2||node.values[0]?.kind!=="unsigned"||node.values[0].value>12n||node.values[1]?.kind!=="unsigned"||node.values[1].value>0xffff_ffffn)throw new TypeError("ProtocolVersion requires [major<=12,uint32]"); }
  public static new(major:bigint,minor:bigint):ProtocolVersion { const node:CborValue={kind:"array",values:[uintNode(major),uintNode(minor)],encoding:{kind:"definite",width:0}};ProtocolVersion.validateNode(node);return new ProtocolVersion(node); }
  public major():bigint { const node=this.cborNode();if(node.kind!=="array"||node.values[0]?.kind!=="unsigned")throw new TypeError("invalid protocol version");return node.values[0].value; }
  public minor():bigint { const node=this.cborNode();if(node.kind!=="array"||node.values[1]?.kind!=="unsigned")throw new TypeError("invalid protocol version");return node.values[1].value; }
  public override to_js_value():unknown { return {major:Number(this.major()),minor:Number(this.minor())}; }
}

export class VRFCert extends ConwayData {
  public static override validateNode(node:CborValue):void { if(node.kind!=="array"||node.values.length!==2||node.values[0]?.kind!=="bytes"||node.values[1]?.kind!=="bytes"||node.values[1].value.length!==80)throw new TypeError("VRFCert requires [bytes, bytes80]"); }
  public static new(output:Uint8Array,proof:Uint8Array):VRFCert { const node:CborValue={kind:"array",values:[inputNode(output),inputNode(proof)],encoding:{kind:"definite",width:0}};VRFCert.validateNode(node);return new VRFCert(node); }
  public output():Uint8Array { const node=this.cborNode();if(node.kind!=="array"||node.values[0]?.kind!=="bytes")throw new TypeError("invalid VRFCert");return copyBytes(node.values[0].value); }
  public proof():Uint8Array { const node=this.cborNode();if(node.kind!=="array"||node.values[1]?.kind!=="bytes")throw new TypeError("invalid VRFCert");return copyBytes(node.values[1].value); }
  public override to_js_value():unknown { return {output:[...this.output()],proof:[...this.proof()]}; }
}

export class ConwayList<T extends ConwayData> {
  readonly #values: T[] = [];
  public static new<T>(this: new()=>T): T { return new this(); }
  public len(): number { return this.#values.length; }
  public is_empty(): boolean { return this.#values.length === 0; }
  public add(value: T): void { this.#values.push(value); }
  public get(index: number): T { const value=this.#values[index]; if(value===undefined)throw new RangeError("list index is out of range"); return value; }
  public values(): readonly T[] { return [...this.#values]; }
}

export class ConwayMap<K, V> {
  readonly #entries: Array<[K,V]> = [];
  public static new<T>(this: new()=>T): T { return new this(); }
  protected equal(left: K, right: K): boolean {
    if (left instanceof ConwayData && right instanceof ConwayData) return bytesEqual(left.to_canonical_cbor_bytes(), right.to_canonical_cbor_bytes());
    if (left instanceof Uint8Array && right instanceof Uint8Array) return bytesEqual(left,right);
    if (typeof left === "object" && left !== null && "to_raw_bytes" in left && typeof left.to_raw_bytes === "function" && typeof right === "object" && right !== null && "to_raw_bytes" in right && typeof right.to_raw_bytes === "function") return bytesEqual(left.to_raw_bytes() as Uint8Array,right.to_raw_bytes() as Uint8Array);
    if (typeof left === "object" && left !== null && "to_cbor_bytes" in left && typeof left.to_cbor_bytes === "function" && typeof right === "object" && right !== null && "to_cbor_bytes" in right && typeof right.to_cbor_bytes === "function") return bytesEqual(left.to_cbor_bytes() as Uint8Array,right.to_cbor_bytes() as Uint8Array);
    return Object.is(left,right);
  }
  public len(): number { return this.#entries.length; }
  public is_empty(): boolean { return this.#entries.length===0; }
  public insert(key: K, value: V): V | undefined { const index=this.#entries.findIndex(([item])=>this.equal(item,key)); if(index<0){this.#entries.push([key,value]);return undefined} const prior=this.#entries[index]?.[1]; this.#entries[index]=[key,value]; return prior; }
  public set(key: K, value: V): void { this.insert(key,value); }
  public get(key: K): V | undefined { return this.#entries.find(([item])=>this.equal(item,key))?.[1]; }
  public keys(): K[] { return this.#entries.map(([key])=>key); }
  public entries(): ReadonlyArray<readonly [K,V]> { return this.#entries.map(([key,value])=>[key,value]); }
}

export class TransactionMetadatumList extends ConwayList<TransactionMetadatum> {}

export class MetadatumMap {
  readonly #entries: Array<readonly [TransactionMetadatum,TransactionMetadatum]> = [];
  public static new(): MetadatumMap { return new MetadatumMap(); }
  public len(): number { return this.#entries.length; }
  public insert(key: TransactionMetadatum,value: TransactionMetadatum): void { this.#entries.push([key,value]); }
  public set(key: TransactionMetadatum,value: TransactionMetadatum): void { const canonical=key.to_canonical_cbor_hex();for(let index=this.#entries.length-1;index>=0;index-=1)if(this.#entries[index]?.[0].to_canonical_cbor_hex()===canonical)this.#entries.splice(index,1);this.#entries.push([key,value]); }
  public get(key: TransactionMetadatum): TransactionMetadatum | undefined { const found=this.#entries.find(([item])=>item.to_canonical_cbor_hex()===key.to_canonical_cbor_hex());return found?.[1]; }
  public get_all(key: TransactionMetadatum): TransactionMetadatumList | undefined { const found=this.#entries.filter(([item])=>item.to_canonical_cbor_hex()===key.to_canonical_cbor_hex());if(found.length===0)return undefined;const out=TransactionMetadatumList.new();for(const [,value] of found)out.add(value);return out; }
  public keys(): TransactionMetadatumList { const out=TransactionMetadatumList.new();for(const [key] of this.#entries)out.add(key);return out; }
  public entries(): ReadonlyArray<readonly [TransactionMetadatum,TransactionMetadatum]> { return [...this.#entries]; }
}

export class MetadatumList extends TransactionMetadatumList {}

export class Metadata extends ConwayData {
  protected static override readonly wireShape: ConwayWireShape = "map";
  public static new(): Metadata { return new Metadata({kind:"map",entries:[],encoding:{kind:"definite",width:0}}); }
  public len(): number { const node=this.cborNode();if(node.kind!=="map")throw new TypeError("Metadata requires a map");return node.entries.length; }
  public get(label: bigint): TransactionMetadatum|undefined { const node=this.cborNode();if(node.kind!=="map")throw new TypeError("Metadata requires a map");const value=node.entries.find(([key])=>key.kind==="unsigned"&&key.value===label)?.[1];return value===undefined?undefined:new TransactionMetadatum(value); }
  public set(label: bigint,value:TransactionMetadatum):void { if(label<0n||label>UINT64_MAX)throw new RangeError("metadata label must fit uint64");const node=this.cborNode();if(node.kind!=="map")throw new TypeError("Metadata requires a map");const entries=node.entries.filter(([key])=>key.kind!=="unsigned"||key.value!==label);entries.push([uintNode(label),decodeCbor(value.to_cbor_bytes())]);this.replaceCborNode({kind:"map",entries,encoding:{kind:"definite",width:0}}); }
}

export class TransactionMetadatum extends ConwayData {
  public static new_int(value: Int): TransactionMetadatum { return new TransactionMetadatum(decodeCbor(value.to_cbor_bytes())); }
  public static new_bytes(value: Uint8Array): TransactionMetadatum { if(value.length>64)throw new RangeError("metadata bytes are limited to 64 bytes");return new TransactionMetadatum({kind:"bytes",value:copyBytes(value),encoding:{kind:"definite",width:0}}); }
  public static new_text(value: string): TransactionMetadatum { if(new TextEncoder().encode(value).length>64)throw new RangeError("metadata text is limited to 64 UTF-8 bytes");return new TransactionMetadatum({kind:"text",value,encoding:{kind:"definite",width:0}}); }
  public static new_list(value: MetadatumList): TransactionMetadatum { return new TransactionMetadatum({kind:"array",values:value.values().map((item)=>decodeCbor(item.to_cbor_bytes())),encoding:{kind:"definite",width:0}}); }
  public static new_map(value: MetadatumMap): TransactionMetadatum { return new TransactionMetadatum({kind:"map",entries:value.entries().map(([key,item])=>[decodeCbor(key.to_cbor_bytes()),decodeCbor(item.to_cbor_bytes())]),encoding:{kind:"definite",width:0}}); }
  public kind(): number { const node=this.cborNode();return node.kind==="map"?0:node.kind==="array"?1:node.kind==="unsigned"||node.kind==="negative"?2:node.kind==="bytes"?3:node.kind==="text"?4:-1; }
  public as_int(): Int | undefined { const node=this.cborNode();return node.kind==="unsigned"||node.kind==="negative"?Int.from_cbor_bytes(encodeCbor(node)):undefined; }
  public as_bytes(): Uint8Array | undefined { const node=this.cborNode();return node.kind==="bytes"?copyBytes(node.value):undefined; }
  public as_text(): string | undefined { const node=this.cborNode();return node.kind==="text"?node.value:undefined; }
  public as_list(): MetadatumList | undefined { const node=this.cborNode();if(node.kind!=="array")return undefined;const out=MetadatumList.new();for(const item of node.values)out.add(new TransactionMetadatum(item));return out; }
  public as_map(): MetadatumMap | undefined { const node=this.cborNode();if(node.kind!=="map")return undefined;const out=MetadatumMap.new();for(const [key,value] of node.entries)out.insert(new TransactionMetadatum(key),new TransactionMetadatum(value));return out; }
  public to_json_value(): unknown { const node=this.cborNode();if(node.kind==="unsigned"||node.kind==="negative")return {int:Number(node.value)};if(node.kind==="bytes")return {bytes:bytesToHex(node.value)};if(node.kind==="text")return {string:node.value};if(node.kind==="array")return {list:node.values.map((item)=>new TransactionMetadatum(item).to_json_value())};if(node.kind==="map")return {map:node.entries.map(([key,value])=>({k:new TransactionMetadatum(key).to_json_value(),v:new TransactionMetadatum(value).to_json_value()}))};throw new TypeError("invalid transaction metadatum"); }
  public override to_js_value(): unknown { return this.to_json_value(); }
}

export class MapU64ToArrI64 extends ConwayMap<bigint,BigInt64Array> {
  public override insert(key: bigint,value: BigInt64Array): BigInt64Array | undefined { if(key<0n||key>UINT64_MAX)throw new RangeError("language id must fit uint64");return super.insert(key,new BigInt64Array(value)); }
  public override get(key: bigint): BigInt64Array | undefined { const value=super.get(key);return value===undefined?undefined:new BigInt64Array(value); }
  public override keys(): bigint[] { return super.keys(); }
}

export class CostModels extends ConwayData {
  public static new(inner: MapU64ToArrI64): CostModels {
    const node = costModelsNode(inner);
    CostModels.validateNode(node);
    return new CostModels(node);
  }
  public static override from_json<T extends ConwayData>(this: ConwayConstructor<T>, json: string): T {
    const value: unknown = JSON.parse(json);
    if (typeof value !== "object" || value === null || Array.isArray(value)) {
      throw new TypeError("CostModels JSON must be an object");
    }
    const inner = MapU64ToArrI64.new();
    for (const [languageText, parameters] of Object.entries(value)) {
      if (!/^\d+$/u.test(languageText)) {
        throw new TypeError("CostModels JSON keys must be numeric language ids");
      }
      const language = BigInt(languageText);
      if (language > 255n) throw new RangeError("CostModels language id must be in 0..255");
      if (inner.get(language) !== undefined) {
        throw new TypeError("CostModels JSON contains duplicate language ids");
      }
      if (!Array.isArray(parameters)) {
        throw new TypeError("CostModels JSON parameters must be arrays");
      }
      const costs = parameters.map((parameter) => {
        if (!Number.isSafeInteger(parameter)) {
          throw new TypeError("CostModels JSON parameters must be safe integers");
        }
        const cost = BigInt(parameter);
        if (cost < INT64_MIN || cost > INT64_MAX) {
          throw new RangeError("CostModels parameters must fit int64");
        }
        return cost;
      });
      inner.insert(language, BigInt64Array.from(costs));
    }
    const node = costModelsNode(inner);
    this.validateNode(node);
    return new this(node);
  }
  public get(): MapU64ToArrI64 { const node=this.cborNode();if(node.kind!=="map")throw new TypeError("invalid CostModels");const out=MapU64ToArrI64.new();for(const [key,value] of node.entries){if(key.kind!=="unsigned"||value.kind!=="array")throw new TypeError("invalid cost model");out.insert(key.value,BigInt64Array.from(value.values.map((item)=>{if(item.kind!=="unsigned"&&item.kind!=="negative")throw new TypeError("invalid cost");return item.value;})));}return out; }
  public language_views_encoding(): Uint8Array { const models=this.get();const entries:Array<readonly[CborValue,CborValue]>=[];for(const language of models.keys()){const costs=models.get(language)??new BigInt64Array();if(language===0n){const embedded: CborValue={kind:"array",values:[...costs].map(integerNode),encoding:{kind:"indefinite"}};entries.push([{kind:"bytes",value:Uint8Array.of(0),encoding:{kind:"definite",width:0}},{kind:"bytes",value:encodeCbor(embedded),encoding:{kind:"definite",width:0}}]);}else entries.push([uintNode(language),{kind:"array",values:[...costs].map(integerNode),encoding:{kind:"definite",width:0}}]);}return encodeCbor({kind:"map",entries,encoding:{kind:"definite",width:0}},{mode:"canonical"}); }
  public override to_js_value(): CostModelsJSON {
    const value: CostModelsJSON = {};
    const models = this.get();
    for (const language of models.keys()) {
      const parameters = models.get(language) ?? new BigInt64Array();
      value[language.toString()] = [...parameters].map((parameter) => {
        if (parameter < BigInt(Number.MIN_SAFE_INTEGER) || parameter > BigInt(Number.MAX_SAFE_INTEGER)) {
          throw new RangeError("CostModels parameters must be safe integers for JSON serialization");
        }
        return Number(parameter);
      });
    }
    return value;
  }
}
function costModelsNode(inner: MapU64ToArrI64): CborValue { return {kind:"map",entries:inner.keys().map((key)=>[uintNode(key),{kind:"array",values:[...(inner.get(key)??new BigInt64Array())].map(integerNode),encoding:{kind:"definite",width:0}}]),encoding:{kind:"definite",width:0}}; }

export class MapAssetNameToCoin extends ConwayMap<AssetName, bigint> {}
export class MapAssetNameToU64 extends ConwayMap<AssetName, bigint> {}
export class MapAssetNameToNonZeroInt64 extends ConwayMap<AssetName, bigint> {
  public override insert(key: AssetName, value: bigint): bigint | undefined { if(value<INT64_MIN||value>INT64_MAX)throw new RangeError("mint quantity must fit int64"); return super.insert(key,value); }
}

export class MultiAsset {
  readonly #policies = new ConwayMap<ScriptHash, MapAssetNameToCoin>();
  public static new(): MultiAsset { return new MultiAsset(); }
  public len(): number { return this.#policies.len(); }
  public policy_count(): number { return this.len(); }
  public keys(): ScriptHash[] { return this.#policies.keys(); }
  public get_assets(policy: ScriptHash): MapAssetNameToCoin | undefined { return this.#policies.get(policy); }
  public insert_assets(policy: ScriptHash, assets: MapAssetNameToCoin): MapAssetNameToCoin | undefined { return this.#policies.insert(policy,assets); }
  public get(policy: ScriptHash, asset: AssetName): bigint | undefined { return this.#policies.get(policy)?.get(asset); }
  public get_value(policy: ScriptHash, asset: AssetName): bigint { return this.get(policy,asset)??0n; }
  public insert(policy: ScriptHash, asset: AssetName, amount: bigint): bigint | undefined { if(amount<0n||amount>UINT64_MAX)throw new RangeError("asset coin must fit uint64"); let assets=this.#policies.get(policy); if(assets===undefined){assets=MapAssetNameToCoin.new();this.#policies.insert(policy,assets)} return assets.insert(asset,amount); }
  public set_value(policy: ScriptHash, asset: AssetName, amount: bigint): void { this.insert(policy,asset,amount); }
  #combine(other: MultiAsset, subtract: boolean, clamp: boolean): MultiAsset | undefined { const out=MultiAsset.new(); for(const policy of this.keys())for(const asset of this.get_assets(policy)?.keys()??[])out.insert(policy,asset,this.get_value(policy,asset)); for(const policy of other.keys())for(const asset of other.get_assets(policy)?.keys()??[]){const current=out.get_value(policy,asset), delta=other.get_value(policy,asset); const next=subtract?current-delta:current+delta; if((next<0n&&!clamp)||next>UINT64_MAX)return undefined; out.set_value(policy,asset,next<0n?0n:next)} return out; }
  public checked_add(other: MultiAsset): MultiAsset | undefined { return this.#combine(other,false,false); }
  public checked_sub(other: MultiAsset): MultiAsset | undefined { return this.#combine(other,true,false); }
  public clamped_sub(other: MultiAsset): MultiAsset { return this.#combine(other,true,true) as MultiAsset; }
}

export class Mint extends ConwayMap<ScriptHash, MapAssetNameToNonZeroInt64> {
  public policy_count(): number { return this.len(); }
  public get_assets(policy: ScriptHash): MapAssetNameToNonZeroInt64 | undefined { return this.get(policy); }
  public get_value(policy: ScriptHash, asset: AssetName): bigint { return this.get(policy)?.get(asset)??0n; }
  public set_value(policy: ScriptHash, asset: AssetName, amount: bigint): void { let assets=this.get(policy); if(assets===undefined){assets=MapAssetNameToNonZeroInt64.new() as MapAssetNameToNonZeroInt64;this.insert(policy,assets)} assets.insert(asset,amount); }
  public as_positive_multiasset(): MultiAsset { const out=MultiAsset.new(); for(const policy of this.keys())for(const asset of this.get(policy)?.keys()??[]){const value=this.get_value(policy,asset);if(value>0n)out.insert(policy,asset,value)}return out; }
  public as_negative_multiasset(): MultiAsset { const out=MultiAsset.new(); for(const policy of this.keys())for(const asset of this.get(policy)?.keys()??[]){const value=this.get_value(policy,asset);if(value<0n)out.insert(policy,asset,-value)}return out; }
}

export class Value extends ConwayData {
  readonly #coin: bigint; readonly #assets: MultiAsset | undefined;
  public constructor(node: CborValue, coin?:bigint, assets?:MultiAsset) { super(node); const parsed=coin===undefined?parseValueNode(node):{coin,assets};this.#coin=parsed.coin;this.#assets=parsed.assets; }
  public static new(coin: bigint, assets?: MultiAsset): Value { if(coin<0n||coin>UINT64_MAX)throw new RangeError("coin must fit uint64"); const node: CborValue=assets===undefined?uintNode(coin):{kind:"array",values:[uintNode(coin),multiAssetNode(assets)],encoding:{kind:"definite",width:0}}; return new Value(node,coin,assets); }
  public static zero(): Value { return Value.new(0n); }
  public static from_coin(coin: bigint): Value { return Value.new(coin); }
  public coin(): bigint { return this.#coin; }
  public multi_asset(): MultiAsset | undefined { return this.#assets; }
  public has_multiassets(): boolean { return this.#assets!==undefined&&this.#assets.len()>0; }
  public is_zero(): boolean { return this.#coin===0n&&!this.has_multiassets(); }
  public checked_add(other: Value): Value | undefined { const coin=this.#coin+other.#coin;if(coin>UINT64_MAX)return undefined; const assets=(this.#assets??MultiAsset.new()).checked_add(other.#assets??MultiAsset.new());return assets===undefined?undefined:Value.new(coin,assets); }
  public checked_sub(other: Value): Value | undefined { if(this.#coin<other.#coin)return undefined;const assets=(this.#assets??MultiAsset.new()).checked_sub(other.#assets??MultiAsset.new());return assets===undefined?undefined:Value.new(this.#coin-other.#coin,assets); }
  public clamped_sub(other: Value): Value { return Value.new(this.#coin>other.#coin?this.#coin-other.#coin:0n,(this.#assets??MultiAsset.new()).clamped_sub(other.#assets??MultiAsset.new())); }
  public override to_js_value(): unknown { return { coin: Number(this.#coin), multiasset: this.#assets===undefined?{}:multiAssetJson(this.#assets) }; }
}

function multiAssetNode(value: MultiAsset): CborValue { return {kind:"map",entries:value.keys().map((policy)=>[{kind:"bytes",value:policy.to_raw_bytes(),encoding:{kind:"definite",width:0}},{kind:"map",entries:(value.get_assets(policy)?.keys()??[]).map((asset)=>[{kind:"bytes",value:asset.to_raw_bytes(),encoding:{kind:"definite",width:0}},uintNode(value.get_value(policy,asset))]),encoding:{kind:"definite",width:0}}]),encoding:{kind:"definite",width:0}}; }
function multiAssetJson(value: MultiAsset): unknown { return Object.fromEntries(value.keys().map((policy)=>[policy.to_hex(),Object.fromEntries((value.get_assets(policy)?.keys()??[]).map((asset)=>[asset.to_hex(),Number(value.get_value(policy,asset))]))])); }
function parseValueNode(node:CborValue):{coin:bigint;assets?:MultiAsset}{if(node.kind==="unsigned")return {coin:node.value};if(node.kind!=="array"||node.values.length!==2||node.values[0]?.kind!=="unsigned"||node.values[1]?.kind!=="map")throw new TypeError("Value requires coin or [coin,multiasset]");const assets=MultiAsset.new();for(const [policyNode,bundleNode] of node.values[1].entries){if(policyNode.kind!=="bytes"||policyNode.value.length!==28||bundleNode.kind!=="map")throw new TypeError("invalid multiasset");const policy=ScriptHash.from_raw_bytes(policyNode.value);for(const [assetNode,amountNode] of bundleNode.entries){if(assetNode.kind!=="bytes"||amountNode.kind!=="unsigned")throw new TypeError("invalid asset bundle");assets.insert(policy,AssetName.from_raw_bytes(assetNode.value),amountNode.value);}}return {coin:node.values[0].value,assets};}

export class AlonzoFormatTxOut extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): AlonzoFormatTxOut { const node=modelNode(this.wireShape,values);this.validateNode(node);return new AlonzoFormatTxOut(node); } }
export class AuthCommitteeHotCert extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): AuthCommitteeHotCert { const node=modelNode(this.wireShape,values);this.validateNode(node);return new AuthCommitteeHotCert(node); } }
export class AuxiliaryData extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "choice"; public static new(...values: ConwayInput[]): AuxiliaryData { const node=modelNode(this.wireShape,values);this.validateNode(node);return new AuxiliaryData(node); } }
export class Block extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): Block { const node=modelNode(this.wireShape,values);this.validateNode(node);return new Block(node); } }
export class Certificate extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): Certificate { const node=modelNode(this.wireShape,values);this.validateNode(node);return new Certificate(node); } }
export class Constitution extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): Constitution { const node=modelNode(this.wireShape,values);this.validateNode(node);return new Constitution(node); } }
export class ConwayFormatAuxData extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "tag"; public static new(...values: ConwayInput[]): ConwayFormatAuxData { const node=modelNode(this.wireShape,values);this.validateNode(node);return new ConwayFormatAuxData(node); } }
export class ConwayFormatTxOut extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "map"; public static new(...values: ConwayInput[]): ConwayFormatTxOut { const node=modelNode(this.wireShape,values);this.validateNode(node);return new ConwayFormatTxOut(node); } }
export class DRep extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): DRep { const node=modelNode(this.wireShape,values);this.validateNode(node);return new DRep(node); } }
export class DRepVotingThresholds extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): DRepVotingThresholds { const node=modelNode(this.wireShape,values);this.validateNode(node);return new DRepVotingThresholds(node); } }
export class DatumOption extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): DatumOption { const node=modelNode(this.wireShape,values);this.validateNode(node);return new DatumOption(node); } }
export class GovAction extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): GovAction { const node=modelNode(this.wireShape,values);this.validateNode(node);return new GovAction(node); } }
export class GovActionId extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): GovActionId { const node=modelNode(this.wireShape,values);this.validateNode(node);return new GovActionId(node); } }
export class HardForkInitiationAction extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): HardForkInitiationAction { const node=modelNode(this.wireShape,values);this.validateNode(node);return new HardForkInitiationAction(node); } }
export class Header extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): Header { const node=modelNode(this.wireShape,values);this.validateNode(node);return new Header(node); } }
export class HeaderBody extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): HeaderBody { const node=modelNode(this.wireShape,values);this.validateNode(node);return new HeaderBody(node); } }
export class LegacyRedeemer extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): LegacyRedeemer { const node=modelNode(this.wireShape,values);this.validateNode(node);return new LegacyRedeemer(node); } }
export class MultiHostName extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): MultiHostName { const node=modelNode(this.wireShape,values);this.validateNode(node);return new MultiHostName(node); } }
export class NewConstitution extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): NewConstitution { const node=modelNode(this.wireShape,values);this.validateNode(node);return new NewConstitution(node); } }
export class NoConfidence extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): NoConfidence { const node=modelNode(this.wireShape,values);this.validateNode(node);return new NoConfidence(node); } }
export class Nonce extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): Nonce { const node=modelNode(this.wireShape,values);this.validateNode(node);return new Nonce(node); } }
export class OperationalCert extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): OperationalCert { const node=modelNode(this.wireShape,values);this.validateNode(node);return new OperationalCert(node); } }
export class ParameterChangeAction extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): ParameterChangeAction { const node=modelNode(this.wireShape,values);this.validateNode(node);return new ParameterChangeAction(node); } }
export class PoolMetadata extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): PoolMetadata { const node=modelNode(this.wireShape,values);this.validateNode(node);return new PoolMetadata(node); } }
export class PoolParams extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): PoolParams { const node=modelNode(this.wireShape,values);this.validateNode(node);return new PoolParams(node); } }
export class PoolRegistration extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): PoolRegistration { const node=modelNode(this.wireShape,values);this.validateNode(node);return new PoolRegistration(node); } }
export class PoolRetirement extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): PoolRetirement { const node=modelNode(this.wireShape,values);this.validateNode(node);return new PoolRetirement(node); } }
export class PoolVotingThresholds extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): PoolVotingThresholds { const node=modelNode(this.wireShape,values);this.validateNode(node);return new PoolVotingThresholds(node); } }
export class ProposalProcedure extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): ProposalProcedure { const node=modelNode(this.wireShape,values);this.validateNode(node);return new ProposalProcedure(node); } }
export class ProtocolParamUpdate extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "map"; public static new(...values: ConwayInput[]): ProtocolParamUpdate { const node=modelNode(this.wireShape,values);this.validateNode(node);return new ProtocolParamUpdate(node); } }
export class RedeemerKey extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): RedeemerKey { const node=modelNode(this.wireShape,values);this.validateNode(node);return new RedeemerKey(node); } }
export class RedeemerVal extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): RedeemerVal { const node=modelNode(this.wireShape,values);this.validateNode(node);return new RedeemerVal(node); } }
export class Redeemers extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "choice"; public static new(...values: ConwayInput[]): Redeemers { const node=modelNode(this.wireShape,values);this.validateNode(node);return new Redeemers(node); } }
export class RegCert extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): RegCert { const node=modelNode(this.wireShape,values);this.validateNode(node);return new RegCert(node); } }
export class RegDrepCert extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): RegDrepCert { const node=modelNode(this.wireShape,values);this.validateNode(node);return new RegDrepCert(node); } }
export class Relay extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): Relay { const node=modelNode(this.wireShape,values);this.validateNode(node);return new Relay(node); } }
export class RequiredSigners extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "alias"; public static new(...values: ConwayInput[]): RequiredSigners { const node=modelNode(this.wireShape,values);this.validateNode(node);return new RequiredSigners(node); } }
export class ResignCommitteeColdCert extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): ResignCommitteeColdCert { const node=modelNode(this.wireShape,values);this.validateNode(node);return new ResignCommitteeColdCert(node); } }
export class Script extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): Script { const node=modelNode(this.wireShape,values);this.validateNode(node);return new Script(node); } }
export class ScriptRef extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "tag"; public static new(...values: ConwayInput[]): ScriptRef { const node=modelNode(this.wireShape,values);this.validateNode(node);return new ScriptRef(node); } }
export class ShelleyMAFormatAuxData extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): ShelleyMAFormatAuxData { const node=modelNode(this.wireShape,values);this.validateNode(node);return new ShelleyMAFormatAuxData(node); } }
export class SingleHostAddr extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): SingleHostAddr { const node=modelNode(this.wireShape,values);this.validateNode(node);return new SingleHostAddr(node); } }
export class SingleHostName extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): SingleHostName { const node=modelNode(this.wireShape,values);this.validateNode(node);return new SingleHostName(node); } }
export class StakeDelegation extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): StakeDelegation { const node=modelNode(this.wireShape,values);this.validateNode(node);return new StakeDelegation(node); } }
export class StakeDeregistration extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): StakeDeregistration { const node=modelNode(this.wireShape,values);this.validateNode(node);return new StakeDeregistration(node); } }
export class StakeRegDelegCert extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): StakeRegDelegCert { const node=modelNode(this.wireShape,values);this.validateNode(node);return new StakeRegDelegCert(node); } }
export class StakeRegistration extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): StakeRegistration { const node=modelNode(this.wireShape,values);this.validateNode(node);return new StakeRegistration(node); } }
export class StakeVoteDelegCert extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): StakeVoteDelegCert { const node=modelNode(this.wireShape,values);this.validateNode(node);return new StakeVoteDelegCert(node); } }
export class StakeVoteRegDelegCert extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): StakeVoteRegDelegCert { const node=modelNode(this.wireShape,values);this.validateNode(node);return new StakeVoteRegDelegCert(node); } }
export class Transaction extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): Transaction { const node=modelNode(this.wireShape,values);this.validateNode(node);return new Transaction(node); } }
export class TransactionBody extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "map"; public static new(...values: ConwayInput[]): TransactionBody { const node=modelNode(this.wireShape,values);this.validateNode(node);return new TransactionBody(node); } }
export class TransactionOutput extends ConwayData {
  protected static override readonly wireShape: ConwayWireShape = "choice";
  public static new(address: Address, amount: Value, datumOption?: DatumOption | null, scriptReference?: ScriptRef | null): TransactionOutput {
    const entries: Array<readonly [CborValue, CborValue]> = [
      [uintNode(0n), { kind: "bytes", value: address.to_raw_bytes(), encoding: { kind: "definite", width: 0 } }],
      [uintNode(1n), decodeCbor(amount.to_cbor_bytes())],
    ];
    if (datumOption != null) entries.push([uintNode(2n), decodeCbor(datumOption.to_cbor_bytes())]);
    if (scriptReference != null) entries.push([uintNode(3n), decodeCbor(scriptReference.to_cbor_bytes())]);
    const node: CborValue = { kind: "map", entries, encoding: { kind: "definite", width: 0 } };
    TransactionOutput.validateNode(node);
    return new TransactionOutput(node);
  }
  public static new_alonzo_format_tx_out(value: AlonzoFormatTxOut): TransactionOutput { return TransactionOutput.from_cbor_bytes(value.to_cbor_bytes()); }
  public static new_conway_format_tx_out(value: ConwayFormatTxOut): TransactionOutput { return TransactionOutput.from_cbor_bytes(value.to_cbor_bytes()); }
  private field(key: bigint, index: number): CborValue | undefined {
    const node=this.cborNode();
    if(node.kind==="array")return node.values[index];
    if(node.kind!=="map")return undefined;
    return node.entries.find(([candidate])=>candidate.kind==="unsigned"&&candidate.value===key)?.[1];
  }
  private setField(key: bigint, index: number, value: CborValue): void {
    const node=this.cborNode();
    if(node.kind==="array"){const values=[...node.values];values[index]=value;this.replaceCborNode({...node,values});return;}
    if(node.kind!=="map")throw new TypeError("TransactionOutput requires array or map CBOR");
    const entries=node.entries.filter(([candidate])=>candidate.kind!=="unsigned"||candidate.value!==key);
    entries.push([uintNode(key),value]);this.replaceCborNode({...node,entries});
  }
  public kind(): TransactionOutputKind { return this.cborNode().kind==="array"?TransactionOutputKind.AlonzoFormatTxOut:TransactionOutputKind.ConwayFormatTxOut; }
  public address(): Address { const value=this.field(0n,0);if(value?.kind!=="bytes")throw new TypeError("TransactionOutput address must be bytes");return Address.from_raw_bytes(value.value); }
  public amount(): Value { const value=this.field(1n,1);if(value===undefined)throw new TypeError("TransactionOutput amount is absent");return Value.from_cbor_bytes(encodeCbor(value)); }
  public datum(): DatumOption | undefined { const value=this.cborNode().kind==="map"?this.field(2n,2):undefined;return value===undefined?undefined:DatumOption.from_cbor_bytes(encodeCbor(value)); }
  public datum_hash(): DatumHash | undefined { const value=this.cborNode().kind==="array"?this.field(2n,2):undefined;return value?.kind==="bytes"&&value.value.length===32?DatumHash.from_raw_bytes(value.value):undefined; }
  public script_ref(): ScriptRef | undefined { const value=this.cborNode().kind==="map"?this.field(3n,3):undefined;return value===undefined?undefined:ScriptRef.from_cbor_bytes(encodeCbor(value)); }
  public set_address(value: Address): void { this.setField(0n,0,{kind:"bytes",value:value.to_raw_bytes(),encoding:{kind:"definite",width:0}}); }
  public set_amount(value: Value): void { this.setField(1n,1,decodeCbor(value.to_cbor_bytes())); }
  public as_alonzo_format_tx_out(): AlonzoFormatTxOut | undefined { return this.kind()===TransactionOutputKind.AlonzoFormatTxOut?AlonzoFormatTxOut.from_cbor_bytes(this.to_cbor_bytes()):undefined; }
  public as_conway_format_tx_out(): ConwayFormatTxOut | undefined { return this.kind()===TransactionOutputKind.ConwayFormatTxOut?ConwayFormatTxOut.from_cbor_bytes(this.to_cbor_bytes()):undefined; }
}
export class TransactionWitnessSet extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "map"; public static new(...values: ConwayInput[]): TransactionWitnessSet { const node=modelNode(this.wireShape,values);this.validateNode(node);return new TransactionWitnessSet(node); } }
export class TreasuryWithdrawalsAction extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): TreasuryWithdrawalsAction { const node=modelNode(this.wireShape,values);this.validateNode(node);return new TreasuryWithdrawalsAction(node); } }
export class UnregCert extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): UnregCert { const node=modelNode(this.wireShape,values);this.validateNode(node);return new UnregCert(node); } }
export class UnregDrepCert extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): UnregDrepCert { const node=modelNode(this.wireShape,values);this.validateNode(node);return new UnregDrepCert(node); } }
export class UpdateCommittee extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): UpdateCommittee { const node=modelNode(this.wireShape,values);this.validateNode(node);return new UpdateCommittee(node); } }
export class UpdateDrepCert extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): UpdateDrepCert { const node=modelNode(this.wireShape,values);this.validateNode(node);return new UpdateDrepCert(node); } }
export class Vkeywitness extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): Vkeywitness { const node=modelNode(this.wireShape,values);this.validateNode(node);return new Vkeywitness(node); } }
export class VoteDelegCert extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): VoteDelegCert { const node=modelNode(this.wireShape,values);this.validateNode(node);return new VoteDelegCert(node); } }
export class VoteRegDelegCert extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): VoteRegDelegCert { const node=modelNode(this.wireShape,values);this.validateNode(node);return new VoteRegDelegCert(node); } }
export class Voter extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): Voter { const node=modelNode(this.wireShape,values);this.validateNode(node);return new Voter(node); } }
export class VotingProcedure extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "array"; public static new(...values: ConwayInput[]): VotingProcedure { const node=modelNode(this.wireShape,values);this.validateNode(node);return new VotingProcedure(node); } }
export class VotingProcedures extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "map"; public static new(...values: ConwayInput[]): VotingProcedures { const node=modelNode(this.wireShape,values);this.validateNode(node);return new VotingProcedures(node); } }
export class Withdrawals extends ConwayData { protected static override readonly wireShape: ConwayWireShape = "map"; public static new(...values: ConwayInput[]): Withdrawals { const node=modelNode(this.wireShape,values);this.validateNode(node);return new Withdrawals(node); } }

export class AlonzoFormatTxOutList extends ConwayList<ConwayData> {}
export class AssetNameList extends ConwayList<ConwayData> {}
export class BootstrapWitnessList extends ConwayList<ConwayData> {}
export class CertificateList extends ConwayList<ConwayData> {}
export class CommitteeColdCredentialList extends ConwayList<ConwayData> {}
export class GenesisHashList extends ConwayList<ConwayData> {}
export class GovActionIdList extends ConwayList<ConwayData> {}
export class LanguageList extends ConwayList<ConwayData> {}
export class LegacyRedeemerList extends ConwayList<ConwayData> {}
export class NonEmptyBootstrapWitnessList extends ConwayList<ConwayData> {}
export class NonEmptyCertificateList extends ConwayList<ConwayData> {}
export class NonEmptyLegacyRedeemerList extends ConwayList<ConwayData> {}
export class NonEmptyNativeScriptList extends ConwayList<ConwayData> {}
export class NonEmptyPlutusDataList extends ConwayList<ConwayData> {}
export class NonEmptyPlutusV1ScriptList extends ConwayList<ConwayData> {}
export class NonEmptyPlutusV2ScriptList extends ConwayList<ConwayData> {}
export class NonEmptyPlutusV3ScriptList extends ConwayList<ConwayData> {}
export class NonEmptyProposalProcedureList extends ConwayList<ConwayData> {}
export class NonEmptyTransactionInputList extends ConwayList<ConwayData> {}
export class NonEmptyVkeywitnessList extends ConwayList<ConwayData> {}
export class PlutusV1ScriptList extends ConwayList<ConwayData> {}
export class PlutusV2ScriptList extends ConwayList<ConwayData> {}
export class PlutusV3ScriptList extends ConwayList<ConwayData> {}
export class PolicyIdList extends ConwayList<ConwayData> {}
export class ProposalProcedureList extends ConwayList<ConwayData> {}
export class RedeemerKeyList extends ConwayList<ConwayData> {}
export class RelayList extends ConwayList<ConwayData> {}
export class RewardAccountList extends ConwayList<ConwayData> {}
export class StakeCredentialList extends ConwayList<ConwayData> {}
export class TransactionBodyList extends ConwayList<ConwayData> {}
export class TransactionOutputList extends ConwayList<ConwayData> {}
export class TransactionWitnessSetList extends ConwayList<ConwayData> {}
export class VkeywitnessList extends ConwayList<ConwayData> {}
export class VoterList extends ConwayList<ConwayData> {}

export class MapCommitteeColdCredentialToEpoch extends ConwayMap<ConwayInput, ConwayInput> {}
export class MapGovActionIdToVotingProcedure extends ConwayMap<ConwayInput, ConwayInput> {}
export class MapRedeemerKeyToRedeemerVal extends ConwayMap<ConwayInput, ConwayInput> {}
export class MapStakeCredentialToCoin extends ConwayMap<ConwayInput, ConwayInput> {}
export class MapStakeCredentialToDeltaCoin extends ConwayMap<ConwayInput, ConwayInput> {}
export class MapTransactionIndexToAuxiliaryData extends ConwayMap<ConwayInput, ConwayInput> {}
export class MapTransactionIndexToMetadata extends ConwayMap<ConwayInput, ConwayInput> {}
export class MapVoterToMapGovActionIdToVotingProcedure extends ConwayMap<ConwayInput, ConwayInput> {}
export class NonEmptyMapGovActionIdToVotingProcedure extends ConwayMap<ConwayInput, ConwayInput> {}
export class NonEmptyMapRedeemerKeyToRedeemerVal extends ConwayMap<ConwayInput, ConwayInput> {}
export class TransactionMetadatumLabels extends ConwayMap<ConwayInput, ConwayInput> {}

export enum AuxiliaryDataKind { Shelley = 0, ShelleyMA = 1, Conway = 2 }
export enum CertificateKind { StakeRegistration = 0, StakeDeregistration = 1, StakeDelegation = 2, PoolRegistration = 3, PoolRetirement = 4, RegCert = 5, UnregCert = 6, VoteDelegCert = 7, StakeVoteDelegCert = 8, StakeRegDelegCert = 9, VoteRegDelegCert = 10, StakeVoteRegDelegCert = 11, AuthCommitteeHotCert = 12, ResignCommitteeColdCert = 13, RegDrepCert = 14, UnregDrepCert = 15, UpdateDrepCert = 16 }
export enum CredentialKind { PubKey = 0, Script = 1 }
export enum DRepKind { Key = 0, Script = 1, AlwaysAbstain = 2, AlwaysNoConfidence = 3 }
export enum DatumOptionKind { Hash = 0, Datum = 1 }
export enum GovActionKind { ParameterChangeAction = 0, HardForkInitiationAction = 1, TreasuryWithdrawalsAction = 2, NoConfidence = 3, UpdateCommittee = 4, NewConstitution = 5, InfoAction = 6 }
export enum Language { PlutusV1 = 0, PlutusV2 = 1, PlutusV3 = 2 }
export enum NonceKind { Identity = 0, Hash = 1 }
export enum RedeemerTag { Spend = 0, Mint = 1, Cert = 2, Reward = 3, Voting = 4, Proposing = 5 }
export enum RedeemersKind { ArrLegacyRedeemer = 0, MapRedeemerKeyToRedeemerVal = 1 }
export enum RelayKind { SingleHostAddr = 0, SingleHostName = 1, MultiHostName = 2 }
export enum ScriptKind { Native = 0, PlutusV1 = 1, PlutusV2 = 2, PlutusV3 = 3 }
export enum TransactionMetadatumKind { Map = 0, List = 1, Int = 2, Bytes = 3, Text = 4 }
export enum TransactionOutputKind { AlonzoFormatTxOut = 0, ConwayFormatTxOut = 1 }
export enum Vote { No = 0, Yes = 1, Abstain = 2 }
export enum VoterKind { ConstitutionalCommitteeHotKeyHash = 0, ConstitutionalCommitteeHotScriptHash = 1, DRepKeyHash = 2, DRepScriptHash = 3, StakingPoolKeyHash = 4 }
