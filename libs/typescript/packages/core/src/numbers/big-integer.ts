import { bytesToHex, hexToBytes } from "../bytes/index.js";
import { decodeCbor, encodeCbor } from "../cbor/index.js";
import type { CborValue } from "../cbor/index.js";
import { DeserializeError } from "../errors/index.js";
import { CBOR_INT_MAX, CBOR_INT_MIN, UINT64_MAX } from "./bounds.js";
import { Int } from "./int.js";

const states = new WeakMap<BigInteger, { readonly value: bigint; readonly decoded?: CborValue }>();

function parseDecimal(value: string): bigint {
  if (!/^-?(?:0|[1-9][0-9]*)$/u.test(value)) throw new TypeError("Invalid BigInteger decimal string");
  return BigInt(value);
}

function magnitude(bytes: Uint8Array): bigint {
  let value = 0n;
  for (const byte of bytes) value = (value << 8n) | BigInt(byte);
  return value;
}

function magnitudeBytes(value: bigint): Uint8Array {
  if (value === 0n) return new Uint8Array();
  let hex = value.toString(16);
  if (hex.length % 2 !== 0) hex = `0${hex}`;
  return hexToBytes(hex);
}

function fromNode(node: CborValue): bigint {
  if (node.kind === "unsigned" || node.kind === "negative") return node.value;
  if (node.kind !== "tag" || (node.tag !== 2n && node.tag !== 3n) || node.value.kind !== "bytes") {
    throw new DeserializeError("NO_VARIANT_MATCHED", "BigInteger requires a CBOR integer or bignum", { path: ["BigInteger"] });
  }
  if (node.value.encoding.kind === "definite" && node.value.value.length > 64) {
    throw new DeserializeError("OUT_OF_RANGE", "BigInteger bignum chunks are limited to 64 bytes", { path: ["BigInteger"] });
  }
  if (node.value.encoding.kind === "indefinite" && node.value.encoding.chunks.some((chunk) => chunk.value instanceof Uint8Array && chunk.value.length > 64)) {
    throw new DeserializeError("OUT_OF_RANGE", "BigInteger bignum chunks are limited to 64 bytes", { path: ["BigInteger"] });
  }
  const raw = magnitude(node.value.value);
  return node.tag === 2n ? raw : -raw - 1n;
}

function canonicalNode(value: bigint): CborValue {
  if (value >= CBOR_INT_MIN && value <= CBOR_INT_MAX) {
    return value >= 0n
      ? { kind: "unsigned", value, encoding: { width: 0 } }
      : { kind: "negative", value, encoding: { width: 0 } };
  }
  const negative = value < CBOR_INT_MIN;
  const bytes = magnitudeBytes(negative ? -value - 1n : value);
  const byteNode: CborValue = bytes.length <= 64
    ? { kind: "bytes", value: bytes, encoding: { kind: "definite", width: 0 } }
    : {
        kind: "bytes",
        value: bytes,
        encoding: {
          kind: "indefinite",
          chunks: Array.from({ length: Math.ceil(bytes.length / 64) }, (_, index) => ({
            value: bytes.slice(index * 64, (index + 1) * 64),
            width: 0 as const,
          })),
        },
      };
  return { kind: "tag", tag: negative ? 3n : 2n, value: byteNode, encoding: { width: 0 } };
}

export class BigInteger {
  private constructor(value: bigint, decoded?: CborValue) {
    states.set(this, decoded === undefined ? { value } : { value, decoded });
  }

  public static from_str(value: string): BigInteger { return new BigInteger(parseDecimal(value)); }
  public static from_int(value: Int): BigInteger { return new BigInteger(BigInt(value.to_str())); }
  public static from_json(json: string): BigInteger {
    const value: unknown = JSON.parse(json);
    if (typeof value !== "string") throw new TypeError("BigInteger JSON must be a decimal string");
    return BigInteger.from_str(value);
  }
  public static from_cbor_bytes(bytes: Uint8Array): BigInteger {
    const decoded = decodeCbor(bytes);
    return new BigInteger(fromNode(decoded), decoded);
  }
  public static from_cbor_hex(hex: string): BigInteger { return BigInteger.from_cbor_bytes(hexToBytes(hex)); }
  public as_u64(): bigint | undefined {
    const value = state(this).value;
    return value >= 0n && value <= UINT64_MAX ? value : undefined;
  }
  public as_int(): Int | undefined {
    const value = state(this).value;
    return value >= CBOR_INT_MIN && value <= CBOR_INT_MAX ? Int.new(value) : undefined;
  }
  public to_str(): string { return state(this).value.toString(10); }
  public to_cbor_bytes(): Uint8Array { return encodeCbor(state(this).decoded ?? canonicalNode(state(this).value)); }
  public to_cbor_hex(): string { return bytesToHex(this.to_cbor_bytes()); }
  public to_canonical_cbor_bytes(): Uint8Array { return encodeCbor(canonicalNode(state(this).value), { mode: "canonical" }); }
  public to_canonical_cbor_hex(): string { return bytesToHex(this.to_canonical_cbor_bytes()); }
  public to_json(): string { return JSON.stringify(this.to_str()); }
  public to_js_value(): any { return this.to_str(); }
}

function state(value: BigInteger): { readonly value: bigint; readonly decoded?: CborValue } {
  const found = states.get(value);
  if (found === undefined) throw new TypeError("Invalid BigInteger receiver");
  return found;
}
