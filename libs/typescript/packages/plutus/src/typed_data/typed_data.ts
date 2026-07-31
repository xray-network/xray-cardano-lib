import {
  ConstrPlutusData,
  PlutusData,
  PlutusDataKind,
  PlutusDataList,
  PlutusMap,
} from "@xray-network/cardano-chain";
import {
  BigInteger,
  bytesToHex,
  hexToBytes,
} from "@xray-network/cardano-core";
import { Constr } from "./types.js";
import type {
  AnySchema,
  ArraySchema,
  BooleanSchema,
  BytesSchema,
  DataSchema,
  Datum,
  EnumItemSchema,
  EnumSchema,
  Exact,
  IntegerSchema,
  Json,
  LiteralSchema,
  MapSchema,
  NullableSchema,
  ObjectSchema,
  PlutusDataValue,
  Redeemer,
  SchemaProperties,
  StaticSchema,
  TupleSchema,
} from "./types.js";

export declare namespace Data {
  export type Static<S extends DataSchema> = StaticSchema<S>;
}

interface IntegerOptions {
  readonly minimum?: number | bigint;
  readonly maximum?: number | bigint;
  readonly exclusiveMinimum?: number | bigint;
  readonly exclusiveMaximum?: number | bigint;
}

interface BytesOptions {
  readonly minLength?: number;
  readonly maxLength?: number;
  readonly enum?: readonly string[];
}

interface CollectionOptions {
  readonly minItems?: number;
  readonly maxItems?: number;
}

const textEncoder = new TextEncoder();
const textDecoder = new TextDecoder(undefined, { fatal: true });
const MAX_DATA_DEPTH = 128;

function Integer(options: IntegerOptions = {}): IntegerSchema {
  return { dataType: "integer", ...options };
}

function Bytes(options: BytesOptions = {}): BytesSchema {
  return { dataType: "bytes", ...options };
}

function BooleanType(): BooleanSchema {
  return { dataType: "boolean" };
}

function Any(): AnySchema {
  return { dataType: "any" };
}

function ArrayType<S extends DataSchema>(
  items: S,
  options: CollectionOptions & { readonly uniqueItems?: boolean } = {},
): ArraySchema<S> {
  return { dataType: "list", items, ...options };
}

function MapType<K extends DataSchema, V extends DataSchema>(
  keys: K,
  values: V,
  options: CollectionOptions = {},
): MapSchema<K, V> {
  return { dataType: "map", keys, values, ...options };
}

function ObjectType<const P extends SchemaProperties>(
  properties: P,
  options: { readonly hasConstr?: boolean } = {},
): ObjectSchema<P> {
  return {
    dataType: "object",
    properties,
    hasConstr: options.hasConstr ?? true,
    index: 0,
  };
}

function Tuple<const S extends readonly DataSchema[]>(
  items: S,
  options: { readonly hasConstr?: boolean } = {},
): TupleSchema<S> {
  return {
    dataType: "tuple",
    items,
    hasConstr: options.hasConstr ?? false,
  };
}

function Literal<T extends string>(title: T): LiteralSchema<T> {
  assertConstructorTitle(title);
  return { dataType: "literal", title, index: 0 };
}

function Nullable<S extends DataSchema>(item: S): NullableSchema<S> {
  return { dataType: "nullable", item };
}

function Enum<const S extends readonly EnumItemSchema[]>(items: S): EnumSchema<S> {
  if (items.length === 0) throw new RangeError("Enum requires at least one item");
  for (const item of items) {
    if (item.dataType === "literal") assertConstructorTitle(item.title);
    else {
      const [title] = Object.keys(item.properties);
      if (title === undefined) throw new TypeError("Enum object requires a named constructor");
      assertConstructorTitle(title);
    }
  }
  return { dataType: "enum", items };
}

/**
 * Converts a data value, or a value plus schema, to Plutus CBOR hex.
 */
function to<S extends DataSchema>(value: Exact<StaticSchema<S>>, schema: S): Datum | Redeemer;
function to(value: PlutusDataValue): Datum | Redeemer;
function to(value: unknown, schema?: DataSchema): Datum | Redeemer {
  const data = schema === undefined
    ? value as PlutusDataValue
    : castTo(value, schema);
  return serializeData(data).to_cbor_hex();
}

/**
 * Converts Plutus CBOR hex to a data value, optionally applying a schema.
 */
function from<S extends DataSchema>(raw: Datum | Redeemer, schema: S): StaticSchema<S>;
function from(raw: Datum | Redeemer): PlutusDataValue;
function from(raw: Datum | Redeemer, schema?: DataSchema): unknown {
  const data = deserializeData(PlutusData.from_cbor_hex(raw));
  return schema === undefined ? data : castFrom(data, schema);
}

/**
 * Converts ordinary JSON-like values to the data representation.
 * Strings prefixed with `0x` are interpreted as bytes; other strings are UTF-8 encoded.
 */
function fromJson(json: Json): PlutusDataValue {
  if (typeof json === "string") {
    return json.startsWith("0x")
      ? bytesToHex(hexToBytes(json.slice(2)))
      : bytesToHex(textEncoder.encode(json));
  }
  if (typeof json === "number") {
    if (!Number.isSafeInteger(json)) throw new TypeError("JSON numbers must be safe integers");
    return BigInt(json);
  }
  if (typeof json === "bigint") return json;
  if (Array.isArray(json)) return json.map(fromJson);
  if (typeof json === "object" && json !== null) {
    const map = new Map<PlutusDataValue, PlutusDataValue>();
    for (const [key, value] of Object.entries(json)) map.set(fromJson(key), fromJson(value));
    return map;
  }
  throw new TypeError("Boolean and null JSON values require an explicit schema");
}

/**
 * Converts data to JSON-like values. Constructors require an explicit schema and are rejected here.
 */
function toJson(data: PlutusDataValue): Json {
  if (typeof data === "bigint") {
    const number = Number(data);
    return Number.isSafeInteger(number) ? number : data;
  }
  if (typeof data === "string") {
    const bytes = hexToBytes(data);
    try {
      return textDecoder.decode(bytes);
    } catch {
      return `0x${bytesToHex(bytes)}`;
    }
  }
  if (Array.isArray(data)) return data.map(toJson);
  if (data instanceof Map) {
    const output: Record<string, Json> = Object.create(null) as Record<string, Json>;
    for (const [key, value] of data) {
      const converted = toJson(key);
      if (typeof converted !== "string" && typeof converted !== "number" && typeof converted !== "bigint") {
        throw new TypeError("Only byte strings and integers can be JSON object keys");
      }
      output[String(converted)] = toJson(value);
    }
    return output;
  }
  throw new TypeError("Constructors cannot be converted to JSON without a schema");
}

function castFrom<S extends DataSchema>(data: PlutusDataValue, schema: S): StaticSchema<S> {
  return decodeWithSchema(data, schema, 0) as StaticSchema<S>;
}

function castTo<S extends DataSchema>(
  value: Exact<StaticSchema<S>>,
  schema: S,
): PlutusDataValue {
  return encodeWithSchema(value, schema, 0);
}

function serializeData(data: PlutusDataValue, depth = 0): PlutusData {
  assertDepth(depth);
  if (typeof data === "bigint") {
    return PlutusData.new_integer(BigInteger.from_str(data.toString()));
  }
  if (typeof data === "string") return PlutusData.new_bytes(hexToBytes(data));
  if (data instanceof Constr) {
    const fields = PlutusDataList.from(data.fields.map((field) => serializeData(field, depth + 1)));
    return PlutusData.new_constr_plutus_data(ConstrPlutusData.new(BigInt(data.index), fields));
  }
  if (Array.isArray(data)) {
    return PlutusData.new_list(
      PlutusDataList.from(data.map((item) => serializeData(item, depth + 1))),
    );
  }
  if (data instanceof Map) {
    const map = PlutusMap.new();
    for (const [key, value] of data) {
      map.append(serializeData(key, depth + 1), serializeData(value, depth + 1));
    }
    return PlutusData.new_map(map);
  }
  throw new TypeError("Unsupported data value");
}

function deserializeData(data: PlutusData, depth = 0): PlutusDataValue {
  assertDepth(depth);
  switch (data.kind()) {
    case PlutusDataKind.ConstrPlutusData: {
      const constr = data.as_constr_plutus_data();
      if (constr === undefined) throw new TypeError("Invalid constructor data");
      const index = Number(constr.alternative());
      if (!Number.isSafeInteger(index)) throw new RangeError("Constructor index exceeds safe integer range");
      return new Constr(
        index,
        constr.fields().values().map((field) => deserializeData(field, depth + 1)),
      );
    }
    case PlutusDataKind.Map: {
      const source = data.as_map();
      if (source === undefined) throw new TypeError("Invalid map data");
      const map = new Map<PlutusDataValue, PlutusDataValue>();
      for (const [key, value] of source.entries()) {
        map.set(deserializeData(key, depth + 1), deserializeData(value, depth + 1));
      }
      return map;
    }
    case PlutusDataKind.List: {
      const list = data.as_list();
      if (list === undefined) throw new TypeError("Invalid list data");
      return list.values().map((item) => deserializeData(item, depth + 1));
    }
    case PlutusDataKind.Integer: {
      const integer = data.as_integer();
      if (integer === undefined) throw new TypeError("Invalid integer data");
      return BigInt(integer.to_str());
    }
    case PlutusDataKind.Bytes: {
      const bytes = data.as_bytes();
      if (bytes === undefined) throw new TypeError("Invalid bytes data");
      return bytesToHex(bytes);
    }
  }
}

function decodeWithSchema(data: PlutusDataValue, schema: DataSchema, depth: number): unknown {
  assertDepth(depth);
  switch (schema.dataType) {
    case "integer":
      if (typeof data !== "bigint") throw new TypeError("Expected integer data");
      checkInteger(data, schema);
      return data;
    case "bytes":
      if (typeof data !== "string") throw new TypeError("Expected byte-string data");
      checkBytes(data, schema);
      return data;
    case "boolean":
      if (!(data instanceof Constr) || data.fields.length !== 0 || (data.index !== 0 && data.index !== 1)) {
        throw new TypeError("Expected boolean constructor");
      }
      return data.index === 1;
    case "any":
      return data;
    case "list":
      if (!Array.isArray(data)) throw new TypeError("Expected list data");
      checkCollection(data.length, schema);
      if (schema.uniqueItems === true && !structurallyUnique(data)) {
        throw new TypeError("List contains duplicate items");
      }
      return data.map((item) => decodeWithSchema(item, schema.items, depth + 1));
    case "tuple": {
      const fields = schema.hasConstr
        ? constructorFields(data, 0, "tuple")
        : arrayValue(data, "tuple");
      if (fields.length !== schema.items.length) throw new TypeError("Tuple length does not match schema");
      return fields.map((field, index) => decodeWithSchema(field, required(schema.items[index], "tuple schema"), depth + 1));
    }
    case "object": {
      const fields = schema.hasConstr
        ? constructorFields(data, schema.index, "object")
        : arrayValue(data, "object");
      const entries = Object.entries(schema.properties);
      if (fields.length !== entries.length) throw new TypeError("Object fields do not match schema");
      return Object.fromEntries(entries.map(([name, field], index) => [
        name,
        decodeWithSchema(required(fields[index], "object field"), field, depth + 1),
      ]));
    }
    case "map": {
      if (!(data instanceof Map)) throw new TypeError("Expected map data");
      checkCollection(data.size, schema);
      const output = new Map<unknown, unknown>();
      for (const [key, value] of data) {
        output.set(
          decodeWithSchema(key, schema.keys, depth + 1),
          decodeWithSchema(value, schema.values, depth + 1),
        );
      }
      return output;
    }
    case "literal":
      if (!(data instanceof Constr) || data.index !== schema.index || data.fields.length !== 0) {
        throw new TypeError(`Expected ${schema.title} constructor`);
      }
      return schema.title;
    case "nullable":
      if (!(data instanceof Constr)) throw new TypeError("Expected nullable constructor");
      if (data.index === 1 && data.fields.length === 0) return null;
      if (data.index === 0 && data.fields.length === 1) {
        return decodeWithSchema(required(data.fields[0], "nullable field"), schema.item, depth + 1);
      }
      throw new TypeError("Invalid nullable constructor");
    case "enum":
      return decodeEnum(data, schema, depth + 1);
  }
}

function encodeWithSchema(value: unknown, schema: DataSchema, depth: number): PlutusDataValue {
  assertDepth(depth);
  switch (schema.dataType) {
    case "integer":
      if (typeof value !== "bigint") throw new TypeError("Expected bigint");
      checkInteger(value, schema);
      return value;
    case "bytes":
      if (typeof value !== "string") throw new TypeError("Expected hexadecimal byte string");
      checkBytes(value, schema);
      return value;
    case "boolean":
      if (typeof value !== "boolean") throw new TypeError("Expected boolean");
      return new Constr(value ? 1 : 0, []);
    case "any":
      return value as PlutusDataValue;
    case "list":
      if (!Array.isArray(value)) throw new TypeError("Expected array");
      checkCollection(value.length, schema);
      if (schema.uniqueItems === true && !structurallyUnique(value)) {
        throw new TypeError("List contains duplicate items");
      }
      return value.map((item) => encodeWithSchema(item, schema.items, depth + 1));
    case "tuple": {
      if (!Array.isArray(value) || value.length !== schema.items.length) {
        throw new TypeError("Tuple length does not match schema");
      }
      const fields = value.map((item, index) =>
        encodeWithSchema(item, required(schema.items[index], "tuple schema"), depth + 1));
      return schema.hasConstr ? new Constr(0, fields) : fields;
    }
    case "object": {
      if (typeof value !== "object" || value === null || Array.isArray(value)) {
        throw new TypeError("Expected object");
      }
      const record = value as Record<string, unknown>;
      const entries = Object.entries(schema.properties);
      if (Object.keys(record).length !== entries.length) throw new TypeError("Object fields do not match schema");
      const fields = entries.map(([name, field]) => {
        if (!Object.hasOwn(record, name)) throw new TypeError(`Missing object field ${name}`);
        return encodeWithSchema(record[name], field, depth + 1);
      });
      return schema.hasConstr ? new Constr(schema.index, fields) : fields;
    }
    case "map": {
      if (!(value instanceof Map)) throw new TypeError("Expected Map");
      checkCollection(value.size, schema);
      const output = new Map<PlutusDataValue, PlutusDataValue>();
      for (const [key, item] of value) {
        output.set(
          encodeWithSchema(key, schema.keys, depth + 1),
          encodeWithSchema(item, schema.values, depth + 1),
        );
      }
      return output;
    }
    case "literal":
      if (value !== schema.title) throw new TypeError(`Expected literal ${schema.title}`);
      return new Constr(schema.index, []);
    case "nullable":
      return value === null
        ? new Constr(1, [])
        : new Constr(0, [encodeWithSchema(value, schema.item, depth + 1)]);
    case "enum":
      return encodeEnum(value, schema, depth + 1);
  }
}

function decodeEnum(data: PlutusDataValue, schema: EnumSchema, depth: number): unknown {
  if (!(data instanceof Constr)) throw new TypeError("Expected enum constructor");
  const item = schema.items[data.index];
  if (item === undefined) throw new TypeError(`Unknown enum constructor ${data.index}`);
  if (item.dataType === "literal") {
    if (data.fields.length !== 0) throw new TypeError("Literal enum constructor must have no fields");
    return item.title;
  }
  const [title, payloadSchema] = singleProperty(item.properties, "enum object");
  let payload: unknown;
  if (payloadSchema.dataType === "tuple") {
    payload = decodeWithSchema(data.fields, { ...payloadSchema, hasConstr: false }, depth);
  } else if (payloadSchema.dataType === "object") {
    payload = decodeWithSchema(data.fields, { ...payloadSchema, hasConstr: false }, depth);
  } else {
    if (data.fields.length !== 1) throw new TypeError("Enum payload requires one field");
    payload = decodeWithSchema(required(data.fields[0], "enum field"), payloadSchema, depth);
  }
  return { [title]: payload };
}

function encodeEnum(value: unknown, schema: EnumSchema, depth: number): PlutusDataValue {
  if (typeof value === "string") {
    const index = schema.items.findIndex((item) => item.dataType === "literal" && item.title === value);
    if (index < 0) throw new TypeError(`Unknown enum literal ${value}`);
    return new Constr(index, []);
  }
  if (typeof value !== "object" || value === null || Array.isArray(value)) {
    throw new TypeError("Expected enum literal or object");
  }
  const [title, payload] = singleProperty(value as Record<string, unknown>, "enum value");
  const index = schema.items.findIndex((item) =>
    item.dataType === "object" && Object.keys(item.properties)[0] === title);
  if (index < 0) throw new TypeError(`Unknown enum constructor ${title}`);
  const item = required(schema.items[index], "enum schema");
  if (item.dataType !== "object") throw new TypeError("Invalid enum schema");
  const [, payloadSchema] = singleProperty(item.properties, "enum object");
  if (payloadSchema.dataType === "tuple") {
    const encoded = encodeWithSchema(payload, { ...payloadSchema, hasConstr: false }, depth);
    return new Constr(index, arrayValue(encoded, "enum tuple"));
  }
  if (payloadSchema.dataType === "object") {
    const encoded = encodeWithSchema(payload, { ...payloadSchema, hasConstr: false }, depth);
    return new Constr(index, arrayValue(encoded, "enum object"));
  }
  const encoded = encodeWithSchema(payload, payloadSchema, depth);
  return new Constr(index, Array.isArray(encoded) ? encoded : [encoded]);
}

function checkInteger(value: bigint, schema: IntegerSchema): void {
  if (schema.minimum !== undefined && value < BigInt(schema.minimum)) {
    throw new RangeError(`Integer ${value} is below minimum ${schema.minimum}`);
  }
  if (schema.maximum !== undefined && value > BigInt(schema.maximum)) {
    throw new RangeError(`Integer ${value} is above maximum ${schema.maximum}`);
  }
  if (schema.exclusiveMinimum !== undefined && value <= BigInt(schema.exclusiveMinimum)) {
    throw new RangeError(`Integer ${value} is not above ${schema.exclusiveMinimum}`);
  }
  if (schema.exclusiveMaximum !== undefined && value >= BigInt(schema.exclusiveMaximum)) {
    throw new RangeError(`Integer ${value} is not below ${schema.exclusiveMaximum}`);
  }
}

function checkBytes(value: string, schema: BytesSchema): void {
  const bytes = hexToBytes(value);
  if (schema.enum !== undefined && !schema.enum.includes(value)) {
    throw new RangeError(`Byte string ${value} is not an allowed value`);
  }
  if (schema.minLength !== undefined && bytes.length < schema.minLength) {
    throw new RangeError(`Byte string is shorter than ${schema.minLength} bytes`);
  }
  if (schema.maxLength !== undefined && bytes.length > schema.maxLength) {
    throw new RangeError(`Byte string is longer than ${schema.maxLength} bytes`);
  }
}

function checkCollection(length: number, schema: CollectionOptions): void {
  if (schema.minItems !== undefined && length < schema.minItems) {
    throw new RangeError(`Collection contains fewer than ${schema.minItems} items`);
  }
  if (schema.maxItems !== undefined && length > schema.maxItems) {
    throw new RangeError(`Collection contains more than ${schema.maxItems} items`);
  }
}

function structurallyUnique(values: readonly unknown[]): boolean {
  const fingerprints = values.map((value) => stableValue(value));
  return new Set(fingerprints).size === fingerprints.length;
}

function stableValue(value: unknown): string {
  if (typeof value === "bigint") return `i:${value}`;
  if (typeof value === "string") return `b:${value}`;
  if (typeof value === "boolean") return `o:${value}`;
  if (value === null) return "n";
  if (Array.isArray(value)) return `l:[${value.map(stableValue).join(",")}]`;
  if (value instanceof Constr) return `c:${value.index}[${value.fields.map(stableValue).join(",")}]`;
  if (value instanceof Map) {
    return `m:{${[...value].map(([key, item]) => `${stableValue(key)}=${stableValue(item)}`).join(",")}}`;
  }
  if (typeof value === "object") {
    return `r:{${Object.entries(value as Record<string, unknown>).sort(([left], [right]) => left.localeCompare(right)).map(([key, item]) => `${key}=${stableValue(item)}`).join(",")}}`;
  }
  return `${typeof value}:${String(value)}`;
}

function constructorFields(data: PlutusDataValue, index: number, name: string): PlutusDataValue[] {
  if (!(data instanceof Constr) || data.index !== index) {
    throw new TypeError(`Expected ${name} constructor ${index}`);
  }
  return data.fields;
}

function arrayValue(data: PlutusDataValue, name: string): PlutusDataValue[] {
  if (!Array.isArray(data)) throw new TypeError(`Expected ${name} list`);
  return data;
}

function assertConstructorTitle(title: string): void {
  if (title.length === 0 || title[0] !== title[0]?.toUpperCase()) {
    throw new TypeError(`Constructor '${title}' must start with an uppercase letter`);
  }
}

function assertDepth(depth: number): void {
  if (depth > MAX_DATA_DEPTH) throw new RangeError(`Data nesting exceeds ${MAX_DATA_DEPTH}`);
}

function required<T>(value: T | undefined, name: string): T {
  if (value === undefined) throw new TypeError(`Missing ${name}`);
  return value;
}

function singleProperty<T>(
  value: Readonly<Record<string, T>>,
  name: string,
): readonly [string, T] {
  const entries = Object.entries(value);
  if (entries.length !== 1) throw new TypeError(`${name} must have exactly one property`);
  return required(entries[0], name);
}

export const Data = {
  Integer,
  Bytes,
  Boolean: BooleanType,
  Any,
  Array: ArrayType,
  Map: MapType,
  Object: ObjectType,
  Enum,
  Tuple,
  Literal,
  Nullable,
  to,
  from,
  fromJson,
  toJson,
  void: (): Datum | Redeemer => "d87980",
  castFrom,
  castTo,
} as const;
