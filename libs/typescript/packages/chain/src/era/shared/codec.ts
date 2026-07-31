import {
  bytesToHex,
  copyBytes,
  decodeCbor,
  encodeCbor,
  hexToBytes,
  type CborValue,
} from "@xray-network/cardano-core";
import { decodeBech32, encodeBech32 } from "@xray-network/cardano-core/bech32";

type HistoricalConstructor<T extends HistoricalData = HistoricalData> = {
  new (node: CborValue): T;
  validateNode(node: CborValue): void;
};

export function uint(value: bigint): CborValue {
  if (value < 0n) return { kind: "negative", value, encoding: { width: 0 } };
  return { kind: "unsigned", value, encoding: { width: 0 } };
}

const MAX_JSON_DEPTH = 128;
const MAX_JSON_NODES = 100_000;
interface JsonBudget { nodes: number }

export function inputNode(value: unknown, depth = 0, budget: JsonBudget = { nodes: 0 }): CborValue {
  if (depth > MAX_JSON_DEPTH) throw new RangeError(`JSON nesting exceeds ${MAX_JSON_DEPTH}`);
  budget.nodes += 1;
  if (budget.nodes > MAX_JSON_NODES) throw new RangeError(`JSON value count exceeds ${MAX_JSON_NODES}`);
  if (value instanceof HistoricalData) return decodeCbor(value.to_cbor_bytes());
  if (value instanceof Uint8Array) {
    return { kind: "bytes", value: copyBytes(value), encoding: { kind: "definite", width: 0 } };
  }
  if (typeof value === "bigint") return uint(value);
  if (typeof value === "number" && Number.isSafeInteger(value)) return uint(BigInt(value));
  if (typeof value === "string") {
    return { kind: "text", value, encoding: { kind: "definite", width: 0 } };
  }
  if (typeof value === "boolean") return { kind: "boolean", value };
  if (value === null || value === undefined) return { kind: "null" };
  if (Array.isArray(value)) {
    return {
      kind: "array",
      values: value.map((item) => inputNode(item, depth + 1, budget)),
      encoding: { kind: "definite", width: 0 },
    };
  }
  if (typeof value === "object" && "to_cbor_bytes" in value) {
    const serializable = value as { to_cbor_bytes(): Uint8Array };
    return decodeCbor(serializable.to_cbor_bytes());
  }
  if (typeof value === "object") {
    return {
      kind: "map",
      entries: Object.entries(value as Record<string, unknown>).map(([key, item]) => [
        inputNode(key, depth + 1, budget),
        inputNode(item, depth + 1, budget),
      ]),
      encoding: { kind: "definite", width: 0 },
    };
  }
  throw new TypeError(`Unsupported historical-era value ${String(value)}`);
}

export function nodeJson(node: CborValue): unknown {
  switch (node.kind) {
    case "unsigned":
    case "negative":
      return node.value <= BigInt(Number.MAX_SAFE_INTEGER) && node.value >= BigInt(Number.MIN_SAFE_INTEGER)
        ? Number(node.value)
        : node.value.toString();
    case "bytes": return bytesToHex(node.value);
    case "text":
    case "boolean":
    case "float":
    case "simple": return node.value;
    case "null": return null;
    case "undefined": return undefined;
    case "array": return node.values.map(nodeJson);
    case "map": return node.entries.map(([key, value]) => [nodeJson(key), nodeJson(value)]);
    case "tag": return { tag: Number(node.tag), value: nodeJson(node.value) };
  }
}

/** Shared lossless codec base for historical-era data shapes. */
export class HistoricalData {
  #node: CborValue;

  public constructor(node: CborValue) { this.#node = node; }

  public static validateNode(_node: CborValue): void {}

  public static from_cbor_bytes(bytes: Uint8Array): HistoricalData {
    const Owner = this as unknown as HistoricalConstructor;
    const node = decodeCbor(bytes);
    Owner.validateNode(node);
    return new Owner(node);
  }

  public static from_cbor_hex(hex: string): HistoricalData {
    return this.from_cbor_bytes(hexToBytes(hex));
  }

  public static from_json(json: string): HistoricalData {
    const Owner = this as unknown as HistoricalConstructor;
    const node = inputNode(JSON.parse(json));
    Owner.validateNode(node);
    return new Owner(node);
  }

  public static new(...values: unknown[]): HistoricalData {
    const Owner = this as unknown as HistoricalConstructor;
    const node: CborValue = {
      kind: "array",
      values: values.map((value) => inputNode(value)),
      encoding: { kind: "definite", width: 0 },
    };
    Owner.validateNode(node);
    return new Owner(node);
  }

  public cbor_node(): CborValue { return this.#node; }
  protected replace_node(node: CborValue): void { this.#node = node; }
  public to_cbor_bytes(): Uint8Array { return encodeCbor(this.#node); }
  public to_cbor_hex(): string { return bytesToHex(this.to_cbor_bytes()); }
  public to_canonical_cbor_bytes(): Uint8Array {
    return encodeCbor(this.#node, { mode: "canonical" });
  }
  public to_canonical_cbor_hex(): string { return bytesToHex(this.to_canonical_cbor_bytes()); }
  public to_js_value(): unknown { return nodeJson(this.#node); }
  public to_json(): string { return JSON.stringify(this.to_js_value()); }
}

export class HistoricalList<T> extends HistoricalData {
  public constructor(node: CborValue = {
    kind: "array",
    values: [],
    encoding: { kind: "definite", width: 0 },
  }) {
    super(node);
    if (node.kind !== "array") throw new TypeError("Historical list requires a CBOR array");
  }

  public len(): number { return this.values().length; }
  public get(index: number): T {
    const value = this.values()[index];
    if (value === undefined) throw new RangeError(`Index ${index} is outside the list`);
    return value as T;
  }
  public add(value: T): void {
    const node = this.cbor_node();
    if (node.kind !== "array") throw new TypeError("Historical list requires a CBOR array");
    const values = [...this.values(), value];
    const replacement: CborValue = {
      kind: "array",
      values: values.map((item) => inputNode(item)),
      encoding: { kind: "definite", width: 0 },
    };
    const Owner = this.constructor as unknown as HistoricalConstructor;
    Owner.validateNode(replacement);
    this.replace_node(replacement);
  }
  protected values(): unknown[] {
    const node = this.cbor_node();
    if (node.kind !== "array") throw new TypeError("Historical list requires a CBOR array");
    return node.values.map((value) => new HistoricalData(value));
  }
}

abstract class Blake2bValue {
  readonly #bytes: Uint8Array;
  protected constructor(bytes: Uint8Array, length: number) {
    if (bytes.length !== length) throw new RangeError(`Expected ${length} bytes, found ${bytes.length}`);
    this.#bytes = copyBytes(bytes);
  }
  public to_raw_bytes(): Uint8Array { return copyBytes(this.#bytes); }
  public to_hex(): string { return bytesToHex(this.#bytes); }
  public to_bech32(prefix: string): string { return encodeBech32(prefix, this.#bytes); }
}

export class Blake2b224 extends Blake2bValue {
  private constructor(bytes: Uint8Array) { super(bytes, 28); }
  public static from_raw_bytes(bytes: Uint8Array): Blake2b224 { return new Blake2b224(bytes); }
  public static from_hex(hex: string): Blake2b224 { return new Blake2b224(hexToBytes(hex)); }
  public static from_bech32(value: string): Blake2b224 { return new Blake2b224(decodeBech32(value).bytes); }
}

export class Blake2b256 extends Blake2bValue {
  private constructor(bytes: Uint8Array) { super(bytes, 32); }
  public static from_raw_bytes(bytes: Uint8Array): Blake2b256 { return new Blake2b256(bytes); }
  public static from_hex(hex: string): Blake2b256 { return new Blake2b256(hexToBytes(hex)); }
  public static from_bech32(value: string): Blake2b256 { return new Blake2b256(decodeBech32(value).bytes); }
}

export function mapValue(node: CborValue, key: bigint): CborValue | undefined {
  if (node.kind !== "map") return undefined;
  return node.entries.find(([candidate]) => candidate.kind === "unsigned" && candidate.value === key)?.[1];
}

export function arrayValue(node: CborValue, index: number): CborValue | undefined {
  return node.kind === "array" ? node.values[index] : undefined;
}
