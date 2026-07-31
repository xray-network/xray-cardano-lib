import { assertBigIntInRange } from "../shared/index.js";

export const UINT64_MAX = 0xffff_ffff_ffff_ffffn;
export const INT64_MIN = -0x8000_0000_0000_0000n;
export const INT64_MAX = 0x7fff_ffff_ffff_ffffn;
export const CBOR_INT_MIN = -0x1_0000_0000_0000_0000n;
export const CBOR_INT_MAX = UINT64_MAX;

export function asUint64(value: bigint): bigint {
  return assertBigIntInRange("uint64", value, 0n, UINT64_MAX);
}

export function asInt64(value: bigint): bigint {
  return assertBigIntInRange("int64", value, INT64_MIN, INT64_MAX);
}
