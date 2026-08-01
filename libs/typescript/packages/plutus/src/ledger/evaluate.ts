import {
  CardanoBoundsError,
  CardanoError,
  DeserializeError,
  INT64_MAX,
  INT64_MIN,
  bytesEqual,
  decodeCbor,
  encodeCbor,
  type CborValue,
} from "@xray-network/xray-cardano-lib-core";
import { blake2b224, blake2b256 } from "@xray-network/xray-cardano-lib-crypto";
import { decodeProgramEnvelopeCompatible, encodePlutusData } from "../uplc/flat.js";
import {
  evaluateProgram,
  type MachineBudget,
  type MachineCosts,
} from "../uplc/machine.js";
import { makeScriptContext } from "./context.js";

export interface RawEvaluation {
  readonly redeemer: Uint8Array;
  readonly cost: MachineBudget;
  readonly logs: readonly string[];
}

interface Redeemer {
  readonly tag: number;
  readonly index: bigint;
  readonly data: CborValue;
}

interface Script {
  readonly language: 0 | 1 | 2;
  readonly bytes: Uint8Array;
  readonly hash: Uint8Array;
}

interface Utxo {
  readonly inputBytes: Uint8Array;
  readonly input: CborValue;
  readonly output: CborValue;
}

const definite = { kind: "definite", width: 0 } as const;

export function evaluateRaw(
  txBytes: Uint8Array,
  utxosBytes: readonly (readonly [Uint8Array, Uint8Array])[],
  costModelsBytes: Uint8Array,
  maximum: MachineBudget,
  slotConfig: readonly [bigint, bigint, bigint],
  protocolMajor: number,
  runPhaseOne: boolean,
): readonly RawEvaluation[] {
  assertProtocol(protocolMajor);
  assertBudget(maximum);
  if (slotConfig[2] <= 0n) {
    throw new CardanoBoundsError("slot length milliseconds", 1n, INT64_MAX, slotConfig[2]);
  }
  const tx = decode("transaction", txBytes);
  if (tx.kind !== "array" || tx.values.length < 3) malformed("transaction must be a ledger transaction array");
  const body = tx.values[0];
  const witnesses = tx.values[1];
  if (body?.kind !== "map" || witnesses?.kind !== "map") malformed("transaction body and witnesses must be maps");
  const redeemers = parseRedeemers(mapGet(witnesses, 5n));
  const utxos = parseUtxos(utxosBytes);
  const scripts = parseScripts(witnesses, utxos);
  const models = parseCostModels(costModelsBytes);
  if (runPhaseOne) {
    const supplied = new Set(redeemers.map(pointerKey));
    const needed = neededPointers(body, scripts, utxos);
    for (const pointer of supplied) if (!needed.has(pointer)) evaluateError(`extra redeemer ${pointer}`);
    for (const pointer of needed) if (!supplied.has(pointer)) evaluateError(`missing redeemer ${pointer}`);
  }
  const resolved = redeemers.map((redeemer) => ({
    redeemer,
    script: resolveScript(redeemer, body, scripts, utxos),
  }));
  const output: RawEvaluation[] = [];
  for (const { redeemer, script } of resolved) {
    assertLanguage(script.language, protocolMajor);
    const model = models.get(script.language);
    if (model === undefined) evaluateError(`missing Plutus V${script.language + 1} cost model`, redeemer);
    const program = decodeProgramEnvelopeCompatible(
      script.bytes,
      script.language < 2,
      {
        ...(protocolMajor >= 11
          ? { maxUniverseHeader: 32, maxConstrFields: 1_024 }
          : {}),
        enforceDataWireLimit: script.language !== 0,
      },
    );
    if (protocolMajor < 11 && program.version[1] >= 1n) {
      evaluateError("UPLC 1.1.0 is unavailable before protocol 11", redeemer);
    }
    validateBuiltins(program.term, maximumBuiltin(script.language, protocolMajor), redeemer);
    const context = makeScriptContext(
      redeemer,
      redeemers,
      body,
      witnesses,
      utxos,
      slotConfig,
      protocolMajor,
      script.language,
    );
    const arguments_ = script.language === 2
      ? [context]
      : legacyArguments(redeemer, context, body, utxos, witnesses);
    try {
      const result = evaluateProgram(
        program,
        arguments_,
        maximum,
        machineCosts(model, script.language),
        semanticsVariant(script.language, protocolMajor),
      );
      if (script.language === 2 && !result.isUnit) evaluateError("Plutus V3 script returned a non-Unit value", redeemer);
      output.push({
        redeemer: encodeRedeemer(redeemer, result.budget),
        cost: result.budget,
        logs: result.logs,
      });
    } catch (cause) {
      if (cause instanceof CardanoError && cause.path.length !== 0) throw cause;
      throw new CardanoError("EVALUATE", `redeemer ${pointerKey(redeemer)} evaluation failed`, {
        cause,
        path: ["redeemers", redeemer.tag, Number(redeemer.index)],
      });
    }
  }
  return Object.freeze(output.map((item) => Object.freeze({
    redeemer: Uint8Array.from(item.redeemer),
    cost: Object.freeze({ ...item.cost }),
    logs: Object.freeze([...item.logs]),
  })));
}

function parseRedeemers(value: CborValue | undefined): Redeemer[] {
  if (value === undefined) return [];
  const output: Redeemer[] = [];
  if (value.kind === "array") {
    for (const item of value.values) {
      if (item.kind !== "array" || item.values.length !== 4) malformed("redeemer must be [tag,index,data,ex_units]");
      output.push(readRedeemer(item.values[0], item.values[1], item.values[2]));
    }
  } else if (value.kind === "map") {
    for (const [key, item] of value.entries) {
      if (key.kind !== "array" || key.values.length !== 2 || item.kind !== "array" || item.values.length !== 2) {
        malformed("map redeemer must be [tag,index] => [data,ex_units]");
      }
      output.push(readRedeemer(key.values[0], key.values[1], item.values[0]));
    }
  } else malformed("redeemers must be an array or map");
  const seen = new Set<string>();
  for (const redeemer of output) {
    const key = pointerKey(redeemer);
    if (seen.has(key)) malformed(`duplicate redeemer ${key}`);
    seen.add(key);
  }
  return output.sort((left, right) => left.tag - right.tag || compareBigint(left.index, right.index));
}

function readRedeemer(tag: CborValue | undefined, index: CborValue | undefined, data: CborValue | undefined): Redeemer {
  if (tag?.kind !== "unsigned" || tag.value > 5n || index?.kind !== "unsigned" || data === undefined) {
    malformed("invalid redeemer pointer");
  }
  return { tag: Number(tag.value), index: index.value, data };
}

function parseScripts(
  witnesses: Extract<CborValue, { kind: "map" }>,
  utxos: readonly Utxo[],
): Script[] {
  const scripts: Script[] = [];
  for (const [key, language] of [[3n, 0], [6n, 1], [7n, 2]] as const) {
    const value = mapGet(witnesses, key);
    if (value === undefined) continue;
    const values = setValues(value);
    if (values === undefined) malformed("Plutus script witnesses must be sets");
    for (const item of values) {
      if (item.kind !== "bytes") malformed("Plutus script witness must be bytes");
      const prefix = Uint8Array.of(language + 1);
      const hashInput = new Uint8Array(prefix.length + item.value.length);
      hashInput.set(prefix);
      hashInput.set(item.value, 1);
      scripts.push({
        language,
        bytes: Uint8Array.from(item.value),
        hash: blake2b224(hashInput),
      });
    }
  }
  for (const utxo of utxos) {
    if (utxo.output.kind !== "map") continue;
    const reference = mapGet(utxo.output, 3n);
    if (reference?.kind !== "tag" || reference.tag !== 24n || reference.value.kind !== "bytes") continue;
    const script = decode("reference script", reference.value.value);
    if (
      script.kind !== "array" ||
      script.values.length !== 2 ||
      script.values[0]?.kind !== "unsigned" ||
      script.values[0].value < 1n ||
      script.values[0].value > 3n ||
      script.values[1]?.kind !== "bytes"
    ) continue;
    const language = Number(script.values[0].value - 1n) as 0 | 1 | 2;
    const bytes = Uint8Array.from(script.values[1].value);
    const hashInput = new Uint8Array(bytes.length + 1);
    hashInput[0] = language + 1;
    hashInput.set(bytes, 1);
    const hash = blake2b224(hashInput);
    if (!scripts.some((candidate) => candidate.language === language && bytesEqual(candidate.hash, hash))) {
      scripts.push({ language, bytes, hash });
    }
  }
  return scripts;
}

function parseUtxos(values: readonly (readonly [Uint8Array, Uint8Array])[]): Utxo[] {
  const output: Utxo[] = [];
  for (const [inputBytes, outputBytes] of values) {
    const copiedInput = Uint8Array.from(inputBytes);
    if (output.some((item) => bytesEqual(item.inputBytes, copiedInput))) malformed("duplicate UTxO input");
    const input = decode("UTxO input", copiedInput);
    if (input.kind !== "array" || input.values.length !== 2) malformed("UTxO input must be [transaction_id,index]");
    const outputValue = decode("UTxO output", Uint8Array.from(outputBytes));
    if (outputValue.kind !== "array" && outputValue.kind !== "map") malformed("UTxO output must be an array or map");
    output.push({ inputBytes: copiedInput, input, output: outputValue });
  }
  return output;
}

function parseCostModels(bytes: Uint8Array): Map<number, readonly bigint[]> {
  const value = decode("cost models", bytes);
  if (value.kind !== "map") malformed("CostModels must be a CBOR map");
  const output = new Map<number, readonly bigint[]>();
  for (const [key, model] of value.entries) {
    if (key.kind !== "unsigned" || key.value > BigInt(Number.MAX_SAFE_INTEGER) || model.kind !== "array") {
      malformed("invalid CostModels entry");
    }
    const parameters = model.values.map((parameter) => {
      if (parameter.kind !== "unsigned" && parameter.kind !== "negative") malformed("cost parameter must be an integer");
      if (parameter.value < INT64_MIN || parameter.value > INT64_MAX) {
        throw new CardanoBoundsError("cost model parameter", INT64_MIN, INT64_MAX, parameter.value);
      }
      return parameter.value;
    });
    output.set(Number(key.value), Object.freeze(parameters));
  }
  return output;
}

function machineCosts(parameters: readonly bigint[], language: number): MachineCosts {
  const value = (index: number): bigint => parameters[index] ?? INT64_MAX;
  const pair = (index: number): MachineBudget => ({
    cpu: value(index),
    memory: value(index + 1),
  });
  const constrIndex = language === 0 ? 175 : language === 1 ? 185 : 193;
  return {
    apply: pair(17),
    builtin: pair(19),
    constant: pair(21),
    delay: pair(23),
    force: pair(25),
    lambda: pair(27),
    startup: pair(29),
    variable: pair(31),
    constr: pair(constrIndex),
    case_: pair(constrIndex + 2),
    costModel: {
      language: language as 0 | 1 | 2,
      parameters,
    },
  };
}

function resolveScript(
  redeemer: Redeemer,
  body: Extract<CborValue, { kind: "map" }>,
  scripts: readonly Script[],
  utxos: readonly Utxo[],
): Script {
  const hash = purposeScriptHash(redeemer, body, utxos);
  if (hash === undefined) evaluateError(`cannot resolve script purpose ${pointerKey(redeemer)}`, redeemer);
  const matches = scripts.filter((script) => bytesEqual(script.hash, hash));
  if (matches.length !== 1) evaluateError(matches.length === 0 ? "missing Plutus script" : "ambiguous Plutus script", redeemer);
  return matches[0] as Script;
}

function purposeScriptHash(
  redeemer: Redeemer,
  body: Extract<CborValue, { kind: "map" }>,
  utxos: readonly Utxo[],
): Uint8Array | undefined {
  switch (redeemer.tag) {
    case 0: {
      const inputs = mapGet(body, 0n);
      const inputValues = setValues(inputs);
      if (inputValues === undefined) return undefined;
      const sorted = [...inputValues].sort(compareCbor);
      const input = sorted[Number(redeemer.index)];
      if (input === undefined) return undefined;
      const encoded = encodeCbor(input, { mode: "canonical" });
      const utxo = utxos.find((candidate) => bytesEqual(encodeCbor(candidate.input, { mode: "canonical" }), encoded));
      if (utxo === undefined) evaluateError("missing spending UTxO", redeemer);
      return paymentScriptHash(utxo.output);
    }
    case 1: {
      const mint = mapGet(body, 9n);
      if (mint?.kind !== "map") return undefined;
      const policies = mint.entries.map(([policy]) => policy).filter((policy) => policy.kind === "bytes").sort(compareCbor);
      const policy = policies[Number(redeemer.index)];
      return policy?.kind === "bytes" && policy.value.length === 28 ? policy.value : undefined;
    }
    case 2: {
      const certificates = mapGet(body, 4n);
      const certificate = certificates?.kind === "array" ? certificates.values[Number(redeemer.index)] : undefined;
      return certificate === undefined ? undefined : firstScriptCredential(certificate);
    }
    case 3: {
      const withdrawals = mapGet(body, 5n);
      if (withdrawals?.kind !== "map") return undefined;
      const accounts = withdrawals.entries.map(([account]) => account).filter((account) => account.kind === "bytes").sort(compareCbor);
      const account = accounts[Number(redeemer.index)];
      return account?.kind === "bytes" && (account.value[0] ?? 0) >> 4 === 0xf ? account.value.slice(-28) : undefined;
    }
    case 4: {
      const voting = mapGet(body, 19n);
      if (voting?.kind !== "map") return undefined;
      const voters = voting.entries.map(([voter]) => voter).sort(compareCbor);
      const voter = voters[Number(redeemer.index)];
      return voter === undefined ? undefined : firstScriptCredential(voter);
    }
    case 5: {
      const proposals = mapGet(body, 20n);
      const proposal = proposals?.kind === "array" ? proposals.values[Number(redeemer.index)] : undefined;
      return proposal === undefined ? undefined : firstScriptCredential(proposal);
    }
    default: return undefined;
  }
}

function neededPointers(
  body: Extract<CborValue, { kind: "map" }>,
  scripts: readonly Script[],
  utxos: readonly Utxo[],
): Set<string> {
  const known = (hash: Uint8Array | undefined): boolean =>
    hash !== undefined && scripts.some((script) => bytesEqual(script.hash, hash));
  const output = new Set<string>();
  const inputs = mapGet(body, 0n);
  const inputValues = setValues(inputs);
  if (inputValues !== undefined) {
    const sorted = [...inputValues].sort(compareCbor);
    for (let index = 0; index < sorted.length; index += 1) {
      const input = sorted[index] as CborValue;
      const utxo = utxos.find((candidate) => compareCbor(candidate.input, input) === 0);
      if (utxo !== undefined && known(paymentScriptHash(utxo.output))) output.add(`0:${index}`);
    }
  }
  const mint = mapGet(body, 9n);
  if (mint?.kind === "map") {
    const policies = mint.entries.map(([policy]) => policy).filter((policy) => policy.kind === "bytes").sort(compareCbor);
    for (let index = 0; index < policies.length; index += 1) {
      const policy = policies[index];
      if (policy?.kind === "bytes" && known(policy.value)) output.add(`1:${index}`);
    }
  }
  collectIndexedCredentials(mapGet(body, 4n), 2, known, output);
  const withdrawals = mapGet(body, 5n);
  if (withdrawals?.kind === "map") {
    const accounts = withdrawals.entries.map(([account]) => account).filter((account) => account.kind === "bytes").sort(compareCbor);
    for (let index = 0; index < accounts.length; index += 1) {
      const account = accounts[index];
      const hash = account?.kind === "bytes" && (account.value[0] ?? 0) >> 4 === 0xf
        ? account.value.slice(-28)
        : undefined;
      if (known(hash)) output.add(`3:${index}`);
    }
  }
  const voting = mapGet(body, 19n);
  if (voting?.kind === "map") {
    const voters = voting.entries.map(([voter]) => voter).sort(compareCbor);
    for (let index = 0; index < voters.length; index += 1) {
      if (known(firstScriptCredential(voters[index] as CborValue))) output.add(`4:${index}`);
    }
  }
  collectIndexedCredentials(mapGet(body, 20n), 5, known, output);
  return output;
}

function collectIndexedCredentials(
  value: CborValue | undefined,
  tag: number,
  known: (hash: Uint8Array | undefined) => boolean,
  output: Set<string>,
): void {
  if (value?.kind !== "array") return;
  for (let index = 0; index < value.values.length; index += 1) {
    if (known(firstScriptCredential(value.values[index] as CborValue))) output.add(`${tag}:${index}`);
  }
}

function paymentScriptHash(output: CborValue): Uint8Array | undefined {
  const address = output.kind === "array"
    ? output.values[0]
    : output.kind === "map"
      ? mapGet(output, 0n)
      : undefined;
  if (address?.kind !== "bytes" || address.value.length < 29) return undefined;
  const kind = (address.value[0] ?? 0) >> 4;
  return kind === 1 || kind === 3 || kind === 5 || kind === 7 ? address.value.slice(1, 29) : undefined;
}

function firstScriptCredential(value: CborValue): Uint8Array | undefined {
  if (
    value.kind === "array" &&
    value.values.length === 2 &&
    value.values[0]?.kind === "unsigned" &&
    value.values[0].value === 1n &&
    value.values[1]?.kind === "bytes" &&
    value.values[1].value.length === 28
  ) return value.values[1].value;
  if (value.kind === "array") {
    for (const item of value.values) {
      const found = firstScriptCredential(item);
      if (found !== undefined) return found;
    }
  }
  if (value.kind === "map") {
    for (const [key, item] of value.entries) {
      const found = firstScriptCredential(key) ?? firstScriptCredential(item);
      if (found !== undefined) return found;
    }
  }
  if (value.kind === "tag") return firstScriptCredential(value.value);
  return undefined;
}

function legacyArguments(
  redeemer: Redeemer,
  context: CborValue,
  body: Extract<CborValue, { kind: "map" }>,
  utxos: readonly Utxo[],
  witnesses: Extract<CborValue, { kind: "map" }>,
): CborValue[] {
  if (redeemer.tag !== 0) return [redeemer.data, context];
  const inputs = mapGet(body, 0n);
  const inputValues = setValues(inputs);
  const input = inputValues === undefined ? undefined : [...inputValues].sort(compareCbor)[Number(redeemer.index)];
  const utxo = input === undefined ? undefined : utxos.find((candidate) => compareCbor(candidate.input, input) === 0);
  const datum = utxo === undefined ? undefined : spendingDatum(utxo.output, witnesses);
  return datum === undefined ? [redeemer.data, context] : [datum, redeemer.data, context];
}

function spendingDatum(
  output: CborValue,
  witnesses: Extract<CborValue, { kind: "map" }>,
): CborValue | undefined {
  if (output.kind === "map") {
    const option = mapGet(output, 2n);
    if (option?.kind === "array" && option.values[0]?.kind === "unsigned") {
      if (option.values[0].value === 1n) return option.values[1];
      if (option.values[0].value === 0n && option.values[1]?.kind === "bytes") {
        return findDatum(option.values[1].value, witnesses);
      }
    }
  }
  if (output.kind === "array" && output.values[2]?.kind === "bytes") return findDatum(output.values[2].value, witnesses);
  return undefined;
}

function findDatum(hash: Uint8Array, witnesses: Extract<CborValue, { kind: "map" }>): CborValue | undefined {
  const datums = mapGet(witnesses, 4n);
  const values = setValues(datums);
  if (values === undefined) return undefined;
  return values.find((datum) => bytesEqual(blake2b256(encodePlutusData(datum)), hash));
}

function encodeRedeemer(redeemer: Redeemer, budget: MachineBudget): Uint8Array {
  return encodeCbor({
    kind: "array",
    values: [
      unsignedNode(BigInt(redeemer.tag)),
      unsignedNode(redeemer.index),
      redeemer.data,
      { kind: "array", values: [unsignedNode(budget.memory), unsignedNode(budget.cpu)], encoding: definite },
    ],
    encoding: definite,
  }, { mode: "canonical" });
}

function validateBuiltins(term: import("../uplc/ast.js").UplcTerm, maximum: number, redeemer: Redeemer): void {
  const stack = [term];
  while (stack.length !== 0) {
    const current = stack.pop();
    if (current === undefined) break;
    if (current.kind === "builtin" && current.tag > maximum) {
      evaluateError(`builtin ${current.tag} is unavailable for this language/protocol`, redeemer);
    }
    if (current.kind === "delay" || current.kind === "force") stack.push(current.term);
    else if (current.kind === "lambda") stack.push(current.body);
    else if (current.kind === "apply") stack.push(current.function, current.argument);
    else if (current.kind === "constr") stack.push(...current.fields);
    else if (current.kind === "case") stack.push(current.scrutinee, ...current.branches);
  }
}

function maximumBuiltin(language: number, protocol: number): number {
  if (language === 0) return protocol >= 9 ? (protocol >= 11 ? 100 : 87) : 50;
  if (language === 1) return protocol >= 11 ? 100 : protocol >= 9 ? 87 : 53;
  return protocol >= 11 ? 100 : 87;
}

function semanticsVariant(language: number, protocol: number): "A" | "B" | "C" | "D" | "E" {
  if (protocol < 9) return "A";
  if (protocol < 11) return language === 2 ? "C" : "B";
  return language === 2 ? "E" : "D";
}

function assertLanguage(language: number, protocol: number): void {
  const minimum = language === 0 ? 5 : language === 1 ? 7 : 9;
  if (protocol < minimum) throw new CardanoError("UNSUPPORTED", `Plutus V${language + 1} is unavailable at protocol ${protocol}`);
}

function assertProtocol(protocol: number): void {
  if (!Number.isSafeInteger(protocol) || protocol < 5 || protocol > 11) {
    throw new CardanoError("UNSUPPORTED", `protocol major ${protocol} is unsupported`);
  }
}

function assertBudget(budget: MachineBudget): void {
  for (const [name, value] of [["CPU budget", budget.cpu], ["memory budget", budget.memory]] as const) {
    if (value < 0n || value > INT64_MAX) throw new CardanoBoundsError(name, 0n, INT64_MAX, value);
  }
}

function decode(label: string, bytes: Uint8Array): CborValue {
  try { return decodeCbor(Uint8Array.from(bytes)); }
  catch (cause) {
    if (cause instanceof DeserializeError) throw cause;
    throw new DeserializeError("INVALID_CBOR", `invalid ${label} CBOR`, { cause });
  }
}

function mapGet(map: Extract<CborValue, { kind: "map" }>, key: bigint): CborValue | undefined {
  return map.entries.find(([candidate]) => candidate.kind === "unsigned" && candidate.value === key)?.[1];
}

function setValues(value: CborValue | undefined): readonly CborValue[] | undefined {
  if (value?.kind === "array") return value.values;
  if (value?.kind === "tag" && value.tag === 258n && value.value.kind === "array") {
    return value.value.values;
  }
  return undefined;
}

function unsignedNode(value: bigint): CborValue {
  if (value < 0n) throw new RangeError("expected unsigned integer");
  return { kind: "unsigned", value, encoding: { width: 0 } };
}

function integerNode(value: bigint): CborValue {
  return value >= 0n ? unsignedNode(value) : { kind: "negative", value, encoding: { width: 0 } };
}

function pointerKey(redeemer: Redeemer): string {
  return `${redeemer.tag}:${redeemer.index}`;
}

function compareBigint(left: bigint, right: bigint): number {
  return left < right ? -1 : left > right ? 1 : 0;
}

function compareCbor(left: CborValue, right: CborValue): number {
  const a = encodeCbor(left, { mode: "canonical" });
  const b = encodeCbor(right, { mode: "canonical" });
  const length = Math.min(a.length, b.length);
  for (let index = 0; index < length; index += 1) {
    const difference = (a[index] ?? 0) - (b[index] ?? 0);
    if (difference !== 0) return difference;
  }
  return a.length - b.length;
}

function malformed(message: string): never {
  throw new DeserializeError("INVALID_STRUCTURE", message);
}

function evaluateError(message: string, redeemer?: Redeemer): never {
  throw new CardanoError("EVALUATE", message, redeemer === undefined
    ? {}
    : { path: ["redeemers", redeemer.tag, Number(redeemer.index)] });
}
