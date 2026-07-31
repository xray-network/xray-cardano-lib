const alphabet = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
const alphabetIndex = new Map([...alphabet].map((character, index) => [character, index]));

function polymod(values: readonly number[]): number {
  const generators = [0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3];
  let checksum = 1;
  for (const value of values) {
    const top = checksum >>> 25;
    checksum = ((checksum & 0x1ff_ffff) << 5) ^ value;
    for (let index = 0; index < 5; index += 1) {
      if (((top >>> index) & 1) !== 0) checksum ^= generators[index] ?? 0;
    }
  }
  return checksum >>> 0;
}

function expand(prefix: string): number[] {
  return [...prefix].map((character) => character.charCodeAt(0) >>> 5)
    .concat([0], [...prefix].map((character) => character.charCodeAt(0) & 31));
}

function convertBits(values: readonly number[], from: number, to: number, pad: boolean): number[] {
  let accumulator = 0;
  let bits = 0;
  const output: number[] = [];
  const mask = (1 << to) - 1;
  for (const value of values) {
    if (value < 0 || value >>> from !== 0) throw new TypeError("Invalid Bech32 data value");
    accumulator = (accumulator << from) | value;
    bits += from;
    while (bits >= to) { bits -= to; output.push((accumulator >>> bits) & mask); }
  }
  if (pad && bits > 0) output.push((accumulator << (to - bits)) & mask);
  else if (!pad && (bits >= from || ((accumulator << (to - bits)) & mask) !== 0)) {
    throw new TypeError("Invalid Bech32 padding");
  }
  return output;
}

export function encodeBech32(prefix: string, bytes: Uint8Array): string {
  if (!/^[!-~]+$/u.test(prefix) || prefix !== prefix.toLowerCase()) throw new TypeError("Invalid Bech32 prefix");
  const words = convertBits([...bytes], 8, 5, true);
  const checksum = polymod(expand(prefix).concat(words, [0, 0, 0, 0, 0, 0])) ^ 1;
  const suffix = Array.from({ length: 6 }, (_, index) => (checksum >>> (5 * (5 - index))) & 31);
  return `${prefix}1${words.concat(suffix).map((value) => alphabet[value]).join("")}`;
}

export function decodeBech32(value: string): { readonly prefix: string; readonly bytes: Uint8Array } {
  if (value !== value.toLowerCase() && value !== value.toUpperCase()) throw new TypeError("Mixed-case Bech32 string");
  const normalized = value.toLowerCase();
  const separator = normalized.lastIndexOf("1");
  if (separator < 1 || separator + 7 > normalized.length) throw new TypeError("Invalid Bech32 string");
  const prefix = normalized.slice(0, separator);
  const words = [...normalized.slice(separator + 1)].map((character) => {
    const word = alphabetIndex.get(character);
    if (word === undefined) throw new TypeError("Invalid Bech32 character");
    return word;
  });
  if (polymod(expand(prefix).concat(words)) !== 1) throw new TypeError("Invalid Bech32 checksum");
  return { prefix, bytes: Uint8Array.from(convertBits(words.slice(0, -6), 5, 8, false)) };
}
