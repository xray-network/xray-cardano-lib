import { CardanoBoundsError } from "../errors/index.js";
import type { Cloneable } from "./types.js";

export function assertBigIntInRange(
  name: string,
  value: bigint,
  minimum: bigint,
  maximum: bigint,
): bigint {
  if (value < minimum || value > maximum) {
    throw new CardanoBoundsError(name, minimum, maximum, value);
  }

  return value;
}

export function cloneValue<Value>(value: Cloneable<Value>): Value {
  return value.clone();
}
