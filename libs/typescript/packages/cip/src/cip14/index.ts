import { decodeBech32, encodeBech32 } from "@xray-network/xray-cardano-lib-core/bech32";
import { AssetName } from "@xray-network/xray-cardano-lib-chain/conway";
import { blake2b160, ScriptHash } from "@xray-network/xray-cardano-lib-crypto";

const ASSET_HRP = "asset";
const FINGERPRINT_LENGTH = 20;

export class AssetFingerprint {
  readonly #digest: Uint8Array;

  private constructor(digest: Uint8Array) {
    if (digest.length !== FINGERPRINT_LENGTH) {
      throw new RangeError("asset fingerprint must contain 20 bytes");
    }
    this.#digest = Uint8Array.from(digest);
  }

  public static from_parts(policyId: ScriptHash, assetName: AssetName): AssetFingerprint {
    const policy = policyId.to_raw_bytes();
    const name = assetName.to_raw_bytes();
    const message = new Uint8Array(policy.length + name.length);
    message.set(policy);
    message.set(name, policy.length);
    return new AssetFingerprint(blake2b160(message));
  }

  public static from_bech32(text: string): AssetFingerprint {
    const decoded = decodeBech32(text);
    if (decoded.prefix !== ASSET_HRP) throw new TypeError("asset fingerprint HRP must be 'asset'");
    return new AssetFingerprint(decoded.bytes);
  }

  public to_bech32(): string {
    return encodeBech32(ASSET_HRP, this.#digest);
  }

  public to_raw_bytes(): Uint8Array {
    return Uint8Array.from(this.#digest);
  }

  public equals(other: AssetFingerprint): boolean {
    const right = other.#digest;
    if (this.#digest.length !== right.length) return false;
    let difference = 0;
    for (let index = 0; index < this.#digest.length; index += 1) {
      difference |= (this.#digest[index] ?? 0) ^ (right[index] ?? 0);
    }
    return difference === 0;
  }
}
