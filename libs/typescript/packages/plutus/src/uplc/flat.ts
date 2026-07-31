import {
  CardanoBoundsError,
  DeserializeError,
  decodeCbor,
  encodeCbor,
  type CborValue,
} from "@xray-network/cardano-core";
import type { UplcConstant, UplcProgram, UplcTerm, UplcType } from "./ast.js";

const MAX_INPUT_BYTES = 16 * 1024 * 1024;
const MAX_DEPTH = 4_096;
const textDecoder = new TextDecoder(undefined, { fatal: true });
const textEncoder = new TextEncoder();

export interface ProgramDecodeOptions {
  readonly maxUniverseHeader?: number;
  readonly maxConstrFields?: number;
  readonly enforceDataWireLimit?: boolean;
}

class BitReader {
  readonly #bytes: Uint8Array;
  #bit = 0;

  public constructor(bytes: Uint8Array) {
    this.#bytes = bytes;
  }

  public bits(width: number): number {
    if (width < 0 || width > 32 || this.#bit + width > this.#bytes.length * 8) {
      throw flatError("TRUNCATED_INPUT", "truncated Flat input", this.#bit);
    }
    let value = 0;
    for (let count = 0; count < width; count += 1) {
      const byte = this.#bytes[this.#bit >> 3] ?? 0;
      value = value * 2 + ((byte >> (7 - (this.#bit & 7))) & 1);
      this.#bit += 1;
    }
    return value;
  }

  public natural(): bigint {
    let value = 0n;
    let shift = 0n;
    for (let count = 0; count < 10_000; count += 1) {
      const byte = this.bits(8);
      value |= BigInt(byte & 0x7f) << shift;
      if ((byte & 0x80) === 0) return value;
      shift += 7n;
    }
    throw flatError("OUT_OF_RANGE", "Flat natural is too large", this.#bit);
  }

  public word64(): bigint {
    const value = this.natural();
    if (value > 0xffff_ffff_ffff_ffffn) {
      throw flatError("OUT_OF_RANGE", "Flat value exceeds Word64", this.#bit);
    }
    return value;
  }

  public filler(): void {
    for (let count = 0; count < 8; count += 1) {
      if (this.bits(1) === 1) return;
    }
    throw flatError("INVALID_STRUCTURE", "invalid Flat byte-alignment filler", this.#bit);
  }

  public byteString(): Uint8Array {
    this.filler();
    const chunks: Uint8Array[] = [];
    let length = 0;
    for (;;) {
      const chunkLength = this.bits(8);
      if (chunkLength === 0) break;
      const chunk = new Uint8Array(chunkLength);
      for (let index = 0; index < chunkLength; index += 1) chunk[index] = this.bits(8);
      chunks.push(chunk);
      length += chunkLength;
      if (length > MAX_INPUT_BYTES) {
        throw new CardanoBoundsError("Flat byte string length", 0n, BigInt(MAX_INPUT_BYTES), BigInt(length));
      }
    }
    const output = new Uint8Array(length);
    let offset = 0;
    for (const chunk of chunks) {
      output.set(chunk, offset);
      offset += chunk.length;
    }
    return output;
  }

  public finish(): void {
    this.filler();
    if (this.#bit !== this.#bytes.length * 8) {
      throw flatError("TRAILING_DATA", "trailing Flat data", this.#bit);
    }
  }
}

class BitWriter {
  readonly #bytes: number[] = [];
  #current = 0;
  #used = 0;

  public bits(width: number, value: number): void {
    if (!Number.isSafeInteger(value) || value < 0 || (width < 31 && value >= 2 ** width)) {
      throw new RangeError(`Flat value ${value} does not fit ${width} bits`);
    }
    for (let index = width - 1; index >= 0; index -= 1) {
      this.#current = (this.#current << 1) | ((value >> index) & 1);
      this.#used += 1;
      if (this.#used === 8) {
        this.#bytes.push(this.#current);
        this.#current = 0;
        this.#used = 0;
      }
    }
  }

  public natural(value: bigint): void {
    if (value < 0n) throw new RangeError("Flat natural cannot be negative");
    let remaining = value;
    do {
      const low = Number(remaining & 0x7fn);
      remaining >>= 7n;
      this.bits(8, low | (remaining === 0n ? 0 : 0x80));
    } while (remaining !== 0n);
  }

  public filler(): void {
    do this.bits(1, this.#used === 7 ? 1 : 0);
    while (this.#used !== 0);
  }

  public byteString(value: Uint8Array): void {
    this.filler();
    for (let offset = 0; offset < value.length; offset += 255) {
      const length = Math.min(255, value.length - offset);
      this.bits(8, length);
      for (let index = 0; index < length; index += 1) this.bits(8, value[offset + index] ?? 0);
    }
    this.bits(8, 0);
  }

  public finish(): Uint8Array {
    this.filler();
    return Uint8Array.from(this.#bytes);
  }
}

export function decodeProgramEnvelope(
  bytes: Uint8Array,
  options: ProgramDecodeOptions = {},
): UplcProgram {
  if (bytes.length > MAX_INPUT_BYTES) {
    throw new CardanoBoundsError("serialized script length", 0n, BigInt(MAX_INPUT_BYTES), BigInt(bytes.length));
  }
  let envelope: CborValue;
  try {
    envelope = decodeCbor(Uint8Array.from(bytes));
  } catch (cause) {
    throw asDeserialize(cause, "invalid serialized Plutus script");
  }
  if (envelope.kind !== "bytes") {
    throw new DeserializeError("INVALID_STRUCTURE", "serialized Plutus script must be a CBOR byte string");
  }
  return decodeFlatProgram(envelope.value, options);
}

export function decodeProgramEnvelopeCompatible(
  bytes: Uint8Array,
  allowCborRemainder: boolean,
  options: ProgramDecodeOptions = {},
): UplcProgram {
  if (!allowCborRemainder) return decodeProgramEnvelope(bytes, options);
  if (bytes.length > MAX_INPUT_BYTES) {
    throw new CardanoBoundsError("serialized script length", 0n, BigInt(MAX_INPUT_BYTES), BigInt(bytes.length));
  }
  const first = bytes[0];
  if (first === undefined || (first >> 5) !== 2 || (first & 0x1f) === 31) {
    throw new DeserializeError("INVALID_STRUCTURE", "serialized Plutus script must start with a definite CBOR byte string");
  }
  const additional = first & 0x1f;
  let offset = 1;
  let length: bigint;
  if (additional < 24) length = BigInt(additional);
  else {
    const width = additional === 24 ? 1 : additional === 25 ? 2 : additional === 26 ? 4 : additional === 27 ? 8 : 0;
    if (width === 0 || offset + width > bytes.length) {
      throw new DeserializeError("TRUNCATED_INPUT", "truncated serialized Plutus script");
    }
    length = 0n;
    for (let index = 0; index < width; index += 1) length = (length << 8n) | BigInt(bytes[offset + index] ?? 0);
    offset += width;
  }
  if (length > BigInt(MAX_INPUT_BYTES) || length > BigInt(bytes.length - offset)) {
    throw new DeserializeError("TRUNCATED_INPUT", "truncated serialized Plutus script payload");
  }
  return decodeFlatProgram(bytes.slice(offset, offset + Number(length)), options);
}

export function encodeProgramEnvelope(program: UplcProgram): Uint8Array {
  return encodeCbor({
    kind: "bytes",
    value: encodeFlatProgram(program),
    encoding: { kind: "definite", width: 0 },
  }, { mode: "canonical" });
}

export function decodeFlatProgram(
  bytes: Uint8Array,
  options: ProgramDecodeOptions = {},
): UplcProgram {
  const reader = new BitReader(bytes);
  try {
    const version = [reader.natural(), reader.natural(), reader.natural()] as const;
    if (!isVersion(version, 1n, 0n, 0n) && !isVersion(version, 1n, 1n, 0n)) {
      throw new DeserializeError("INVALID_STRUCTURE", `unsupported UPLC version ${version.join(".")}`);
    }
    const term = decodeTerm(reader, version, 0, options);
    reader.finish();
    return { version, term };
  } catch (cause) {
    throw asDeserialize(cause, "invalid Flat UPLC program");
  }
}

export function encodeFlatProgram(program: UplcProgram): Uint8Array {
  const writer = new BitWriter();
  writer.natural(program.version[0]);
  writer.natural(program.version[1]);
  writer.natural(program.version[2]);
  encodeTerm(writer, program.term);
  return writer.finish();
}

function decodeTerm(
  reader: BitReader,
  version: UplcProgram["version"],
  depth: number,
  options: ProgramDecodeOptions,
): UplcTerm {
  type Frame =
    | { readonly kind: "delay" | "lambda" | "force" }
    | { readonly kind: "apply-function" }
    | { readonly kind: "apply-argument"; readonly function_: UplcTerm }
    | { readonly kind: "case-scrutinee" }
    | {
      readonly kind: "term-list";
      readonly owner:
        | { readonly kind: "constr"; readonly tag: bigint }
        | { readonly kind: "case"; readonly scrutinee: UplcTerm };
      readonly terms: readonly UplcTerm[];
      readonly maximum: number;
    };
  const frames: Frame[] = [];
  let completed: UplcTerm | undefined;

  for (;;) {
    if (completed === undefined) {
      if (depth + frames.length > 1_000_000) {
        throw flatError("DEPTH_LIMIT_EXCEEDED", "UPLC term is too deeply nested");
      }
      const tag = reader.bits(4);
      switch (tag) {
        case 0: completed = { kind: "var", index: reader.word64() }; break;
        case 1: frames.push({ kind: "delay" }); break;
        case 2: frames.push({ kind: "lambda" }); break;
        case 3: frames.push({ kind: "apply-function" }); break;
        case 4:
          completed = { kind: "constant", constant: decodeConstant(reader, depth + 1, options) };
          break;
        case 5: frames.push({ kind: "force" }); break;
        case 6: completed = { kind: "error" }; break;
        case 7: completed = { kind: "builtin", tag: reader.bits(7) }; break;
        case 8: {
          assertVersion110(version, "constr");
          const owner = { kind: "constr", tag: reader.word64() } as const;
          if (reader.bits(1) === 0) completed = { ...owner, fields: [] };
          else {
            frames.push({
              kind: "term-list",
              owner,
              terms: [],
              maximum: options.maxConstrFields ?? 1_000_000,
            });
          }
          break;
        }
        case 9:
          assertVersion110(version, "case");
          frames.push({ kind: "case-scrutinee" });
          break;
        default:
          throw flatError("INVALID_STRUCTURE", `unknown UPLC term tag ${tag}`);
      }
      if (completed === undefined) continue;
    }

    const frame = frames.pop();
    if (frame === undefined) return completed;
    switch (frame.kind) {
      case "delay": completed = { kind: "delay", term: completed }; break;
      case "lambda": completed = { kind: "lambda", body: completed }; break;
      case "force": completed = { kind: "force", term: completed }; break;
      case "apply-function":
        frames.push({ kind: "apply-argument", function_: completed });
        completed = undefined;
        break;
      case "apply-argument":
        completed = { kind: "apply", function: frame.function_, argument: completed };
        break;
      case "case-scrutinee": {
        const owner = { kind: "case", scrutinee: completed } as const;
        if (reader.bits(1) === 0) completed = { ...owner, branches: [] };
        else {
          frames.push({ kind: "term-list", owner, terms: [], maximum: 1_000_000 });
          completed = undefined;
        }
        break;
      }
      case "term-list": {
        const terms = [...frame.terms, completed];
        if (terms.length > frame.maximum) {
          throw flatError("OUT_OF_RANGE", "UPLC term list is too long");
        }
        if (reader.bits(1) === 1) {
          frames.push({ ...frame, terms });
          completed = undefined;
        } else {
          completed = frame.owner.kind === "constr"
            ? { kind: "constr", tag: frame.owner.tag, fields: terms }
            : { kind: "case", scrutinee: frame.owner.scrutinee, branches: terms };
        }
        break;
      }
    }
  }
}

function encodeTerm(writer: BitWriter, term: UplcTerm): void {
  type Operation =
    | { readonly kind: "term"; readonly term: UplcTerm }
    | { readonly kind: "bit"; readonly value: 0 | 1 };
  const operations: Operation[] = [{ kind: "term", term }];
  const pushList = (terms: readonly UplcTerm[]): void => {
    operations.push({ kind: "bit", value: 0 });
    for (let index = terms.length - 1; index >= 0; index -= 1) {
      operations.push({ kind: "term", term: terms[index] as UplcTerm });
      operations.push({ kind: "bit", value: 1 });
    }
  };
  while (operations.length !== 0) {
    const operation = operations.pop() as Operation;
    if (operation.kind === "bit") {
      writer.bits(1, operation.value);
      continue;
    }
    const current = operation.term;
    switch (current.kind) {
      case "var": writer.bits(4, 0); writer.natural(current.index); break;
      case "delay":
        writer.bits(4, 1);
        operations.push({ kind: "term", term: current.term });
        break;
      case "lambda":
        writer.bits(4, 2);
        operations.push({ kind: "term", term: current.body });
        break;
      case "apply":
        writer.bits(4, 3);
        operations.push({ kind: "term", term: current.argument });
        operations.push({ kind: "term", term: current.function });
        break;
      case "constant": writer.bits(4, 4); encodeConstant(writer, current.constant); break;
      case "force":
        writer.bits(4, 5);
        operations.push({ kind: "term", term: current.term });
        break;
      case "error": writer.bits(4, 6); break;
      case "builtin": writer.bits(4, 7); writer.bits(7, current.tag); break;
      case "constr":
        writer.bits(4, 8);
        writer.natural(current.tag);
        pushList(current.fields);
        break;
      case "case":
        writer.bits(4, 9);
        pushList(current.branches);
        operations.push({ kind: "term", term: current.scrutinee });
        break;
    }
  }
}

function decodeConstant(
  reader: BitReader,
  depth: number,
  options: ProgramDecodeOptions,
): UplcConstant {
  const tags: number[] = [];
  while (reader.bits(1) === 1) {
    if (tags.length >= (options.maxUniverseHeader ?? 1_000_000)) {
      throw flatError("OUT_OF_RANGE", "UPLC universe header exceeds the protocol limit");
    }
    tags.push(reader.bits(4));
  }
  const cursor = { value: 0 };
  const type = parseType(tags, cursor, depth);
  if (cursor.value !== tags.length) throw flatError("INVALID_STRUCTURE", "invalid UPLC universe application");
  return { type, value: decodeValue(reader, type, depth + 1, options) };
}

function encodeConstant(writer: BitWriter, constant: UplcConstant): void {
  for (const tag of typeTags(constant.type)) {
    writer.bits(1, 1);
    writer.bits(4, tag);
  }
  writer.bits(1, 0);
  encodeValue(writer, constant.type, constant.value);
}

function parseType(tags: readonly number[], cursor: { value: number }, depth: number): UplcType {
  if (depth > 32) throw flatError("DEPTH_LIMIT_EXCEEDED", "UPLC universe type is too deeply nested");
  const tag = tags[cursor.value++];
  switch (tag) {
    case 0: return { kind: "integer" };
    case 1: return { kind: "bytes" };
    case 2: return { kind: "string" };
    case 3: return { kind: "unit" };
    case 4: return { kind: "boolean" };
    case 8: return { kind: "data" };
    case 9: return { kind: "bls-g1" };
    case 10: return { kind: "bls-g2" };
    case 11: return { kind: "bls-ml" };
    case 13: return { kind: "value" };
    case 7: {
      const constructor = tags[cursor.value++];
      if (constructor === 5) return { kind: "list", item: parseType(tags, cursor, depth + 1) };
      if (constructor === 12) return { kind: "array", item: parseType(tags, cursor, depth + 1) };
      if (constructor === 7 && tags[cursor.value++] === 6) {
        const first = parseType(tags, cursor, depth + 1);
        return { kind: "pair", first, second: parseType(tags, cursor, depth + 1) };
      }
      throw flatError("INVALID_STRUCTURE", "unsupported UPLC universe type application");
    }
    default: throw flatError("INVALID_STRUCTURE", `unknown UPLC universe tag ${String(tag)}`);
  }
}

function typeTags(type: UplcType): number[] {
  switch (type.kind) {
    case "integer": return [0];
    case "bytes": return [1];
    case "string": return [2];
    case "unit": return [3];
    case "boolean": return [4];
    case "data": return [8];
    case "bls-g1": return [9];
    case "bls-g2": return [10];
    case "bls-ml": return [11];
    case "value": return [13];
    case "list": return [7, 5, ...typeTags(type.item)];
    case "array": return [7, 12, ...typeTags(type.item)];
    case "pair": return [7, 7, 6, ...typeTags(type.first), ...typeTags(type.second)];
  }
}

function decodeValue(
  reader: BitReader,
  type: UplcType,
  depth: number,
  options: ProgramDecodeOptions,
): unknown {
  if (depth > MAX_DEPTH) throw flatError("DEPTH_LIMIT_EXCEEDED", "UPLC constant is too deeply nested");
  switch (type.kind) {
    case "integer": {
      const natural = reader.natural();
      return (natural & 1n) === 0n ? natural >> 1n : -((natural >> 1n) + 1n);
    }
    case "bytes": return reader.byteString();
    case "string": {
      try {
        return textDecoder.decode(reader.byteString());
      } catch (cause) {
        throw new DeserializeError("INVALID_STRUCTURE", "UPLC string is not valid UTF-8", { cause });
      }
    }
    case "unit": return null;
    case "boolean": return reader.bits(1) === 1;
    case "data": {
      const bytes = reader.byteString();
      try {
        const value = decodeCbor(bytes);
        validatePlutusDataNode(value, 0, options.enforceDataWireLimit ?? true);
        return value;
      } catch (cause) {
        throw asDeserialize(cause, "invalid Data constant");
      }
    }
    case "bls-g1":
    case "bls-g2":
    case "bls-ml":
      throw flatError("INVALID_STRUCTURE", `Flat decoding is not supported for ${type.kind}`);
    case "list":
    case "array": {
      const output: unknown[] = [];
      while (reader.bits(1) === 1) output.push(decodeValue(reader, type.item, depth + 1, options));
      return output;
    }
    case "pair":
      return [
        decodeValue(reader, type.first, depth + 1, options),
        decodeValue(reader, type.second, depth + 1, options),
      ] as const;
    case "value":
      return decodeFlatValue(reader, depth + 1, options);
  }
}

function encodeValue(writer: BitWriter, type: UplcType, value: unknown): void {
  switch (type.kind) {
    case "integer": {
      const integer = value as bigint;
      writer.natural(integer >= 0n ? integer << 1n : ((-integer) << 1n) - 1n);
      break;
    }
    case "bytes": writer.byteString(value as Uint8Array); break;
    case "string": writer.byteString(textEncoder.encode(value as string)); break;
    case "unit": break;
    case "boolean": writer.bits(1, value === true ? 1 : 0); break;
    case "data":
      writer.byteString(encodePlutusData(value as CborValue));
      break;
    case "bls-g1":
    case "bls-g2":
    case "bls-ml":
      throw new TypeError(`Flat encoding is not supported for ${type.kind}`);
    case "list":
    case "array":
      for (const item of value as readonly unknown[]) {
        writer.bits(1, 1);
        encodeValue(writer, type.item, item);
      }
      writer.bits(1, 0);
      break;
    case "pair": {
      const pair = value as readonly [unknown, unknown];
      encodeValue(writer, type.first, pair[0]);
      encodeValue(writer, type.second, pair[1]);
      break;
    }
    case "value":
      encodeFlatValue(writer, value as FlatValue);
      break;
  }
}

export type FlatValue = readonly (readonly [
  currency: Uint8Array,
  tokens: readonly (readonly [token: Uint8Array, quantity: bigint])[],
])[];

function decodeFlatValue(
  reader: BitReader,
  depth: number,
  options: ProgramDecodeOptions,
): FlatValue {
  const outer: Array<readonly [Uint8Array, Array<readonly [Uint8Array, bigint]>]> = [];
  while (reader.bits(1) === 1) {
    const currency = reader.byteString();
    if (currency.length > 32) throw flatError("OUT_OF_RANGE", "Value currency exceeds 32 bytes");
    const inner: Array<readonly [Uint8Array, bigint]> = [];
    while (reader.bits(1) === 1) {
      const token = reader.byteString();
      if (token.length > 32) throw flatError("OUT_OF_RANGE", "Value token name exceeds 32 bytes");
      inner.push([token, decodeValue(reader, { kind: "integer" }, depth + 1, options) as bigint]);
    }
    outer.push([currency, inner]);
  }
  return outer;
}

function encodeFlatValue(writer: BitWriter, value: FlatValue): void {
  for (const [currency, tokens] of value) {
    writer.bits(1, 1);
    writer.byteString(currency);
    for (const [token, quantity] of tokens) {
      writer.bits(1, 1);
      writer.byteString(token);
      encodeValue(writer, { kind: "integer" }, quantity);
    }
    writer.bits(1, 0);
  }
  writer.bits(1, 0);
}

function assertVersion110(version: UplcProgram["version"], construct: string): void {
  if (!isVersion(version, 1n, 1n, 0n)) {
    throw flatError("INVALID_STRUCTURE", `${construct} requires UPLC 1.1.0`);
  }
}

function isVersion(
  version: UplcProgram["version"],
  major: bigint,
  minor: bigint,
  patch: bigint,
): boolean {
  return version[0] === major && version[1] === minor && version[2] === patch;
}

function flatError(
  failure: ConstructorParameters<typeof DeserializeError>[0],
  message: string,
  bitOffset?: number,
): DeserializeError {
  return new DeserializeError(failure, message, bitOffset === undefined ? {} : { offset: bitOffset >> 3 });
}

function asDeserialize(cause: unknown, message: string): DeserializeError {
  if (cause instanceof DeserializeError) return cause;
  return new DeserializeError("INVALID_STRUCTURE", message, { cause });
}

export function encodePlutusData(value: CborValue): Uint8Array {
  validatePlutusDataNode(value);
  return encodeCbor(normalizeData(value));
}

export function validatePlutusDataNode(
  value: CborValue,
  depth = 0,
  enforceWireLimit = true,
): void {
  if (depth > 128) throw flatError("DEPTH_LIMIT_EXCEEDED", "Plutus Data is too deeply nested");
  if (value.kind === "unsigned" || value.kind === "negative") return;
  if (value.kind === "bytes") {
    const valid = value.encoding.kind === "definite"
      ? !enforceWireLimit || value.value.length <= 64
      : !enforceWireLimit || value.encoding.chunks.every(
        (chunk) => chunk.value instanceof Uint8Array && chunk.value.length <= 64,
      );
    if (!valid) {
      throw flatError("OUT_OF_RANGE", "Plutus Data byte chunks are limited to 64 bytes");
    }
    return;
  }
  if (value.kind === "array") {
    for (const item of value.values) validatePlutusDataNode(item, depth + 1, enforceWireLimit);
    return;
  }
  if (value.kind === "map") {
    for (const [key, item] of value.entries) {
      validatePlutusDataNode(key, depth + 1, enforceWireLimit);
      validatePlutusDataNode(item, depth + 1, enforceWireLimit);
    }
    return;
  }
  if (value.kind === "tag") {
    if (value.tag === 2n || value.tag === 3n) {
      if (value.value.kind !== "bytes") throw flatError("INVALID_STRUCTURE", "Plutus Data bignum must contain bytes");
      validatePlutusDataNode(value.value, depth + 1, enforceWireLimit);
      return;
    }
    if ((value.tag >= 121n && value.tag <= 127n) || (value.tag >= 1280n && value.tag <= 1400n)) {
      if (value.value.kind !== "array") throw flatError("INVALID_STRUCTURE", "Plutus Data constructor fields must be a list");
      for (const item of value.value.values) {
        validatePlutusDataNode(item, depth + 1, enforceWireLimit);
      }
      return;
    }
    if (value.tag === 102n && value.value.kind === "array" && value.value.values.length === 2) {
      const [alternative, fields] = value.value.values;
      if (alternative?.kind !== "unsigned" || fields?.kind !== "array") {
        throw flatError("INVALID_STRUCTURE", "invalid general Plutus Data constructor");
      }
      for (const item of fields.values) validatePlutusDataNode(item, depth + 1, enforceWireLimit);
      return;
    }
  }
  throw flatError("INVALID_STRUCTURE", `unsupported ${value.kind} in Plutus Data`);
}

function normalizeData(value: CborValue): CborValue {
  switch (value.kind) {
    case "unsigned":
    case "negative":
      return { kind: value.kind, value: value.value, encoding: { width: 0 } };
    case "bytes": {
      const bytes = Uint8Array.from(value.value);
      return {
        kind: "bytes",
        value: bytes,
        encoding: bytes.length <= 64
          ? { kind: "definite", width: 0 }
          : {
              kind: "indefinite",
              chunks: Array.from(
                { length: Math.ceil(bytes.length / 64) },
                (_, index) => ({ value: bytes.slice(index * 64, (index + 1) * 64), width: 0 }),
              ),
            },
      };
    }
    case "array":
      return {
        kind: "array",
        values: value.values.map(normalizeData),
        encoding: { kind: "definite", width: 0 },
      };
    case "map":
      return {
        kind: "map",
        entries: value.entries.map(([key, item]) => [normalizeData(key), normalizeData(item)]),
        encoding: { kind: "definite", width: 0 },
      };
    case "tag":
      if (value.tag === 2n || value.tag === 3n) {
        if (value.value.kind !== "bytes") throw new TypeError("Plutus Data bignum must contain bytes");
        return { kind: "tag", tag: value.tag, value: normalizeData(value.value), encoding: { width: 0 } };
      }
      return {
        kind: "tag",
        tag: value.tag,
        value: normalizeData(value.value),
        encoding: { width: 0 },
      };
    default:
      throw new TypeError(`unsupported ${value.kind} in Plutus Data`);
  }
}
