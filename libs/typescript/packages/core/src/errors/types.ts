import type { CardanoError } from "./cardano-error.js";

export type CardanoErrorCode =
  | "BOUNDS"
  | "DESERIALIZE"
  | "EVALUATE"
  | "INVARIANT"
  | "PLATFORM"
  | "SERIALIZE"
  | "UNSUPPORTED"
  | "UNEXPECTED";

export type ErrorPathComponent = string | number;

export interface CardanoErrorOptions {
  readonly cause?: unknown;
  readonly path?: readonly ErrorPathComponent[];
}

export type DeserializeFailure =
  | "BREAK_IN_DEFINITE_LENGTH"
  | "DEPTH_LIMIT_EXCEEDED"
  | "DUPLICATE_KEY"
  | "ENDING_BREAK_MISSING"
  | "FIXED_VALUE_MISMATCH"
  | "INVALID_CBOR"
  | "INVALID_STRUCTURE"
  | "NO_VARIANT_MATCHED"
  | "OUT_OF_RANGE"
  | "RANGE_CHECK"
  | "TAG_MISMATCH"
  | "TRAILING_DATA"
  | "TRUNCATED_INPUT";

export type CardanoResult<Value, Failure extends Error = CardanoError> =
  | { readonly ok: true; readonly value: Value }
  | { readonly ok: false; readonly error: Failure };
