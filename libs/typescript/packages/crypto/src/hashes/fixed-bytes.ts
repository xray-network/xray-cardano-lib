import {
  assertByteLength,
  bytesEqual,
  bytesToHex,
  copyBytes,
  hexToBytes,
} from "@xray-network/xray-cardano-lib-core";
import {
  decodeBech32 as decodeBech32Value,
  encodeBech32 as encodeBech32Value,
} from "@xray-network/xray-cardano-lib-core/bech32";

export function encodeBech32(prefix: string, bytes: Uint8Array): string { return encodeBech32Value(prefix,bytes); }
export function decodeBech32WithPrefix(value: string): { readonly prefix:string;readonly bytes:Uint8Array } { return decodeBech32Value(value); }
export function decodeBech32(value: string): Uint8Array { return decodeBech32Value(value).bytes; }

const states = new WeakMap<FixedBytes, Uint8Array>();

export abstract class FixedBytes {
  protected constructor(bytes: Uint8Array, expected: number, name: string) {
    assertByteLength(name, bytes, expected);
    states.set(this, copyBytes(bytes));
  }
  public to_raw_bytes(): Uint8Array { return copyBytes(state(this)); }
  public to_hex(): string { return bytesToHex(state(this)); }
  public to_bech32(prefix: string): string { return encodeBech32(prefix, state(this)); }
  public equals(other: FixedBytes): boolean { return bytesEqual(state(this), state(other)); }
  protected static raw(hex: string): Uint8Array { return hexToBytes(hex); }
  protected static bech32(value: string): Uint8Array { return decodeBech32(value); }
}

function state(value: FixedBytes): Uint8Array {
  const found = states.get(value);
  if (found === undefined) throw new TypeError("Invalid fixed-byte receiver");
  return found;
}
