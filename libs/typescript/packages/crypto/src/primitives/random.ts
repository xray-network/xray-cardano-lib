import {
  CardanoBoundsError,
  CardanoError,
} from "@xray-network/cardano-core";
import type { SecureRandomSource } from "@xray-network/cardano-core";

export function systemSecureRandomSource(): SecureRandomSource {
  const webCrypto = globalThis.crypto;
  if (typeof webCrypto === "undefined") {
    throw new CardanoError("PLATFORM", "Web Crypto getRandomValues is unavailable");
  }

  return {
    fill(target: Uint8Array): void {
      webCrypto.getRandomValues(target);
    },
  };
}

export function secureRandomBytes(length: number, source?: SecureRandomSource): Uint8Array {
  if (!Number.isSafeInteger(length) || length < 0 || length > 65_536) {
    const actual = Number.isFinite(length) && Number.isInteger(length) ? BigInt(length) : -1n;
    throw new CardanoBoundsError("random byte length", 0n, 65_536n, actual);
  }

  const bytes = new Uint8Array(length);
  (source ?? systemSecureRandomSource()).fill(bytes);
  return bytes;
}
