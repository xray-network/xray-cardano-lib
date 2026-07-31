import { CardanoBoundsError } from "../errors/index.js";

export function copyBytes(bytes: Uint8Array): Uint8Array {
  return bytes.slice();
}

export function bytesEqual(left: Uint8Array, right: Uint8Array): boolean {
  if (left.length !== right.length) {
    return false;
  }

  for (let index = 0; index < left.length; index += 1) {
    if (left[index] !== right[index]) {
      return false;
    }
  }

  return true;
}

export function bytesToHex(bytes: Uint8Array): string {
  let result = "";
  for (const byte of bytes) {
    result += byte.toString(16).padStart(2, "0");
  }
  return result;
}

export function hexToBytes(hex: string): Uint8Array {
  if (hex.length % 2 !== 0 || !/^[0-9a-fA-F]*$/u.test(hex)) {
    throw new TypeError("Hex input must contain an even number of hexadecimal characters");
  }
  const result = new Uint8Array(hex.length / 2);
  for (let index = 0; index < result.length; index += 1) {
    result[index] = Number.parseInt(hex.slice(index * 2, index * 2 + 2), 16);
  }
  return result;
}

export function assertByteLength(
  name: string,
  bytes: Uint8Array,
  expectedLength: number,
): void {
  if (bytes.length !== expectedLength) {
    throw new CardanoBoundsError(
      name,
      BigInt(expectedLength),
      BigInt(expectedLength),
      BigInt(bytes.length),
    );
  }
}
