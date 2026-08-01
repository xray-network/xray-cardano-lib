import type { CborValue } from "@xray-network/xray-cardano-lib-core";

export type CborValidator = (node: CborValue, path?: string) => void;

export function invalid(path: string, expected: string): never {
  throw new TypeError(`${path} requires ${expected}`);
}

export function unsigned(maximum?: bigint): CborValidator {
  return (node, path = "value") => {
    if (node.kind !== "unsigned" || (maximum !== undefined && node.value > maximum)) {
      invalid(path, maximum === undefined ? "an unsigned integer" : `an unsigned integer <= ${maximum}`);
    }
  };
}

export function signed(minimum?: bigint, maximum?: bigint): CborValidator {
  return (node, path = "value") => {
    if (node.kind !== "unsigned" && node.kind !== "negative") invalid(path, "an integer");
    if (
      (minimum !== undefined && node.value < minimum) ||
      (maximum !== undefined && node.value > maximum)
    ) {
      invalid(path, `an integer in ${minimum ?? "-infinity"}..${maximum ?? "infinity"}`);
    }
  };
}

export function bytes(length?: number | readonly [number, number]): CborValidator {
  return (node, path = "value") => {
    if (node.kind !== "bytes") invalid(path, "CBOR bytes");
    if (length === undefined) return;
    const valid = typeof length === "number"
      ? node.value.length === length
      : node.value.length >= length[0] && node.value.length <= length[1];
    if (!valid) {
      invalid(path, typeof length === "number"
        ? `${length}-byte CBOR bytes`
        : `${length[0]}..${length[1]} byte CBOR bytes`);
    }
  };
}

export function text(maximumUtf8Bytes?: number): CborValidator {
  return (node, path = "value") => {
    if (node.kind !== "text") invalid(path, "CBOR text");
    if (
      maximumUtf8Bytes !== undefined &&
      new TextEncoder().encode(node.value).length > maximumUtf8Bytes
    ) {
      invalid(path, `text no longer than ${maximumUtf8Bytes} UTF-8 bytes`);
    }
  };
}

export const boolean: CborValidator = (node, path = "value") => {
  if (node.kind !== "boolean") invalid(path, "a boolean");
};

export const nullValue: CborValidator = (node, path = "value") => {
  if (node.kind !== "null") invalid(path, "null");
};

export function optional(validator: CborValidator): CborValidator {
  return (node, path = "value") => {
    if (node.kind !== "null") validator(node, path);
  };
}

export function oneOf(...validators: readonly CborValidator[]): CborValidator {
  return (node, path = "value") => {
    for (const validator of validators) {
      try {
        validator(node, path);
        return;
      } catch (error) {
        if (!(error instanceof TypeError || error instanceof RangeError)) throw error;
      }
    }
    invalid(path, "one of the allowed CDDL choices");
  };
}

export function arrayOf(
  validator: CborValidator,
  minimum = 0,
  maximum = Number.MAX_SAFE_INTEGER,
): CborValidator {
  return (node, path = "value") => {
    if (node.kind !== "array" || node.values.length < minimum || node.values.length > maximum) {
      invalid(path, `an array with ${minimum}..${maximum} entries`);
    }
    node.values.forEach((item, index) => validator(item, `${path}[${index}]`));
  };
}

export function tuple(...validators: readonly CborValidator[]): CborValidator {
  return (node, path = "value") => {
    if (node.kind !== "array" || node.values.length !== validators.length) {
      invalid(path, `an array with exactly ${validators.length} entries`);
    }
    validators.forEach((validator, index) => validator(node.values[index] as CborValue, `${path}[${index}]`));
  };
}

export function tupleRange(
  validators: readonly CborValidator[],
  minimum: number,
  maximum = validators.length,
): CborValidator {
  return (node, path = "value") => {
    if (node.kind !== "array" || node.values.length < minimum || node.values.length > maximum) {
      invalid(path, `an array with ${minimum}..${maximum} entries`);
    }
    node.values.forEach((item, index) => {
      const validator = validators[index];
      if (validator !== undefined) validator(item, `${path}[${index}]`);
    });
  };
}

export function tagged(tag: bigint, validator: CborValidator): CborValidator {
  return (node, path = "value") => {
    if (node.kind !== "tag" || node.tag !== tag) invalid(path, `CBOR tag ${tag}`);
    validator(node.value, `${path}.value`);
  };
}

export function discriminated(
  variants: Readonly<Record<number, readonly CborValidator[]>>,
): CborValidator {
  return (node, path = "value") => {
    if (node.kind !== "array" || node.values[0]?.kind !== "unsigned") {
      invalid(path, "a discriminated CBOR array");
    }
    const tag = Number(node.values[0].value);
    const fields = variants[tag];
    if (fields === undefined || node.values.length !== fields.length + 1) {
      invalid(path, `a supported discriminator and exact field count`);
    }
    fields.forEach((validator, index) => validator(
      node.values[index + 1] as CborValue,
      `${path}[${index + 1}]`,
    ));
  };
}

export function mapOf(
  keyValidator: CborValidator,
  valueValidator: CborValidator,
  minimum = 0,
): CborValidator {
  return (node, path = "value") => {
    if (node.kind !== "map" || node.entries.length < minimum) {
      invalid(path, `a map with at least ${minimum} entries`);
    }
    node.entries.forEach(([key, value], index) => {
      keyValidator(key, `${path}.key[${index}]`);
      valueValidator(value, `${path}.value[${index}]`);
    });
  };
}

export interface MapField {
  readonly required?: boolean;
  readonly validate: CborValidator;
}

export function fixedMap(
  fields: Readonly<Record<number, MapField>>,
  allowUnknown = false,
): CborValidator {
  return (node, path = "value") => {
    if (node.kind !== "map") invalid(path, "a CBOR map");
    const present = new Set<number>();
    node.entries.forEach(([key, value], index) => {
      if (key.kind !== "unsigned" || key.value > BigInt(Number.MAX_SAFE_INTEGER)) {
        invalid(`${path}.key[${index}]`, "an unsigned integer field key");
      }
      const numericKey = Number(key.value);
      const field = fields[numericKey];
      if (field === undefined) {
        if (!allowUnknown) invalid(`${path}.key[${index}]`, "a supported field key");
        return;
      }
      present.add(numericKey);
      field.validate(value, `${path}.${numericKey}`);
    });
    for (const [key, field] of Object.entries(fields)) {
      if (field.required === true && !present.has(Number(key))) {
        invalid(path, `mandatory field ${key}`);
      }
    }
  };
}

export function exactUnsigned(expected: bigint): CborValidator {
  return (node, path = "value") => {
    if (node.kind !== "unsigned" || node.value !== expected) invalid(path, `unsigned integer ${expected}`);
  };
}

export const uint8 = unsigned(0xffn);
export const uint16 = unsigned(0xffffn);
export const uint32 = unsigned(0xffff_ffffn);
export const uint64 = unsigned(0xffff_ffff_ffff_ffffn);
export const int32 = signed(-0x8000_0000n, 0x7fff_ffffn);
export const int64 = signed(-0x8000_0000_0000_0000n, 0x7fff_ffff_ffff_ffffn);
export const hash28 = bytes(28);
export const hash32 = bytes(32);
export const signature64 = bytes(64);
export const anyCbor: CborValidator = () => {};

export function taggedSet(item: CborValidator, nonempty = false): CborValidator {
  const values = arrayOf(item, nonempty ? 1 : 0);
  return oneOf(values, tagged(258n, values));
}

export function rational(unit = false): CborValidator {
  return tagged(30n, (node, path = "value") => {
    tuple(unsigned(), unsigned())(node, path);
    if (node.kind !== "array") return;
    const numerator = node.values[0];
    const denominator = node.values[1];
    if (
      numerator?.kind !== "unsigned" ||
      denominator?.kind !== "unsigned" ||
      denominator.value === 0n ||
      (unit && numerator.value > denominator.value)
    ) {
      invalid(path, unit ? "a rational in 0..1 with positive denominator" : "a rational with positive denominator");
    }
  });
}
