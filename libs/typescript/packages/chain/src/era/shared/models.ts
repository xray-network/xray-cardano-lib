import {
  BigInteger,
  DeserializeError,
  UINT64_MAX,
  bytesEqual,
  bytesToHex,
  copyBytes,
  decodeCbor,
  encodeCbor,
  hexToBytes,
} from "@xray-network/cardano-core";
import type { CborHeadWidth, CborLengthEncoding, CborValue } from "@xray-network/cardano-core";
import { Ed25519KeyHash, ScriptHash, TransactionHash, blake2b224 } from "@xray-network/cardano-crypto";
import type { NativeScriptJSON, PlutusDataJSON, TransactionInputJSON } from "./json-types.js";
import { Ed25519KeyHashList } from "../conway/model.js";

type ArrayNode = Extract<CborValue, { readonly kind: "array" }>;
type MapNode = Extract<CborValue, { readonly kind: "map" }>;

function fail(failure: ConstructorParameters<typeof DeserializeError>[0], message: string, path: string): never {
  throw new DeserializeError(failure, message, { path: [path] });
}

function arrayNode(node: CborValue, name: string, length?: number): ArrayNode {
  if (node.kind !== "array") fail("INVALID_STRUCTURE", `${name} must be a CBOR array`, name);
  if (length !== undefined && node.values.length !== length) fail("INVALID_STRUCTURE", `${name} requires ${length} fields`, name);
  return node;
}

function mapNode(node: CborValue, name: string): MapNode {
  if (node.kind !== "map") fail("INVALID_STRUCTURE", `${name} must be a CBOR map`, name);
  return node;
}

function uint(node: CborValue, name: string): bigint {
  if (node.kind !== "unsigned") fail("INVALID_STRUCTURE", `${name} must be an unsigned integer`, name);
  return node.value;
}

function checkedU64(value: bigint, name: string): bigint {
  if (value < 0n || value > UINT64_MAX) fail("OUT_OF_RANGE", `${name} must fit uint64`, name);
  return value;
}

function cloneHash<T extends TransactionHash | Ed25519KeyHash>(value: T): T {
  return (value instanceof TransactionHash
    ? TransactionHash.from_raw_bytes(value.to_raw_bytes())
    : Ed25519KeyHash.from_raw_bytes(value.to_raw_bytes())) as T;
}

abstract class CborModel {
  protected abstract node(): CborValue;
  protected abstract jsonValue(): unknown;
  public to_cbor_bytes(): Uint8Array { return encodeCbor(this.node()); }
  public to_cbor_hex(): string { return bytesToHex(this.to_cbor_bytes()); }
  public to_canonical_cbor_bytes(): Uint8Array { return encodeCbor(this.node(), { mode: "canonical" }); }
  public to_canonical_cbor_hex(): string { return bytesToHex(this.to_canonical_cbor_bytes()); }
  public to_json(): string { return JSON.stringify(this.jsonValue(), null, 2); }
  public to_js_value(): any { return this.jsonValue(); }
}

interface TransactionInputState { readonly transactionId: TransactionHash; readonly index: bigint; readonly cbor: ArrayNode }
const transactionInputs = new WeakMap<TransactionInput, TransactionInputState>();

export class TransactionInput extends CborModel {
  private constructor(state: TransactionInputState) { super(); transactionInputs.set(this, state); }
  public static new(transactionId: TransactionHash, index: bigint): TransactionInput {
    checkedU64(index, "TransactionInput.index");
    const hash = cloneHash(transactionId);
    return new TransactionInput({
      transactionId: hash,
      index,
      cbor: {
        kind: "array",
        values: [
          { kind: "bytes", value: hash.to_raw_bytes(), encoding: { kind: "definite", width: 0 } },
          { kind: "unsigned", value: index, encoding: { width: 0 } },
        ],
        encoding: { kind: "definite", width: 0 },
      },
    });
  }
  public static from_cbor_bytes(bytes: Uint8Array): TransactionInput {
    const cbor = arrayNode(decodeCbor(bytes), "TransactionInput", 2);
    const hashNode = cbor.values[0];
    if (hashNode?.kind !== "bytes" || hashNode.value.length !== 32) fail("INVALID_STRUCTURE", "TransactionInput.transaction_id must be 32 bytes", "TransactionInput.transaction_id");
    const indexNode = cbor.values[1];
    if (indexNode === undefined) fail("INVALID_STRUCTURE", "TransactionInput.index is missing", "TransactionInput.index");
    const index = checkedU64(uint(indexNode, "TransactionInput.index"), "TransactionInput.index");
    return new TransactionInput({ transactionId: TransactionHash.from_raw_bytes(hashNode.value), index, cbor });
  }
  public static from_cbor_hex(hex: string): TransactionInput { return TransactionInput.from_cbor_bytes(hexToBytes(hex)); }
  public static from_json(json: string): TransactionInput {
    const value = JSON.parse(json) as Partial<TransactionInputJSON>;
    if (typeof value.transaction_id !== "string" || typeof value.index !== "number" || !Number.isSafeInteger(value.index) || value.index < 0) {
      throw new TypeError("Invalid TransactionInput JSON");
    }
    return TransactionInput.new(TransactionHash.from_hex(value.transaction_id), BigInt(value.index));
  }
  public transaction_id(): TransactionHash { return cloneHash(transactionInputState(this).transactionId); }
  public index(): bigint { return transactionInputState(this).index; }
  protected node(): CborValue { return transactionInputState(this).cbor; }
  protected jsonValue(): TransactionInputJSON {
    const state = transactionInputState(this);
    return { transaction_id: state.transactionId.to_hex(), index: Number(state.index) };
  }
}

function transactionInputState(value: TransactionInput): TransactionInputState {
  const state = transactionInputs.get(value);
  if (state === undefined) throw new TypeError("Invalid TransactionInput receiver");
  return state;
}

export class TransactionInputList {
  readonly #values: TransactionInput[] = [];
  private constructor() {}
  public static new(): TransactionInputList { return new TransactionInputList(); }
  public len(): number { return this.#values.length; }
  public get(index: number): TransactionInput {
    const value = this.#values[index];
    if (value === undefined) throw new RangeError("TransactionInputList index is out of range");
    return TransactionInput.from_cbor_bytes(value.to_cbor_bytes());
  }
  public add(value: TransactionInput): void { this.#values.push(TransactionInput.from_cbor_bytes(value.to_cbor_bytes())); }
}

/** Internal vertical slice proving optional tag-258 set preservation and duplicate wire order. */
export class TaggedTransactionInputSet extends CborModel {
  readonly #values: TransactionInput[];
  readonly #tagged: boolean;
  readonly #encoding: CborLengthEncoding;
  readonly #tagWidth: CborHeadWidth;
  private constructor(values: TransactionInput[], tagged: boolean, encoding: CborLengthEncoding, tagWidth: CborHeadWidth) {
    super(); this.#values = values; this.#tagged = tagged; this.#encoding = encoding; this.#tagWidth = tagWidth;
  }
  public static new(): TaggedTransactionInputSet { return new TaggedTransactionInputSet([], true, { kind: "definite", width: 0 }, 2); }
  public static from_cbor_bytes(bytes: Uint8Array): TaggedTransactionInputSet {
    const decoded = decodeCbor(bytes);
    const tagged = decoded.kind === "tag";
    if (tagged && decoded.tag !== 258n) fail("TAG_MISMATCH", "Set tag must be 258", "TaggedTransactionInputSet");
    const array = arrayNode(tagged ? decoded.value : decoded, "TaggedTransactionInputSet");
    return new TaggedTransactionInputSet(array.values.map((value) => TransactionInput.from_cbor_bytes(encodeCbor(value))), tagged, array.encoding, tagged && decoded.kind === "tag" ? decoded.encoding.width : 2);
  }
  public len(): number { return this.#values.length; }
  public add(value: TransactionInput): void { this.#values.push(TransactionInput.from_cbor_bytes(value.to_cbor_bytes())); }
  protected node(): CborValue {
    const array: CborValue = { kind: "array", values: this.#values.map((value) => decodeCbor(value.to_cbor_bytes())), encoding: this.#encoding };
    return this.#tagged ? { kind: "tag", tag: 258n, value: array, encoding: { width: this.#tagWidth } } : array;
  }
  protected jsonValue(): unknown { return this.#values.map((value) => value.to_js_value()); }
}

export enum NativeScriptKind {
  ScriptPubkey = 0,
  ScriptAll = 1,
  ScriptAny = 2,
  ScriptNOfK = 3,
  ScriptInvalidBefore = 4,
  ScriptInvalidHereafter = 5,
}

type NativeState =
  | { readonly kind: NativeScriptKind.ScriptPubkey; readonly hash: Ed25519KeyHash; readonly cbor: ArrayNode }
  | { readonly kind: NativeScriptKind.ScriptAll; readonly scripts: readonly NativeScript[]; readonly cbor: ArrayNode }
  | { readonly kind: NativeScriptKind.ScriptAny; readonly scripts: readonly NativeScript[]; readonly cbor: ArrayNode }
  | { readonly kind: NativeScriptKind.ScriptNOfK; readonly n: bigint; readonly scripts: readonly NativeScript[]; readonly cbor: ArrayNode }
  | { readonly kind: NativeScriptKind.ScriptInvalidBefore; readonly before: bigint; readonly cbor: ArrayNode }
  | { readonly kind: NativeScriptKind.ScriptInvalidHereafter; readonly after: bigint; readonly cbor: ArrayNode };

const nativeStates = new WeakMap<NativeScript, NativeState>();

function scriptArray(values: readonly CborValue[]): ArrayNode {
  return { kind: "array", values, encoding: { kind: "definite", width: 0 } };
}

function nativeFromState(state: NativeState): NativeScript {
  const value = Object.create(NativeScript.prototype) as NativeScript;
  nativeStates.set(value, state);
  return value;
}

function nativeState(value: NativeScript): NativeState {
  const state = nativeStates.get(value);
  if (state === undefined) throw new TypeError("Invalid NativeScript receiver");
  return state;
}

function cloneNative(value: NativeScript): NativeScript { return NativeScript.from_cbor_bytes(value.to_cbor_bytes()); }

function parseNative(node: CborValue): NativeScript {
  const cbor = arrayNode(node, "NativeScript");
  const tagNode = cbor.values[0];
  if (tagNode === undefined) fail("INVALID_STRUCTURE", "NativeScript tag is missing", "NativeScript");
  const tag = uint(tagNode, "NativeScript.tag");
  if (tag === 0n) {
    if (cbor.values.length !== 2) fail("INVALID_STRUCTURE", "ScriptPubkey requires 2 fields", "ScriptPubkey");
    const bytes = cbor.values[1];
    if (bytes?.kind !== "bytes" || bytes.value.length !== 28) fail("INVALID_STRUCTURE", "ScriptPubkey key hash must be 28 bytes", "ScriptPubkey.ed25519_key_hash");
    return nativeFromState({ kind: NativeScriptKind.ScriptPubkey, hash: Ed25519KeyHash.from_raw_bytes(bytes.value), cbor });
  }
  if (tag === 1n || tag === 2n) {
    if (cbor.values.length !== 2) fail("INVALID_STRUCTURE", "Script collection requires 2 fields", "NativeScript");
    const child = cbor.values[1];
    if (child === undefined) fail("INVALID_STRUCTURE", "Script collection is missing its list", "NativeScript");
    const list = arrayNode(child, "NativeScript.native_scripts");
    return tag === 1n
      ? nativeFromState({ kind: NativeScriptKind.ScriptAll, scripts: list.values.map(parseNative), cbor })
      : nativeFromState({ kind: NativeScriptKind.ScriptAny, scripts: list.values.map(parseNative), cbor });
  }
  if (tag === 3n) {
    if (cbor.values.length !== 3 || cbor.values[1] === undefined || cbor.values[2] === undefined) fail("INVALID_STRUCTURE", "ScriptNOfK requires 3 fields", "ScriptNOfK");
    const list = arrayNode(cbor.values[2], "ScriptNOfK.native_scripts");
    return nativeFromState({ kind: NativeScriptKind.ScriptNOfK, n: checkedU64(uint(cbor.values[1], "ScriptNOfK.n"), "ScriptNOfK.n"), scripts: list.values.map(parseNative), cbor });
  }
  if (tag === 4n || tag === 5n) {
    if (cbor.values.length !== 2 || cbor.values[1] === undefined) fail("INVALID_STRUCTURE", "Timelock script requires 2 fields", "NativeScript");
    const slot = checkedU64(uint(cbor.values[1], "NativeScript.slot"), "NativeScript.slot");
    return tag === 4n
      ? nativeFromState({ kind: NativeScriptKind.ScriptInvalidBefore, before: slot, cbor })
      : nativeFromState({ kind: NativeScriptKind.ScriptInvalidHereafter, after: slot, cbor });
  }
  fail("FIXED_VALUE_MISMATCH", `Unknown NativeScript tag ${tag}`, "NativeScript.tag");
}

export class NativeScript extends CborModel {
  private constructor() { super(); }
  public static new_script_pubkey(hash: Ed25519KeyHash): NativeScript {
    const copy = cloneHash(hash);
    return nativeFromState({ kind: NativeScriptKind.ScriptPubkey, hash: copy, cbor: scriptArray([
      { kind: "unsigned", value: 0n, encoding: { width: 0 } },
      { kind: "bytes", value: copy.to_raw_bytes(), encoding: { kind: "definite", width: 0 } },
    ]) });
  }
  public static new_script_all(values: NativeScriptList): NativeScript { return nativeCollection(NativeScriptKind.ScriptAll, values.values()); }
  public static new_script_any(values: NativeScriptList): NativeScript { return nativeCollection(NativeScriptKind.ScriptAny, values.values()); }
  public static new_script_n_of_k(n: bigint, values: NativeScriptList): NativeScript {
    checkedU64(n, "ScriptNOfK.n");
    const scripts = values.values();
    return nativeFromState({ kind: NativeScriptKind.ScriptNOfK, n, scripts, cbor: scriptArray([
      { kind: "unsigned", value: 3n, encoding: { width: 0 } },
      { kind: "unsigned", value: n, encoding: { width: 0 } },
      scriptArray(scripts.map((script) => nativeState(script).cbor)),
    ]) });
  }
  public static new_script_invalid_before(before: bigint): NativeScript { return nativeTimelock(NativeScriptKind.ScriptInvalidBefore, checkedU64(before, "ScriptInvalidBefore.before")); }
  public static new_script_invalid_hereafter(after: bigint): NativeScript { return nativeTimelock(NativeScriptKind.ScriptInvalidHereafter, checkedU64(after, "ScriptInvalidHereafter.after")); }
  public static from_cbor_bytes(bytes: Uint8Array): NativeScript { return parseNative(decodeCbor(bytes)); }
  public static from_cbor_hex(hex: string): NativeScript { return NativeScript.from_cbor_bytes(hexToBytes(hex)); }
  public static from_json(json: string): NativeScript { return nativeFromJson(JSON.parse(json) as NativeScriptJSON); }
  public kind(): NativeScriptKind { return nativeState(this).kind; }
  public hash(): ScriptHash { return ScriptHash.from_raw_bytes(blake2b224(Uint8Array.from([0,...this.to_cbor_bytes()]))); }
  public verify(lowerBound:bigint|null|undefined,upperBound:bigint|null|undefined,keyHashes:Ed25519KeyHashList):boolean { const available=Array.from({length:keyHashes.len()},(_,index)=>keyHashes.get(index).to_hex());const visit=(script:NativeScript):boolean=>{const state=nativeState(script);switch(state.kind){case NativeScriptKind.ScriptPubkey:return available.includes(state.hash.to_hex());case NativeScriptKind.ScriptAll:return state.scripts.every(visit);case NativeScriptKind.ScriptAny:return state.scripts.some(visit);case NativeScriptKind.ScriptNOfK:return state.scripts.filter(visit).length>=Number(state.n);case NativeScriptKind.ScriptInvalidBefore:return lowerBound!=null&&lowerBound>=state.before;case NativeScriptKind.ScriptInvalidHereafter:return upperBound!=null&&upperBound<state.after;}};return visit(this); }
  public get_required_signers():Ed25519KeyHashList { const output=Ed25519KeyHashList.new(),seen=new Set<string>();const visit=(script:NativeScript):void=>{const state=nativeState(script);if(state.kind===NativeScriptKind.ScriptPubkey){const hex=state.hash.to_hex();if(!seen.has(hex)){seen.add(hex);output.add(state.hash);}return;}if(state.kind===NativeScriptKind.ScriptAll||state.kind===NativeScriptKind.ScriptAny||state.kind===NativeScriptKind.ScriptNOfK)state.scripts.forEach(visit);};visit(this);return output; }
  public as_script_pubkey(): ScriptPubkey | undefined { return this.kind() === NativeScriptKind.ScriptPubkey ? scriptPubkey(this) : undefined; }
  public as_script_all(): ScriptAll | undefined { return this.kind() === NativeScriptKind.ScriptAll ? scriptAll(this) : undefined; }
  public as_script_any(): ScriptAny | undefined { return this.kind() === NativeScriptKind.ScriptAny ? scriptAny(this) : undefined; }
  public as_script_n_of_k(): ScriptNOfK | undefined { return this.kind() === NativeScriptKind.ScriptNOfK ? scriptNOfK(this) : undefined; }
  public as_script_invalid_before(): ScriptInvalidBefore | undefined { return this.kind() === NativeScriptKind.ScriptInvalidBefore ? scriptInvalidBefore(this) : undefined; }
  public as_script_invalid_hereafter(): ScriptInvalidHereafter | undefined { return this.kind() === NativeScriptKind.ScriptInvalidHereafter ? scriptInvalidHereafter(this) : undefined; }
  protected node(): CborValue { return nativeState(this).cbor; }
  protected jsonValue(): NativeScriptJSON { return nativeJson(this); }
}

function nativeCollection(kind: NativeScriptKind.ScriptAll | NativeScriptKind.ScriptAny, values: readonly NativeScript[]): NativeScript {
  const scripts = values.map(cloneNative);
  return nativeFromState({ kind, scripts, cbor: scriptArray([
    { kind: "unsigned", value: BigInt(kind), encoding: { width: 0 } },
    scriptArray(scripts.map((script) => nativeState(script).cbor)),
  ]) });
}

function nativeTimelock(kind: NativeScriptKind.ScriptInvalidBefore | NativeScriptKind.ScriptInvalidHereafter, slot: bigint): NativeScript {
  const cbor = scriptArray([{ kind: "unsigned", value: BigInt(kind), encoding: { width: 0 } }, { kind: "unsigned", value: slot, encoding: { width: 0 } }]);
  return kind === NativeScriptKind.ScriptInvalidBefore
    ? nativeFromState({ kind, before: slot, cbor })
    : nativeFromState({ kind, after: slot, cbor });
}

function nativeJson(value: NativeScript): NativeScriptJSON {
  const state = nativeState(value);
  switch (state.kind) {
    case NativeScriptKind.ScriptPubkey: return { ScriptPubkey: { ed25519_key_hash: state.hash.to_hex() } };
    case NativeScriptKind.ScriptAll: return { ScriptAll: { native_scripts: state.scripts.map(nativeJson) } };
    case NativeScriptKind.ScriptAny: return { ScriptAny: { native_scripts: state.scripts.map(nativeJson) } };
    case NativeScriptKind.ScriptNOfK: return { ScriptNOfK: { n: Number(state.n), native_scripts: state.scripts.map(nativeJson) } };
    case NativeScriptKind.ScriptInvalidBefore: return { ScriptInvalidBefore: { before: Number(state.before) } };
    case NativeScriptKind.ScriptInvalidHereafter: return { ScriptInvalidHereafter: { after: Number(state.after) } };
  }
}

function nativeFromJson(value: NativeScriptJSON): NativeScript {
  if ("ScriptPubkey" in value) return NativeScript.new_script_pubkey(Ed25519KeyHash.from_hex(value.ScriptPubkey.ed25519_key_hash));
  if ("ScriptAll" in value) return nativeCollection(NativeScriptKind.ScriptAll, value.ScriptAll.native_scripts.map(nativeFromJson));
  if ("ScriptAny" in value) return nativeCollection(NativeScriptKind.ScriptAny, value.ScriptAny.native_scripts.map(nativeFromJson));
  if ("ScriptNOfK" in value) return nativeFromState(nativeState(NativeScript.new_script_n_of_k(BigInt(value.ScriptNOfK.n), NativeScriptList.from(value.ScriptNOfK.native_scripts.map(nativeFromJson)))));
  if ("ScriptInvalidBefore" in value) return NativeScript.new_script_invalid_before(BigInt(value.ScriptInvalidBefore.before));
  return NativeScript.new_script_invalid_hereafter(BigInt(value.ScriptInvalidHereafter.after));
}

export class NativeScriptList {
  readonly #values: NativeScript[];
  private constructor(values: readonly NativeScript[] = []) { this.#values = values.map(cloneNative); }
  public static new(): NativeScriptList { return new NativeScriptList(); }
  public static from(values: readonly NativeScript[]): NativeScriptList { return new NativeScriptList(values); }
  public len(): number { return this.#values.length; }
  public get(index: number): NativeScript { const value = this.#values[index]; if (value === undefined) throw new RangeError("NativeScriptList index is out of range"); return cloneNative(value); }
  public add(value: NativeScript): void { this.#values.push(cloneNative(value)); }
  public values(): NativeScript[] { return this.#values.map(cloneNative); }
}

abstract class ScriptVariant<Value extends NativeScriptKind> extends CborModel {
  protected constructor(protected readonly script: NativeScript, expected: Value) { super(); if (script.kind() !== expected) throw new TypeError("NativeScript variant mismatch"); }
  protected node(): CborValue { return nativeState(this.script).cbor; }
  protected jsonValue(): unknown { return Object.values(nativeJson(this.script))[0]; }
}

export class ScriptPubkey extends ScriptVariant<NativeScriptKind.ScriptPubkey> {
  private constructor(script: NativeScript) { super(script, NativeScriptKind.ScriptPubkey); }
  public static new(hash: Ed25519KeyHash): ScriptPubkey { return new ScriptPubkey(NativeScript.new_script_pubkey(hash)); }
  public static from_cbor_bytes(bytes: Uint8Array): ScriptPubkey { return new ScriptPubkey(NativeScript.from_cbor_bytes(bytes)); }
  public static from_cbor_hex(hex: string): ScriptPubkey { return ScriptPubkey.from_cbor_bytes(hexToBytes(hex)); }
  public static from_json(json: string): ScriptPubkey { return new ScriptPubkey(nativeFromJson({ ScriptPubkey: JSON.parse(json) as { ed25519_key_hash: string } })); }
  public ed25519_key_hash(): Ed25519KeyHash { return cloneHash((nativeState(this.script) as Extract<NativeState, { kind: NativeScriptKind.ScriptPubkey }>).hash); }
}
function scriptPubkey(value: NativeScript): ScriptPubkey { return ScriptPubkey.from_cbor_bytes(value.to_cbor_bytes()); }

export class ScriptAll extends ScriptVariant<NativeScriptKind.ScriptAll> {
  private constructor(script: NativeScript) { super(script, NativeScriptKind.ScriptAll); }
  public static new(values: NativeScriptList): ScriptAll { return new ScriptAll(NativeScript.new_script_all(values)); }
  public static from_cbor_bytes(bytes: Uint8Array): ScriptAll { return new ScriptAll(NativeScript.from_cbor_bytes(bytes)); }
  public static from_cbor_hex(hex: string): ScriptAll { return ScriptAll.from_cbor_bytes(hexToBytes(hex)); }
  public static from_json(json: string): ScriptAll { return new ScriptAll(nativeFromJson({ ScriptAll: JSON.parse(json) as { native_scripts: NativeScriptJSON[] } })); }
  public native_scripts(): NativeScriptList { return NativeScriptList.from((nativeState(this.script) as Extract<NativeState, { kind: NativeScriptKind.ScriptAll }>).scripts); }
}
function scriptAll(value: NativeScript): ScriptAll { return ScriptAll.from_cbor_bytes(value.to_cbor_bytes()); }

export class ScriptAny extends ScriptVariant<NativeScriptKind.ScriptAny> {
  private constructor(script: NativeScript) { super(script, NativeScriptKind.ScriptAny); }
  public static new(values: NativeScriptList): ScriptAny { return new ScriptAny(NativeScript.new_script_any(values)); }
  public static from_cbor_bytes(bytes: Uint8Array): ScriptAny { return new ScriptAny(NativeScript.from_cbor_bytes(bytes)); }
  public static from_cbor_hex(hex: string): ScriptAny { return ScriptAny.from_cbor_bytes(hexToBytes(hex)); }
  public static from_json(json: string): ScriptAny { return new ScriptAny(nativeFromJson({ ScriptAny: JSON.parse(json) as { native_scripts: NativeScriptJSON[] } })); }
  public native_scripts(): NativeScriptList { return NativeScriptList.from((nativeState(this.script) as Extract<NativeState, { kind: NativeScriptKind.ScriptAny }>).scripts); }
}
function scriptAny(value: NativeScript): ScriptAny { return ScriptAny.from_cbor_bytes(value.to_cbor_bytes()); }

export class ScriptNOfK extends ScriptVariant<NativeScriptKind.ScriptNOfK> {
  private constructor(script: NativeScript) { super(script, NativeScriptKind.ScriptNOfK); }
  public static new(n: bigint, values: NativeScriptList): ScriptNOfK { return new ScriptNOfK(NativeScript.new_script_n_of_k(n, values)); }
  public static from_cbor_bytes(bytes: Uint8Array): ScriptNOfK { return new ScriptNOfK(NativeScript.from_cbor_bytes(bytes)); }
  public static from_cbor_hex(hex: string): ScriptNOfK { return ScriptNOfK.from_cbor_bytes(hexToBytes(hex)); }
  public static from_json(json: string): ScriptNOfK { return new ScriptNOfK(nativeFromJson({ ScriptNOfK: JSON.parse(json) as { n: number; native_scripts: NativeScriptJSON[] } })); }
  public n(): bigint { return (nativeState(this.script) as Extract<NativeState, { kind: NativeScriptKind.ScriptNOfK }>).n; }
  public native_scripts(): NativeScriptList { return NativeScriptList.from((nativeState(this.script) as Extract<NativeState, { kind: NativeScriptKind.ScriptNOfK }>).scripts); }
}
function scriptNOfK(value: NativeScript): ScriptNOfK { return ScriptNOfK.from_cbor_bytes(value.to_cbor_bytes()); }

export class ScriptInvalidBefore extends ScriptVariant<NativeScriptKind.ScriptInvalidBefore> {
  private constructor(script: NativeScript) { super(script, NativeScriptKind.ScriptInvalidBefore); }
  public static new(before: bigint): ScriptInvalidBefore { return new ScriptInvalidBefore(NativeScript.new_script_invalid_before(before)); }
  public static from_cbor_bytes(bytes: Uint8Array): ScriptInvalidBefore { return new ScriptInvalidBefore(NativeScript.from_cbor_bytes(bytes)); }
  public static from_cbor_hex(hex: string): ScriptInvalidBefore { return ScriptInvalidBefore.from_cbor_bytes(hexToBytes(hex)); }
  public static from_json(json: string): ScriptInvalidBefore { return new ScriptInvalidBefore(nativeFromJson({ ScriptInvalidBefore: JSON.parse(json) as { before: number } })); }
  public before(): bigint { return (nativeState(this.script) as Extract<NativeState, { kind: NativeScriptKind.ScriptInvalidBefore }>).before; }
}
function scriptInvalidBefore(value: NativeScript): ScriptInvalidBefore { return ScriptInvalidBefore.from_cbor_bytes(value.to_cbor_bytes()); }

export class ScriptInvalidHereafter extends ScriptVariant<NativeScriptKind.ScriptInvalidHereafter> {
  private constructor(script: NativeScript) { super(script, NativeScriptKind.ScriptInvalidHereafter); }
  public static new(after: bigint): ScriptInvalidHereafter { return new ScriptInvalidHereafter(NativeScript.new_script_invalid_hereafter(after)); }
  public static from_cbor_bytes(bytes: Uint8Array): ScriptInvalidHereafter { return new ScriptInvalidHereafter(NativeScript.from_cbor_bytes(bytes)); }
  public static from_cbor_hex(hex: string): ScriptInvalidHereafter { return ScriptInvalidHereafter.from_cbor_bytes(hexToBytes(hex)); }
  public static from_json(json: string): ScriptInvalidHereafter { return new ScriptInvalidHereafter(nativeFromJson({ ScriptInvalidHereafter: JSON.parse(json) as { after: number } })); }
  public after(): bigint { return (nativeState(this.script) as Extract<NativeState, { kind: NativeScriptKind.ScriptInvalidHereafter }>).after; }
}
function scriptInvalidHereafter(value: NativeScript): ScriptInvalidHereafter { return ScriptInvalidHereafter.from_cbor_bytes(value.to_cbor_bytes()); }

export enum PlutusDataKind { ConstrPlutusData = 0, Map = 1, List = 2, Integer = 3, Bytes = 4 }
type PlutusState =
  | { readonly kind: PlutusDataKind.ConstrPlutusData; readonly constr: ConstrPlutusData; readonly cbor: CborValue }
  | { readonly kind: PlutusDataKind.Map; readonly map: PlutusMap; readonly cbor: CborValue }
  | { readonly kind: PlutusDataKind.List; readonly list: readonly PlutusData[]; readonly cbor: ArrayNode }
  | { readonly kind: PlutusDataKind.Integer; readonly integer: BigInteger; readonly cbor: CborValue }
  | { readonly kind: PlutusDataKind.Bytes; readonly bytes: Uint8Array; readonly cbor: CborValue };
const plutusStates = new WeakMap<PlutusData, PlutusState>();

function plutusFromState(state: PlutusState): PlutusData { const value = Object.create(PlutusData.prototype) as PlutusData; plutusStates.set(value, state); return value; }
function plutusState(value: PlutusData): PlutusState { const state = plutusStates.get(value); if (state === undefined) throw new TypeError("Invalid PlutusData receiver"); return state; }
function clonePlutus(value: PlutusData): PlutusData { return PlutusData.from_cbor_bytes(value.to_cbor_bytes()); }

function parsePlutus(node: CborValue): PlutusData {
  if (node.kind === "unsigned" || node.kind === "negative" || (node.kind === "tag" && (node.tag === 2n || node.tag === 3n))) {
    return plutusFromState({ kind: PlutusDataKind.Integer, integer: BigInteger.from_cbor_bytes(encodeCbor(node)), cbor: node });
  }
  if (node.kind === "bytes") {
    const invalid = node.encoding.kind === "definite"
      ? node.value.length > 64
      : node.encoding.chunks.some((chunk) => !(chunk.value instanceof Uint8Array) || chunk.value.length > 64);
    if (invalid) fail("OUT_OF_RANGE", "PlutusData byte chunks are limited to 64 bytes", "PlutusData.Bytes");
    return plutusFromState({ kind: PlutusDataKind.Bytes, bytes: copyBytes(node.value), cbor: node });
  }
  if (node.kind === "array") return plutusFromState({ kind: PlutusDataKind.List, list: node.values.map(parsePlutus), cbor: node });
  if (node.kind === "map") {
    const map = PlutusMap.fromNode(node);
    return plutusFromState({ kind: PlutusDataKind.Map, map, cbor: node });
  }
  if (node.kind === "tag") {
    const constr = ConstrPlutusData.fromNode(node);
    return plutusFromState({ kind: PlutusDataKind.ConstrPlutusData, constr, cbor: node });
  }
  fail("NO_VARIANT_MATCHED", "No PlutusData variant matched", "PlutusData");
}

export class PlutusData extends CborModel {
  private constructor() { super(); }
  public static new_integer(value: BigInteger): PlutusData { const cbor = decodeCbor(value.to_cbor_bytes()); return plutusFromState({ kind: PlutusDataKind.Integer, integer: BigInteger.from_cbor_bytes(value.to_cbor_bytes()), cbor }); }
  public static new_bytes(bytes: Uint8Array): PlutusData {
    const copy = copyBytes(bytes);
    const cbor: CborValue = copy.length <= 64
      ? { kind: "bytes", value: copy, encoding: { kind: "definite", width: 0 } }
      : { kind: "bytes", value: copy, encoding: { kind: "indefinite", chunks: Array.from({ length: Math.ceil(copy.length / 64) }, (_, index) => ({ value: copy.slice(index * 64, (index + 1) * 64), width: 0 as const })) } };
    return plutusFromState({ kind: PlutusDataKind.Bytes, bytes: copy, cbor });
  }
  public static new_list(value: PlutusDataList): PlutusData { const list = value.values(); const cbor = scriptArray(list.map((item) => plutusState(item).cbor)); return plutusFromState({ kind: PlutusDataKind.List, list, cbor }); }
  public static new_map(value: PlutusMap): PlutusData { const map = PlutusMap.from_cbor_bytes(value.to_cbor_bytes()); return plutusFromState({ kind: PlutusDataKind.Map, map, cbor: map.nodeValue() }); }
  public static new_constr_plutus_data(value: ConstrPlutusData): PlutusData { const constr = ConstrPlutusData.from_cbor_bytes(value.to_cbor_bytes()); return plutusFromState({ kind: PlutusDataKind.ConstrPlutusData, constr, cbor: constr.nodeValue() }); }
  public static from_cbor_bytes(bytes: Uint8Array): PlutusData { return parsePlutus(decodeCbor(bytes)); }
  public static from_cbor_hex(hex: string): PlutusData { return PlutusData.from_cbor_bytes(hexToBytes(hex)); }
  public static from_json(json: string): PlutusData { return plutusFromJson(JSON.parse(json) as PlutusDataJSON); }
  public kind(): PlutusDataKind { return plutusState(this).kind; }
  public as_integer(): BigInteger | undefined { const state = plutusState(this); return state.kind === PlutusDataKind.Integer ? BigInteger.from_cbor_bytes(state.integer.to_cbor_bytes()) : undefined; }
  public as_bytes(): Uint8Array | undefined { const state = plutusState(this); return state.kind === PlutusDataKind.Bytes ? copyBytes(state.bytes) : undefined; }
  public as_list(): PlutusDataList | undefined { const state = plutusState(this); return state.kind === PlutusDataKind.List ? PlutusDataList.from(state.list) : undefined; }
  public as_map(): PlutusMap | undefined { const state = plutusState(this); return state.kind === PlutusDataKind.Map ? PlutusMap.from_cbor_bytes(state.map.to_cbor_bytes()) : undefined; }
  public as_constr_plutus_data(): ConstrPlutusData | undefined { const state = plutusState(this); return state.kind === PlutusDataKind.ConstrPlutusData ? ConstrPlutusData.from_cbor_bytes(state.constr.to_cbor_bytes()) : undefined; }
  public equals(other: PlutusData): boolean { return bytesEqual(this.to_canonical_cbor_bytes(), other.to_canonical_cbor_bytes()); }
  public to_cardano_node_format(): PlutusData { return nodeFormat(this); }
  public override to_js_value(): any { return serdePlutus(this); }
  protected node(): CborValue { return plutusState(this).cbor; }
  protected jsonValue(): PlutusDataJSON { return plutusJson(this); }
}

function serdeNumber(value: string): { readonly "$serde_json::private::Number": string } {
  return { "$serde_json::private::Number": value };
}

function serdePlutus(value: PlutusData): unknown {
  const state = plutusState(value);
  switch (state.kind) {
    case PlutusDataKind.ConstrPlutusData: return { constructor: serdeNumber(state.constr.alternative().toString()), fields: state.constr.fields().values().map(serdePlutus) };
    case PlutusDataKind.Map: return { map: state.map.entries().map(([key, item]) => ({ k: serdePlutus(key), v: serdePlutus(item) })) };
    case PlutusDataKind.List: return { list: state.list.map(serdePlutus) };
    case PlutusDataKind.Integer: return { int: serdeNumber(state.integer.to_str()) };
    case PlutusDataKind.Bytes: return { bytes: bytesToHex(state.bytes) };
  }
}

function plutusJson(value: PlutusData): PlutusDataJSON {
  const state = plutusState(value);
  switch (state.kind) {
    case PlutusDataKind.ConstrPlutusData: return { constructor: Number(state.constr.alternative()), fields: state.constr.fields().values().map(plutusJson) };
    case PlutusDataKind.Map: return { map: state.map.entries().map(([key, item]) => ({ k: plutusJson(key), v: plutusJson(item) })) };
    case PlutusDataKind.List: return { list: state.list.map(plutusJson) };
    case PlutusDataKind.Integer: return { int: Number(state.integer.to_str()) };
    case PlutusDataKind.Bytes: return { bytes: bytesToHex(state.bytes) };
  }
}

const MAX_JSON_DEPTH = 128;
const MAX_JSON_NODES = 100_000;
interface JsonBudget { nodes: number }

function plutusFromJson(value: PlutusDataJSON, depth = 0, budget: JsonBudget = { nodes: 0 }): PlutusData {
  if (depth > MAX_JSON_DEPTH) throw new RangeError(`JSON nesting exceeds ${MAX_JSON_DEPTH}`);
  budget.nodes += 1;
  if (budget.nodes > MAX_JSON_NODES) throw new RangeError(`JSON value count exceeds ${MAX_JSON_NODES}`);
  if (typeof value !== "object" || value === null || Array.isArray(value)) throw new TypeError("Invalid PlutusData JSON");
  if (Object.hasOwn(value, "constructor")) {
    const constructor = value as Extract<PlutusDataJSON, { constructor: number }>;
    if (!Number.isSafeInteger(constructor.constructor) || constructor.constructor < 0 || !Array.isArray(constructor.fields)) throw new TypeError("Invalid Plutus constructor JSON");
    return PlutusData.new_constr_plutus_data(ConstrPlutusData.new(BigInt(constructor.constructor), PlutusDataList.from(constructor.fields.map((item) => plutusFromJson(item, depth + 1, budget)))));
  }
  if (Object.hasOwn(value, "map")) {
    const entries = (value as Extract<PlutusDataJSON, { map: unknown }>).map;
    if (!Array.isArray(entries)) throw new TypeError("Invalid Plutus map JSON");
    const map = PlutusMap.new(); for (const entry of entries) { if (typeof entry !== "object" || entry === null || !("k" in entry) || !("v" in entry)) throw new TypeError("Invalid Plutus map entry JSON"); map.append(plutusFromJson(entry.k, depth + 1, budget), plutusFromJson(entry.v, depth + 1, budget)); } return PlutusData.new_map(map);
  }
  if (Object.hasOwn(value, "list")) { const list=(value as Extract<PlutusDataJSON, { list: unknown }>).list;if(!Array.isArray(list))throw new TypeError("Invalid Plutus list JSON");return PlutusData.new_list(PlutusDataList.from(list.map((item) => plutusFromJson(item, depth + 1, budget)))); }
  if (Object.hasOwn(value, "int")) { const integer=(value as Extract<PlutusDataJSON, { int: number }>).int;if(!Number.isSafeInteger(integer))throw new TypeError("Invalid Plutus integer JSON");return PlutusData.new_integer(BigInteger.from_str(String(integer))); }
  const bytes=(value as Extract<PlutusDataJSON, { bytes: string }>).bytes;if(typeof bytes!=="string")throw new TypeError("Invalid Plutus bytes JSON");return PlutusData.new_bytes(hexToBytes(bytes));
}

function nodeFormat(value: PlutusData): PlutusData {
  const state = plutusState(value);
  if (state.kind === PlutusDataKind.Integer) return PlutusData.new_integer(state.integer);
  if (state.kind === PlutusDataKind.Bytes) return PlutusData.new_bytes(state.bytes);
  if (state.kind === PlutusDataKind.List) {
    const items = state.list.map(nodeFormat);
    const node: ArrayNode = { kind: "array", values: items.map((item) => plutusState(item).cbor), encoding: items.length === 0 ? { kind: "definite", width: 0 } : { kind: "indefinite" } };
    return plutusFromState({ kind: PlutusDataKind.List, list: items, cbor: node });
  }
  if (state.kind === PlutusDataKind.Map) {
    const entries = state.map.entries().map(([key, item]) => [nodeFormat(key), nodeFormat(item)] as const);
    const map = PlutusMap.fromEntries(entries, entries.length === 0 ? { kind: "definite", width: 0 } : { kind: "indefinite" });
    return plutusFromState({ kind: PlutusDataKind.Map, map, cbor: map.nodeValue() });
  }
  const fields = state.constr.fields().values().map(nodeFormat);
  return PlutusData.new_constr_plutus_data(ConstrPlutusData.new(state.constr.alternative(), PlutusDataList.from(fields)));
}

export class PlutusDataList {
  readonly #values: PlutusData[];
  private constructor(values: readonly PlutusData[] = []) { this.#values = values.map(clonePlutus); }
  public static new(): PlutusDataList { return new PlutusDataList(); }
  public static from(values: readonly PlutusData[]): PlutusDataList { return new PlutusDataList(values); }
  public len(): number { return this.#values.length; }
  public get(index: number): PlutusData { const value = this.#values[index]; if (value === undefined) throw new RangeError("PlutusDataList index is out of range"); return clonePlutus(value); }
  public add(value: PlutusData): void { this.#values.push(clonePlutus(value)); }
  public values(): PlutusData[] { return this.#values.map(clonePlutus); }
}

interface ConstrState { readonly alternative: bigint; readonly fields: readonly PlutusData[]; readonly cbor: CborValue }
const constrStates = new WeakMap<ConstrPlutusData, ConstrState>();

export class ConstrPlutusData extends CborModel {
  private constructor(state: ConstrState) { super(); constrStates.set(this, state); }
  public static new(alternative: bigint, fields: PlutusDataList): ConstrPlutusData {
    checkedU64(alternative, "ConstrPlutusData.alternative");
    const values = fields.values();
    const tag = alternative <= 6n ? 121n + alternative : alternative <= 127n ? 1273n + alternative : 102n;
    const body = tag === 102n
      ? scriptArray([{ kind: "unsigned", value: alternative, encoding: { width: 0 } }, scriptArray(values.map((value) => plutusState(value).cbor))])
      : scriptArray(values.map((value) => plutusState(value).cbor));
    return new ConstrPlutusData({ alternative, fields: values, cbor: { kind: "tag", tag, value: body, encoding: { width: 0 } } });
  }
  public static fromNode(node: CborValue): ConstrPlutusData {
    if (node.kind !== "tag") fail("INVALID_STRUCTURE", "ConstrPlutusData must be tagged", "ConstrPlutusData");
    if (node.tag === 102n) {
      const body = arrayNode(node.value, "ConstrPlutusData", 2);
      if (body.values[0] === undefined || body.values[1] === undefined) fail("INVALID_STRUCTURE", "General constructor is incomplete", "ConstrPlutusData");
      const alternative = checkedU64(uint(body.values[0], "ConstrPlutusData.alternative"), "ConstrPlutusData.alternative");
      const fields = arrayNode(body.values[1], "ConstrPlutusData.fields").values.map(parsePlutus);
      return new ConstrPlutusData({ alternative, fields, cbor: node });
    }
    const alternative = node.tag >= 121n && node.tag <= 127n ? node.tag - 121n : node.tag >= 1280n && node.tag <= 1400n ? node.tag - 1273n : undefined;
    if (alternative === undefined) fail("TAG_MISMATCH", `Invalid constructor tag ${node.tag}`, "ConstrPlutusData");
    return new ConstrPlutusData({ alternative, fields: arrayNode(node.value, "ConstrPlutusData.fields").values.map(parsePlutus), cbor: node });
  }
  public static from_cbor_bytes(bytes: Uint8Array): ConstrPlutusData { return ConstrPlutusData.fromNode(decodeCbor(bytes)); }
  public static from_cbor_hex(hex: string): ConstrPlutusData { return ConstrPlutusData.from_cbor_bytes(hexToBytes(hex)); }
  public static from_json(json: string): ConstrPlutusData { const value = JSON.parse(json) as { alternative: number; fields: PlutusDataJSON[] };if(!Number.isSafeInteger(value.alternative)||value.alternative<0||!Array.isArray(value.fields))throw new TypeError("Invalid ConstrPlutusData JSON");const budget:JsonBudget={nodes:0};return ConstrPlutusData.new(BigInt(value.alternative), PlutusDataList.from(value.fields.map((item)=>plutusFromJson(item,1,budget)))); }
  public alternative(): bigint { return constrState(this).alternative; }
  public fields(): PlutusDataList { return PlutusDataList.from(constrState(this).fields); }
  public nodeValue(): CborValue { return constrState(this).cbor; }
  public override to_js_value(): any { const state = constrState(this); return { alternative: Number(state.alternative), fields: state.fields.map(serdePlutus) }; }
  protected node(): CborValue { return this.nodeValue(); }
  protected jsonValue(): unknown { const state = constrState(this); return { alternative: Number(state.alternative), fields: state.fields.map(plutusJson) }; }
}
function constrState(value: ConstrPlutusData): ConstrState { const state = constrStates.get(value); if (state === undefined) throw new TypeError("Invalid ConstrPlutusData receiver"); return state; }

interface PlutusMapState { entries: Array<readonly [PlutusData, PlutusData]>; encoding: CborLengthEncoding }
const plutusMaps = new WeakMap<PlutusMap, PlutusMapState>();

export class PlutusMap extends CborModel {
  private constructor(state: PlutusMapState) { super(); plutusMaps.set(this, state); }
  public static new(): PlutusMap { return new PlutusMap({ entries: [], encoding: { kind: "definite", width: 0 } }); }
  public static fromNode(node: MapNode): PlutusMap { return new PlutusMap({ entries: node.entries.map(([key, value]) => [parsePlutus(key), parsePlutus(value)]), encoding: node.encoding }); }
  public static fromEntries(entries: readonly (readonly [PlutusData, PlutusData])[], encoding: CborLengthEncoding): PlutusMap { return new PlutusMap({ entries: entries.map(([key, value]) => [clonePlutus(key), clonePlutus(value)]), encoding }); }
  public static from_cbor_bytes(bytes: Uint8Array): PlutusMap { return PlutusMap.fromNode(mapNode(decodeCbor(bytes), "PlutusMap")); }
  public static from_cbor_hex(hex: string): PlutusMap { return PlutusMap.from_cbor_bytes(hexToBytes(hex)); }
  public len(): number { return plutusMapState(this).entries.length; }
  public is_empty(): boolean { return this.len() === 0; }
  public set(key: PlutusData, value: PlutusData): void { const state = plutusMapState(this); state.entries = state.entries.filter(([existing]) => !existing.equals(key)); state.entries.push([clonePlutus(key), clonePlutus(value)]); }
  public append(key: PlutusData, value: PlutusData): void { plutusMapState(this).entries.push([clonePlutus(key), clonePlutus(value)]); }
  public get(key: PlutusData): PlutusData | undefined { const found = plutusMapState(this).entries.find(([existing]) => existing.equals(key)); return found === undefined ? undefined : clonePlutus(found[1]); }
  public get_all(key: PlutusData): PlutusDataList | undefined { const found = plutusMapState(this).entries.filter(([existing]) => existing.equals(key)).map((entry) => entry[1]); return found.length === 0 ? undefined : PlutusDataList.from(found); }
  public keys(): PlutusDataList { return PlutusDataList.from(plutusMapState(this).entries.map(([key]) => key)); }
  public entries(): Array<readonly [PlutusData, PlutusData]> { return plutusMapState(this).entries.map(([key, value]) => [clonePlutus(key), clonePlutus(value)]); }
  public nodeValue(): MapNode { const state = plutusMapState(this); return { kind: "map", entries: state.entries.map(([key, value]) => [plutusState(key).cbor, plutusState(value).cbor]), encoding: state.encoding }; }
  protected node(): CborValue { return this.nodeValue(); }
  protected jsonValue(): unknown { return { map: this.entries().map(([key, value]) => ({ k: plutusJson(key), v: plutusJson(value) })) }; }
}
function plutusMapState(value: PlutusMap): PlutusMapState { const state = plutusMaps.get(value); if (state === undefined) throw new TypeError("Invalid PlutusMap receiver"); return state; }
