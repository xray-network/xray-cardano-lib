import { bytesToHex } from "@xray-network/xray-cardano-lib-core";
import { blake2b512 } from "@xray-network/xray-cardano-lib-crypto";

const CHECKSUM_ALPHABET = "ABCDEJHKLNOPSTXZ";
const PERSONALIZATION = new TextEncoder().encode("wallets checksum");
const PUBLIC_KEY_HASH = /^[0-9a-f]{56}$/u;

export interface CIP4Checksum {
  readonly checksumId: string;
  readonly checksumImage: string;
}

function fnv1a32(value: string): number {
  let hash = 0x811c_9dc5;
  for (let index = 0; index < value.length; index += 1) {
    hash ^= value.charCodeAt(index);
    hash = Math.imul(hash, 0x0100_0193) >>> 0;
  }
  return hash;
}

function checksumId(checksumImage: string): string {
  const hash = fnv1a32(checksumImage);
  const a = hash & 0xff;
  const b = hash >>> 8 & 0xff;
  const c = hash >>> 16 & 0xff;
  const d = hash >>> 24 & 0xff;
  const letters = (value: number): string =>
    `${CHECKSUM_ALPHABET[Math.floor(value / 16)]}${CHECKSUM_ALPHABET[value % 16]}`;
  const numbers = `${((c << 8) + d) % 10_000}`.padStart(4, "0");
  return `${letters(a)}${letters(b)}-${numbers}`;
}

export class CIP4 {
  private constructor() {}

  public static calculateChecksum(publicKeyHashHex: string): CIP4Checksum {
    if (!PUBLIC_KEY_HASH.test(publicKeyHashHex)) {
      throw new TypeError("CIP4 public key hash must be 28-byte lowercase hexadecimal");
    }
    const checksumImage = bytesToHex(
      blake2b512(new TextEncoder().encode(publicKeyHashHex), PERSONALIZATION),
    );
    return {
      checksumId: checksumId(checksumImage),
      checksumImage,
    };
  }
}
