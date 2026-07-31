import {
  CardanoError,
  INT64_MAX,
  bytesEqual,
  type CborValue,
} from "@xray-network/cardano-core";
import {
  blake2b224,
  blake2b256,
  bls12_381_add,
  bls12_381_compress,
  bls12_381_equal,
  bls12_381_final_verify,
  bls12_381_hash_to_group,
  bls12_381_miller_loop,
  bls12_381_mul_ml_result,
  bls12_381_neg,
  bls12_381_scalar_mul,
  bls12_381_uncompress,
  keccak_256,
  ripemd_160,
  sha2_256,
  sha3_256,
  verifyEd25519Uplc,
  verifySecp256k1EcdsaUplc,
  verifySecp256k1SchnorrUplc,
} from "@xray-network/cardano-crypto";
import type { UplcConstant, UplcProgram, UplcTerm, UplcType } from "./ast.js";
import {
  builtinCost,
  defaultBuiltinCostModel,
  makeBuiltinCostModel,
  type BuiltinCostModel,
  type CostStream,
  type SemanticsVariant,
} from "./cost-model.js";
import { encodePlutusData } from "./flat.js";

type Environment = readonly Value[];
type Value =
  | { readonly kind: "constant"; readonly constant: UplcConstant }
  | { readonly kind: "closure"; readonly body: UplcTerm; readonly environment: Environment }
  | { readonly kind: "delay"; readonly term: UplcTerm; readonly environment: Environment }
  | { readonly kind: "builtin"; readonly tag: number; readonly forces: number; readonly arguments: readonly Value[] }
  | { readonly kind: "constr"; readonly tag: bigint; readonly fields: readonly Value[] };

export interface MachineBudget {
  readonly cpu: bigint;
  readonly memory: bigint;
}

export interface MachineResult {
  readonly budget: MachineBudget;
  readonly logs: readonly string[];
  readonly value: UplcTerm;
  readonly isUnit: boolean;
}

export interface MachineCosts {
  readonly apply: MachineBudget;
  readonly builtin: MachineBudget;
  readonly constant: MachineBudget;
  readonly delay: MachineBudget;
  readonly force: MachineBudget;
  readonly lambda: MachineBudget;
  readonly startup: MachineBudget;
  readonly variable: MachineBudget;
  readonly constr: MachineBudget;
  readonly case_: MachineBudget;
  readonly builtinModel?: BuiltinCostModel;
  readonly costModel?: {
    readonly language: 0 | 1 | 2;
    readonly parameters: readonly bigint[];
  };
}

interface Meter {
  cpu: bigint;
  memory: bigint;
  readonly maximum: MachineBudget;
}

const textDecoder = new TextDecoder(undefined, { fatal: true });
const textEncoder = new TextEncoder();
const MAX_STEPS = 10_000_000;
const costModelCache = new WeakMap<object, Map<SemanticsVariant, BuiltinCostModel>>();
const DEFAULT_BUILTIN_MODEL = defaultBuiltinCostModel("E");

const BUILTIN_ARITY = [
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 3, 1, 2, 2, 2, 2, 1, 1, 1, 3,
  2, 2, 1, 1, 3, 2, 2, 1, 1, 3, 2, 1, 1, 1,
  6, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1,
  3, 3, 2, 1, 2, 2, 2, 1, 1, 2, 1, 2, 2, 2, 1, 1,
  2, 2, 2, 1, 1, 3, 2, 3, 3, 3, 1, 2, 3, 2, 2, 2,
  1, 1, 1, 3, 2, 1, 1, 2, 2, 2, 4, 3, 2, 2, 1, 1, 2,
] as const;

const BUILTIN_FORCES = [
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
] as const;

export function evaluateProgram(
  program: UplcProgram,
  arguments_: readonly CborValue[],
  maximum: MachineBudget,
  costs: MachineCosts = defaultMachineCosts(),
  semantics: "A" | "B" | "C" | "D" | "E" = "E",
): MachineResult {
  const meter: Meter = { cpu: 0n, memory: 0n, maximum };
  charge(meter, costs.startup.cpu, costs.startup.memory);
  const logs: string[] = [];
  let term: UplcTerm = program.term;
  for (const argument of arguments_) {
    term = {
      kind: "apply",
      function: term,
      argument: { kind: "constant", constant: { type: { kind: "data" }, value: argument } },
    };
  }
  assertClosed(term);
  const value = evaluate(term, [], meter, logs, { value: 0 }, costs, semantics);
  return {
    budget: { cpu: meter.cpu, memory: meter.memory },
    logs: Object.freeze([...logs]),
    value: dischargeValue(value),
    isUnit: value.kind === "constant" && value.constant.type.kind === "unit",
  };
}

function dischargeValue(value: Value): UplcTerm {
  type Task =
    | { readonly kind: "value"; readonly value: Value }
    | { readonly kind: "term"; readonly term: UplcTerm; readonly environment: Environment; readonly depth: bigint }
    | { readonly kind: "unary"; readonly wrapper: "delay" | "lambda" | "force" }
    | { readonly kind: "apply" }
    | { readonly kind: "constr"; readonly tag: bigint; readonly count: number }
    | { readonly kind: "case"; readonly branchCount: number }
    | { readonly kind: "builtin"; readonly tag: number; readonly forces: number; readonly count: number }
    | { readonly kind: "shift"; readonly amount: bigint };
  const tasks: Task[] = [{ kind: "value", value }];
  const output: UplcTerm[] = [];
  const pushTerms = (
    terms: readonly UplcTerm[],
    environment: Environment,
    depth: bigint,
  ): void => {
    for (let index = terms.length - 1; index >= 0; index -= 1) {
      tasks.push({ kind: "term", term: terms[index] as UplcTerm, environment, depth });
    }
  };
  const pushValues = (values: readonly Value[]): void => {
    for (let index = values.length - 1; index >= 0; index -= 1) {
      tasks.push({ kind: "value", value: values[index] as Value });
    }
  };

  while (tasks.length !== 0) {
    const task = tasks.pop() as Task;
    if (task.kind === "value") {
      const current = task.value;
      switch (current.kind) {
        case "constant":
          output.push({ kind: "constant", constant: current.constant });
          break;
        case "closure":
          tasks.push({ kind: "unary", wrapper: "lambda" });
          tasks.push({ kind: "term", term: current.body, environment: current.environment, depth: 1n });
          break;
        case "delay":
          tasks.push({ kind: "unary", wrapper: "delay" });
          tasks.push({ kind: "term", term: current.term, environment: current.environment, depth: 0n });
          break;
        case "constr":
          tasks.push({ kind: "constr", tag: current.tag, count: current.fields.length });
          pushValues(current.fields);
          break;
        case "builtin":
          tasks.push({
            kind: "builtin",
            tag: current.tag,
            forces: current.forces,
            count: current.arguments.length,
          });
          pushValues(current.arguments);
          break;
      }
      continue;
    }
    if (task.kind === "term") {
      const current = task.term;
      switch (current.kind) {
        case "var":
          if (current.index <= task.depth) output.push(current);
          else {
            const replacement = task.environment[Number(current.index - task.depth - 1n)];
            if (replacement === undefined) fail("free UPLC variable during discharge");
            tasks.push({ kind: "shift", amount: task.depth });
            tasks.push({ kind: "value", value: replacement });
          }
          break;
        case "delay":
        case "force":
          tasks.push({ kind: "unary", wrapper: current.kind });
          tasks.push({
            kind: "term",
            term: current.term,
            environment: task.environment,
            depth: task.depth,
          });
          break;
        case "lambda":
          tasks.push({ kind: "unary", wrapper: "lambda" });
          tasks.push({
            kind: "term",
            term: current.body,
            environment: task.environment,
            depth: task.depth + 1n,
          });
          break;
        case "apply":
          tasks.push({ kind: "apply" });
          tasks.push({
            kind: "term",
            term: current.argument,
            environment: task.environment,
            depth: task.depth,
          });
          tasks.push({
            kind: "term",
            term: current.function,
            environment: task.environment,
            depth: task.depth,
          });
          break;
        case "constr":
          tasks.push({ kind: "constr", tag: current.tag, count: current.fields.length });
          pushTerms(current.fields, task.environment, task.depth);
          break;
        case "case":
          tasks.push({ kind: "case", branchCount: current.branches.length });
          pushTerms(current.branches, task.environment, task.depth);
          tasks.push({
            kind: "term",
            term: current.scrutinee,
            environment: task.environment,
            depth: task.depth,
          });
          break;
        default:
          output.push(current);
          break;
      }
      continue;
    }
    switch (task.kind) {
      case "unary": {
        const child = output.pop() as UplcTerm;
        output.push(task.wrapper === "lambda"
          ? { kind: "lambda", body: child }
          : task.wrapper === "delay"
            ? { kind: "delay", term: child }
            : { kind: "force", term: child });
        break;
      }
      case "apply": {
        const argument = output.pop() as UplcTerm;
        const function_ = output.pop() as UplcTerm;
        output.push({ kind: "apply", function: function_, argument });
        break;
      }
      case "constr": {
        const fields = output.splice(output.length - task.count, task.count);
        output.push({ kind: "constr", tag: task.tag, fields });
        break;
      }
      case "case": {
        const values = output.splice(output.length - task.branchCount - 1, task.branchCount + 1);
        output.push({
          kind: "case",
          scrutinee: values[0] as UplcTerm,
          branches: values.slice(1),
        });
        break;
      }
      case "builtin": {
        const arguments_ = output.splice(output.length - task.count, task.count);
        let term: UplcTerm = { kind: "builtin", tag: task.tag };
        for (let count = 0; count < task.forces; count += 1) term = { kind: "force", term };
        for (const argument of arguments_) term = { kind: "apply", function: term, argument };
        output.push(term);
        break;
      }
      case "shift":
        output.push(shiftTerm(output.pop() as UplcTerm, task.amount));
        break;
      default:
        break;
    }
  }
  return output[0] as UplcTerm;
}

function shiftTerm(term: UplcTerm, amount: bigint, cutoff = 0n): UplcTerm {
  type Task =
    | { readonly kind: "term"; readonly term: UplcTerm; readonly cutoff: bigint }
    | { readonly kind: "unary"; readonly wrapper: "delay" | "lambda" | "force" }
    | { readonly kind: "apply" }
    | { readonly kind: "constr"; readonly tag: bigint; readonly count: number }
    | { readonly kind: "case"; readonly branchCount: number };
  const tasks: Task[] = [{ kind: "term", term, cutoff }];
  const output: UplcTerm[] = [];
  const pushTerms = (terms: readonly UplcTerm[], innerCutoff: bigint): void => {
    for (let index = terms.length - 1; index >= 0; index -= 1) {
      tasks.push({ kind: "term", term: terms[index] as UplcTerm, cutoff: innerCutoff });
    }
  };
  while (tasks.length !== 0) {
    const task = tasks.pop() as Task;
    if (task.kind === "term") {
      const current = task.term;
      switch (current.kind) {
        case "var":
          output.push(current.index > task.cutoff
            ? { kind: "var", index: current.index + amount }
            : current);
          break;
        case "delay":
        case "force":
          tasks.push({ kind: "unary", wrapper: current.kind });
          tasks.push({ kind: "term", term: current.term, cutoff: task.cutoff });
          break;
        case "lambda":
          tasks.push({ kind: "unary", wrapper: "lambda" });
          tasks.push({ kind: "term", term: current.body, cutoff: task.cutoff + 1n });
          break;
        case "apply":
          tasks.push({ kind: "apply" });
          tasks.push({ kind: "term", term: current.argument, cutoff: task.cutoff });
          tasks.push({ kind: "term", term: current.function, cutoff: task.cutoff });
          break;
        case "constr":
          tasks.push({ kind: "constr", tag: current.tag, count: current.fields.length });
          pushTerms(current.fields, task.cutoff);
          break;
        case "case":
          tasks.push({ kind: "case", branchCount: current.branches.length });
          pushTerms(current.branches, task.cutoff);
          tasks.push({ kind: "term", term: current.scrutinee, cutoff: task.cutoff });
          break;
        default:
          output.push(current);
          break;
      }
      continue;
    }
    if (task.kind === "unary") {
      const child = output.pop() as UplcTerm;
      output.push(task.wrapper === "lambda"
        ? { kind: "lambda", body: child }
        : task.wrapper === "delay"
          ? { kind: "delay", term: child }
          : { kind: "force", term: child });
    } else if (task.kind === "apply") {
      const argument = output.pop() as UplcTerm;
      const function_ = output.pop() as UplcTerm;
      output.push({ kind: "apply", function: function_, argument });
    } else if (task.kind === "constr") {
      output.push({
        kind: "constr",
        tag: task.tag,
        fields: output.splice(output.length - task.count, task.count),
      });
    } else {
      const values = output.splice(output.length - task.branchCount - 1, task.branchCount + 1);
      output.push({
        kind: "case",
        scrutinee: values[0] as UplcTerm,
        branches: values.slice(1),
      });
    }
  }
  return output[0] as UplcTerm;
}

function evaluate(
  term: UplcTerm,
  environment: Environment,
  meter: Meter,
  logs: string[],
  steps: { value: number },
  costs: MachineCosts,
  semantics: "A" | "B" | "C" | "D" | "E",
): Value {
  type Frame =
    | { readonly kind: "force" }
    | { readonly kind: "apply-function"; readonly argument: UplcTerm; readonly environment: Environment }
    | { readonly kind: "apply-argument"; readonly function_: Value }
    | {
      readonly kind: "constr";
      readonly tag: bigint;
      readonly terms: readonly UplcTerm[];
      readonly environment: Environment;
      readonly next: number;
      readonly values: readonly Value[];
    }
    | { readonly kind: "case"; readonly branches: readonly UplcTerm[]; readonly environment: Environment }
    | { readonly kind: "apply-fields"; readonly fields: readonly Value[]; readonly next: number };

  const frames: Frame[] = [];
  let currentTerm = term;
  let currentEnvironment = environment;
  let currentValue: Value | undefined;
  let computing = true;

  const enterApply = (function_: Value, argument: Value): void => {
    if (function_.kind === "closure") {
      currentTerm = function_.body;
      currentEnvironment = [argument, ...function_.environment];
      computing = true;
      return;
    }
    currentValue = applyBuiltin(function_, argument, meter, logs, costs, semantics);
    computing = false;
  };

  for (;;) {
    if (computing) {
      steps.value += 1;
      if (steps.value > MAX_STEPS) fail("UPLC step limit exceeded");
      const stepCost =
        currentTerm.kind === "apply" ? costs.apply
        : currentTerm.kind === "builtin" ? costs.builtin
        : currentTerm.kind === "constant" ? costs.constant
        : currentTerm.kind === "delay" ? costs.delay
        : currentTerm.kind === "force" ? costs.force
        : currentTerm.kind === "lambda" ? costs.lambda
        : currentTerm.kind === "var" ? costs.variable
        : currentTerm.kind === "constr" ? costs.constr
        : currentTerm.kind === "case" ? costs.case_
        : { cpu: 0n, memory: 0n };
      charge(meter, stepCost.cpu, stepCost.memory);
      switch (currentTerm.kind) {
        case "var":
          if (currentTerm.index < 1n || currentTerm.index > BigInt(currentEnvironment.length)) {
            fail("free UPLC variable");
          }
          currentValue = currentEnvironment[Number(currentTerm.index - 1n)] as Value;
          computing = false;
          break;
        case "delay":
          currentValue = { kind: "delay", term: currentTerm.term, environment: currentEnvironment };
          computing = false;
          break;
        case "lambda":
          currentValue = { kind: "closure", body: currentTerm.body, environment: currentEnvironment };
          computing = false;
          break;
        case "constant":
          currentValue = { kind: "constant", constant: currentTerm.constant };
          computing = false;
          break;
        case "builtin":
          if (currentTerm.tag < 0 || currentTerm.tag > 100) {
            fail(`unsupported builtin tag ${currentTerm.tag}`);
          }
          currentValue = { kind: "builtin", tag: currentTerm.tag, forces: 0, arguments: [] };
          computing = false;
          break;
        case "error":
          fail("explicit UPLC error");
        case "force":
          frames.push({ kind: "force" });
          currentTerm = currentTerm.term;
          break;
        case "apply":
          frames.push({
            kind: "apply-function",
            argument: currentTerm.argument,
            environment: currentEnvironment,
          });
          currentTerm = currentTerm.function;
          break;
        case "constr":
          if (currentTerm.fields.length === 0) {
            currentValue = { kind: "constr", tag: currentTerm.tag, fields: [] };
            computing = false;
          } else {
            frames.push({
              kind: "constr",
              tag: currentTerm.tag,
              terms: currentTerm.fields,
              environment: currentEnvironment,
              next: 1,
              values: [],
            });
            currentTerm = currentTerm.fields[0] as UplcTerm;
          }
          break;
        case "case":
          frames.push({
            kind: "case",
            branches: currentTerm.branches,
            environment: currentEnvironment,
          });
          currentTerm = currentTerm.scrutinee;
          break;
      }
      continue;
    }

    const value = currentValue as Value;
    const frame = frames.pop();
    if (frame === undefined) return value;
    switch (frame.kind) {
      case "force":
        if (value.kind === "delay") {
          currentTerm = value.term;
          currentEnvironment = value.environment;
          computing = true;
        } else if (value.kind === "builtin") {
          const expected = BUILTIN_FORCES[value.tag];
          if (expected === undefined || value.forces >= expected) fail("unexpected force");
          currentValue = { ...value, forces: value.forces + 1 };
        } else fail("attempted to force a non-delay");
        break;
      case "apply-function":
        frames.push({ kind: "apply-argument", function_: value });
        currentTerm = frame.argument;
        currentEnvironment = frame.environment;
        computing = true;
        break;
      case "apply-argument":
        enterApply(frame.function_, value);
        break;
      case "constr": {
        const values = [...frame.values, value];
        if (frame.next < frame.terms.length) {
          frames.push({ ...frame, next: frame.next + 1, values });
          currentTerm = frame.terms[frame.next] as UplcTerm;
          currentEnvironment = frame.environment;
          computing = true;
        } else {
          currentValue = { kind: "constr", tag: frame.tag, fields: values };
        }
        break;
      }
      case "case": {
        let tag: bigint;
        let fields: readonly Value[];
        if (value.kind === "constant") {
          ({ tag, fields } = selectConstantCase(value.constant, frame.branches));
        } else {
          if (
            value.kind !== "constr" ||
            value.tag < 0n ||
            value.tag >= BigInt(frame.branches.length)
          ) fail("case scrutinee does not select a branch");
          tag = value.tag;
          fields = value.fields;
        }
        if (fields.length !== 0) frames.push({ kind: "apply-fields", fields, next: 0 });
        currentTerm = frame.branches[Number(tag)] as UplcTerm;
        currentEnvironment = frame.environment;
        computing = true;
        break;
      }
      case "apply-fields":
        if (frame.next < frame.fields.length) {
          frames.push({ ...frame, next: frame.next + 1 });
          enterApply(value, frame.fields[frame.next] as Value);
        }
        break;
    }
  }
}

function selectConstantCase(
  constant_: UplcConstant,
  branches: readonly UplcTerm[],
): { readonly tag: bigint; readonly fields: readonly Value[] } {
  let tag: bigint;
  let fields: Value[] = [];
  switch (constant_.type.kind) {
    case "unit":
      if (branches.length !== 1) fail("unit case requires exactly one branch");
      tag = 0n;
      break;
    case "boolean":
      if (branches.length > 2) fail("boolean case accepts at most two branches");
      tag = constant_.value === true ? 1n : 0n;
      break;
    case "integer":
      tag = constant_.value as bigint;
      break;
    case "list": {
      const values = listValue(constant_);
      if (values.length === 0) {
        tag = 1n;
      } else {
        tag = 0n;
        fields = [
          con(constant_.type.item, values[0]),
          con(constant_.type, values.slice(1)),
        ];
      }
      break;
    }
    case "pair":
      if (branches.length !== 1) fail("pair case requires exactly one branch");
      tag = 0n;
      fields = [pairValue(constant_, 0), pairValue(constant_, 1)];
      break;
    default:
      fail("constant cannot be used as a case scrutinee");
  }
  if (tag < 0n || tag >= BigInt(branches.length)) fail("constant case does not select a branch");
  return { tag, fields };
}

function applyBuiltin(
  function_: Value,
  argument: Value,
  meter: Meter,
  logs: string[],
  costs: MachineCosts,
  semantics: "A" | "B" | "C" | "D" | "E",
): Value {
  if (function_.kind !== "builtin") fail("attempted to apply a non-function");
  const expectedForces = BUILTIN_FORCES[function_.tag];
  const arity = BUILTIN_ARITY[function_.tag];
  if (expectedForces === undefined || arity === undefined || function_.forces !== expectedForces) {
    fail("builtin was applied before instantiation");
  }
  const arguments_ = [...function_.arguments, argument];
  if (arguments_.length < arity) return { ...function_, arguments: arguments_ };
  const builtin = builtinBudget(function_.tag, arguments_, costs, semantics);
  charge(meter, builtin.cpu, builtin.memory);
  return runBuiltin(function_.tag, arguments_, logs, semantics);
}

function runBuiltin(
  tag: number,
  values: readonly Value[],
  logs: string[],
  semantics: "A" | "B" | "C" | "D" | "E",
): Value {
  const c = (index: number): UplcConstant => constant(values[index]);
  const i = (index: number): bigint => typed(c(index), "integer") as bigint;
  const b = (index: number): Uint8Array => typed(c(index), "bytes") as Uint8Array;
  const s = (index: number): string => typed(c(index), "string") as string;
  const bool = (index: number): boolean => typed(c(index), "boolean") as boolean;
  const data = (index: number): CborValue => typed(c(index), "data") as CborValue;
  const int = (value: bigint): Value => con({ kind: "integer" }, value);
  const bytes = (value: Uint8Array): Value => con({ kind: "bytes" }, Uint8Array.from(value));
  const boolean = (value: boolean): Value => con({ kind: "boolean" }, value);
  switch (tag) {
    case 0: return int(i(0) + i(1));
    case 1: return int(i(0) - i(1));
    case 2: return int(i(0) * i(1));
    case 3: return int(euclideanDiv(i(0), nonzero(i(1))));
    case 4: return int(i(0) / nonzero(i(1)));
    case 5: return int(i(0) % nonzero(i(1)));
    case 6: return int(euclideanMod(i(0), nonzero(i(1))));
    case 7: return boolean(i(0) === i(1));
    case 8: return boolean(i(0) < i(1));
    case 9: return boolean(i(0) <= i(1));
    case 10: return bytes(concat(b(0), b(1)));
    case 11: {
      const byte = i(0);
      if ((semantics === "C" || semantics === "E") && (byte < 0n || byte > 255n)) {
        fail("byte value is out of bounds");
      }
      return bytes(concat(Uint8Array.of(Number(byte & 0xffn)), b(1)));
    }
    case 12: {
      const source = b(2);
      const start = clampIndex(i(0), source.length);
      const length = i(1) <= 0n ? 0 : Number(i(1) > BigInt(source.length) ? source.length : i(1));
      return bytes(source.slice(start, start + length));
    }
    case 13: return int(BigInt(b(0).length));
    case 14: {
      const index = i(1);
      if (index < 0n || index >= BigInt(b(0).length)) fail("byte string index out of bounds");
      return int(BigInt(b(0)[Number(index)] ?? 0));
    }
    case 15: return boolean(bytesEqual(b(0), b(1)));
    case 16: return boolean(compareBytes(b(0), b(1)) < 0);
    case 17: return boolean(compareBytes(b(0), b(1)) <= 0);
    case 18: return bytes(sha2_256(b(0)));
    case 19: return bytes(sha3_256(b(0)));
    case 20: return bytes(blake2b256(b(0)));
    case 21: return boolean(verifyEd25519Uplc(b(0), b(2), b(1)));
    case 22: return con({ kind: "string" }, s(0) + s(1));
    case 23: return boolean(s(0) === s(1));
    case 24: return bytes(textEncoder.encode(s(0)));
    case 25:
      try { return con({ kind: "string" }, textDecoder.decode(b(0))); }
      catch { fail("invalid UTF-8"); }
    case 26: return bool(0) ? values[1] as Value : values[2] as Value;
    case 27: typed(c(0), "unit"); return values[1] as Value;
    case 28: logs.push(s(0)); return values[1] as Value;
    case 29: return pairValue(c(0), 0);
    case 30: return pairValue(c(0), 1);
    case 31: return listValue(c(0)).length === 0 ? values[1] as Value : values[2] as Value;
    case 32: {
      const list = c(1);
      const items = listValue(list);
      if (!sameType(c(0).type, list.type.kind === "list" ? list.type.item : list.type)) fail("list element type mismatch");
      return con(list.type, [c(0).value, ...items]);
    }
    case 33: {
      const list = c(0);
      const items = listValue(list);
      if (items.length === 0 || list.type.kind !== "list") fail("head of empty list");
      return con(list.type.item, items[0]);
    }
    case 34: {
      const list = c(0);
      const items = listValue(list);
      if (items.length === 0) fail("tail of empty list");
      return con(list.type, items.slice(1));
    }
    case 35: return boolean(listValue(c(0)).length === 0);
    case 36: return chooseData(data(0), values);
    case 37: return con({ kind: "data" }, makeConstrData(i(0), dataList(c(1))));
    case 38: return con({ kind: "data" }, { kind: "map", entries: dataPairList(c(0)), encoding: definite() });
    case 39: return con({ kind: "data" }, { kind: "array", values: dataList(c(0)), encoding: definite() });
    case 40: return con({ kind: "data" }, integerNode(i(0)));
    case 41: return con({ kind: "data" }, bytesNode(b(0)));
    case 42: {
      const [constructor, fields] = unConstrData(data(0));
      return con({ kind: "pair", first: { kind: "integer" }, second: { kind: "list", item: { kind: "data" } } }, [constructor, fields]);
    }
    case 43: return con({ kind: "list", item: { kind: "pair", first: { kind: "data" }, second: { kind: "data" } } }, unMapData(data(0)));
    case 44: return con({ kind: "list", item: { kind: "data" } }, unListData(data(0)));
    case 45: return int(dataInteger(data(0)));
    case 46: return bytes(dataBytes(data(0)));
    case 47: return boolean(bytesEqual(canonicalData(data(0)), canonicalData(data(1))));
    case 48: return con({ kind: "pair", first: { kind: "data" }, second: { kind: "data" } }, [data(0), data(1)]);
    case 49: typed(c(0), "unit"); return con({ kind: "list", item: { kind: "data" } }, []);
    case 50: typed(c(0), "unit"); return con({ kind: "list", item: { kind: "pair", first: { kind: "data" }, second: { kind: "data" } } }, []);
    case 51: return bytes(canonicalData(data(0)));
    case 52: return boolean(verifySecp256k1EcdsaUplc(b(0), b(2), b(1)));
    case 53: return boolean(verifySecp256k1SchnorrUplc(b(0), b(2), b(1)));
    case 54: return con({ kind: "bls-g1" }, bls12_381_add("g1", bls(c(0), "bls-g1"), bls(c(1), "bls-g1")));
    case 55: return con({ kind: "bls-g1" }, bls12_381_neg("g1", bls(c(0), "bls-g1")));
    case 56: return con({ kind: "bls-g1" }, bls12_381_scalar_mul("g1", i(0), bls(c(1), "bls-g1")));
    case 57: return boolean(bls12_381_equal("g1", bls(c(0), "bls-g1"), bls(c(1), "bls-g1")));
    case 58: return con({ kind: "bls-g1" }, bls12_381_hash_to_group("g1", b(0), b(1)));
    case 59: return bytes(bls12_381_compress("g1", bls(c(0), "bls-g1")));
    case 60: return con({ kind: "bls-g1" }, bls12_381_uncompress("g1", b(0)));
    case 61: return con({ kind: "bls-g2" }, bls12_381_add("g2", bls(c(0), "bls-g2"), bls(c(1), "bls-g2")));
    case 62: return con({ kind: "bls-g2" }, bls12_381_neg("g2", bls(c(0), "bls-g2")));
    case 63: return con({ kind: "bls-g2" }, bls12_381_scalar_mul("g2", i(0), bls(c(1), "bls-g2")));
    case 64: return boolean(bls12_381_equal("g2", bls(c(0), "bls-g2"), bls(c(1), "bls-g2")));
    case 65: return con({ kind: "bls-g2" }, bls12_381_hash_to_group("g2", b(0), b(1)));
    case 66: return bytes(bls12_381_compress("g2", bls(c(0), "bls-g2")));
    case 67: return con({ kind: "bls-g2" }, bls12_381_uncompress("g2", b(0)));
    case 68: return con({ kind: "bls-ml" }, bls12_381_miller_loop(bls(c(0), "bls-g1"), bls(c(1), "bls-g2")));
    case 69: return con({ kind: "bls-ml" }, bls12_381_mul_ml_result(bls(c(0), "bls-ml"), bls(c(1), "bls-ml")));
    case 70: return boolean(bls12_381_final_verify(bls(c(0), "bls-ml"), bls(c(1), "bls-ml")));
    case 71: return bytes(keccak_256(b(0)));
    case 72: return bytes(blake2b224(b(0)));
    case 73: return bytes(integerToBytes(bool(0), i(1), i(2)));
    case 74: return int(bytesToInteger(bool(0), b(1)));
    case 75: return bytes(zipBytes(bool(0), b(1), b(2), (x, y) => x & y));
    case 76: return bytes(zipBytes(bool(0), b(1), b(2), (x, y) => x | y));
    case 77: return bytes(zipBytes(bool(0), b(1), b(2), (x, y) => x ^ y));
    case 78: return bytes(Uint8Array.from(b(0), (value) => value ^ 0xff));
    case 79: return boolean(readBit(b(0), i(1)));
    case 80: return bytes(writeBits(b(0), integerList(c(1)), bool(2)));
    case 81: {
      const length = i(0), byte = i(1);
      if (length < 0n || length > 8192n || byte < 0n || byte > 255n) fail("replicateByte argument out of bounds");
      return bytes(new Uint8Array(Number(length)).fill(Number(byte)));
    }
    case 82: return bytes(shiftBytes(b(0), boundedShift(i(1), semantics)));
    case 83: return bytes(rotateBytes(b(0), boundedShift(i(1), semantics)));
    case 84: return int(BigInt(countBits(b(0))));
    case 85: return int(BigInt(firstSetBit(b(0))));
    case 86: return bytes(ripemd_160(b(0)));
    case 87: {
      const modulus = i(2);
      return int(expMod(i(0), i(1), modulus));
    }
    case 88: {
      const count = i(0), list = c(1);
      return con(list.type, listValue(list).slice(count <= 0n ? 0 : Number(count)));
    }
    case 89: return int(BigInt(arrayValue(c(0)).length));
    case 90: {
      const list = c(0);
      if (list.type.kind !== "list") fail("expected list");
      return con({ kind: "array", item: list.type.item }, listValue(list));
    }
    case 91: {
      const array = c(0), items = arrayValue(array), index = i(1);
      if (array.type.kind !== "array" || index < 0n || index >= BigInt(items.length)) fail("array index out of bounds");
      return con(array.type.item, items[Number(index)]);
    }
    case 92: return con({ kind: "bls-g1" }, multiScalarMul("g1", c(0), c(1)));
    case 93: return con({ kind: "bls-g2" }, multiScalarMul("g2", c(0), c(1)));
    case 94: return con({ kind: "value" }, insertCoin(b(0), b(1), i(2), flatValue(c(3))));
    case 95: return int(lookupCoin(b(0), b(1), flatValue(c(2))));
    case 96: return con({ kind: "value" }, unionValue(flatValue(c(0)), flatValue(c(1))));
    case 97: return boolean(valueContains(flatValue(c(0)), flatValue(c(1))));
    case 98: return con({ kind: "data" }, valueData(flatValue(c(0))));
    case 99: return con({ kind: "value" }, unValueData(data(0)));
    case 100: return con({ kind: "value" }, scaleValue(i(0), flatValue(c(1))));
    default: fail(`builtin ${tag} is not implemented by this runtime`);
  }
}

function con(type: UplcType, value: unknown): Value {
  return { kind: "constant", constant: { type, value } };
}

function constant(value: Value | undefined): UplcConstant {
  if (value?.kind !== "constant") fail("builtin expected a constant");
  return value.constant;
}

function typed(constant_: UplcConstant, kind: UplcType["kind"]): unknown {
  if (constant_.type.kind !== kind) fail(`builtin expected ${kind}`);
  return constant_.value;
}

function pairValue(value: UplcConstant, index: 0 | 1): Value {
  if (value.type.kind !== "pair" || !Array.isArray(value.value)) fail("builtin expected pair");
  return con(index === 0 ? value.type.first : value.type.second, value.value[index]);
}

function listValue(value: UplcConstant): readonly unknown[] {
  if (value.type.kind !== "list" || !Array.isArray(value.value)) fail("builtin expected list");
  return value.value;
}

function arrayValue(value: UplcConstant): readonly unknown[] {
  if (value.type.kind !== "array" || !Array.isArray(value.value)) fail("builtin expected array");
  return value.value;
}

function dataList(value: UplcConstant): CborValue[] {
  if (value.type.kind !== "list" || value.type.item.kind !== "data" || !Array.isArray(value.value)) {
    fail("builtin expected Data list");
  }
  return value.value as CborValue[];
}

function dataPairList(value: UplcConstant): Array<readonly [CborValue, CborValue]> {
  if (
    value.type.kind !== "list" ||
    value.type.item.kind !== "pair" ||
    value.type.item.first.kind !== "data" ||
    value.type.item.second.kind !== "data" ||
    !Array.isArray(value.value)
  ) fail("builtin expected Data pair list");
  return value.value as Array<readonly [CborValue, CborValue]>;
}

function integerList(value: UplcConstant): bigint[] {
  if (value.type.kind !== "list" || value.type.item.kind !== "integer" || !Array.isArray(value.value)) {
    fail("builtin expected integer list");
  }
  return value.value as bigint[];
}

function bls(value: UplcConstant, kind: "bls-g1" | "bls-g2" | "bls-ml"): Uint8Array {
  if (value.type.kind !== kind || !(value.value instanceof Uint8Array)) fail(`builtin expected ${kind}`);
  return value.value;
}

function multiScalarMul(
  group: "g1" | "g2",
  scalarsConstant: UplcConstant,
  pointsConstant: UplcConstant,
): Uint8Array {
  const scalars = integerList(scalarsConstant);
  const pointKind = group === "g1" ? "bls-g1" : "bls-g2";
  if (
    pointsConstant.type.kind !== "list" ||
    pointsConstant.type.item.kind !== pointKind ||
    !Array.isArray(pointsConstant.value)
  ) fail("invalid BLS12-381 multi-scalar multiplication arguments");
  const points = pointsConstant.value as Uint8Array[];
  const length = Math.min(points.length, scalars.length);
  if (length === 0) {
    const output = new Uint8Array(group === "g1" ? 48 : 96);
    output[0] = 0xc0;
    return output;
  }
  let result = bls12_381_scalar_mul(group, scalars[0] as bigint, points[0] as Uint8Array);
  for (let index = 1; index < length; index += 1) {
    result = bls12_381_add(
      group,
      result,
      bls12_381_scalar_mul(group, scalars[index] as bigint, points[index] as Uint8Array),
    );
  }
  return result;
}

type RuntimeFlatValue = readonly (readonly [
  Uint8Array,
  readonly (readonly [Uint8Array, bigint])[],
])[];

function flatValue(value: UplcConstant): RuntimeFlatValue {
  if (value.type.kind !== "value" || !Array.isArray(value.value)) fail("builtin expected value");
  return value.value as RuntimeFlatValue;
}

function insertCoin(
  currency: Uint8Array,
  token: Uint8Array,
  quantity: bigint,
  value: RuntimeFlatValue,
): RuntimeFlatValue {
  assertQuantity(quantity);
  const map = valueMap(value);
  const currencyKey = byteKey(currency);
  const tokenKey = byteKey(token);
  if (quantity === 0n) {
    const inner = map.get(currencyKey);
    if (inner === undefined) return mapValue(map);
    inner.tokens.delete(tokenKey);
    if (inner.tokens.size === 0) map.delete(currencyKey);
    return mapValue(map);
  }
  if (currency.length > 32 || token.length > 32) fail("Value key exceeds 32 bytes");
  const inner = map.get(currencyKey) ?? { bytes: Uint8Array.from(currency), tokens: new Map() };
  inner.tokens.set(tokenKey, { bytes: Uint8Array.from(token), quantity });
  if (inner.tokens.size === 0) map.delete(currencyKey);
  else map.set(currencyKey, inner);
  return mapValue(map);
}

function lookupCoin(currency: Uint8Array, token: Uint8Array, value: RuntimeFlatValue): bigint {
  return valueMap(value).get(byteKey(currency))?.tokens.get(byteKey(token))?.quantity ?? 0n;
}

function unionValue(left: RuntimeFlatValue, right: RuntimeFlatValue): RuntimeFlatValue {
  let output = left;
  for (const [currency, tokens] of right) {
    for (const [token, quantity] of tokens) {
      output = insertCoin(currency, token, lookupCoin(currency, token, output) + quantity, output);
    }
  }
  return output;
}

function valueContains(left: RuntimeFlatValue, right: RuntimeFlatValue): boolean {
  for (const [, tokens] of [...left, ...right]) {
    for (const [, quantity] of tokens) if (quantity < 0n) fail("valueContains requires non-negative values");
  }
  for (const [currency, tokens] of right) {
    for (const [token, quantity] of tokens) if (lookupCoin(currency, token, left) < quantity) return false;
  }
  return true;
}

function scaleValue(scalar: bigint, value: RuntimeFlatValue): RuntimeFlatValue {
  let output: RuntimeFlatValue = [];
  for (const [currency, tokens] of value) {
    for (const [token, quantity] of tokens) output = insertCoin(currency, token, scalar * quantity, output);
  }
  return output;
}

function valueData(value: RuntimeFlatValue): CborValue {
  let size = 0;
  const entries = value.map(([currency, tokens]) => {
    size += tokens.length;
    return [
      bytesNode(currency),
      {
        kind: "map",
        entries: tokens.map(([token, quantity]) => [bytesNode(token), integerNode(quantity)] as const),
        encoding: definite(),
      },
    ] as const;
  });
  if (size > 40_000) fail("Value exceeds Data conversion size limit");
  return { kind: "map", entries, encoding: definite() };
}

function unValueData(data: CborValue): RuntimeFlatValue {
  if (data.kind !== "map") fail("Value Data must be a map");
  const output: Array<readonly [Uint8Array, Array<readonly [Uint8Array, bigint]>]> = [];
  let previousCurrency: Uint8Array | undefined;
  for (const [currency, tokens] of data.entries) {
    if (currency.kind !== "bytes" || tokens.kind !== "map") fail("invalid Value Data");
    if (
      currency.value.length > 32 ||
      previousCurrency !== undefined && compareBytes(previousCurrency, currency.value) >= 0
    ) fail("invalid Value currency ordering");
    if (tokens.entries.length === 0) fail("Value contains an empty token map");
    const inner: Array<readonly [Uint8Array, bigint]> = [];
    let previousToken: Uint8Array | undefined;
    for (const [token, quantity] of tokens.entries) {
      if (token.kind !== "bytes") fail("invalid Value token Data");
      const amount = dataInteger(quantity);
      if (
        token.value.length > 32 ||
        previousToken !== undefined && compareBytes(previousToken, token.value) >= 0 ||
        amount === 0n
      ) fail("invalid Value token ordering or quantity");
      assertQuantity(amount);
      inner.push([Uint8Array.from(token.value), amount]);
      previousToken = token.value;
    }
    output.push([Uint8Array.from(currency.value), inner]);
    previousCurrency = currency.value;
  }
  return output;
}

interface MutableValueCurrency {
  readonly bytes: Uint8Array;
  readonly tokens: Map<string, { readonly bytes: Uint8Array; readonly quantity: bigint }>;
}

function valueMap(value: RuntimeFlatValue): Map<string, MutableValueCurrency> {
  const output = new Map<string, MutableValueCurrency>();
  for (const [currency, tokens] of value) {
    const currencyKey = byteKey(currency);
    if (currency.length > 32 || output.has(currencyKey)) fail("invalid Value currency");
    const inner: MutableValueCurrency = { bytes: Uint8Array.from(currency), tokens: new Map() };
    for (const [token, quantity] of tokens) {
      const tokenKey = byteKey(token);
      if (token.length > 32 || quantity === 0n || inner.tokens.has(tokenKey)) fail("invalid Value token");
      assertQuantity(quantity);
      inner.tokens.set(tokenKey, { bytes: Uint8Array.from(token), quantity });
    }
    if (inner.tokens.size === 0) fail("Value contains an empty token map");
    output.set(currencyKey, inner);
  }
  return output;
}

function mapValue(value: Map<string, MutableValueCurrency>): RuntimeFlatValue {
  return [...value.values()]
    .sort((left, right) => compareBytes(left.bytes, right.bytes))
    .map((currency) => [
      currency.bytes,
      [...currency.tokens.values()]
        .sort((left, right) => compareBytes(left.bytes, right.bytes))
        .map((token) => [token.bytes, token.quantity] as const),
    ] as const);
}

function assertQuantity(value: bigint): void {
  if (value < -(1n << 127n) || value >= 1n << 127n) fail("Value quantity exceeds signed 128-bit bounds");
}

function byteKey(value: Uint8Array): string {
  let output = "";
  for (const byte of value) output += byte.toString(16).padStart(2, "0");
  return output;
}

function chooseData(value: CborValue, arguments_: readonly Value[]): Value {
  if (isConstrData(value)) return arguments_[1] as Value;
  if (value.kind === "map") return arguments_[2] as Value;
  if (value.kind === "array") return arguments_[3] as Value;
  if (value.kind === "unsigned" || value.kind === "negative" || isBigInteger(value)) return arguments_[4] as Value;
  if (value.kind === "bytes") return arguments_[5] as Value;
  fail("invalid Plutus Data");
}

function makeConstrData(alternative: bigint, fields: readonly CborValue[]): CborValue {
  const array: CborValue = { kind: "array", values: fields, encoding: definite() };
  if (alternative >= 0n && alternative <= 6n) {
    return { kind: "tag", tag: 121n + alternative, value: array, encoding: { width: 0 } };
  }
  if (alternative >= 7n && alternative <= 127n) {
    return { kind: "tag", tag: 1280n + alternative - 7n, value: array, encoding: { width: 0 } };
  }
  return {
    kind: "tag",
    tag: 102n,
    value: { kind: "array", values: [integerNode(alternative), array], encoding: definite() },
    encoding: { width: 0 },
  };
}

function unConstrData(value: CborValue): readonly [bigint, CborValue[]] {
  if (value.kind !== "tag") fail("expected constructor Data");
  if (value.tag >= 121n && value.tag <= 127n && value.value.kind === "array") {
    return [value.tag - 121n, [...value.value.values]];
  }
  if (value.tag >= 1280n && value.tag <= 1400n && value.value.kind === "array") {
    return [value.tag - 1280n + 7n, [...value.value.values]];
  }
  if (value.tag === 102n && value.value.kind === "array" && value.value.values.length === 2) {
    const [tag, fields] = value.value.values;
    if ((tag?.kind === "unsigned" || tag?.kind === "negative") && fields?.kind === "array") {
      return [tag.value, [...fields.values]];
    }
  }
  fail("invalid constructor Data");
}

function isConstrData(value: CborValue): boolean {
  try { unConstrData(value); return true; } catch { return false; }
}

function unMapData(value: CborValue): Array<readonly [CborValue, CborValue]> {
  if (value.kind !== "map") fail("expected map Data");
  return [...value.entries];
}

function unListData(value: CborValue): CborValue[] {
  if (value.kind !== "array") fail("expected list Data");
  return [...value.values];
}

function dataInteger(value: CborValue): bigint {
  if (value.kind === "unsigned" || value.kind === "negative") return value.value;
  if (isBigInteger(value) && value.value.kind === "bytes") {
    const magnitude = bytesToInteger(true, value.value.value);
    return value.tag === 2n ? magnitude : -magnitude - 1n;
  }
  fail("expected integer Data");
}

function dataBytes(value: CborValue): Uint8Array {
  if (value.kind !== "bytes") fail("expected bytes Data");
  return value.value;
}

function isBigInteger(value: CborValue): value is Extract<CborValue, { kind: "tag" }> {
  return value.kind === "tag" && (value.tag === 2n || value.tag === 3n);
}

function canonicalData(value: CborValue): Uint8Array {
  return encodePlutusData(value);
}

function integerNode(value: bigint): CborValue {
  if (value >= 0n && value <= 0xffff_ffff_ffff_ffffn) {
    return { kind: "unsigned", value, encoding: { width: 0 } };
  }
  if (value < 0n && value >= -0x1_0000_0000_0000_0000n) {
    return { kind: "negative", value, encoding: { width: 0 } };
  }
  const positive = value >= 0n;
  return {
    kind: "tag",
    tag: positive ? 2n : 3n,
    value: bytesNode(integerToBytes(false, 0n, positive ? value : -value - 1n)),
    encoding: { width: 0 },
  };
}

function bytesNode(value: Uint8Array): CborValue {
  return { kind: "bytes", value: Uint8Array.from(value), encoding: definite() };
}

function definite(): { readonly kind: "definite"; readonly width: 0 } {
  return { kind: "definite", width: 0 };
}

function sameType(left: UplcType, right: UplcType): boolean {
  if (left.kind !== right.kind) return false;
  if (left.kind === "list" && right.kind === "list") return sameType(left.item, right.item);
  if (left.kind === "array" && right.kind === "array") return sameType(left.item, right.item);
  if (left.kind === "pair" && right.kind === "pair") {
    return sameType(left.first, right.first) && sameType(left.second, right.second);
  }
  return true;
}

function assertClosed(term: UplcTerm): void {
  const stack: Array<readonly [UplcTerm, bigint]> = [[term, 0n]];
  while (stack.length !== 0) {
    const [value, depth] = stack.pop() as readonly [UplcTerm, bigint];
    switch (value.kind) {
      case "var": if (value.index < 1n || value.index > depth) fail("free UPLC variable"); break;
      case "lambda": stack.push([value.body, depth + 1n]); break;
      case "delay":
      case "force": stack.push([value.term, depth]); break;
      case "apply": stack.push([value.argument, depth], [value.function, depth]); break;
      case "constr":
        for (const field of value.fields) stack.push([field, depth]);
        break;
      case "case":
        stack.push([value.scrutinee, depth]);
        for (const branch of value.branches) stack.push([branch, depth]);
        break;
      default: break;
    }
  }
}

function charge(meter: Meter, cpu: bigint, memory: bigint): void {
  meter.cpu = saturatingAdd(meter.cpu, cpu);
  meter.memory = saturatingAdd(meter.memory, memory);
  if (meter.cpu > meter.maximum.cpu || meter.memory > meter.maximum.memory) fail("UPLC budget exhausted");
}

function saturatingAdd(left: bigint, right: bigint): bigint {
  const result = left + right;
  return result > INT64_MAX ? INT64_MAX : result;
}

function builtinCpu(tag: number, values: readonly Value[]): bigint {
  return 1_000n + BigInt(tag + 1) * 100n + values.reduce((sum, value) => sum + memoryOf(value) * 50n, 0n);
}

function builtinMemory(_tag: number, values: readonly Value[]): bigint {
  return 1n + values.reduce((sum, value) => sum + memoryOf(value), 0n);
}

function builtinBudget(
  tag: number,
  values: readonly Value[],
  costs: MachineCosts,
  semantics: SemanticsVariant,
): MachineBudget {
  const supplied = costs.costModel;
  if (supplied === undefined) {
    return costs.builtinModel === undefined
      ? { cpu: builtinCpu(tag, values), memory: builtinMemory(tag, values) }
      : builtinCost(
        tag,
        values.map((value, index) => builtinMemoryStream(tag, index, value, semantics)),
        costs.builtinModel,
      );
  }
  let variants = costModelCache.get(supplied);
  if (variants === undefined) {
    variants = new Map();
    costModelCache.set(supplied, variants);
  }
  let model = variants.get(semantics);
  if (model === undefined) {
    model = makeBuiltinCostModel(supplied.language, supplied.parameters, semantics);
    variants.set(semantics, model);
  }
  return builtinCost(
    tag,
    values.map((value, index) => builtinMemoryStream(tag, index, value, semantics)),
    model,
  );
}

function memoryOf(value: Value): bigint {
  return builtinMemoryStream(-1, 0, value, "A").reduce((sum, item) => sum + item, 0n);
}

function builtinMemoryStream(
  tag: number,
  index: number,
  value: Value,
  semantics: SemanticsVariant,
): CostStream {
  if (value.kind !== "constant") return [1n];
  const constant_ = value.constant;
  if (
    (semantics === "D" || semantics === "E") &&
    (tag === 22 || tag === 23 || tag === 24) &&
    constant_.type.kind === "string"
  ) {
    return [BigInt(Math.floor(textEncoder.encode(constant_.value as string).length / 4))];
  }
  if (
    (tag === 73 && index === 1 || tag === 81 && index === 0) &&
    constant_.type.kind === "integer"
  ) {
    const integer = constant_.value as bigint;
    return [integer === 0n ? 0n : (absolute(integer) - 1n) / 8n + 1n];
  }
  if (
    (tag === 82 || tag === 83) && index === 1 ||
    tag === 88 && index === 0
  ) {
    if (constant_.type.kind === "integer") return [absolute(constant_.value as bigint)];
  }
  if ((tag === 94 && index === 3 || tag === 95 && index === 2) && constant_.type.kind === "value") {
    return [valueMaximumDepth(flatValue(constant_))];
  }
  if (
    (tag === 96 || tag === 97) ||
    tag === 98 && index === 0 ||
    tag === 100 && index === 1
  ) {
    if (constant_.type.kind === "value") return [BigInt(valueTotalSize(flatValue(constant_)))];
  }
  if (tag === 99 && index === 0 && constant_.type.kind === "data") {
    return dataNodeCount(constant_.value as CborValue);
  }
  switch (constant_.type.kind) {
    case "integer": {
      const integer = constant_.value as bigint;
      return [integer === 0n ? 1n : BigInt(Math.floor((absolute(integer).toString(2).length - 1) / 64) + 1)];
    }
    case "bytes": {
      const length = (constant_.value as Uint8Array).length;
      return [length === 0 ? 1n : BigInt(Math.floor((length - 1) / 8) + 1)];
    }
    case "string": return [BigInt(Array.from(constant_.value as string).length)];
    case "unit":
    case "boolean": return [1n];
    case "data": return dataMemoryStream(constant_.value as CborValue);
    case "bls-g1": return [18n];
    case "bls-g2": return [36n];
    case "bls-ml": return [72n];
    case "list":
    case "array": return [BigInt((constant_.value as readonly unknown[]).length)];
    case "pair": return [0x7fff_ffff_ffff_ffffn];
    case "value": return [BigInt(valueTotalSize(flatValue(constant_)))];
  }
}

function dataMemoryStream(root: CborValue): CostStream {
  const output: bigint[] = [];
  const stack = [root];
  while (stack.length !== 0) {
    const value = stack.pop() as CborValue;
    if (isConstrData(value)) {
      const [, fields] = unConstrData(value);
      output.push(4n);
      for (let index = fields.length - 1; index >= 0; index -= 1) stack.push(fields[index] as CborValue);
    } else if (value.kind === "map") {
      output.push(4n);
      for (let index = value.entries.length - 1; index >= 0; index -= 1) {
        const [key, item] = value.entries[index] as readonly [CborValue, CborValue];
        stack.push(item, key);
      }
    } else if (value.kind === "array") {
      output.push(4n);
      for (let index = value.values.length - 1; index >= 0; index -= 1) {
        stack.push(value.values[index] as CborValue);
      }
    } else if (value.kind === "unsigned" || value.kind === "negative" || isBigInteger(value)) {
      const integer = dataInteger(value);
      const words = integer === 0n
        ? 1n
        : BigInt(Math.floor((absolute(integer).toString(2).length - 1) / 64) + 1);
      output.push(4n + words);
    } else if (value.kind === "bytes") {
      const words = value.value.length === 0
        ? 1n
        : BigInt(Math.floor((value.value.length - 1) / 8) + 1);
      output.push(4n + words);
    } else {
      fail("invalid Plutus Data for memory costing");
    }
  }
  return output.length === 0 ? [0n] : output;
}

function dataNodeCount(root: CborValue): CostStream {
  let count = 0n;
  const stack = [root];
  while (stack.length !== 0) {
    const value = stack.pop() as CborValue;
    count += 1n;
    if (isConstrData(value)) {
      stack.push(...unConstrData(value)[1]);
    } else if (value.kind === "map") {
      for (const [key, item] of value.entries) stack.push(key, item);
    } else if (value.kind === "array") {
      stack.push(...value.values);
    }
  }
  return [count];
}

function valueTotalSize(value: RuntimeFlatValue): number {
  return value.reduce((total, [, tokens]) => total + tokens.length, 0);
}

function valueMaximumDepth(value: RuntimeFlatValue): bigint {
  const outer = value.length;
  const inner = value.reduce((maximum, [, tokens]) => Math.max(maximum, tokens.length), 0);
  const outerDepth = outer === 0 ? 0 : Math.floor(Math.log2(outer)) + 1;
  const innerDepth = inner === 0 ? 0 : Math.floor(Math.log2(inner)) + 1;
  return BigInt(outerDepth + innerDepth);
}

function absolute(value: bigint): bigint {
  return value < 0n ? -value : value;
}

function nonzero(value: bigint): bigint {
  if (value === 0n) fail("division by zero");
  return value;
}

function euclideanDiv(left: bigint, right: bigint): bigint {
  const quotient = left / right;
  const remainder = left % right;
  return remainder !== 0n && ((remainder < 0n) !== (right < 0n)) ? quotient - 1n : quotient;
}

function euclideanMod(left: bigint, right: bigint): bigint {
  return left - euclideanDiv(left, right) * right;
}

function concat(left: Uint8Array, right: Uint8Array): Uint8Array {
  const output = new Uint8Array(left.length + right.length);
  output.set(left);
  output.set(right, left.length);
  return output;
}

function compareBytes(left: Uint8Array, right: Uint8Array): number {
  const length = Math.min(left.length, right.length);
  for (let index = 0; index < length; index += 1) {
    const difference = (left[index] ?? 0) - (right[index] ?? 0);
    if (difference !== 0) return difference;
  }
  return left.length - right.length;
}

function clampIndex(value: bigint, length: number): number {
  if (value <= 0n) return 0;
  return value >= BigInt(length) ? length : Number(value);
}

function integerToBytes(bigEndian: boolean, requestedLength: bigint, value: bigint): Uint8Array {
  if (value < 0n || requestedLength < 0n || requestedLength > 8192n) fail("integerToByteString argument out of bounds");
  if (requestedLength === 0n && value.toString(2).length > 65_536) {
    fail("integerToByteString input exceeds the unbounded output limit");
  }
  const bytes_: number[] = [];
  let remaining = value;
  while (remaining > 0n) {
    bytes_.push(Number(remaining & 0xffn));
    remaining >>= 8n;
  }
  const minimum = bytes_.length;
  const length = requestedLength === 0n ? minimum : Number(requestedLength);
  if (minimum > length) fail("integer does not fit requested byte string length");
  const output = new Uint8Array(length);
  if (bigEndian) {
    for (let index = 0; index < minimum; index += 1) output[length - 1 - index] = bytes_[index] ?? 0;
  } else output.set(bytes_);
  return output;
}

function bytesToInteger(bigEndian: boolean, bytes: Uint8Array): bigint {
  let value = 0n;
  if (bigEndian) for (const byte of bytes) value = (value << 8n) | BigInt(byte);
  else for (let index = bytes.length - 1; index >= 0; index -= 1) value = (value << 8n) | BigInt(bytes[index] ?? 0);
  return value;
}

function zipBytes(
  pad: boolean,
  left: Uint8Array,
  right: Uint8Array,
  operation: (left: number, right: number) => number,
): Uint8Array {
  const length = pad ? Math.max(left.length, right.length) : Math.min(left.length, right.length);
  const longer = left.length >= right.length ? left : right;
  const output = pad ? Uint8Array.from(longer) : new Uint8Array(length);
  for (let index = 0; index < Math.min(left.length, right.length); index += 1) {
    output[index] = operation(left[index] ?? 0, right[index] ?? 0);
  }
  return output;
}

function readBit(bytes: Uint8Array, index: bigint): boolean {
  if (index < 0n || index >= BigInt(bytes.length * 8)) fail("bit index out of bounds");
  const offset = Number(index);
  return (((bytes[bytes.length - 1 - (offset >> 3)] ?? 0) >> (offset & 7)) & 1) === 1;
}

function writeBits(bytes: Uint8Array, indices: readonly bigint[], bit: boolean): Uint8Array {
  const output = Uint8Array.from(bytes);
  for (const index of indices) {
    if (index < 0n || index >= BigInt(output.length * 8)) fail("bit index out of bounds");
    const offset = Number(index), byte = output.length - 1 - (offset >> 3), mask = 1 << (offset & 7);
    output[byte] = bit ? (output[byte] ?? 0) | mask : (output[byte] ?? 0) & ~mask;
  }
  return output;
}

function shiftBytes(bytes: Uint8Array, amount: bigint): Uint8Array {
  const bits = BigInt(bytes.length * 8);
  if (bits === 0n || amount >= bits || amount <= -bits) return new Uint8Array(bytes.length);
  const value = bytesToInteger(true, bytes);
  const mask = (1n << bits) - 1n;
  return integerToBytes(true, bits / 8n, amount >= 0n ? (value << amount) & mask : value >> -amount);
}

function rotateBytes(bytes: Uint8Array, amount: bigint): Uint8Array {
  const bits = BigInt(bytes.length * 8);
  if (bits === 0n) return Uint8Array.from(bytes);
  const shift = ((amount % bits) + bits) % bits;
  const value = bytesToInteger(true, bytes), mask = (1n << bits) - 1n;
  return integerToBytes(true, bits / 8n, ((value << shift) | (value >> (bits - shift))) & mask);
}

function boundedShift(
  amount: bigint,
  semantics: "A" | "B" | "C" | "D" | "E",
): bigint {
  if (
    (semantics === "D" || semantics === "E") &&
    (amount < -0x8000_0000_0000_0000n || amount > 0x7fff_ffff_ffff_ffffn)
  ) fail("byte string shift exceeds signed 64-bit bounds");
  return amount;
}

function countBits(bytes: Uint8Array): number {
  let count = 0;
  for (const byte of bytes) {
    let value = byte;
    while (value !== 0) { value &= value - 1; count += 1; }
  }
  return count;
}

function firstSetBit(bytes: Uint8Array): number {
  for (let index = 0; index < bytes.length * 8; index += 1) if (readBit(bytes, BigInt(index))) return index;
  return -1;
}

function modPow(base: bigint, exponent: bigint, modulus: bigint): bigint {
  let result = 1n, factor = euclideanMod(base, modulus), power = exponent;
  while (power > 0n) {
    if ((power & 1n) === 1n) result = (result * factor) % modulus;
    factor = (factor * factor) % modulus;
    power >>= 1n;
  }
  return result;
}

function expMod(base: bigint, exponent: bigint, modulus: bigint): bigint {
  const limit = 1n << 8191n;
  if (
    modulus <= 0n ||
    modulus >= limit ||
    base < -limit ||
    base >= limit ||
    exponent < -limit ||
    exponent >= limit
  ) fail("invalid modular exponentiation arguments");
  if (modulus === 1n) return 0n;
  if (exponent >= 0n) return modPow(base, exponent, modulus);
  const inverse = modularInverse(base, modulus);
  if (inverse === undefined) fail("base is not invertible modulo the modulus");
  return modPow(inverse, -exponent, modulus);
}

function modularInverse(value: bigint, modulus: bigint): bigint | undefined {
  let oldR = euclideanMod(value, modulus), r = modulus;
  let oldS = 1n, s = 0n;
  while (r !== 0n) {
    const quotient = oldR / r;
    [oldR, r] = [r, oldR - quotient * r];
    [oldS, s] = [s, oldS - quotient * s];
  }
  return oldR === 1n ? euclideanMod(oldS, modulus) : undefined;
}

function fail(message: string): never {
  throw new CardanoError("EVALUATE", message);
}

export function defaultMachineCosts(): MachineCosts {
  const step = { cpu: 16_000n, memory: 100n };
  return {
    apply: step,
    builtin: step,
    constant: step,
    delay: step,
    force: step,
    lambda: step,
    startup: { cpu: 100n, memory: 100n },
    variable: step,
    constr: step,
    case_: step,
    builtinModel: DEFAULT_BUILTIN_MODEL,
  };
}
