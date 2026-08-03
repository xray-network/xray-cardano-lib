import { ConstrPlutusData, PlutusData, PlutusDataList } from "@xray-network/xray-cardano-lib-chain";
import { AssetName } from "@xray-network/xray-cardano-lib-chain/conway";
import { ScriptHash } from "@xray-network/xray-cardano-lib-crypto";
import { make_labeled_asset_name, split_labeled_asset_name } from "../cip67/index.js";

export enum CIP68TokenClass { NFT = 222, FT = 333, RFT = 444 }
export const CIP68_REFERENCE_LABEL = 100;

function recognized(value: number): value is CIP68TokenClass { return value === 222 || value === 333 || value === 444; }
function sameBytes(left: Uint8Array, right: Uint8Array): boolean { return left.length === right.length && left.every((value, index) => value === right[index]); }

export function make_cip68_user_asset_name(tokenClass: CIP68TokenClass, content: Uint8Array): AssetName {
  if (!recognized(tokenClass)) throw new RangeError("unsupported CIP-68 token class");
  return make_labeled_asset_name(tokenClass, content);
}
export function make_cip68_reference_asset_name(content: Uint8Array): AssetName { return make_labeled_asset_name(CIP68_REFERENCE_LABEL, content); }

export function validate_cip68_asset_pair(userPolicy: ScriptHash, user: AssetName, referencePolicy: ScriptHash, reference: AssetName): CIP68TokenClass {
  if (!sameBytes(userPolicy.to_raw_bytes(), referencePolicy.to_raw_bytes())) throw new TypeError("CIP-68 user and reference policies differ");
  const userParts = split_labeled_asset_name(user), referenceParts = split_labeled_asset_name(reference);
  if (!recognized(userParts.label)) throw new TypeError("asset does not use a supported CIP-68 user label");
  if (referenceParts.label !== CIP68_REFERENCE_LABEL) throw new TypeError("CIP-68 reference asset must use label 100");
  if (!sameBytes(userParts.content, referenceParts.content)) throw new TypeError("CIP-68 user and reference contents differ");
  return userParts.label;
}

export class CIP68Datum {
  readonly #data: PlutusData; readonly #metadata: PlutusData; readonly #extra: PlutusData; readonly #version: number;
  private constructor(data: PlutusData, metadata: PlutusData, version: number, extra: PlutusData) {
    this.#data = PlutusData.from_cbor_bytes(data.to_cbor_bytes()); this.#metadata = PlutusData.from_cbor_bytes(metadata.to_cbor_bytes());
    this.#version = version; this.#extra = PlutusData.from_cbor_bytes(extra.to_cbor_bytes()); Object.freeze(this);
  }
  public static new(metadata: PlutusData, version: number, extra?: PlutusData): CIP68Datum {
    if (!Number.isInteger(version) || version < 1 || version > 4) throw new RangeError("CIP-68 datum version must be 1 through 4");
    const unit = extra ?? PlutusData.new_constr_plutus_data(ConstrPlutusData.new(0n, PlutusDataList.new()));
    const fields = PlutusDataList.from([metadata, PlutusData.new_integer((awaitBigInteger(String(version)))), unit]);
    return CIP68Datum.from_data(PlutusData.new_constr_plutus_data(ConstrPlutusData.new(0n, fields)));
  }
  public static from_data(data: PlutusData): CIP68Datum {
    const constructor = data.as_constr_plutus_data();
    if (constructor?.alternative() !== 0n || constructor.fields().len() !== 3) throw new TypeError("CIP-68 datum must be constructor 0 with exactly three fields");
    const fields = constructor.fields(), integer = fields.get(1).as_integer();
    if (integer === undefined || !/^[1-4]$/.test(integer.to_str())) throw new RangeError("CIP-68 datum version must be 1 through 4");
    return new CIP68Datum(data, fields.get(0), Number(integer.to_str()), fields.get(2));
  }
  public validate_for(tokenClass: CIP68TokenClass, policy?: ScriptHash, userAssetName?: AssetName): void {
    if (!recognized(tokenClass)) throw new RangeError("unsupported CIP-68 token class");
    if (tokenClass === CIP68TokenClass.RFT && this.#version < 2) throw new TypeError("CIP-68 RFT metadata starts at version 2");
    enforceDataLimits(this.#data);
    let metadata = this.#metadata;
    if (this.#version === 4) {
      if (policy === undefined || userAssetName === undefined) throw new TypeError("CIP-68 version 4 validation requires policy and user asset identity");
      const parts = split_labeled_asset_name(userAssetName);
      if (parts.label !== tokenClass) throw new TypeError("CIP-68 datum class differs from the user asset label");
      metadata = requiredMapValue(requiredMapValue(requiredMapValue(this.#metadata, new TextEncoder().encode("721"), "721"), policy.to_raw_bytes(), "policy"), parts.content, "asset content");
    } else {
      const entries = this.#metadata.as_map()?.entries();
      if (entries?.some(([key]) => equal(key.as_bytes() ?? new Uint8Array(), new TextEncoder().encode("721")))) throw new TypeError("CIP-68 direct metadata must not contain a nested 721 key");
    }
    validateMetadata(metadata, tokenClass, this.#version);
  }
  public version(): number { return this.#version; }
  public metadata(): PlutusData { return PlutusData.from_cbor_bytes(this.#metadata.to_cbor_bytes()); }
  public extra(): PlutusData { return PlutusData.from_cbor_bytes(this.#extra.to_cbor_bytes()); }
  public to_data(): PlutusData { return PlutusData.from_cbor_bytes(this.#data.to_cbor_bytes()); }
}

import { BigInteger } from "@xray-network/xray-cardano-lib-core";
function awaitBigInteger(value: string): BigInteger { return BigInteger.from_str(value); }

function equal(left: Uint8Array, right: Uint8Array): boolean { return left.length === right.length && left.every((byte, index) => byte === right[index]); }
function requiredMapValue(data: PlutusData, key: Uint8Array, name: string): PlutusData {
  const entries = data.as_map()?.entries(); if (entries === undefined) throw new TypeError(`CIP-68 ${name} container must be a map`);
  const matches = entries.filter(([candidate]) => { const bytes = candidate.as_bytes(); return bytes !== undefined && equal(bytes, key); });
  if (matches.length !== 1) throw new TypeError(`CIP-68 ${name} path must occur exactly once`);
  return matches[0]![1];
}
function textKey(value: PlutusData): string | undefined { const bytes = value.as_bytes(); if (bytes === undefined) return undefined; try { return new TextDecoder("utf-8", { fatal: true }).decode(bytes); } catch { return undefined; } }
function enforceDataLimits(data: PlutusData): void {
  const budget = { nodes: 0, mapEntries: 0, bytes: 0 };
  const visit = (value: PlutusData, depth: number): void => {
    if (depth > 128) throw new RangeError("CIP-68 Data depth exceeds 128");
    if (++budget.nodes > 100_000) throw new RangeError("CIP-68 Data node count exceeds 100000");
    const bytes = value.as_bytes(); if (bytes !== undefined) { budget.bytes += bytes.length; if (budget.bytes > 16_777_216) throw new RangeError("CIP-68 aggregate byte strings exceed 16 MiB"); return; }
    const list = value.as_list(); if (list !== undefined) { list.values().forEach((item) => visit(item, depth + 1)); return; }
    const map = value.as_map(); if (map !== undefined) { const entries = map.entries(); budget.mapEntries += entries.length; if (budget.mapEntries > 10_000) throw new RangeError("CIP-68 metadata maps exceed 10000 entries"); entries.forEach(([key, item]) => { visit(key, depth + 1); visit(item, depth + 1); }); return; }
    const constructor = value.as_constr_plutus_data(); if (constructor !== undefined) constructor.fields().values().forEach((item) => visit(item, depth + 1));
  };
  visit(data, 0);
}
function uriText(value: PlutusData, version: number): string {
  const direct = value.as_bytes(); let bytes: Uint8Array;
  if (direct !== undefined) bytes = direct;
  else {
    const chunks = value.as_list(); if (version < 3 || chunks === undefined || chunks.len() === 0) throw new TypeError("CIP-68 URI chunks require version 3 or 4");
    const values = chunks.values().map((item) => { const chunk = item.as_bytes(); if (chunk === undefined) throw new TypeError("CIP-68 URI chunks must be byte strings"); return chunk; });
    bytes = new Uint8Array(values.reduce((sum, item) => sum + item.length, 0)); let offset = 0; for (const item of values) { bytes.set(item, offset); offset += item.length; }
  }
  let text: string; try { text = new TextDecoder("utf-8", { fatal: true }).decode(bytes); } catch { throw new TypeError("CIP-68 URI must be valid UTF-8"); }
  if (!/^(?:https:\/\/|ipfs:|ar:|data:[^,]+,).+/u.test(text)) throw new TypeError("CIP-68 URI must use https, ipfs, ar, or data");
  return text;
}
function validateFiles(value: PlutusData, version: number): void {
  const files = value.as_list(); if (files === undefined) throw new TypeError("CIP-68 files must be a list");
  for (const file of files.values()) {
    const entries = file.as_map()?.entries(); if (entries === undefined) throw new TypeError("CIP-68 file entry must be a map");
    const keys = entries.map(([key]) => textKey(key)); if (!keys.includes("mediaType") || !keys.includes("src")) throw new TypeError("CIP-68 file entry requires mediaType and src");
    for (const name of ["mediaType", "src", "name"]) if (keys.filter((key) => key === name).length > 1) throw new TypeError(`duplicate CIP-68 file key ${name}`);
    for (const [key, item] of entries) { const name = textKey(key); if (name === "src") uriText(item, version); else if ((name === "mediaType" || name === "name") && item.as_bytes() === undefined) throw new TypeError(`CIP-68 file ${name} must be bytes`); }
  }
}
function validateMetadata(data: PlutusData, tokenClass: CIP68TokenClass, version: number): void {
  const entries = data.as_map()?.entries(); if (entries === undefined) throw new TypeError("CIP-68 metadata must be a map");
  const known = new Map<string, PlutusData>();
  for (const [key, value] of entries) { const name = textKey(key); if (name === undefined) continue; if (known.has(name)) throw new TypeError(`duplicate CIP-68 metadata key ${name}`); known.set(name, value); }
  const required = tokenClass === CIP68TokenClass.FT ? ["name", "description"] : ["name", "image"];
  for (const name of required) if (!known.has(name)) throw new TypeError(`CIP-68 ${tokenClass} metadata requires ${name}`);
  for (const name of ["name", "description", "ticker", "mediaType"]) if (known.has(name) && known.get(name)?.as_bytes() === undefined) throw new TypeError(`CIP-68 ${name} must be bytes`);
  for (const name of ["image", "url", "logo"]) { const value = known.get(name); if (value !== undefined) uriText(value, version); }
  const decimals = known.get("decimals"); if (decimals !== undefined && decimals.as_integer() === undefined) throw new TypeError("CIP-68 decimals must be an integer");
  const files = known.get("files"); if (files !== undefined) validateFiles(files, version);
}
