import { decodeCbor, encodeCbor } from "../cbor/index.js";
import { DeserializeError } from "../errors/index.js";
import { assertBigIntInRange } from "../shared/index.js";

export const BYRON_MAINNET_NETWORK_MAGIC = 764_824_073;
export const BYRON_TESTNET_NETWORK_MAGIC = 1_097_911_063;
export const SANCHO_TESTNET_NETWORK_MAGIC = 4;
export const PREPROD_NETWORK_MAGIC = 1;
export const PREVIEW_NETWORK_MAGIC = 2;
const protocolMagicValues = new WeakMap<ProtocolMagic, number>();

export class ProtocolMagic {
  private constructor(value: number) {
    if (!Number.isSafeInteger(value)) throw new TypeError("ProtocolMagic must be an integer");
    protocolMagicValues.set(
      this,
      Number(assertBigIntInRange("ProtocolMagic", BigInt(value), 0n, 0xffff_ffffn)),
    );
  }
  public static new(value: number): ProtocolMagic { return new ProtocolMagic(value); }
  public to_int(): number {
    const value = protocolMagicValues.get(this);
    if (value === undefined) throw new TypeError("Invalid ProtocolMagic receiver");
    return value;
  }
}

export function decodeProtocolMagic(bytes: Uint8Array): ProtocolMagic {
  const value = decodeCbor(bytes);
  if (value.kind !== "unsigned" || value.value > 0xffff_ffffn) {
    throw new DeserializeError("OUT_OF_RANGE", "ProtocolMagic must be a CBOR uint32", { path: ["ProtocolMagic"] });
  }
  return ProtocolMagic.new(Number(value.value));
}

export function encodeProtocolMagic(value: ProtocolMagic): Uint8Array {
  return encodeCbor({ kind: "unsigned", value: BigInt(value.to_int()), encoding: { width: 0 } });
}
