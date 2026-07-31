import { decodeCbor, encodeCbor } from "../cbor/index.js";
import type { CborHeadWidth, CborValue } from "../cbor/index.js";
import { DeserializeError } from "../errors/index.js";
import { assertBigIntInRange } from "../shared/index.js";
import { CBOR_INT_MAX, CBOR_INT_MIN } from "./bounds.js";
const intStates = new WeakMap<Int, { readonly value: bigint; readonly width: CborHeadWidth | undefined }>();

/** Full CBOR integer range used by the compatibility API. */
export class Int {
  private constructor(value: bigint, width?: CborHeadWidth) {
    intStates.set(this, {
      value: assertBigIntInRange("Int", value, CBOR_INT_MIN, CBOR_INT_MAX),
      width,
    });
  }

  public static new(value: bigint): Int { return new Int(value); }
  public static from_str(value: string): Int {
    if (!/^-?(?:0|[1-9][0-9]*)$/u.test(value)) throw new TypeError("Invalid Int decimal string");
    return new Int(BigInt(value));
  }
  public static from_json(json: string): Int {
    const value: unknown = JSON.parse(json);
    if (typeof value !== "string") throw new TypeError("Int JSON must be a decimal string");
    return Int.from_str(value);
  }
  public static from_cbor_bytes(bytes: Uint8Array): Int {
    const decoded = decodeCbor(bytes);
    if (decoded.kind !== "unsigned" && decoded.kind !== "negative") {
      throw new DeserializeError("NO_VARIANT_MATCHED", "Int must be an unsigned or negative CBOR integer", { path: ["Int"] });
    }
    return new Int(decoded.value, decoded.encoding.width);
  }
  public to_cbor_bytes(): Uint8Array { return encodeCbor(intCbor(this), { mode: "preserve" }); }
  public to_str(): string { return intState(this).value.toString(10); }
  public to_json(): string { return JSON.stringify(this.to_str()); }
  public to_json_value(): any { return this.to_str(); }
}

function intState(value: Int): { readonly value: bigint; readonly width: CborHeadWidth | undefined } {
  const state = intStates.get(value);
  if (state === undefined) throw new TypeError("Invalid Int receiver");
  return state;
}

function intCbor(value: Int): CborValue {
  const state = intState(value);
  const encoding = { width: state.width ?? 0 };
  return state.value >= 0n
    ? { kind: "unsigned", value: state.value, encoding }
    : { kind: "negative", value: state.value, encoding };
}
