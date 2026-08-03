import { PlutusData } from "@xray-network/xray-cardano-lib-chain";
import { blake2b224 } from "@xray-network/xray-cardano-lib-crypto";
import { encodeCbor, type CborValue } from "@xray-network/xray-cardano-lib-core";
import { SerializedPlutusScript } from "../uplc/serialized-script.js";
import type { UplcConstant } from "../uplc/ast.js";

const MAX_INPUT = 16 * 1024 * 1024, MAX_DEPTH = 128, MAX_NODES = 100_000;
const META_SCHEMA = "https://cips.cardano.org/cips/cip57/schemas/plutus-blueprint.json";
export interface BlueprintViolation { readonly code: string; readonly path: string; readonly message: string }
export interface PlutusBlueprint { readonly preamble: Readonly<Record<string, unknown>>; readonly validators: readonly Readonly<Record<string, unknown>>[]; readonly definitions?: Readonly<Record<string, unknown>>; readonly source: Readonly<Record<string, unknown>> }
interface JsonObject extends Record<string, unknown> {
  schema?: JsonObject; title?: unknown; purpose?: unknown; dataType?: unknown; $schema?: unknown; $vocabulary?: unknown;
  preamble?: JsonObject; plutusVersion?: unknown; validators?: unknown; redeemer?: unknown; datum?: unknown; parameters?: unknown;
  compiledCode?: unknown; hash?: unknown; definitions?: unknown; not?: unknown; minimum?: unknown; maximum?: unknown; multipleOf?: unknown;
  minLength?: unknown; maxLength?: unknown; enum?: unknown; minItems?: unknown; maxItems?: unknown; uniqueItems?: unknown; items?: unknown;
  keys?: unknown; values?: unknown; index?: unknown; fields?: unknown;
  $ref?: unknown; allOf?: unknown; anyOf?: unknown; oneOf?: unknown; exclusiveMinimum?: unknown;
  exclusiveMaximum?: unknown; left?: unknown; right?: unknown;
  description?: unknown; compiler?: unknown; name?: unknown; version?: unknown; type?: unknown; value?: unknown; kind?: unknown;
}

const schemaDefinitions = new WeakMap<object, Readonly<Record<string, unknown>>>();

function plain(value: unknown): value is JsonObject { return typeof value === "object" && value !== null && !Array.isArray(value); }
function pointer(path: string, key: string | number): string { return `${path}/${String(key).replaceAll("~", "~0").replaceAll("/", "~1")}`; }
function freezeClone(value: unknown, depth = 0, budget = { nodes: 0 }): unknown {
  if (depth > MAX_DEPTH) throw new RangeError(`blueprint nesting exceeds ${MAX_DEPTH}`);
  if (++budget.nodes > MAX_NODES) throw new RangeError(`blueprint value count exceeds ${MAX_NODES}`);
  if (Array.isArray(value)) return Object.freeze(value.map((item) => freezeClone(item, depth + 1, budget)));
  if (plain(value)) {
    const output = Object.create(null) as Record<string, unknown>;
    for (const [key, item] of Object.entries(value)) {
      if (key === "__proto__" || key === "prototype" || key === "constructor") throw new TypeError(`unsafe blueprint property ${key}`);
      output[key] = freezeClone(item, depth + 1, budget);
    }
    return Object.freeze(output);
  }
  return value;
}
function assertString(value: unknown, path: string): asserts value is string { if (typeof value !== "string") throw new TypeError(`${path} must be a string`); }
function assertNoDuplicateJsonKeys(source: string): void {
  let offset = 0;
  const whitespace = (): void => { while (/\s/u.test(source[offset] ?? "")) offset += 1; };
  const string = (): string => {
    const start = offset; if (source[offset++] !== '"') throw new SyntaxError(`expected string at ${start}`);
    while (offset < source.length) {
      const character = source[offset++];
      if (character === '"') return JSON.parse(source.slice(start, offset)) as string;
      if (character === "\\") { if (source[offset] === "u") offset += 5; else offset += 1; }
      else if ((character?.charCodeAt(0) ?? 0) < 0x20) throw new SyntaxError(`invalid JSON string at ${start}`);
    }
    throw new SyntaxError(`unterminated JSON string at ${start}`);
  };
  const value = (depth: number): void => {
    if (depth > MAX_DEPTH) throw new RangeError(`blueprint nesting exceeds ${MAX_DEPTH}`);
    whitespace(); const character = source[offset];
    if (character === '"') { string(); return; }
    if (character === "{") {
      offset += 1; whitespace(); const keys = new Set<string>(); if (source[offset] === "}") { offset += 1; return; }
      while (true) {
        whitespace(); const key = string(); if (keys.has(key)) throw new SyntaxError(`duplicate JSON object key ${key}`); keys.add(key);
        whitespace(); if (source[offset++] !== ":") throw new SyntaxError(`expected ':' after ${key}`); value(depth + 1); whitespace();
        if (source[offset] === "}") { offset += 1; return; } if (source[offset++] !== ",") throw new SyntaxError("expected ',' in object");
      }
    }
    if (character === "[") {
      offset += 1; whitespace(); if (source[offset] === "]") { offset += 1; return; }
      while (true) { value(depth + 1); whitespace(); if (source[offset] === "]") { offset += 1; return; } if (source[offset++] !== ",") throw new SyntaxError("expected ',' in array"); }
    }
    const match = /^(?:true|false|null|-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)/u.exec(source.slice(offset));
    if (match === null) throw new SyntaxError(`invalid JSON value at ${offset}`); offset += match[0].length;
  };
  value(0); whitespace(); if (offset !== source.length) throw new SyntaxError(`trailing JSON input at ${offset}`);
}
function hex(text: string, path: string): Uint8Array {
  if (!/^(?:[0-9a-fA-F]{2})*$/.test(text)) throw new TypeError(`${path} must be even-length base16`);
  return Uint8Array.from(text.match(/../g)?.map((value) => Number.parseInt(value, 16)) ?? []);
}
function hexText(value: Uint8Array): string { return Array.from(value, (byte) => byte.toString(16).padStart(2, "0")).join(""); }
function validatePurpose(value: unknown, path: string): Set<string> | undefined {
  if (value === undefined) return undefined;
  const values = typeof value === "string" ? [value] : plain(value) && Array.isArray(value.oneOf) ? value.oneOf : [];
  const valid = new Set(["spend", "mint", "withdraw", "publish"]);
  if (values.length === 0 || values.some((purpose) => typeof purpose !== "string" || !valid.has(purpose)) || new Set(values).size !== values.length) throw new TypeError(`${path} is invalid`);
  return new Set(values as string[]);
}
function integerKeyword(value: unknown, path: string, nonnegative = false): void {
  const parsed = bound(value); if (parsed === undefined || (nonnegative && parsed < 0n)) throw new TypeError(`${path} must be an exact${nonnegative ? " nonnegative" : ""} integer`);
}
function countKeyword(value: unknown, path: string): void {
  if (!Number.isSafeInteger(value) || (value as number) < 0) throw new TypeError(`${path} must be a nonnegative safe integer`);
}
function resolvePointer(reference: string, definitions: JsonObject, path: string): JsonObject {
  if (!reference.startsWith("#/definitions/")) throw new TypeError(`${path} must be a document-local #/definitions reference`);
  const parts = reference.slice(2).split("/").map((part) => part.replaceAll("~1", "/").replaceAll("~0", "~"));
  let current: unknown = { definitions };
  for (const part of parts) { if (!plain(current) || !Object.hasOwn(current, part)) throw new TypeError(`${path} does not resolve`); current = current[part]; }
  if (!plain(current)) throw new TypeError(`${path} must resolve to a schema object`); return current;
}
function validateSchema(schema: unknown, path: string, parameter: boolean, definitions: JsonObject,
                        seen = new Set<JsonObject>()): void {
  if (!plain(schema)) throw new TypeError(`${path} must be a schema object`);
  if (seen.has(schema)) return; seen.add(schema);
  if (schema.$ref !== undefined) { assertString(schema.$ref, `${path}/$ref`); validateSchema(resolvePointer(schema.$ref, definitions, `${path}/$ref`), path, parameter, definitions, seen); }
  let applicators = 0;
  for (const keyword of ["allOf", "anyOf", "oneOf"] as const) if (schema[keyword] !== undefined) {
    applicators += 1; const branches = schema[keyword]; if (!Array.isArray(branches) || branches.length === 0) throw new TypeError(`${path}/${keyword} must be a nonempty schema array`);
    branches.forEach((branch, index) => validateSchema(branch, pointer(pointer(path, keyword), index), parameter, definitions, seen));
  }
  if (schema.not !== undefined) { applicators += 1; validateSchema(schema.not, `${path}/not`, parameter, definitions, seen); }
  if (applicators > 1) throw new TypeError(`${path} must contain at most one applicator`);
  if (schema.title !== undefined) assertString(schema.title, `${path}/title`);
  if (schema.description !== undefined) assertString(schema.description, `${path}/description`);
  const type = schema.dataType;
  if (type === undefined) return;
  const dataTypes = new Set(["integer", "bytes", "list", "map", "constructor"]), builtinTypes = new Set(["#unit", "#boolean", "#integer", "#bytes", "#string", "#pair", "#list"]);
  if (typeof type !== "string" || (!dataTypes.has(type) && !(parameter && builtinTypes.has(type)))) throw new TypeError(`${path}/dataType is unsupported`);
  for (const keyword of ["minimum", "maximum", "exclusiveMinimum", "exclusiveMaximum"] as const) if (schema[keyword] !== undefined) integerKeyword(schema[keyword], `${path}/${keyword}`);
  if (schema.multipleOf !== undefined) { integerKeyword(schema.multipleOf, `${path}/multipleOf`); if ((bound(schema.multipleOf) ?? 0n) <= 0n) throw new TypeError(`${path}/multipleOf must be positive`); }
  for (const keyword of ["minLength", "maxLength", "minItems", "maxItems"] as const) if (schema[keyword] !== undefined) countKeyword(schema[keyword], `${path}/${keyword}`);
  if (schema.uniqueItems !== undefined && typeof schema.uniqueItems !== "boolean") throw new TypeError(`${path}/uniqueItems must be boolean`);
  if (schema.enum !== undefined) { if (!Array.isArray(schema.enum) || schema.enum.some((item) => typeof item !== "string" || !/^(?:[0-9a-fA-F]{2})*$/u.test(item))) throw new TypeError(`${path}/enum must contain base16 byte strings`); }
  if (type === "list") {
    if (schema.items === undefined) throw new TypeError(`${path}/items is required`);
    if (Array.isArray(schema.items)) schema.items.forEach((item, index) => validateSchema(item, pointer(`${path}/items`, index), parameter, definitions, seen));
    else validateSchema(schema.items, `${path}/items`, parameter, definitions, seen);
  }
  if (type === "#list") { if (schema.items === undefined) throw new TypeError(`${path}/items is required`); validateSchema(schema.items, `${path}/items`, false, definitions, seen); }
  if (type === "map") { if (schema.keys === undefined || schema.values === undefined) throw new TypeError(`${path} requires keys and values`); validateSchema(schema.keys, `${path}/keys`, false, definitions, seen); validateSchema(schema.values, `${path}/values`, false, definitions, seen); }
  if (type === "constructor") { integerKeyword(schema.index, `${path}/index`, true); if (!Array.isArray(schema.fields)) throw new TypeError(`${path}/fields is required`); schema.fields.forEach((item, index) => validateSchema(item, pointer(`${path}/fields`, index), false, definitions, seen)); }
  if (type === "#pair") { if (schema.left === undefined || schema.right === undefined) throw new TypeError(`${path} requires left and right`); validateSchema(schema.left, `${path}/left`, false, definitions, seen); validateSchema(schema.right, `${path}/right`, false, definitions, seen); }
}
function validateArgument(value: unknown, path: string, parameter: boolean, definitions: JsonObject): Set<string> | undefined {
  if (!plain(value) || !plain(value.schema)) throw new TypeError(`${path}/schema is required`);
  if (value.title !== undefined) assertString(value.title, `${path}/title`);
  if (value.description !== undefined) assertString(value.description, `${path}/description`);
  const purposes = validatePurpose(value.purpose, `${path}/purpose`);
  validateSchema(value.schema, `${path}/schema`, parameter, definitions);
  return purposes;
}
function validateDocument(value: unknown): PlutusBlueprint {
  if (!plain(value)) throw new TypeError("blueprint must be an object");
  if (value.$schema !== undefined && value.$schema !== META_SCHEMA) throw new TypeError("unsupported blueprint $schema");
  if (plain(value.$vocabulary)) for (const [name, required] of Object.entries(value.$vocabulary)) if (required === true && !name.includes("cip57")) throw new TypeError(`unsupported required blueprint vocabulary ${name}`);
  if (!plain(value.preamble)) throw new TypeError("/preamble is required");
  for (const key of Object.keys(value.preamble)) if (!["title", "description", "version", "plutusVersion", "compiler", "license"].includes(key)) throw new TypeError(`/preamble/${key} is not allowed`);
  for (const key of ["title", "version"] as const) assertString(value.preamble[key], `/preamble/${key}`);
  if (!(["v1", "v2", "v3"] as unknown[]).includes(value.preamble.plutusVersion)) throw new TypeError("/preamble/plutusVersion must be v1, v2, or v3");
  for (const key of ["description", "license"] as const) if (value.preamble[key] !== undefined) assertString(value.preamble[key], `/preamble/${key}`);
  if (value.preamble.compiler !== undefined) { if (!plain(value.preamble.compiler)) throw new TypeError("/preamble/compiler must be an object"); for (const key of Object.keys(value.preamble.compiler)) if (!["name", "version"].includes(key)) throw new TypeError(`/preamble/compiler/${key} is not allowed`); assertString(value.preamble.compiler.name, "/preamble/compiler/name"); if (value.preamble.compiler.version !== undefined) assertString(value.preamble.compiler.version, "/preamble/compiler/version"); }
  if (!Array.isArray(value.validators)) throw new TypeError("/validators is required");
  if (value.validators.length > 10_000) throw new RangeError("/validators exceeds 10000 entries");
  const definitions = value.definitions === undefined ? Object.create(null) as JsonObject : value.definitions;
  if (!plain(definitions)) throw new TypeError("/definitions must be an object");
  if (Object.keys(definitions).length + value.validators.length > 10_000) throw new RangeError("validators plus definitions exceed 10000 entries");
  for (const [name, schema] of Object.entries(definitions)) validateSchema(schema, pointer("/definitions", name), true, definitions);
  for (const [index, validator] of value.validators.entries()) {
    const path = `/validators/${index}`;
    if (!plain(validator)) throw new TypeError(`${path} must be an object`);
    assertString(validator.title, `${path}/title`); if (validator.description !== undefined) assertString(validator.description, `${path}/description`);
    const purposeSets: Set<string>[] = []; const redeemerPurposes = validateArgument(validator.redeemer, `${path}/redeemer`, false, definitions); if (redeemerPurposes) purposeSets.push(redeemerPurposes);
    if (validator.datum !== undefined) { const purposes = validateArgument(validator.datum, `${path}/datum`, false, definitions); if (purposes) purposeSets.push(purposes); }
    if (validator.parameters !== undefined) {
      if (!Array.isArray(validator.parameters)) throw new TypeError(`${path}/parameters must be an array`);
      validator.parameters.forEach((parameter, item) => { const purposes = validateArgument(parameter, `${path}/parameters/${item}`, true, definitions); if (purposes) purposeSets.push(purposes); });
    }
    for (let left = 0; left < purposeSets.length; left += 1) for (let right = left + 1; right < purposeSets.length; right += 1) if ([...purposeSets[left]!].some((purpose) => purposeSets[right]!.has(purpose))) throw new TypeError(`${path} argument purpose alternatives overlap`);
    if ((validator.compiledCode === undefined) !== (validator.hash === undefined)) {
      if (validator.compiledCode === undefined) { assertString(validator.hash, `${path}/hash`); if (!/^[0-9a-f]{56}$/.test(validator.hash)) throw new TypeError(`${path}/hash must be 56 lowercase hex characters`); }
      else throw new TypeError(`${path}/compiledCode requires hash`);
    }
    if (validator.compiledCode !== undefined) {
      assertString(validator.compiledCode, `${path}/compiledCode`); assertString(validator.hash, `${path}/hash`);
      const code = hex(validator.compiledCode, `${path}/compiledCode`); SerializedPlutusScript.from_single_cbor(code);
      const prefix = value.preamble.plutusVersion === "v1" ? 1 : value.preamble.plutusVersion === "v2" ? 2 : 3;
      const payload = new Uint8Array(code.length + 1); payload[0] = prefix; payload.set(code, 1);
      if (validator.hash !== hexText(blake2b224(payload))) throw new TypeError(`${path}/hash does not match compiledCode`);
    }
  }
  const source = freezeClone(value) as Readonly<Record<string, unknown>>;
  const frozenDefinitions = (source["definitions"] ?? Object.freeze(Object.create(null))) as Readonly<Record<string, unknown>>;
  const registerSchemas = (item: unknown, seen = new Set<object>()): void => { if (!plain(item) || seen.has(item)) return; seen.add(item); schemaDefinitions.set(item, frozenDefinitions); for (const child of Object.values(item)) if (Array.isArray(child)) child.forEach((entry) => registerSchemas(entry, seen)); else registerSchemas(child, seen); };
  registerSchemas(source);
  const output: PlutusBlueprint = { preamble: source["preamble"] as Readonly<Record<string, unknown>>, validators: source["validators"] as readonly Readonly<Record<string, unknown>>[], source };
  if (source["definitions"] !== undefined) (output as { definitions: Readonly<Record<string, unknown>> }).definitions = source["definitions"] as Readonly<Record<string, unknown>>;
  return Object.freeze(output);
}
export function parse_plutus_blueprint(json: string): PlutusBlueprint {
  if (new TextEncoder().encode(json).length > MAX_INPUT) throw new RangeError(`blueprint JSON exceeds ${MAX_INPUT} bytes`);
  assertNoDuplicateJsonKeys(json);
  return validateDocument(JSON.parse(json));
}
export function validate_plutus_blueprint(value: unknown): PlutusBlueprint { return validateDocument(value); }

function bound(value: unknown): bigint | undefined {
  if (typeof value === "number" && Number.isSafeInteger(value)) return BigInt(value);
  if (typeof value === "string" && /^(?:0|-?[1-9][0-9]*)$/.test(value) && value !== "-0") return BigInt(value);
  return undefined;
}
function canonical(value: PlutusData): string { return value.to_canonical_cbor_hex(); }
export function validate_blueprint_value(schema: unknown, value: unknown): readonly BlueprintViolation[] {
  const violations: BlueprintViolation[] = [], budget = { nodes: 0 }, schemaIds = new WeakMap<object, number>(); let nextSchemaId = 0;
  const definitions = plain(schema) ? schemaDefinitions.get(schema) ?? (plain(schema.definitions) ? schema.definitions : Object.freeze(Object.create(null)) as Readonly<Record<string, unknown>>) : Object.freeze(Object.create(null)) as Readonly<Record<string, unknown>>;
  const add = (quiet: boolean, code: string, path: string, message: string): void => { if (!quiet) violations.push(Object.freeze({ code, path, message })); };
  const asData = (candidate: unknown): PlutusData | undefined => {
    if (candidate instanceof PlutusData) return candidate;
    if (plain(candidate) && plain(candidate.type) && candidate.type.kind === "data") {
      try { return PlutusData.from_cbor_bytes(encodeCbor(candidate.value as CborValue)); } catch { return undefined; }
    }
    return undefined;
  };
  const visit = (rule: unknown, candidate: unknown, path: string, depth: number, active: Set<string>, quiet = false): boolean => {
    if (depth > MAX_DEPTH || ++budget.nodes > MAX_NODES) { add(quiet, "LIMIT", path, "blueprint validation limit exceeded"); return false; }
    if (!plain(rule)) { add(quiet, "SCHEMA", path, "schema must be an object"); return false; }
    let schemaId = schemaIds.get(rule); if (schemaId === undefined) { schemaId = nextSchemaId++; schemaIds.set(rule, schemaId); }
    const data = asData(candidate), identity = data ? canonical(data) : JSON.stringify(candidate, (_key, item) => typeof item === "bigint" ? `${item}n` : item);
    const state = `${schemaId}:${identity}`; if (active.has(state)) { add(quiet, "REFERENCE_CYCLE", path, "schema reference did not consume a value node"); return false; }
    const nextActive = new Set(active); nextActive.add(state); const start = violations.length;
    if (rule.$ref !== undefined) {
      if (typeof rule.$ref !== "string" || !rule.$ref.startsWith("#/definitions/")) { add(quiet, "REFERENCE", path, "only local definitions references are supported"); return false; }
      const parts = rule.$ref.slice(2).split("/").map((part) => part.replaceAll("~1", "/").replaceAll("~0", "~")); let target: unknown = { definitions };
      for (const part of parts) target = plain(target) ? target[part] : undefined;
      if (!plain(target)) { add(quiet, "REFERENCE", path, "local schema reference does not resolve"); return false; }
      return visit(target, candidate, path, depth + 1, nextActive, quiet);
    }
    if (rule.allOf !== undefined) {
      if (!Array.isArray(rule.allOf) || rule.allOf.length === 0) { add(quiet, "SCHEMA", path, "allOf must be nonempty"); return false; }
      let matched = true; for (const branch of rule.allOf) matched = visit(branch, candidate, path, depth + 1, nextActive, quiet) && matched; return matched;
    }
    for (const keyword of ["anyOf", "oneOf"] as const) if (rule[keyword] !== undefined) {
      const branches = rule[keyword]; if (!Array.isArray(branches) || branches.length === 0) { add(quiet, "SCHEMA", path, `${keyword} must be nonempty`); return false; }
      const success = branches.filter((branch) => visit(branch, candidate, path, depth + 1, nextActive, true)).length;
      const matched = keyword === "anyOf" ? success > 0 : success === 1; if (!matched) add(quiet, keyword.toUpperCase(), path, `${keyword} did not match`); return matched;
    }
    if (rule.not !== undefined) { const matched = visit(rule.not, candidate, path, depth + 1, nextActive, true); if (matched) add(quiet, "NOT", path, "not schema matched"); return !matched; }
    const type = rule.dataType; if (type === undefined) return true;
    if (typeof type === "string" && type.startsWith("#")) {
      if (!plain(candidate) || !plain(candidate.type) || typeof candidate.type.kind !== "string") { add(quiet, "TYPE", path, "builtin schema requires a UPLC constant"); return false; }
      const constant = candidate as unknown as UplcConstant, expected = type.slice(1); if (constant.type.kind !== expected) { add(quiet, "TYPE", path, `expected ${type} constant`); return false; }
      if (type === "#pair") { const pair = constant.value; if (!Array.isArray(pair) || pair.length !== 2 || !plain(constant.type) || constant.type.kind !== "pair") { add(quiet, "TYPE", path, "pair constant must contain two values"); return false; } visit(rule.left, { type: constant.type.first, value: pair[0] }, pointer(path, 0), depth + 1, nextActive, quiet); visit(rule.right, { type: constant.type.second, value: pair[1] }, pointer(path, 1), depth + 1, nextActive, quiet); }
      if (type === "#list") { if (!Array.isArray(constant.value) || constant.type.kind !== "list") { add(quiet, "TYPE", path, "list constant must contain a list"); return false; } constant.value.forEach((item, index) => visit(rule.items, { type: constant.type.kind === "list" ? constant.type.item : { kind: "data" }, value: item }, pointer(path, index), depth + 1, nextActive, quiet)); }
      return quiet ? true : violations.length === start;
    }
    if (data === undefined) { add(quiet, "TYPE", path, "value must use the existing PlutusData binding"); return false; }
    if (type === "integer") {
      const integer = data.as_integer(); if (integer === undefined) { add(quiet, "TYPE", path, "expected integer Data"); return false; }
      const number = BigInt(integer.to_str()), minimum = bound(rule.minimum), maximum = bound(rule.maximum), exclusiveMinimum = bound(rule.exclusiveMinimum), exclusiveMaximum = bound(rule.exclusiveMaximum), multiple = bound(rule.multipleOf);
      if (minimum !== undefined && number < minimum) add(quiet, "MINIMUM", path, "integer is below minimum");
      if (maximum !== undefined && number > maximum) add(quiet, "MAXIMUM", path, "integer is above maximum");
      if (exclusiveMinimum !== undefined && number <= exclusiveMinimum) add(quiet, "EXCLUSIVE_MINIMUM", path, "integer is not above exclusive minimum");
      if (exclusiveMaximum !== undefined && number >= exclusiveMaximum) add(quiet, "EXCLUSIVE_MAXIMUM", path, "integer is not below exclusive maximum");
      if (multiple !== undefined && (multiple <= 0n || number % multiple !== 0n)) add(quiet, "MULTIPLE_OF", path, "integer is not a valid multiple");
    } else if (type === "bytes") {
      const bytes = data.as_bytes(); if (bytes === undefined) { add(quiet, "TYPE", path, "expected bytes Data"); return false; }
      if (typeof rule.minLength === "number" && bytes.length < rule.minLength) add(quiet, "MIN_LENGTH", path, "byte string is too short");
      if (typeof rule.maxLength === "number" && bytes.length > rule.maxLength) add(quiet, "MAX_LENGTH", path, "byte string is too long");
      if (Array.isArray(rule.enum) && !rule.enum.some((item) => typeof item === "string" && item.toLowerCase() === hexText(bytes))) add(quiet, "ENUM", path, "byte string is outside enum");
    } else if (type === "list") {
      const list = data.as_list(); if (list === undefined) { add(quiet, "TYPE", path, "expected list Data"); return false; }
      if (typeof rule.minItems === "number" && list.len() < rule.minItems) add(quiet, "MIN_ITEMS", path, "list is too short");
      if (typeof rule.maxItems === "number" && list.len() > rule.maxItems) add(quiet, "MAX_ITEMS", path, "list is too long");
      if (rule.uniqueItems === true && new Set(list.values().map(canonical)).size !== list.len()) add(quiet, "UNIQUE_ITEMS", path, "list items are not unique");
      if (Array.isArray(rule.items)) rule.items.forEach((item, index) => { if (index < list.len()) visit(item, list.get(index), pointer(path, index), depth + 1, nextActive, quiet); });
      else if (plain(rule.items)) list.values().forEach((item, index) => visit(rule.items, item, pointer(path, index), depth + 1, nextActive, quiet));
    } else if (type === "map") {
      const map = data.as_map(); if (map === undefined) { add(quiet, "TYPE", path, "expected map Data"); return false; } const entries = map.entries();
      if (typeof rule.minItems === "number" && entries.length < rule.minItems) add(quiet, "MIN_ITEMS", path, "map is too small"); if (typeof rule.maxItems === "number" && entries.length > rule.maxItems) add(quiet, "MAX_ITEMS", path, "map is too large");
      entries.forEach(([key, item], index) => { if (rule.keys !== undefined) visit(rule.keys, key, pointer(path, `${index}/key`), depth + 1, nextActive, quiet); if (rule.values !== undefined) visit(rule.values, item, pointer(path, index), depth + 1, nextActive, quiet); });
    } else if (type === "constructor") {
      const constructor = data.as_constr_plutus_data(); if (constructor === undefined) { add(quiet, "TYPE", path, "expected constructor Data"); return false; }
      const index = bound(rule.index); if (index === undefined || index < 0n) add(quiet, "SCHEMA", path, "constructor index is required"); else if (constructor.alternative() !== index) add(quiet, "INDEX", path, "constructor index differs");
      if (Array.isArray(rule.fields)) { if (constructor.fields().len() !== rule.fields.length) add(quiet, "FIELDS", path, "constructor field count differs"); rule.fields.forEach((item, position) => { if (position < constructor.fields().len()) visit(item, constructor.fields().get(position), pointer(path, position), depth + 1, nextActive, quiet); }); }
    } else add(quiet, "SCHEMA", path, `unsupported dataType ${String(type)}`);
    return quiet ? violations.length === start : violations.length === start;
  };
  visit(schema, value, "", 0, new Set()); return Object.freeze(violations);
}
