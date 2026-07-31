import { assertByteLength, copyBytes } from "@xray-network/cardano-core";
import { extendedPublicKey, extendedSign } from "../primitives/crypto.js";

interface LegacyState {
  readonly bytes: Uint8Array;
  disposed: boolean;
}
const states = new WeakMap<LegacyDaedalusPrivateKey, LegacyState>();

function state(value: LegacyDaedalusPrivateKey): LegacyState {
  const found = states.get(value);
  if (found === undefined) throw new TypeError("Invalid LegacyDaedalusPrivateKey receiver");
  if (found.disposed) throw new TypeError("LegacyDaedalusPrivateKey has been disposed");
  return found;
}

export class LegacyDaedalusPrivateKey {
  private constructor(bytes: Uint8Array) {
    assertByteLength("LegacyDaedalusPrivateKey", bytes, 96);
    states.set(this, { bytes: copyBytes(bytes), disposed: false });
  }

  public chaincode(): Uint8Array {
    return state(this).bytes.slice(64, 96);
  }
  public dispose(): void {
    const found = states.get(this);
    if (found !== undefined && !found.disposed) {
      found.bytes.fill(0);
      found.disposed = true;
    }
  }
}

/** @internal Construction boundary used by the Byron address port. */
export function legacyPrivateKeyFromRawBytes(bytes: Uint8Array): LegacyDaedalusPrivateKey {
  const Constructor = LegacyDaedalusPrivateKey as unknown as new (
    value: Uint8Array,
  ) => LegacyDaedalusPrivateKey;
  return new Constructor(bytes);
}

export function legacyPublicKey(key: LegacyDaedalusPrivateKey): Uint8Array {
  const bytes = state(key).bytes;
  const output = new Uint8Array(64);
  output.set(extendedPublicKey(bytes.subarray(0, 64)), 0);
  output.set(bytes.subarray(64, 96), 32);
  return output;
}

export function legacySign(key: LegacyDaedalusPrivateKey, message: Uint8Array): Uint8Array {
  return extendedSign(state(key).bytes.subarray(0, 64), message);
}
