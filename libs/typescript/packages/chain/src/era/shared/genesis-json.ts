export type JsonRecord = Record<string, unknown>;

export function record(value: unknown, name: string): JsonRecord {
  if (typeof value !== "object" || value === null || Array.isArray(value)) {
    throw new TypeError(`${name} must be an object`);
  }
  return value as JsonRecord;
}

export function stringValue(value: unknown, name: string): string {
  if (typeof value !== "string") throw new TypeError(`${name} must be a string`);
  return value;
}

export function integer(value: unknown, name: string): number {
  if (typeof value !== "number" || !Number.isSafeInteger(value) || value < 0) {
    throw new TypeError(`${name} must be a non-negative safe integer`);
  }
  return value;
}

export function bigintValue(value: unknown, name: string): bigint {
  const text = typeof value === "string" || typeof value === "number" ? String(value) : "";
  if (!/^\d+$/u.test(text)) throw new TypeError(`${name} must be an unsigned integer`);
  return BigInt(text);
}

export function base64(value: string, urlSafe = false): Uint8Array {
  const normalized = (
    urlSafe ? value.replace(/-/gu, "+").replace(/_/gu, "/") : value
  ).padEnd(Math.ceil(value.length / 4) * 4, "=");
  let binary: string;
  try {
    binary = atob(normalized);
  } catch {
    throw new TypeError("invalid Base64 value");
  }
  return Uint8Array.from(binary, (character) => character.charCodeAt(0));
}
