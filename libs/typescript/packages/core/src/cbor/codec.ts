import { bytesEqual, copyBytes } from "../bytes/index.js";
import { DeserializeError } from "../errors/index.js";
import { DEFAULT_CBOR_LIMITS } from "./limits.js";
import type {
  CborByteChunk,
  CborDecoderLimits,
  CborHeadWidth,
  CborLengthEncoding,
  CborMode,
  CborStringEncoding,
  CborTextChunk,
  CborValue,
  DecodeCborOptions,
  EncodeCborOptions,
} from "./types.js";

const utf8Decoder = new TextDecoder("utf-8", { fatal: true });
const utf8Encoder = new TextEncoder();
const MAX_U64 = 0xffff_ffff_ffff_ffffn;

function failure(
  kind: ConstructorParameters<typeof DeserializeError>[0],
  message: string,
  offset: number,
): DeserializeError {
  return new DeserializeError(kind, message, { offset });
}

function concat(chunks: readonly Uint8Array[]): Uint8Array {
  const length = chunks.reduce((sum, chunk) => sum + chunk.length, 0);
  const result = new Uint8Array(length);
  let offset = 0;
  for (const chunk of chunks) { result.set(chunk, offset); offset += chunk.length; }
  return result;
}

function minimalWidth(value: bigint): CborHeadWidth {
  if (value <= 23n) return 0;
  if (value <= 0xffn) return 1;
  if (value <= 0xffffn) return 2;
  if (value <= 0xffff_ffffn) return 4;
  if (value <= MAX_U64) return 8;
  throw new RangeError("CBOR head value exceeds uint64");
}

function fitsWidth(value: bigint, width: CborHeadWidth): boolean {
  return width === 0 ? value <= 23n : value < (1n << BigInt(width * 8));
}

class Reader {
  readonly #bytes: Uint8Array;
  readonly #limits: CborDecoderLimits;
  #offset = 0;
  #tokens = 0;

  public constructor(bytes: Uint8Array, options: DecodeCborOptions) {
    this.#bytes = bytes;
    this.#limits = { ...DEFAULT_CBOR_LIMITS, ...options.limits };
    for (const [key, value] of Object.entries(this.#limits)) {
      if (!Number.isSafeInteger(value) || value < 0) throw new RangeError(`${key} must be a non-negative safe integer`);
    }
  }

  public get offset(): number { return this.#offset; }
  public get done(): boolean { return this.#offset === this.#bytes.length; }
  #peek(): number | undefined { return this.#bytes[this.#offset]; }
  #take(): number {
    const value = this.#bytes[this.#offset];
    if (value === undefined) throw failure("TRUNCATED_INPUT", "Unexpected end of CBOR input", this.#offset);
    this.#offset += 1;
    return value;
  }
  #takeBytes(length: number): Uint8Array {
    if (length > this.#bytes.length - this.#offset) {
      throw failure("TRUNCATED_INPUT", "CBOR value extends past the end of input", this.#offset);
    }
    const result = this.#bytes.slice(this.#offset, this.#offset + length);
    this.#offset += length;
    return result;
  }
  #headValue(additional: number): { value: bigint; width: CborHeadWidth } {
    if (additional < 24) return { value: BigInt(additional), width: 0 };
    const width = additional === 24 ? 1 : additional === 25 ? 2 : additional === 26 ? 4 : additional === 27 ? 8 : undefined;
    if (width === undefined) throw failure("INVALID_CBOR", `Reserved additional information ${additional}`, this.#offset - 1);
    const bytes = this.#takeBytes(width);
    let value = 0n;
    for (const byte of bytes) value = (value << 8n) | BigInt(byte);
    return { value, width };
  }
  #length(value: bigint, kind: "collection" | "string"): number {
    const maximum = kind === "string" ? this.#limits.maxStringBytes : this.#limits.maxCollectionLength;
    if (value > BigInt(maximum) || value > BigInt(Number.MAX_SAFE_INTEGER)) {
      throw failure("OUT_OF_RANGE", `CBOR ${kind} length ${value} exceeds configured limit ${maximum}`, this.#offset);
    }
    return Number(value);
  }
  #decodeText(bytes: Uint8Array, offset: number): string {
    try { return utf8Decoder.decode(bytes); }
    catch (cause) { throw new DeserializeError("INVALID_CBOR", "CBOR text contains invalid UTF-8", { cause, offset }); }
  }
  #nested(depth: number, component: string | number): CborValue {
    try { return this.value(depth); }
    catch (error) {
      if (error instanceof DeserializeError) throw error.annotate(component);
      throw error;
    }
  }

  public value(depth = 0): CborValue {
    if (depth > this.#limits.maxDepth) throw failure("DEPTH_LIMIT_EXCEEDED", "CBOR nesting depth limit exceeded", this.#offset);
    this.#tokens += 1;
    if (this.#tokens > this.#limits.maxTokens) throw failure("OUT_OF_RANGE", "CBOR token limit exceeded", this.#offset);
    const start = this.#offset;
    const initial = this.#take();
    const major = initial >>> 5;
    const additional = initial & 31;
    if (initial === 0xff) throw failure("BREAK_IN_DEFINITE_LENGTH", "Unexpected CBOR break", start);

    if (major === 0 || major === 1 || major === 6) {
      if (additional === 31) throw failure("INVALID_CBOR", "Indefinite marker is invalid for this CBOR type", start);
      const head = this.#headValue(additional);
      if (major === 0) return { kind: "unsigned", value: head.value, encoding: { width: head.width }, span: { start, end: this.#offset } };
      if (major === 1) return { kind: "negative", value: -1n - head.value, encoding: { width: head.width }, span: { start, end: this.#offset } };
      const value = this.#nested(depth + 1, `tag(${head.value})`);
      return { kind: "tag", tag: head.value, value, encoding: { width: head.width }, span: { start, end: this.#offset } };
    }

    if (major === 2 || major === 3) {
      if (additional === 31) {
        const byteChunks: CborByteChunk[] = [];
        const textChunks: CborTextChunk[] = [];
        let total = 0;
        while (this.#peek() !== 0xff) {
          if (this.#peek() === undefined) throw failure("ENDING_BREAK_MISSING", "Indefinite string is missing its break", this.#offset);
          const chunkStart = this.#offset;
          const chunkInitial = this.#take();
          if ((chunkInitial >>> 5) !== major || (chunkInitial & 31) === 31) {
            throw failure("INVALID_STRUCTURE", "Indefinite string contains an invalid chunk", chunkStart);
          }
          const head = this.#headValue(chunkInitial & 31);
          const length = this.#length(head.value, "string");
          total += length;
          if (total > this.#limits.maxStringBytes) throw failure("OUT_OF_RANGE", "Indefinite string exceeds configured limit", chunkStart);
          const bytes = this.#takeBytes(length);
          if (major === 2) byteChunks.push({ value: bytes, width: head.width });
          else textChunks.push({ value: this.#decodeText(bytes, chunkStart), width: head.width });
        }
        this.#take();
        if (major === 2) {
          return { kind: "bytes", value: concat(byteChunks.map((chunk) => chunk.value)), encoding: { kind: "indefinite", chunks: byteChunks }, span: { start, end: this.#offset } };
        }
        return { kind: "text", value: textChunks.map((chunk) => chunk.value).join(""), encoding: { kind: "indefinite", chunks: textChunks }, span: { start, end: this.#offset } };
      }
      const head = this.#headValue(additional);
      const length = this.#length(head.value, "string");
      const bytes = this.#takeBytes(length);
      if (major === 2) return { kind: "bytes", value: bytes, encoding: { kind: "definite", width: head.width }, span: { start, end: this.#offset } };
      return { kind: "text", value: this.#decodeText(bytes, start), encoding: { kind: "definite", width: head.width }, span: { start, end: this.#offset } };
    }

    if (major === 4 || major === 5) {
      const indefinite = additional === 31;
      const head = indefinite ? undefined : this.#headValue(additional);
      const length = head === undefined ? undefined : this.#length(head.value, "collection");
      const encoding: CborLengthEncoding = head === undefined ? { kind: "indefinite" } : { kind: "definite", width: head.width };
      if (major === 4) {
        const values: CborValue[] = [];
        if (length === undefined) {
          while (this.#peek() !== 0xff) {
            if (this.#peek() === undefined) throw failure("ENDING_BREAK_MISSING", "Indefinite array is missing its break", this.#offset);
            if (values.length >= this.#limits.maxCollectionLength) throw failure("OUT_OF_RANGE", "Array exceeds configured limit", this.#offset);
            values.push(this.#nested(depth + 1, values.length));
          }
          this.#take();
        } else for (let index = 0; index < length; index += 1) values.push(this.#nested(depth + 1, index));
        return { kind: "array", values, encoding, span: { start, end: this.#offset } };
      }
      const entries: Array<readonly [CborValue, CborValue]> = [];
      if (length === undefined) {
        while (this.#peek() !== 0xff) {
          if (this.#peek() === undefined) throw failure("ENDING_BREAK_MISSING", "Indefinite map is missing its break", this.#offset);
          if (entries.length >= this.#limits.maxCollectionLength) throw failure("OUT_OF_RANGE", "Map exceeds configured limit", this.#offset);
          const key = this.#nested(depth + 1, `key(${entries.length})`);
          if (this.#peek() === 0xff) throw failure("INVALID_STRUCTURE", "Indefinite map break appears where a value is required", this.#offset);
          entries.push([key, this.#nested(depth + 1, entries.length)]);
        }
        this.#take();
      } else for (let index = 0; index < length; index += 1) {
        entries.push([
          this.#nested(depth + 1, `key(${index})`),
          this.#nested(depth + 1, index),
        ]);
      }
      return { kind: "map", entries, encoding, span: { start, end: this.#offset } };
    }

    if (major === 7) {
      if (additional === 20 || additional === 21) return { kind: "boolean", value: additional === 21, span: { start, end: this.#offset } };
      if (additional === 22) return { kind: "null", span: { start, end: this.#offset } };
      if (additional === 23) return { kind: "undefined", span: { start, end: this.#offset } };
      if (additional < 20) return { kind: "simple", value: additional, width: 0, span: { start, end: this.#offset } };
      if (additional === 24) return { kind: "simple", value: this.#take(), width: 1, span: { start, end: this.#offset } };
      if (additional === 25 || additional === 26 || additional === 27) {
        const width = additional === 25 ? 2 : additional === 26 ? 4 : 8;
        const raw = this.#takeBytes(width);
        const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        const value = width === 2 ? decodeHalf(view.getUint16(0)) : width === 4 ? view.getFloat32(0) : view.getFloat64(0);
        return { kind: "float", value, width, span: { start, end: this.#offset } };
      }
      throw failure("INVALID_CBOR", `Invalid CBOR simple value ${additional}`, start);
    }
    throw failure("INVALID_CBOR", `Unknown CBOR major type ${major}`, start);
  }
}

class Writer {
  readonly #bytes: number[] = [];
  readonly #mode: CborMode;
  public constructor(mode: CborMode) { this.#mode = mode; }
  #head(major: number, value: bigint, preferred?: CborHeadWidth): void {
    if (value < 0n || value > MAX_U64) throw new RangeError("CBOR head value must fit uint64");
    const width = this.#mode === "preserve" && preferred !== undefined && fitsWidth(value, preferred) ? preferred : minimalWidth(value);
    const additional = width === 0 ? Number(value) : width === 1 ? 24 : width === 2 ? 25 : width === 4 ? 26 : 27;
    this.#bytes.push((major << 5) | additional);
    for (let shift = width * 8 - 8; shift >= 0; shift -= 8) this.#bytes.push(Number((value >> BigInt(shift)) & 0xffn));
  }
  #raw(bytes: Uint8Array): void { for (const byte of bytes) this.#bytes.push(byte); }
  #length(major: number, length: number, encoding: CborLengthEncoding | CborStringEncoding): boolean {
    if (this.#mode === "preserve" && encoding.kind === "indefinite") { this.#bytes.push((major << 5) | 31); return true; }
    this.#head(major, BigInt(length), encoding.kind === "definite" ? encoding.width : undefined);
    return false;
  }
  public value(value: CborValue): void {
    switch (value.kind) {
      case "unsigned": this.#head(0, value.value, value.encoding.width); break;
      case "negative": {
        if (value.value >= 0n) throw new RangeError("Negative CBOR value must be less than zero");
        this.#head(1, -1n - value.value, value.encoding.width); break;
      }
      case "bytes": this.#byteString(value); break;
      case "text": this.#textString(value); break;
      case "array": {
        const indefinite = this.#length(4, value.values.length, value.encoding);
        for (const item of value.values) this.value(item);
        if (indefinite) this.#bytes.push(0xff);
        break;
      }
      case "map": {
        const entries = this.#mode === "canonical" ? canonicalEntries(value.entries) : value.entries;
        const indefinite = this.#length(5, entries.length, value.encoding);
        for (const [key, entryValue] of entries) { this.value(key); this.value(entryValue); }
        if (indefinite) this.#bytes.push(0xff);
        break;
      }
      case "tag": this.#head(6, value.tag, value.encoding.width); this.value(value.value); break;
      case "boolean": this.#bytes.push(value.value ? 0xf5 : 0xf4); break;
      case "null": this.#bytes.push(0xf6); break;
      case "undefined": this.#bytes.push(0xf7); break;
      case "simple": {
        if (value.value < 0 || value.value > 255 || !Number.isInteger(value.value)) throw new RangeError("Simple value must be a byte");
        if (this.#mode === "preserve" && value.width === 1 || value.value >= 24) this.#bytes.push(0xf8, value.value);
        else this.#bytes.push(0xe0 | value.value);
        break;
      }
      case "float": this.#float(value.value, this.#mode === "preserve" ? value.width : canonicalFloatWidth(value.value)); break;
    }
  }
  #byteString(value: Extract<CborValue, { kind: "bytes" }>): void {
    if (this.#mode === "preserve" && value.encoding.kind === "indefinite") {
      const chunks = value.encoding.chunks as readonly CborByteChunk[];
      if (chunks.every((chunk) => chunk.value instanceof Uint8Array) && bytesEqual(concat(chunks.map((chunk) => chunk.value)), value.value)) {
        this.#bytes.push(0x5f);
        for (const chunk of chunks) { this.#head(2, BigInt(chunk.value.length), chunk.width); this.#raw(chunk.value); }
        this.#bytes.push(0xff); return;
      }
    }
    this.#head(2, BigInt(value.value.length), value.encoding.kind === "definite" ? value.encoding.width : undefined);
    this.#raw(value.value);
  }
  #textString(value: Extract<CborValue, { kind: "text" }>): void {
    if (this.#mode === "preserve" && value.encoding.kind === "indefinite") {
      const chunks = value.encoding.chunks as readonly CborTextChunk[];
      if (chunks.every((chunk) => typeof chunk.value === "string") && chunks.map((chunk) => chunk.value).join("") === value.value) {
        this.#bytes.push(0x7f);
        for (const chunk of chunks) { const bytes = utf8Encoder.encode(chunk.value); this.#head(3, BigInt(bytes.length), chunk.width); this.#raw(bytes); }
        this.#bytes.push(0xff); return;
      }
    }
    const bytes = utf8Encoder.encode(value.value);
    this.#head(3, BigInt(bytes.length), value.encoding.kind === "definite" ? value.encoding.width : undefined);
    this.#raw(bytes);
  }
  #float(value: number, width: 2 | 4 | 8): void {
    this.#bytes.push(width === 2 ? 0xf9 : width === 4 ? 0xfa : 0xfb);
    if (width === 2) { const half = encodeHalf(value); this.#bytes.push(half >>> 8, half & 0xff); return; }
    const bytes = new Uint8Array(width);
    const view = new DataView(bytes.buffer);
    if (width === 4) view.setFloat32(0, value); else view.setFloat64(0, value);
    this.#raw(bytes);
  }
  public finish(): Uint8Array { return Uint8Array.from(this.#bytes); }
}

function canonicalEntries(entries: readonly (readonly [CborValue, CborValue])[]): readonly (readonly [CborValue, CborValue])[] {
  return [...entries].sort(([left], [right]) => {
    const a = encodeCbor(left, { mode: "canonical" });
    const b = encodeCbor(right, { mode: "canonical" });
    if (a.length !== b.length) return a.length - b.length;
    for (let index = 0; index < a.length; index += 1) {
      const difference = (a[index] as number) - (b[index] as number);
      if (difference !== 0) return difference;
    }
    return 0;
  });
}

function decodeHalf(bits: number): number {
  const sign = (bits & 0x8000) === 0 ? 1 : -1;
  const exponent = (bits >>> 10) & 0x1f;
  const fraction = bits & 0x3ff;
  if (exponent === 0) return sign * 2 ** -14 * (fraction / 1024);
  if (exponent === 31) return fraction === 0 ? sign * Infinity : Number.NaN;
  return sign * 2 ** (exponent - 15) * (1 + fraction / 1024);
}

function encodeHalf(value: number): number {
  if (Number.isNaN(value)) return 0x7e00;
  const float = new Float32Array([value]);
  const bits = new Uint32Array(float.buffer)[0] as number;
  const sign = (bits >>> 16) & 0x8000;
  let exponent = ((bits >>> 23) & 0xff) - 127 + 15;
  let fraction = bits & 0x7fffff;
  if (exponent <= 0) {
    if (exponent < -10) return sign;
    fraction = (fraction | 0x800000) >> (1 - exponent);
    return sign | ((fraction + 0x1000) >> 13);
  }
  if (exponent >= 31) return sign | 0x7c00;
  const rounded = fraction + 0x1000;
  if ((rounded & 0x800000) !== 0) { exponent += 1; fraction = 0; }
  if (exponent >= 31) return sign | 0x7c00;
  return sign | (exponent << 10) | ((fraction + 0x1000) >> 13);
}

function sameNumber(left: number, right: number): boolean {
  return Object.is(left, right) || Number.isNaN(left) && Number.isNaN(right);
}

function canonicalFloatWidth(value: number): 2 | 4 | 8 {
  if (sameNumber(decodeHalf(encodeHalf(value)), value)) return 2;
  if (sameNumber(Math.fround(value), value)) return 4;
  return 8;
}

/** Decode exactly one CBOR item; trailing bytes are rejected. */
export function decodeCbor(bytes: Uint8Array, options: DecodeCborOptions = {}): CborValue {
  const reader = new Reader(copyBytes(bytes), options);
  const value = reader.value();
  if (!reader.done) throw failure("TRAILING_DATA", "Trailing data follows the CBOR item", reader.offset);
  return value;
}

export function encodeCbor(value: CborValue, options: EncodeCborOptions = {}): Uint8Array {
  const writer = new Writer(options.mode ?? "preserve");
  writer.value(value);
  return writer.finish();
}

/** Decode the byte string carried by tag 24 as a strict nested CBOR item. */
export function decodeEmbeddedCbor(value: CborValue, options: DecodeCborOptions = {}): CborValue {
  if (value.kind !== "tag" || value.tag !== 24n || value.value.kind !== "bytes") {
    throw new DeserializeError("TAG_MISMATCH", "Expected tag 24 containing a byte string");
  }
  return decodeCbor(value.value.value, options);
}
