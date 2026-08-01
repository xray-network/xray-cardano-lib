import { INT64_MAX } from "@xray-network/xray-cardano-lib-core";
import {
  COST_MODEL_PARAMETER_NAMES,
  DEFAULT_BUILTIN_MODELS,
} from "./cost-model-data.js";

export type SemanticsVariant = "A" | "B" | "C" | "D" | "E";
export type CostStream = readonly bigint[];

export interface BuiltinCostModel {
  readonly entries: Readonly<Record<string, BuiltinCostEntry>>;
}

interface BuiltinCostEntry {
  readonly cpu: CostingModel;
  readonly memory: CostingModel;
}

interface CostingModel {
  readonly type: string;
  readonly arguments: unknown;
}

const BUILTIN_NAMES = Object.freeze([
  "addInteger", "subtractInteger", "multiplyInteger", "divideInteger",
  "quotientInteger", "remainderInteger", "modInteger", "equalsInteger",
  "lessThanInteger", "lessThanEqualsInteger", "appendByteString", "consByteString",
  "sliceByteString", "lengthOfByteString", "indexByteString", "equalsByteString",
  "lessThanByteString", "lessThanEqualsByteString", "sha2_256", "sha3_256",
  "blake2b_256", "verifyEd25519Signature", "appendString", "equalsString",
  "encodeUtf8", "decodeUtf8", "ifThenElse", "chooseUnit", "trace", "fstPair",
  "sndPair", "chooseList", "mkCons", "headList", "tailList", "nullList",
  "chooseData", "constrData", "mapData", "listData", "iData", "bData",
  "unConstrData", "unMapData", "unListData", "unIData", "unBData", "equalsData",
  "mkPairData", "mkNilData", "mkNilPairData", "serialiseData",
  "verifyEcdsaSecp256k1Signature", "verifySchnorrSecp256k1Signature",
  "bls12_381_G1_add", "bls12_381_G1_neg", "bls12_381_G1_scalarMul",
  "bls12_381_G1_equal", "bls12_381_G1_hashToGroup", "bls12_381_G1_compress",
  "bls12_381_G1_uncompress", "bls12_381_G2_add", "bls12_381_G2_neg",
  "bls12_381_G2_scalarMul", "bls12_381_G2_equal", "bls12_381_G2_hashToGroup",
  "bls12_381_G2_compress", "bls12_381_G2_uncompress", "bls12_381_millerLoop",
  "bls12_381_mulMlResult", "bls12_381_finalVerify", "keccak_256", "blake2b_224",
  "integerToByteString", "byteStringToInteger", "andByteString", "orByteString",
  "xorByteString", "complementByteString", "readBit", "writeBits", "replicateByte",
  "shiftByteString", "rotateByteString", "countSetBits", "findFirstSetBit",
  "ripemd_160", "expModInteger", "dropList", "lengthOfArray", "listToArray",
  "indexArray", "bls12_381_G1_multiScalarMul", "bls12_381_G2_multiScalarMul",
  "insertCoin", "lookupCoin", "unionValue", "valueContains", "valueData",
  "unValueData", "scaleValue",
] as const);

export function makeBuiltinCostModel(
  language: 0 | 1 | 2,
  parameters: readonly bigint[],
  variant: SemanticsVariant,
): BuiltinCostModel {
  const source = DEFAULT_BUILTIN_MODELS[variant];
  if (!isRecord(source)) throw new TypeError(`missing builtin cost model ${variant}`);
  const entries = clone(source) as Record<string, BuiltinCostEntry>;
  const names = COST_MODEL_PARAMETER_NAMES[language];
  if (names === undefined) throw new RangeError(`invalid Plutus language ${language}`);
  for (let index = 0; index < names.length; index += 1) {
    setParameter(entries, names[index] as string, parameters[index] ?? INT64_MAX);
  }
  return Object.freeze({ entries: deepFreeze(entries) });
}

export function defaultBuiltinCostModel(variant: SemanticsVariant): BuiltinCostModel {
  const source = DEFAULT_BUILTIN_MODELS[variant];
  if (!isRecord(source)) throw new TypeError(`missing builtin cost model ${variant}`);
  return Object.freeze({
    entries: deepFreeze(clone(source) as Record<string, BuiltinCostEntry>),
  });
}

export function builtinCost(
  tag: number,
  streams: readonly CostStream[],
  model: BuiltinCostModel,
): { readonly cpu: bigint; readonly memory: bigint } {
  const name = BUILTIN_NAMES[tag];
  if (name === undefined) throw new RangeError(`invalid builtin tag ${tag}`);
  const entry = model.entries[name];
  if (entry === undefined) throw new TypeError(`missing cost model for ${name}`);
  return {
    cpu: evaluateModel(entry.cpu, streams),
    memory: evaluateModel(entry.memory, streams),
  };
}

export function builtinTag(name: string): number | undefined {
  const index = BUILTIN_NAMES.indexOf(name as typeof BUILTIN_NAMES[number]);
  return index < 0 ? undefined : index;
}

function evaluateModel(model: CostingModel, streams: readonly CostStream[]): bigint {
  const sizes = streams.map(sum);
  const x = sizes[0] ?? 0n;
  const y = sizes[1] ?? 0n;
  const z = sizes[2] ?? 0n;
  const u = sizes[3] ?? 0n;
  const args = model.arguments;
  let result: bigint;
  switch (model.type) {
    case "constant_cost":
      result = numeric(args);
      break;
    case "linear_in_x": result = linear(args, x); break;
    case "linear_in_y": result = linear(args, y); break;
    case "linear_in_y2": result = linear(args, y); break;
    case "linear_in_z": result = linear(args, z); break;
    case "linear_in_u": result = linear(args, u); break;
    case "quadratic_in_x": result = quadratic(args, x); break;
    case "quadratic_in_y": result = quadratic(args, y); break;
    case "quadratic_in_z": result = quadratic(args, z); break;
    case "added_sizes": result = linear(args, x + y); break;
    case "multiplied_sizes": result = linear(args, x * y); break;
    case "min_size": result = linear(args, x < y ? x : y); break;
    case "max_size": result = linear(args, x > y ? x : y); break;
    case "subtracted_sizes": {
      const record = asRecord(args);
      const minimum = field(record, "minimum");
      const difference = x - y;
      result = field(record, "intercept") + field(record, "slope") *
        (difference > minimum ? difference : minimum);
      break;
    }
    case "linear_in_x_and_y": result = linearTwo(args, x, y); break;
    case "linear_in_y_and_z": result = linearTwo(args, y, z); break;
    case "linear_in_max_yz": result = linear(args, y > z ? y : z); break;
    case "linear_on_diagonal": {
      const record = asRecord(args);
      result = x === y
        ? field(record, "intercept") + field(record, "slope") * x
        : field(record, "constant");
      break;
    }
    case "const_above_diagonal": {
      const record = asRecord(args);
      result = x < y
        ? field(record, "constant")
        : evaluateNested(record["model"], x, y);
      break;
    }
    case "above_and_below_diagonal": {
      const record = asRecord(args);
      result = evaluateNested(record["model"], x > y ? x : y, x > y ? y : x);
      break;
    }
    case "quadratic_in_x_and_y": result = quadraticTwo(args, x, y); break;
    case "with_interaction_in_x_and_y": {
      const record = asRecord(args);
      result = field(record, "c00") + field(record, "c10") * x +
        field(record, "c01") * y + field(record, "c11") * x * y;
      break;
    }
    case "literal_in_y_or_linear_in_z":
      result = y === 0n ? linear(args, z) : y;
      break;
    case "exp_mod_cost": {
      const record = asRecord(args);
      const base = field(record, "coefficient00") +
        field(record, "coefficient11") * y * z +
        field(record, "coefficient12") * y * z * z;
      result = x <= z ? base : base + base / 2n;
      break;
    }
    default:
      throw new TypeError(`unsupported costing model ${model.type}`);
  }
  return clampCost(result);
}

function evaluateNested(value: unknown, x: bigint, y: bigint): bigint {
  if (!isRecord(value) || typeof value["type"] !== "string") throw new TypeError("invalid nested cost model");
  return evaluateModel(value as unknown as CostingModel, [[x], [y]]);
}

function linear(value: unknown, argument: bigint): bigint {
  const record = asRecord(value);
  return field(record, "intercept") + field(record, "slope") * argument;
}

function linearTwo(value: unknown, left: bigint, right: bigint): bigint {
  const record = asRecord(value);
  return field(record, "intercept") +
    field(record, "slope1") * left +
    field(record, "slope2") * right;
}

function quadratic(value: unknown, argument: bigint): bigint {
  const record = asRecord(value);
  return field(record, "c0") + field(record, "c1") * argument +
    field(record, "c2") * argument * argument;
}

function quadraticTwo(value: unknown, left: bigint, right: bigint): bigint {
  const record = asRecord(value);
  const polynomial = field(record, "c00") +
    field(record, "c10") * left +
    field(record, "c01") * right +
    field(record, "c20") * left * left +
    field(record, "c11") * left * right +
    field(record, "c02") * right * right;
  const minimum = field(record, "minimum");
  return polynomial > minimum ? polynomial : minimum;
}

function setParameter(
  entries: Record<string, BuiltinCostEntry>,
  name: string,
  value: bigint,
): void {
  const match = /^(.*)-(cpu|memory)-arguments(?:-(.*))?$/.exec(name);
  if (match === null) return;
  const builtin = entries[match[1] as string];
  if (builtin === undefined) return;
  const metric = builtin[match[2] as "cpu" | "memory"];
  if (match[3] === undefined) {
    (metric as { arguments: unknown }).arguments = value;
    return;
  }
  let target = metric.arguments;
  const path = match[3].split("-");
  for (let index = 0; index < path.length - 1; index += 1) {
    if (!isRecord(target)) return;
    target = target[path[index] as string];
  }
  if (!isRecord(target)) return;
  const leaf = path[path.length - 1] as string;
  if (typeof target[leaf] === "number" || typeof target[leaf] === "bigint") target[leaf] = value;
}

function sum(stream: CostStream): bigint {
  return stream.reduce((total, value) => total + value, 0n);
}

function clampCost(value: bigint): bigint {
  if (value < 0n) return 0n;
  return value > INT64_MAX ? INT64_MAX : value;
}

function numeric(value: unknown): bigint {
  if (typeof value === "bigint") return value;
  if (typeof value === "number" && Number.isSafeInteger(value)) return BigInt(value);
  throw new TypeError("cost parameter is not an integer");
}

function field(value: Record<string, unknown>, name: string): bigint {
  return numeric(value[name]);
}

function asRecord(value: unknown): Record<string, unknown> {
  if (!isRecord(value)) throw new TypeError("invalid cost model arguments");
  return value;
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

function clone(value: unknown): unknown {
  if (Array.isArray(value)) return value.map(clone);
  if (isRecord(value)) {
    return Object.fromEntries(Object.entries(value).map(([key, item]) => [key, clone(item)]));
  }
  return value;
}

function deepFreeze<T>(value: T): T {
  if (typeof value !== "object" || value === null || Object.isFrozen(value)) return value;
  for (const item of Object.values(value)) deepFreeze(item);
  return Object.freeze(value);
}
