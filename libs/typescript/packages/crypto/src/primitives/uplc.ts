import { bls12_381 } from "@noble/curves/bls12-381.js";
import { sha256 } from "@noble/hashes/sha2.js";

const scalarOrder = bls12_381.fields.Fr.ORDER;

type Group = "g1" | "g2";

export function bls12_381_add(group: Group, left: Uint8Array, right: Uint8Array): Uint8Array {
  if (group === "g1") {
    return encodeG1(bls12_381.G1.Point.fromBytes(left).add(bls12_381.G1.Point.fromBytes(right)));
  }
  return encodeG2(bls12_381.G2.Point.fromBytes(left).add(bls12_381.G2.Point.fromBytes(right)));
}

export function bls12_381_neg(group: Group, value: Uint8Array): Uint8Array {
  return group === "g1"
    ? encodeG1(bls12_381.G1.Point.fromBytes(value).negate())
    : encodeG2(bls12_381.G2.Point.fromBytes(value).negate());
}

export function bls12_381_scalar_mul(group: Group, scalar: bigint, value: Uint8Array): Uint8Array {
  if (scalar < -(1n << 4095n) || scalar >= 1n << 4095n) {
    throw new RangeError("BLS12-381 scalar exceeds signed 512-byte bounds");
  }
  const normalized = ((scalar % scalarOrder) + scalarOrder) % scalarOrder;
  if (group === "g1") {
    const parsed = bls12_381.G1.Point.fromBytes(value);
    return encodeG1(normalized === 0n ? bls12_381.G1.Point.ZERO : parsed.multiply(normalized));
  }
  const parsed = bls12_381.G2.Point.fromBytes(value);
  return encodeG2(normalized === 0n ? bls12_381.G2.Point.ZERO : parsed.multiply(normalized));
}

export function bls12_381_equal(group: Group, left: Uint8Array, right: Uint8Array): boolean {
  return group === "g1"
    ? bls12_381.G1.Point.fromBytes(left).equals(bls12_381.G1.Point.fromBytes(right))
    : bls12_381.G2.Point.fromBytes(left).equals(bls12_381.G2.Point.fromBytes(right));
}

export function bls12_381_hash_to_group(
  group: Group,
  message: Uint8Array,
  domain: Uint8Array,
): Uint8Array {
  if (domain.length > 255) throw new RangeError("BLS12-381 domain separation tag exceeds 255 bytes");
  if (domain.length !== 0) {
    const hasher = group === "g1" ? bls12_381.G1 : bls12_381.G2;
    return group === "g1"
      ? encodeG1(bls12_381.G1.hashToCurve(message, { DST: domain }))
      : encodeG2(bls12_381.G2.hashToCurve(message, { DST: domain }));
  }
  const uniform = expandMessageXmd(message, domain, group === "g1" ? 128 : 256);
  const defaults = group === "g1" ? bls12_381.G1.defaults : bls12_381.G2.defaults;
  const field = hashToField(uniform, 2, defaults.m, defaults.p);
  if (group === "g1") {
    const map = bls12_381.G1.mapToCurve as unknown as
      (scalar: bigint) => InstanceType<typeof bls12_381.G1.Point>;
    const left = map(field[0]?.[0] as bigint);
    const right = map(field[1]?.[0] as bigint);
    return encodeG1(left.add(right));
  }
  const map = bls12_381.G2.mapToCurve as unknown as
    (scalar: bigint[]) => InstanceType<typeof bls12_381.G2.Point>;
  const left = map(field[0] as bigint[]);
  const right = map(field[1] as bigint[]);
  return encodeG2(left.add(right));
}

export function bls12_381_compress(group: Group, value: Uint8Array): Uint8Array {
  return group === "g1"
    ? encodeG1(bls12_381.G1.Point.fromBytes(value))
    : encodeG2(bls12_381.G2.Point.fromBytes(value));
}

export function bls12_381_uncompress(group: Group, value: Uint8Array): Uint8Array {
  const expectedLength = group === "g1" ? 48 : 96;
  if (value.length !== expectedLength || ((value[0] ?? 0) & 0x80) === 0) {
    throw new Error(`invalid compressed BLS12-381 ${group.toUpperCase()} point`);
  }
  if (group === "g1") {
    const point = bls12_381.G1.Point.fromBytes(value);
    if (!point.is0()) point.assertValidity();
    return encodeG1(point);
  }
  const point = bls12_381.G2.Point.fromBytes(value);
  if (!point.is0()) point.assertValidity();
  return encodeG2(point);
}

export function bls12_381_miller_loop(g1: Uint8Array, g2: Uint8Array): Uint8Array {
  const left = bls12_381.G1.Point.fromBytes(g1);
  const right = bls12_381.G2.Point.fromBytes(g2);
  const result = left.is0() || right.is0()
    ? bls12_381.fields.Fp12.ONE
    : bls12_381.pairing(left, right, false);
  return Uint8Array.from(bls12_381.fields.Fp12.toBytes(result));
}

export function bls12_381_mul_ml_result(left: Uint8Array, right: Uint8Array): Uint8Array {
  return Uint8Array.from(bls12_381.fields.Fp12.toBytes(
    bls12_381.fields.Fp12.mul(mlResult(left), mlResult(right)),
  ));
}

export function bls12_381_final_verify(left: Uint8Array, right: Uint8Array): boolean {
  const quotient = bls12_381.fields.Fp12.div(mlResult(left), mlResult(right));
  return bls12_381.fields.Fp12.eql(
    bls12_381.fields.Fp12.finalExponentiate(quotient),
    bls12_381.fields.Fp12.ONE,
  );
}

function mlResult(value: Uint8Array) {
  if (value.length !== bls12_381.fields.Fp12.BYTES) throw new Error("invalid BLS12-381 ML result");
  return bls12_381.fields.Fp12.fromBytes(value);
}

function encodeG1(point: InstanceType<typeof bls12_381.G1.Point>): Uint8Array {
  if (point.is0()) return infinity(48);
  return Uint8Array.from(point.toBytes(true));
}

function encodeG2(point: InstanceType<typeof bls12_381.G2.Point>): Uint8Array {
  if (point.is0()) return infinity(96);
  return Uint8Array.from(point.toBytes(true));
}

function infinity(length: number): Uint8Array {
  const output = new Uint8Array(length);
  output[0] = 0xc0;
  return output;
}

function expandMessageXmd(
  message: Uint8Array,
  domain: Uint8Array,
  outputLength: number,
): Uint8Array {
  const destination = concat(domain, Uint8Array.of(domain.length));
  const length = Uint8Array.of(outputLength >>> 8, outputLength & 0xff);
  const first = sha256(concat(
    new Uint8Array(sha256.blockLen),
    message,
    length,
    Uint8Array.of(0),
    destination,
  ));
  const blocks: Uint8Array[] = [];
  let previous = sha256(concat(first, Uint8Array.of(1), destination));
  blocks.push(previous);
  const count = Math.ceil(outputLength / sha256.outputLen);
  for (let index = 2; index <= count; index += 1) {
    previous = sha256(concat(xor(first, previous), Uint8Array.of(index), destination));
    blocks.push(previous);
  }
  return concat(...blocks).slice(0, outputLength);
}

function hashToField(
  uniform: Uint8Array,
  count: number,
  extensionDegree: number,
  modulus: bigint,
): bigint[][] {
  const chunkLength = 64;
  return Array.from({ length: count }, (_, index) =>
    Array.from({ length: extensionDegree }, (_, component) => {
      const offset = chunkLength * (component + index * extensionDegree);
      let value = 0n;
      for (const byte of uniform.subarray(offset, offset + chunkLength)) {
        value = (value << 8n) | BigInt(byte);
      }
      return value % modulus;
    }),
  );
}

function xor(left: Uint8Array, right: Uint8Array): Uint8Array {
  return Uint8Array.from(left, (value, index) => value ^ (right[index] ?? 0));
}

function concat(...values: readonly Uint8Array[]): Uint8Array {
  const output = new Uint8Array(values.reduce((size, value) => size + value.length, 0));
  let offset = 0;
  for (const value of values) {
    output.set(value, offset);
    offset += value.length;
  }
  return output;
}
