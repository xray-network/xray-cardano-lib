import { chacha20poly1305 } from "@noble/ciphers/chacha.js";
import { ed25519 } from "@noble/curves/ed25519.js";
import { schnorr, secp256k1 } from "@noble/curves/secp256k1.js";
import { blake2b } from "@noble/hashes/blake2.js";
import { hmac } from "@noble/hashes/hmac.js";
import { ripemd160 as nobleRipemd160 } from "@noble/hashes/legacy.js";
import { pbkdf2 } from "@noble/hashes/pbkdf2.js";
import { keccak_256 as nobleKeccak256, sha3_256 as nobleSha3_256 } from "@noble/hashes/sha3.js";
import { sha256, sha512 } from "@noble/hashes/sha2.js";

const ED25519_ORDER = 2n ** 252n + 27742317777372353535851937790883648493n;

export function concatenateBytes(...parts: readonly Uint8Array[]): Uint8Array {
  const output = new Uint8Array(parts.reduce((length, part) => length + part.length, 0));
  let offset = 0;
  for (const part of parts) {
    output.set(part, offset);
    offset += part.length;
  }
  return output;
}

export function readLittleEndian(bytes: Uint8Array): bigint {
  let value = 0n;
  for (let index = bytes.length - 1; index >= 0; index -= 1) {
    value = (value << 8n) | BigInt(bytes[index] ?? 0);
  }
  return value;
}

export function writeLittleEndian(value: bigint, length: number): Uint8Array {
  const output = new Uint8Array(length);
  let remaining = value;
  for (let index = 0; index < length; index += 1) {
    output[index] = Number(remaining & 0xffn);
    remaining >>= 8n;
  }
  return output;
}

export function blake2b224(data: Uint8Array): Uint8Array {
  return Uint8Array.from(blake2b(data, { dkLen: 28 }));
}

export function blake2b160(data: Uint8Array): Uint8Array {
  return Uint8Array.from(blake2b(Uint8Array.from(data), { dkLen: 20 }));
}

export function blake2b256(data: Uint8Array): Uint8Array {
  return Uint8Array.from(blake2b(data, { dkLen: 32 }));
}

export function blake2b512(data: Uint8Array, personalization?: Uint8Array): Uint8Array {
  if (personalization !== undefined && personalization.length !== 16) {
    throw new RangeError("Blake2b personalization must contain 16 bytes");
  }
  return Uint8Array.from(blake2b(
    data,
    personalization === undefined ? { dkLen: 64 } : { dkLen: 64, personalization },
  ));
}

export function sha3_256(data: Uint8Array): Uint8Array {
  return Uint8Array.from(nobleSha3_256(data));
}

export function sha2_256(data: Uint8Array): Uint8Array {
  return Uint8Array.from(sha256(data));
}

export function keccak_256(data: Uint8Array): Uint8Array {
  return Uint8Array.from(nobleKeccak256(data));
}

export function ripemd_160(data: Uint8Array): Uint8Array {
  return Uint8Array.from(nobleRipemd160(data));
}

export function sha512Digest(data: Uint8Array): Uint8Array {
  return Uint8Array.from(sha512(data));
}

export function hmacSha512(key: Uint8Array, data: Uint8Array): Uint8Array {
  return Uint8Array.from(hmac(sha512, key, data));
}

export function pbkdf2Sha512(
  password: Uint8Array,
  salt: Uint8Array,
  iterations: number,
  length: number,
): Uint8Array {
  return Uint8Array.from(pbkdf2(sha512, password, salt, { c: iterations, dkLen: length }));
}

export function normalPublicKey(seed: Uint8Array): Uint8Array {
  return Uint8Array.from(ed25519.getPublicKey(seed));
}

export function normalSign(seed: Uint8Array, message: Uint8Array): Uint8Array {
  return Uint8Array.from(ed25519.sign(message, seed));
}

export function extendedPublicKey(extended: Uint8Array): Uint8Array {
  const scalar = readLittleEndian(extended.subarray(0, 32)) % ED25519_ORDER;
  return Uint8Array.from(ed25519.Point.BASE.multiply(scalar).toBytes());
}

export function extendedSign(extended: Uint8Array, message: Uint8Array): Uint8Array {
  const scalar = readLittleEndian(extended.subarray(0, 32)) % ED25519_ORDER;
  const prefix = extended.subarray(32, 64);
  const publicKey = extendedPublicKey(extended);
  const nonce = readLittleEndian(sha512Digest(concatenateBytes(prefix, message))) % ED25519_ORDER;
  const encodedR = Uint8Array.from(ed25519.Point.BASE.multiply(nonce).toBytes());
  const challenge =
    readLittleEndian(sha512Digest(concatenateBytes(encodedR, publicKey, message))) % ED25519_ORDER;
  const encodedS = writeLittleEndian((nonce + challenge * scalar) % ED25519_ORDER, 32);
  return concatenateBytes(encodedR, encodedS);
}

export function verifyEd25519(
  publicKey: Uint8Array,
  signature: Uint8Array,
  message: Uint8Array,
): boolean {
  try {
    return ed25519.verify(signature, message, publicKey, { zip215: false });
  } catch {
    return false;
  }
}

export function verifyEd25519Uplc(
  publicKey: Uint8Array,
  signature: Uint8Array,
  message: Uint8Array,
): boolean {
  if (publicKey.length !== 32 || signature.length !== 64) {
    throw new Error("invalid Ed25519 public key or signature length");
  }
  return verifyEd25519(publicKey, signature, message);
}

export function verifySecp256k1Ecdsa(
  publicKey: Uint8Array,
  signature: Uint8Array,
  messageHash: Uint8Array,
): boolean {
  if (publicKey.length !== 33 || signature.length !== 64 || messageHash.length !== 32) return false;
  try {
    return secp256k1.verify(signature, messageHash, publicKey, {
      format: "compact",
      lowS: true,
      prehash: false,
    });
  } catch {
    return false;
  }
}

export function verifySecp256k1EcdsaUplc(
  publicKey: Uint8Array,
  signature: Uint8Array,
  messageHash: Uint8Array,
): boolean {
  if (publicKey.length !== 33 || signature.length !== 64 || messageHash.length !== 32) {
    throw new Error("invalid ECDSA public key, message, or signature length");
  }
  const point = secp256k1.Point.fromBytes(publicKey);
  point.assertValidity();
  secp256k1.Signature.fromBytes(signature, "compact");
  return verifySecp256k1Ecdsa(publicKey, signature, messageHash);
}

export function verifySecp256k1Schnorr(
  publicKey: Uint8Array,
  signature: Uint8Array,
  message: Uint8Array,
): boolean {
  if (publicKey.length !== 32 || signature.length !== 64) return false;
  try {
    return schnorr.verify(signature, message, publicKey);
  } catch {
    return false;
  }
}

export function verifySecp256k1SchnorrUplc(
  publicKey: Uint8Array,
  signature: Uint8Array,
  message: Uint8Array,
): boolean {
  if (publicKey.length !== 32 || signature.length !== 64) {
    throw new Error("invalid Schnorr public key or signature length");
  }
  let coordinate = 0n;
  for (const byte of publicKey) coordinate = (coordinate << 8n) | BigInt(byte);
  schnorr.utils.lift_x(coordinate);
  return verifySecp256k1Schnorr(publicKey, signature, message);
}

export function addPublicKeyScalar(publicKey: Uint8Array, scalar: bigint): Uint8Array {
  const point = ed25519.Point.fromBytes(publicKey, false);
  const offset = ed25519.Point.BASE.multiply(scalar % ED25519_ORDER);
  return Uint8Array.from(point.add(offset).toBytes());
}

export function chacha20Poly1305Encrypt(
  key: Uint8Array,
  nonce: Uint8Array,
  plaintext: Uint8Array,
): { readonly ciphertext: Uint8Array; readonly tag: Uint8Array } {
  const combined = Uint8Array.from(chacha20poly1305(key, nonce, new Uint8Array()).encrypt(plaintext));
  return {
    ciphertext: combined.slice(0, -16),
    tag: combined.slice(-16),
  };
}

export function chacha20Poly1305Decrypt(
  key: Uint8Array,
  nonce: Uint8Array,
  ciphertext: Uint8Array,
  tag: Uint8Array,
): Uint8Array {
  return Uint8Array.from(
    chacha20poly1305(key, nonce, new Uint8Array()).decrypt(concatenateBytes(ciphertext, tag)),
  );
}
