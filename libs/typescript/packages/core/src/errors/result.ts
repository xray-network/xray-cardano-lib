import { CardanoError } from "./cardano-error.js";
import type { CardanoResult } from "./types.js";

export function resultOrThrow<Value, Failure extends Error>(
  result: CardanoResult<Value, Failure>,
): Value {
  if (result.ok) return result.value;
  throw result.error;
}

export function unknownToError(error: unknown): Error {
  return error instanceof Error
    ? error
    : new CardanoError("UNEXPECTED", "A non-Error value was thrown", { cause: error });
}
