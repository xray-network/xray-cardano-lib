import { copyBytes, decodeCbor, encodeCbor } from "@xray-network/xray-cardano-lib-core";
import { decodeFlatProgram } from "./flat.js";

export enum SerializedPlutusScriptKind {
  RawFlat = 0,
  SingleCbor = 1,
  DoubleCbor = 2,
}

function cborBytes(value: Uint8Array, name: string): Uint8Array {
  const decoded = decodeCbor(value);
  if (decoded.kind !== "bytes") throw new TypeError(`${name} must be an exact CBOR byte string`);
  return copyBytes(decoded.value);
}

function encodeBytes(value: Uint8Array): Uint8Array {
  return encodeCbor({kind:"bytes",value:copyBytes(value),encoding:{kind:"definite",width:0}}, {mode:"canonical"});
}

/** Explicit, immutable ownership of one serialized UPLC envelope level. */
export class SerializedPlutusScript {
  readonly #kind: SerializedPlutusScriptKind;
  readonly #raw: Uint8Array;
  readonly #single: Uint8Array;
  readonly #serialized: Uint8Array;

  private constructor(kind: SerializedPlutusScriptKind, raw: Uint8Array, single: Uint8Array, serialized: Uint8Array) {
    decodeFlatProgram(raw);
    this.#kind=kind;
    this.#raw=copyBytes(raw);
    this.#single=copyBytes(single);
    this.#serialized=copyBytes(serialized);
  }

  public static from_raw_flat(value: Uint8Array): SerializedPlutusScript {
    const raw=copyBytes(value), single=encodeBytes(raw);
    return new SerializedPlutusScript(SerializedPlutusScriptKind.RawFlat,raw,single,raw);
  }

  public static from_single_cbor(value: Uint8Array): SerializedPlutusScript {
    const single=copyBytes(value), raw=cborBytes(single,"single-CBOR Plutus script");
    return new SerializedPlutusScript(SerializedPlutusScriptKind.SingleCbor,raw,single,single);
  }

  public static from_double_cbor(value: Uint8Array): SerializedPlutusScript {
    const serialized=copyBytes(value), single=cborBytes(serialized,"double-CBOR Plutus script");
    const raw=cborBytes(single,"nested Plutus script");
    return new SerializedPlutusScript(SerializedPlutusScriptKind.DoubleCbor,raw,single,serialized);
  }

  public kind(): SerializedPlutusScriptKind { return this.#kind; }
  public bytes(): Uint8Array { return copyBytes(this.#serialized); }
  public to_raw_flat(): Uint8Array { return copyBytes(this.#raw); }
  public to_single_cbor(): Uint8Array { return copyBytes(this.#single); }
  public to_double_cbor(): Uint8Array {
    return this.#kind===SerializedPlutusScriptKind.DoubleCbor?copyBytes(this.#serialized):encodeBytes(this.#single);
  }
}
