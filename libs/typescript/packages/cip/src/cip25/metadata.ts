import {
  AssetName,
  Metadata,
  TransactionMetadatum,
} from "@xray-network/xray-cardano-lib-chain";
import {
  UINT64_MAX,
  bytesToHex,
  decodeCbor,
  encodeCbor,
  hexToBytes,
} from "@xray-network/xray-cardano-lib-core";
import type { CborValue } from "@xray-network/xray-cardano-lib-core";
import { ScriptHash } from "@xray-network/xray-cardano-lib-crypto";

const METADATA_LABEL = 721n;
const utf8 = new TextEncoder();
const utf8Fatal = new TextDecoder("utf-8", { fatal: true });

function uint(value: bigint): CborValue {
  if (value < 0n || value > UINT64_MAX) throw new RangeError("value must fit uint64");
  return { kind: "unsigned", value, encoding: { width: 0 } };
}
function bytes(value: Uint8Array): CborValue { return { kind: "bytes", value: value.slice(), encoding: { kind: "definite", width: 0 } }; }
function text(value: string): CborValue { return { kind: "text", value, encoding: { kind: "definite", width: 0 } }; }
function array(values: readonly CborValue[]): CborValue { return { kind: "array", values, encoding: { kind: "definite", width: 0 } }; }
function map(entries: ReadonlyArray<readonly [CborValue, CborValue]>): CborValue { return { kind: "map", entries, encoding: { kind: "definite", width: 0 } }; }
function expectMap(value: CborValue, name: string): Extract<CborValue, { kind: "map" }> { if (value.kind !== "map") throw new TypeError(`${name} requires a CBOR map`); return value; }
function fromHex(value: string): Uint8Array { return hexToBytes(value); }
function jsonObject(value: unknown, name: string): Record<string, unknown> { if (typeof value !== "object" || value === null || Array.isArray(value)) throw new TypeError(`${name} JSON must be an object`); return value as Record<string, unknown>; }

interface Cip25Serializable {
  toNode(): CborValue;
  to_js_value(): unknown;
}

abstract class Cip25Data implements Cip25Serializable {
  public abstract toNode(): CborValue;
  public abstract to_js_value(): unknown;
  public to_cbor_bytes(): Uint8Array { return encodeCbor(this.toNode()); }
  public to_cbor_hex(): string { return bytesToHex(this.to_cbor_bytes()); }
  public to_json(): string { return JSON.stringify(this.to_js_value(), null, 2); }
}

export enum CIP25Version { V1 = 0, V2 = 1 }
export enum CIP25ChunkableStringKind { Single = 0, Chunked = 1 }

export class CIP25String64 extends Cip25Data {
  readonly #value: string;
  private constructor(value: string) { super(); if (utf8.encode(value).length > 64) throw new RangeError("CIP25String64 is limited to 64 UTF-8 bytes"); this.#value = value; }
  public static new(value: string): CIP25String64 { return new CIP25String64(value); }
  public static from_cbor_bytes(value: Uint8Array): CIP25String64 { return CIP25String64.parse(decodeCbor(value)); }
  public static from_cbor_hex(value: string): CIP25String64 { return this.from_cbor_bytes(fromHex(value)); }
  public static from_json(value: string): CIP25String64 { const parsed: unknown = JSON.parse(value); if (typeof parsed !== "string") throw new TypeError("CIP25String64 JSON must be a string"); return this.new(parsed); }
  public static parse(value: CborValue): CIP25String64 { if (value.kind !== "text") throw new TypeError("CIP25String64 requires CBOR text"); return this.new(value.value); }
  public get(): string { return this.#value; }
  public toNode(): CborValue { return text(this.#value); }
  public to_js_value(): string { return this.#value; }
}

export class CIP25String64List {
  readonly #values: CIP25String64[] = [];
  public static new(): CIP25String64List { return new CIP25String64List(); }
  public static from(values: readonly CIP25String64[]): CIP25String64List { const output = this.new(); for (const value of values) output.add(value); return output; }
  public len(): number { return this.#values.length; }
  public get(index: number): CIP25String64 { const value = this.#values[index]; if (value === undefined) throw new RangeError("CIP25String64 index out of bounds"); return CIP25String64.new(value.get()); }
  public add(value: CIP25String64): void { this.#values.push(CIP25String64.new(value.get())); }
  public values(): CIP25String64[] { return this.#values.map((value) => CIP25String64.new(value.get())); }
}

export class CIP25ChunkableString extends Cip25Data {
  readonly #single: CIP25String64 | undefined;
  readonly #chunked: CIP25String64[] | undefined;
  private constructor(single?: CIP25String64, chunked?: readonly CIP25String64[]) { super(); this.#single = single === undefined ? undefined : CIP25String64.new(single.get()); this.#chunked = chunked?.map((value) => CIP25String64.new(value.get())); }
  public static new_single(value: CIP25String64): CIP25ChunkableString { return new CIP25ChunkableString(value); }
  public static new_chunked(value: CIP25String64List): CIP25ChunkableString { return new CIP25ChunkableString(undefined, value.values()); }
  public static from_string(value: string): CIP25ChunkableString {
    const encoded = utf8.encode(value);
    if (encoded.length <= 64) return this.new_single(CIP25String64.new(value));
    const chunks = CIP25String64List.new();
    for (let offset = 0; offset < encoded.length; offset += 64) chunks.add(CIP25String64.new(utf8Fatal.decode(encoded.slice(offset, Math.min(offset + 64, encoded.length)))));
    return this.new_chunked(chunks);
  }
  public static from_cbor_bytes(value: Uint8Array): CIP25ChunkableString { return this.parse(decodeCbor(value)); }
  public static from_cbor_hex(value: string): CIP25ChunkableString { return this.from_cbor_bytes(fromHex(value)); }
  public static from_json(value: string): CIP25ChunkableString {
    const parsed: unknown = JSON.parse(value);
    if (typeof parsed === "string") return this.from_string(parsed);
    const object = jsonObject(parsed, "CIP25ChunkableString");
    if (typeof object["Single"] === "string") return this.new_single(CIP25String64.new(object["Single"]));
    if (Array.isArray(object["Chunked"])) return new CIP25ChunkableString(undefined, object["Chunked"].map((item) => { if (typeof item !== "string") throw new TypeError("chunk must be a string"); return CIP25String64.new(item); }));
    throw new TypeError("invalid CIP25ChunkableString JSON");
  }
  public static parse(value: CborValue): CIP25ChunkableString {
    if (value.kind === "text") return this.new_single(CIP25String64.parse(value));
    if (value.kind === "array") return new CIP25ChunkableString(undefined, value.values.map((item: CborValue) => CIP25String64.parse(item)));
    throw new TypeError("CIP25ChunkableString requires text or an array of text");
  }
  public kind(): CIP25ChunkableStringKind { return this.#single === undefined ? CIP25ChunkableStringKind.Chunked : CIP25ChunkableStringKind.Single; }
  public as_single(): CIP25String64 | undefined { return this.#single === undefined ? undefined : CIP25String64.new(this.#single.get()); }
  public as_chunked(): CIP25String64List | undefined { return this.#chunked === undefined ? undefined : CIP25String64List.from(this.#chunked); }
  public to_string(): string { return this.#single?.get() ?? this.#chunked?.map((value) => value.get()).join("") ?? ""; }
  public toNode(): CborValue { return this.#single === undefined ? array((this.#chunked ?? []).map((value) => value.toNode())) : this.#single.toNode(); }
  public to_js_value(): unknown { return this.#single === undefined ? { Chunked: (this.#chunked ?? []).map((value) => value.get()) } : { Single: this.#single.get() }; }
}

export class CIP25FilesDetails extends Cip25Data {
  readonly #name: CIP25String64; readonly #mediaType: CIP25String64; readonly #src: CIP25ChunkableString;
  private constructor(name: CIP25String64, mediaType: CIP25String64, src: CIP25ChunkableString) { super(); this.#name = CIP25String64.new(name.get()); this.#mediaType = CIP25String64.new(mediaType.get()); this.#src = CIP25ChunkableString.from_cbor_bytes(src.to_cbor_bytes()); }
  public static new(name: CIP25String64, mediaType: CIP25String64, src: CIP25ChunkableString): CIP25FilesDetails { return new CIP25FilesDetails(name, mediaType, src); }
  public static parse(value: CborValue): CIP25FilesDetails { const fields = namedFields(expectMap(value, "CIP25FilesDetails")); return this.new(CIP25String64.parse(required(fields, "name")), CIP25String64.parse(required(fields, "mediaType")), CIP25ChunkableString.parse(required(fields, "src"))); }
  public static from_cbor_bytes(value: Uint8Array): CIP25FilesDetails { return this.parse(decodeCbor(value)); }
  public static from_cbor_hex(value: string): CIP25FilesDetails { return this.from_cbor_bytes(fromHex(value)); }
  public static from_json(value: string): CIP25FilesDetails { const object = jsonObject(JSON.parse(value), "CIP25FilesDetails"); return this.new(CIP25String64.new(String(object["name"])), CIP25String64.new(String(object["media_type"] ?? object["mediaType"])), CIP25ChunkableString.from_json(JSON.stringify(object["src"]))); }
  public name(): CIP25String64 { return CIP25String64.new(this.#name.get()); }
  public media_type(): CIP25String64 { return CIP25String64.new(this.#mediaType.get()); }
  public src(): CIP25ChunkableString { return CIP25ChunkableString.from_cbor_bytes(this.#src.to_cbor_bytes()); }
  public toNode(): CborValue { return map([[text("name"), this.#name.toNode()], [text("mediaType"), this.#mediaType.toNode()], [text("src"), this.#src.toNode()]]); }
  public to_js_value(): unknown { return { name: this.#name.get(), media_type: this.#mediaType.get(), src: this.#src.to_js_value() }; }
}

export class CIP25FilesDetailsList {
  readonly #values: CIP25FilesDetails[] = [];
  public static new(): CIP25FilesDetailsList { return new CIP25FilesDetailsList(); }
  public static from(values: readonly CIP25FilesDetails[]): CIP25FilesDetailsList { const output = this.new(); for (const value of values) output.add(value); return output; }
  public len(): number { return this.#values.length; }
  public get(index: number): CIP25FilesDetails { const value = this.#values[index]; if (value === undefined) throw new RangeError("CIP25FilesDetails index out of bounds"); return CIP25FilesDetails.from_cbor_bytes(value.to_cbor_bytes()); }
  public add(value: CIP25FilesDetails): void { this.#values.push(CIP25FilesDetails.from_cbor_bytes(value.to_cbor_bytes())); }
  public values(): CIP25FilesDetails[] { return this.#values.map((value) => CIP25FilesDetails.from_cbor_bytes(value.to_cbor_bytes())); }
}

export class CIP25MetadataDetails extends Cip25Data {
  readonly #name: CIP25String64; readonly #image: CIP25ChunkableString;
  #mediaType: CIP25String64 | undefined; #description: CIP25ChunkableString | undefined; #files: CIP25FilesDetails[] | undefined;
  private constructor(name: CIP25String64, image: CIP25ChunkableString) { super(); this.#name = CIP25String64.new(name.get()); this.#image = CIP25ChunkableString.from_cbor_bytes(image.to_cbor_bytes()); }
  public static new(name: CIP25String64, image: CIP25ChunkableString): CIP25MetadataDetails { return new CIP25MetadataDetails(name, image); }
  public static parse(value: CborValue): CIP25MetadataDetails {
    const fields = namedFields(expectMap(value, "CIP25MetadataDetails"));
    const output = this.new(CIP25String64.parse(required(fields, "name")), CIP25ChunkableString.parse(required(fields, "image")));
    const media = fields.get("mediaType"), description = fields.get("description"), files = fields.get("files");
    if (media !== undefined) output.set_media_type(CIP25String64.parse(media));
    if (description !== undefined) output.set_description(CIP25ChunkableString.parse(description));
    if (files !== undefined) { if (files.kind !== "array") throw new TypeError("files must be an array"); output.set_files(CIP25FilesDetailsList.from(files.values.map((item: CborValue) => CIP25FilesDetails.parse(item)))); }
    return output;
  }
  public static from_cbor_bytes(value: Uint8Array): CIP25MetadataDetails { return this.parse(decodeCbor(value)); }
  public static from_cbor_hex(value: string): CIP25MetadataDetails { return this.from_cbor_bytes(fromHex(value)); }
  public static from_json(value: string): CIP25MetadataDetails {
    const object = jsonObject(JSON.parse(value), "CIP25MetadataDetails");
    const output = this.new(CIP25String64.new(String(object["name"])), CIP25ChunkableString.from_json(JSON.stringify(object["image"])));
    if (typeof object["media_type"] === "string") output.set_media_type(CIP25String64.new(object["media_type"]));
    if (object["description"] !== undefined) output.set_description(CIP25ChunkableString.from_json(JSON.stringify(object["description"])));
    if (Array.isArray(object["files"])) output.set_files(CIP25FilesDetailsList.from(object["files"].map((item) => CIP25FilesDetails.from_json(JSON.stringify(item)))));
    return output;
  }
  public name(): CIP25String64 { return CIP25String64.new(this.#name.get()); }
  public image(): CIP25ChunkableString { return CIP25ChunkableString.from_cbor_bytes(this.#image.to_cbor_bytes()); }
  public set_media_type(value: CIP25String64): void { this.#mediaType = CIP25String64.new(value.get()); }
  public media_type(): CIP25String64 | undefined { return this.#mediaType === undefined ? undefined : CIP25String64.new(this.#mediaType.get()); }
  public set_description(value: CIP25ChunkableString): void { this.#description = CIP25ChunkableString.from_cbor_bytes(value.to_cbor_bytes()); }
  public description(): CIP25ChunkableString | undefined { return this.#description === undefined ? undefined : CIP25ChunkableString.from_cbor_bytes(this.#description.to_cbor_bytes()); }
  public set_files(value: CIP25FilesDetailsList): void { this.#files = value.values(); }
  public files(): CIP25FilesDetailsList | undefined { return this.#files === undefined ? undefined : CIP25FilesDetailsList.from(this.#files); }
  public toNode(): CborValue {
    const entries: Array<readonly [CborValue, CborValue]> = [[text("name"), this.#name.toNode()]];
    if (this.#files !== undefined) entries.push([text("files"), array(this.#files.map((value) => value.toNode()))]);
    entries.push([text("image"), this.#image.toNode()]);
    if (this.#mediaType !== undefined) entries.push([text("mediaType"), this.#mediaType.toNode()]);
    if (this.#description !== undefined) entries.push([text("description"), this.#description.toNode()]);
    return map(entries);
  }
  public to_js_value(): unknown { return { name: this.#name.get(), image: this.#image.to_js_value(), ...(this.#mediaType === undefined ? {} : { media_type: this.#mediaType.get() }), ...(this.#description === undefined ? {} : { description: this.#description.to_js_value() }), ...(this.#files === undefined ? {} : { files: this.#files.map((value) => value.to_js_value()) }) }; }
}

type NftEntry = { readonly policy: ScriptHash; readonly asset: AssetName; readonly details: CIP25MetadataDetails };

export class CIP25LabelMetadata extends Cip25Data {
  readonly #version: CIP25Version; readonly #nfts = new Map<string, NftEntry>();
  private constructor(version: CIP25Version) { super(); if (version !== CIP25Version.V1 && version !== CIP25Version.V2) throw new RangeError("unknown CIP25 version"); this.#version = version; }
  public static new(version: CIP25Version): CIP25LabelMetadata { return new CIP25LabelMetadata(version); }
  public static parse(value: CborValue): CIP25LabelMetadata {
    const root = expectMap(value, "CIP25LabelMetadata");
    try {
      const result = this.new(CIP25Version.V1);
      for (const [policyNode, assetsNode] of root.entries) {
        if (policyNode.kind !== "text") continue;
        const policy = ScriptHash.from_hex(policyNode.value), assets = expectMap(assetsNode, "CIP25 V1 assets");
        for (const [assetNode, detailsNode] of assets.entries) if (assetNode.kind === "text") result.set(policy, AssetName.from_raw_bytes(utf8.encode(assetNode.value)), CIP25MetadataDetails.parse(detailsNode));
      }
      return result;
    } catch {}
    const fields = namedFields(root), data = required(fields, "data"), version = required(fields, "version");
    if (version.kind !== "unsigned" || version.value !== 2n) throw new TypeError("CIP25 V2 version must be 2");
    const result = this.new(CIP25Version.V2);
    for (const [policyNode, assetsNode] of expectMap(data, "CIP25 V2 data").entries) {
      if (policyNode.kind !== "bytes") continue;
      const policy = ScriptHash.from_raw_bytes(policyNode.value);
      for (const [assetNode, detailsNode] of expectMap(assetsNode, "CIP25 V2 assets").entries) if (assetNode.kind === "bytes") result.set(policy, AssetName.from_raw_bytes(assetNode.value), CIP25MetadataDetails.parse(detailsNode));
    }
    return result;
  }
  public static from_cbor_bytes(value: Uint8Array): CIP25LabelMetadata { return this.parse(decodeCbor(value)); }
  public static from_cbor_hex(value: string): CIP25LabelMetadata { return this.from_cbor_bytes(fromHex(value)); }
  public static from_json(value: string): CIP25LabelMetadata {
    const object = jsonObject(JSON.parse(value), "CIP25LabelMetadata"), version = object["version"] === "V2" || object["version"] === 1 ? CIP25Version.V2 : CIP25Version.V1;
    const output = this.new(version), nfts = jsonObject(object["nfts"] ?? {}, "CIP25 nfts");
    for (const [policyHex, assetsValue] of Object.entries(nfts)) for (const [assetHex, details] of Object.entries(jsonObject(assetsValue, "CIP25 assets"))) output.set(ScriptHash.from_hex(policyHex), AssetName.from_raw_bytes(hexToBytes(assetHex)), CIP25MetadataDetails.from_json(JSON.stringify(details)));
    return output;
  }
  public set(policy: ScriptHash, asset: AssetName, details: CIP25MetadataDetails): CIP25MetadataDetails | undefined {
    if (this.#version === CIP25Version.V1) utf8Fatal.decode(asset.to_raw_bytes());
    const key = `${policy.to_hex()}:${asset.to_hex()}`, previous = this.#nfts.get(key);
    this.#nfts.set(key, { policy: ScriptHash.from_raw_bytes(policy.to_raw_bytes()), asset: AssetName.from_raw_bytes(asset.to_raw_bytes()), details: CIP25MetadataDetails.from_cbor_bytes(details.to_cbor_bytes()) });
    return previous === undefined ? undefined : CIP25MetadataDetails.from_cbor_bytes(previous.details.to_cbor_bytes());
  }
  public get(policy: ScriptHash, asset: AssetName): CIP25MetadataDetails | undefined { const value = this.#nfts.get(`${policy.to_hex()}:${asset.to_hex()}`); return value === undefined ? undefined : CIP25MetadataDetails.from_cbor_bytes(value.details.to_cbor_bytes()); }
  public version(): CIP25Version { return this.#version; }
  public toNode(): CborValue {
    const policies = groupedEntries([...this.#nfts.values()]);
    if (this.#version === CIP25Version.V1) return map(policies.map(([policy, assets]) => [text(policy.to_hex()), map(assets.map((entry) => [text(utf8Fatal.decode(entry.asset.to_raw_bytes())), entry.details.toNode()]))]));
    return map([[text("data"), map(policies.map(([policy, assets]) => [bytes(policy.to_raw_bytes()), map(assets.map((entry) => [bytes(entry.asset.to_raw_bytes()), entry.details.toNode()]))]))], [text("version"), uint(2n)]]);
  }
  public to_js_value(): unknown {
    const nfts: Record<string, Record<string, unknown>> = {};
    for (const entry of this.#nfts.values()) (nfts[entry.policy.to_hex()] ??= {})[entry.asset.to_hex()] = entry.details.to_js_value();
    return { nfts, version: this.#version === CIP25Version.V1 ? "V1" : "V2" };
  }
}

export class CIP25Metadata extends Cip25Data {
  readonly #label: CIP25LabelMetadata;
  private constructor(value: CIP25LabelMetadata) { super(); this.#label = CIP25LabelMetadata.from_cbor_bytes(value.to_cbor_bytes()); }
  public static new(value: CIP25LabelMetadata): CIP25Metadata { return new CIP25Metadata(value); }
  public static from_cbor_bytes(value: Uint8Array): CIP25Metadata { const root = expectMap(decodeCbor(value), "CIP25Metadata"), entry = root.entries.find((item: readonly [CborValue, CborValue]) => item[0].kind === "unsigned" && item[0].value === METADATA_LABEL); if (entry === undefined) throw new TypeError("CIP25 metadata label 721 is missing"); return this.new(CIP25LabelMetadata.parse(entry[1])); }
  public static from_cbor_hex(value: string): CIP25Metadata { return this.from_cbor_bytes(fromHex(value)); }
  public static from_json(value: string): CIP25Metadata { const object = jsonObject(JSON.parse(value), "CIP25Metadata"); return this.new(CIP25LabelMetadata.from_json(JSON.stringify(object["key_721"]))); }
  public static from_metadata(value: Metadata): CIP25Metadata { const label = value.get(METADATA_LABEL); if (label === undefined) throw new TypeError("CIP25 metadata label 721 is missing"); return this.new(CIP25LabelMetadata.from_cbor_bytes(label.to_cbor_bytes())); }
  public key_721(): CIP25LabelMetadata { return CIP25LabelMetadata.from_cbor_bytes(this.#label.to_cbor_bytes()); }
  public to_metadata(): Metadata { const output = Metadata.new(); this.add_to_metadata(output); return output; }
  public add_to_metadata(value: Metadata): void { value.set(METADATA_LABEL, TransactionMetadatum.from_cbor_bytes(this.#label.to_cbor_bytes())); }
  public toNode(): CborValue { return map([[uint(METADATA_LABEL), this.#label.toNode()]]); }
  public to_js_value(): unknown { return { key_721: this.#label.to_js_value() }; }
}

export class CIP25MiniMetadataDetails {
  #name: CIP25String64 | undefined; #image: CIP25ChunkableString | undefined;
  public static new(): CIP25MiniMetadataDetails { return new CIP25MiniMetadataDetails(); }
  public static loose_parse(value: TransactionMetadatum): CIP25MiniMetadataDetails {
    const node = decodeCbor(value.to_cbor_bytes()); if (node.kind !== "map") throw new TypeError("CIP25 loose metadata details require a map");
    const fields = namedFields(node), output = this.new();
    for (const key of ["name", "Name", "title", "id"]) { const candidate = fields.get(key); if (candidate?.kind === "text") { try { output.set_name(CIP25String64.new(candidate.value)); } catch {} break; } }
    const image = fields.get("image");
    if (image !== undefined) { try { output.set_image(CIP25ChunkableString.parse(image)); } catch {} }
    return output;
  }
  public static from_json(value: string): CIP25MiniMetadataDetails { const object = jsonObject(JSON.parse(value), "CIP25MiniMetadataDetails"), output = this.new(); if (typeof object["name"] === "string") output.set_name(CIP25String64.new(object["name"])); if (object["image"] !== undefined) output.set_image(CIP25ChunkableString.from_json(JSON.stringify(object["image"]))); return output; }
  public set_name(value: CIP25String64): void { this.#name = CIP25String64.new(value.get()); }
  public name(): CIP25String64 | undefined { return this.#name === undefined ? undefined : CIP25String64.new(this.#name.get()); }
  public set_image(value: CIP25ChunkableString): void { this.#image = CIP25ChunkableString.from_cbor_bytes(value.to_cbor_bytes()); }
  public image(): CIP25ChunkableString | undefined { return this.#image === undefined ? undefined : CIP25ChunkableString.from_cbor_bytes(this.#image.to_cbor_bytes()); }
  public to_js_value(): unknown { return { name: this.#name?.get() ?? null, image: this.#image?.to_js_value() ?? null }; }
  public to_json(): string { return JSON.stringify(this.to_js_value(), null, 2); }
}

function namedFields(value: Extract<CborValue, { kind: "map" }>): Map<string, CborValue> {
  const output = new Map<string, CborValue>();
  for (const [key, item] of value.entries) if (key.kind === "text") { if (output.has(key.value)) throw new TypeError(`duplicate metadata key ${key.value}`); output.set(key.value, item); }
  return output;
}
function required(fields: Map<string, CborValue>, key: string): CborValue { const value = fields.get(key); if (value === undefined) throw new TypeError(`missing metadata key ${key}`); return value; }
function groupedEntries(values: readonly NftEntry[]): Array<readonly [ScriptHash, NftEntry[]]> {
  const groups = new Map<string, { policy: ScriptHash; values: NftEntry[] }>();
  for (const value of values) { const group = groups.get(value.policy.to_hex()) ?? { policy: value.policy, values: [] }; group.values.push(value); groups.set(value.policy.to_hex(), group); }
  return [...groups.values()].sort((left, right) => left.policy.to_hex().localeCompare(right.policy.to_hex())).map((group) => [group.policy, group.values.sort((left, right) => left.asset.to_hex().localeCompare(right.asset.to_hex()))]);
}

export { AssetName, Metadata, TransactionMetadatum } from "@xray-network/xray-cardano-lib-chain";
export { ScriptHash as PolicyId } from "@xray-network/xray-cardano-lib-crypto";
