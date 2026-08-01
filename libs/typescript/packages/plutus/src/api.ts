import {
  CardanoBoundsError,
  DeserializeError,
  decodeCbor,
  encodeCbor,
  type CborValue,
} from "@xray-network/xray-cardano-lib-core";
import { evaluateRaw } from "./ledger/evaluate.js";
import { dataConstant, type UplcTerm } from "./uplc/ast.js";
import {
  decodeProgramEnvelope,
  encodeProgramEnvelope,
  validatePlutusDataNode,
} from "./uplc/flat.js";

const MAX_STANDALONE_BYTES = 16 * 1024 * 1024;

export interface UplcExBudget {
  readonly cpu: bigint;
  readonly memory: bigint;
}

export interface PhaseTwoEvaluation {
  readonly cost: UplcExBudget;
  readonly logs: readonly string[];
}

export type PhaseTwoRawEvaluation = readonly [
  redeemerBytes: Uint8Array,
  evaluation: PhaseTwoEvaluation,
];

export function apply_params_to_script(
  paramsBytes: Uint8Array,
  plutusScriptBytes: Uint8Array,
): Uint8Array {
  assertStandaloneLength("parameter bytes", paramsBytes);
  assertStandaloneLength("script bytes", plutusScriptBytes);
  let parameters: CborValue;
  try {
    parameters = decodeCbor(Uint8Array.from(paramsBytes));
  } catch (cause) {
    if (cause instanceof DeserializeError) throw cause;
    throw new DeserializeError("INVALID_CBOR", "invalid parameter CBOR", { cause });
  }
  if (parameters.kind !== "array") {
    throw new DeserializeError("INVALID_STRUCTURE", "script parameters must be a CBOR array");
  }
  const program = decodeProgramEnvelope(Uint8Array.from(plutusScriptBytes));
  let term: UplcTerm = program.term;
  for (const parameter of parameters.values) {
    validatePlutusDataNode(parameter);
    term = {
      kind: "apply",
      function: term,
      argument: { kind: "constant", constant: dataConstant(parameter) },
    };
  }
  assertClosed(term);
  return Uint8Array.from(encodeProgramEnvelope({ version: program.version, term }));
}

export function eval_phase_two_raw(
  txBytes: Uint8Array,
  utxosBytes: readonly (readonly [inputBytes: Uint8Array, outputBytes: Uint8Array])[],
  costModelsBytes: Uint8Array,
  maxBudget: readonly [cpu: bigint, memory: bigint],
  slotConfig: readonly [
    zeroTimeMilliseconds: bigint,
    zeroSlot: bigint,
    slotLengthMilliseconds: bigint,
  ],
  protocolMajorVersion: number,
  runPhaseOne: boolean,
): readonly PhaseTwoRawEvaluation[] {
  const results = evaluateRaw(
    Uint8Array.from(txBytes),
    utxosBytes.map(([input, output]) => [Uint8Array.from(input), Uint8Array.from(output)] as const),
    Uint8Array.from(costModelsBytes),
    { cpu: maxBudget[0], memory: maxBudget[1] },
    [...slotConfig],
    protocolMajorVersion,
    runPhaseOne,
  );
  return Object.freeze(results.map((result) => Object.freeze([
    Uint8Array.from(result.redeemer),
    Object.freeze({
      cost: Object.freeze({ cpu: result.cost.cpu, memory: result.cost.memory }),
      logs: Object.freeze([...result.logs]),
    }),
  ] as const)));
}

function assertStandaloneLength(name: string, value: Uint8Array): void {
  if (value.length > MAX_STANDALONE_BYTES) {
    throw new CardanoBoundsError(name, 0n, BigInt(MAX_STANDALONE_BYTES), BigInt(value.length));
  }
}

function assertClosed(term: UplcTerm): void {
  const stack: Array<readonly [UplcTerm, bigint]> = [[term, 0n]];
  while (stack.length !== 0) {
    const [current, depth] = stack.pop() as readonly [UplcTerm, bigint];
    switch (current.kind) {
      case "var":
        if (current.index < 1n || current.index > depth) {
          throw new DeserializeError("INVALID_STRUCTURE", "UPLC program contains a free variable");
        }
        break;
      case "lambda": stack.push([current.body, depth + 1n]); break;
      case "delay":
      case "force": stack.push([current.term, depth]); break;
      case "apply": stack.push([current.argument, depth], [current.function, depth]); break;
      case "constr": for (const field of current.fields) stack.push([field, depth]); break;
      case "case":
        stack.push([current.scrutinee, depth]);
        for (const branch of current.branches) stack.push([branch, depth]);
        break;
      default: break;
    }
  }
}
