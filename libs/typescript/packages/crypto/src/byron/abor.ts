import { concatenateBytes, readLittleEndian, writeLittleEndian } from "../primitives/crypto.js";

const enum Tag {
  U8 = 1,
  U16 = 2,
  U32 = 3,
  U64 = 4,
  U128 = 5,
  Bytes = 6,
  Array = 7,
}

export class AborEncoder {
  readonly #parts: Uint8Array[] = [];
  readonly #arrays: { readonly part: Uint8Array; readonly start: number }[] = [];
  #elements = 0;

  #integer(tag: Tag, value: bigint, length: number): this {
    if (value < 0n || value >= 2n ** BigInt(length * 8)) throw new RangeError("ABOR integer out of range");
    this.#parts.push(Uint8Array.of(tag), writeLittleEndian(value, length));
    this.#elements += 1;
    return this;
  }
  public u8(value: number): this { return this.#integer(Tag.U8, BigInt(value), 1); }
  public u16(value: number): this { return this.#integer(Tag.U16, BigInt(value), 2); }
  public u32(value: number): this { return this.#integer(Tag.U32, BigInt(value), 4); }
  public u64(value: bigint): this { return this.#integer(Tag.U64, value, 8); }
  public u128(value: bigint): this { return this.#integer(Tag.U128, value, 16); }
  public bytes(value: Uint8Array): this {
    if (value.length >= 256) throw new RangeError("ABOR byte strings must be shorter than 256 bytes");
    this.#parts.push(Uint8Array.of(Tag.Bytes, value.length), value.slice());
    this.#elements += 1;
    return this;
  }
  public struct_start(): this {
    const marker = Uint8Array.of(Tag.Array, 0xfe);
    this.#parts.push(marker);
    this.#arrays.push({ part: marker, start: this.#elements + 1 });
    this.#elements += 1;
    return this;
  }
  public struct_end(): this {
    const array = this.#arrays.pop();
    if (array === undefined) throw new TypeError("Unmatched ABOR array end");
    const length = this.#elements - array.start;
    if (length >= 256) throw new RangeError("ABOR arrays must contain fewer than 256 elements");
    array.part[1] = length;
    return this;
  }
  public finalize(): Uint8Array {
    if (this.#arrays.length !== 0) throw new TypeError("Unclosed ABOR array");
    return concatenateBytes(...this.#parts);
  }
}

export class AborDecoder {
  readonly #bytes: Uint8Array;
  #offset = 0;
  public constructor(bytes: Uint8Array) { this.#bytes = bytes; }
  #take(length: number): Uint8Array {
    if (this.#offset + length > this.#bytes.length) throw new TypeError("Truncated ABOR input");
    const result = this.#bytes.subarray(this.#offset, this.#offset + length);
    this.#offset += length;
    return result;
  }
  #expect(tag: Tag, length: number): bigint {
    if ((this.#take(1)[0] ?? 0) !== tag) throw new TypeError("Unexpected ABOR tag");
    return readLittleEndian(this.#take(length));
  }
  public u8(): number { return Number(this.#expect(Tag.U8, 1)); }
  public u16(): number { return Number(this.#expect(Tag.U16, 2)); }
  public u32(): number { return Number(this.#expect(Tag.U32, 4)); }
  public u64(): bigint { return this.#expect(Tag.U64, 8); }
  public u128(): bigint { return this.#expect(Tag.U128, 16); }
  public bytes(): Uint8Array {
    if ((this.#take(1)[0] ?? 0) !== Tag.Bytes) throw new TypeError("Unexpected ABOR tag");
    return this.#take(this.#take(1)[0] ?? 0).slice();
  }
  public array(): number {
    if ((this.#take(1)[0] ?? 0) !== Tag.Array) throw new TypeError("Unexpected ABOR tag");
    return this.#take(1)[0] ?? 0;
  }
  public end(): void {
    if (this.#offset !== this.#bytes.length) throw new TypeError("Pending ABOR input");
  }
}
