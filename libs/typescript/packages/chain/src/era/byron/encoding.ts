const alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
const alphabetIndex = new Map([...alphabet].map((character, index) => [character, index]));

export function encodeBase58(bytes: Uint8Array): string {
  const digits = [0];
  for (const byte of bytes) {
    let carry = byte;
    for (let index = 0; index < digits.length; index += 1) {
      carry += (digits[index] ?? 0) << 8;
      digits[index] = carry % 58;
      carry = Math.floor(carry / 58);
    }
    while (carry > 0) {
      digits.push(carry % 58);
      carry = Math.floor(carry / 58);
    }
  }
  let output = "";
  let offset = 0;
  while (offset < bytes.length && bytes[offset] === 0) {
    output += "1";
    offset += 1;
  }
  for (let index = digits.length - 1; index >= 0; index -= 1) output += alphabet[digits[index] ?? 0];
  return output;
}

export function decodeBase58(text: string): Uint8Array {
  const bytes = [0];
  let zeroCount = 0;
  while (zeroCount < text.length && text[zeroCount] === "1") zeroCount += 1;
  for (let index = zeroCount; index < text.length; index += 1) {
    const digit = alphabetIndex.get(text[index] as string);
    if (digit === undefined) throw new TypeError(`invalid Base58 character at ${index}`);
    let carry = digit;
    for (let byteIndex = 0; byteIndex < bytes.length; byteIndex += 1) {
      carry += (bytes[byteIndex] ?? 0) * 58;
      bytes[byteIndex] = carry & 255;
      carry >>= 8;
    }
    while (carry > 0) {
      bytes.push(carry & 255);
      carry >>= 8;
    }
  }
  let leadingZeros = 0;
  for (let index = bytes.length - 1; index >= 0 && bytes[index] === 0; index -= 1) leadingZeros += 1;
  if (zeroCount > leadingZeros) {
    const addition = leadingZeros > 0 ? zeroCount - leadingZeros - 1 : zeroCount;
    for (let index = 0; index < addition; index += 1) bytes.push(0);
  }
  bytes.reverse();
  return Uint8Array.from(bytes);
}

const crcTable = Array.from({ length: 256 }, (_, index) => {
  let value = index;
  for (let bit = 0; bit < 8; bit += 1) {
    value = (value & 1) !== 0 ? 0xedb8_8320 ^ (value >>> 1) : value >>> 1;
  }
  return value >>> 0;
});

export function crc32(bytes: Uint8Array, initial = 0): number {
  let crc = (initial ^ 0xffff_ffff) >>> 0;
  for (const byte of bytes) crc = ((crcTable[(crc ^ byte) & 255] ?? 0) ^ (crc >>> 8)) >>> 0;
  return (crc ^ 0xffff_ffff) >>> 0;
}

export class Crc32 {
  #value = 0;

  private constructor() {}

  public static new(): Crc32 {
    return new Crc32();
  }

  public update(bytes: Uint8Array): void {
    this.#value = crc32(bytes, this.#value);
  }

  public finalize(): number {
    return this.#value >>> 0;
  }
}
