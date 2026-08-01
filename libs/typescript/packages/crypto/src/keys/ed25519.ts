import {
  assertByteLength,
  bytesToHex,
  CardanoBoundsError,
  copyBytes,
  hexToBytes,
  type SecureRandomSource,
} from "@xray-network/xray-cardano-lib-core";
import { decodeBech32WithPrefix, encodeBech32 } from "../hashes/fixed-bytes.js";
import { Ed25519KeyHash } from "../hashes/types.js";
import {
  addPublicKeyScalar,
  blake2b224,
  concatenateBytes,
  extendedPublicKey,
  extendedSign,
  hmacSha512,
  normalPublicKey,
  normalSign,
  pbkdf2Sha512,
  readLittleEndian,
  verifyEd25519,
  writeLittleEndian,
} from "../primitives/crypto.js";
import { secureRandomBytes } from "../primitives/random.js";

const HARDENED = 0x80000000;
const UINT32_MAX = 0xffffffff;
const UINT256_MODULUS = 2n ** 256n;

type PrivateKeyKind = "normal" | "extended";
interface PrivateKeyState {
  readonly kind: PrivateKeyKind;
  readonly bytes: Uint8Array;
  disposed: boolean;
}
interface BytesState {
  readonly bytes: Uint8Array;
  disposed: boolean;
}

const privateKeyStates = new WeakMap<PrivateKey, PrivateKeyState>();
const publicKeyStates = new WeakMap<PublicKey, BytesState>();
const signatureStates = new WeakMap<Ed25519Signature, BytesState>();
const bip32PrivateStates = new WeakMap<Bip32PrivateKey, BytesState>();
const bip32PublicStates = new WeakMap<Bip32PublicKey, BytesState>();

function assertIndex(index: number): void {
  if (!Number.isInteger(index) || index < 0 || index > UINT32_MAX) {
    const actual = Number.isSafeInteger(index) ? BigInt(index) : -1n;
    throw new CardanoBoundsError("derivation index", 0n, BigInt(UINT32_MAX), actual);
  }
}

function active<Value extends object>(
  states: WeakMap<Value, BytesState>,
  value: Value,
  name: string,
): Uint8Array {
  const found = states.get(value);
  if (found === undefined) throw new TypeError(`Invalid ${name} receiver`);
  if (found.disposed) throw new TypeError(`${name} has been disposed`);
  return found.bytes;
}

function dispose<Value extends object>(states: WeakMap<Value, BytesState>, value: Value): void {
  const found = states.get(value);
  if (found !== undefined && !found.disposed) {
    found.bytes.fill(0);
    found.disposed = true;
  }
}

function privateState(value: PrivateKey): PrivateKeyState {
  const found = privateKeyStates.get(value);
  if (found === undefined) throw new TypeError("Invalid PrivateKey receiver");
  if (found.disposed) throw new TypeError("PrivateKey has been disposed");
  return found;
}

function disposePrivate(value: PrivateKey): void {
  const found = privateKeyStates.get(value);
  if (found !== undefined && !found.disposed) {
    found.bytes.fill(0);
    found.disposed = true;
  }
}

function expectBech32(value: string, prefix: string, length: number, name: string): Uint8Array {
  const decoded = decodeBech32WithPrefix(value);
  if (decoded.prefix !== prefix) {
    throw new TypeError(`${name} Bech32 prefix must be ${prefix}`);
  }
  assertByteLength(name, decoded.bytes, length);
  return decoded.bytes;
}

function normalizeXprv(bytes: Uint8Array): Uint8Array {
  const normalized = copyBytes(bytes);
  normalized[0] = (normalized[0] ?? 0) & 0xf8;
  normalized[31] = ((normalized[31] ?? 0) & 0x1f) | 0x40;
  return normalized;
}

function validateXprv(bytes: Uint8Array): void {
  assertByteLength("Bip32PrivateKey", bytes, 96);
  if (((bytes[0] ?? 0) & 0x07) !== 0 || ((bytes[31] ?? 0) & 0xe0) !== 0x40) {
    throw new TypeError("Bip32PrivateKey has invalid Ed25519 scalar bits");
  }
}

function derivePrivateBytes(parent: Uint8Array, index: number): Uint8Array {
  const kl = parent.subarray(0, 32);
  const kr = parent.subarray(32, 64);
  const chainCode = parent.subarray(64, 96);
  const indexBytes = writeLittleEndian(BigInt(index), 4);
  const hardened = index >= HARDENED;
  const keyMaterial = hardened ? concatenateBytes(kl, kr) : extendedPublicKey(parent.subarray(0, 64));
  const z = hmacSha512(
    chainCode,
    concatenateBytes(Uint8Array.of(hardened ? 0 : 2), keyMaterial, indexBytes),
  );
  const nextChain = hmacSha512(
    chainCode,
    concatenateBytes(Uint8Array.of(hardened ? 1 : 3), keyMaterial, indexBytes),
  );
  const left =
    (readLittleEndian(kl) + 8n * readLittleEndian(z.subarray(0, 28))) % UINT256_MODULUS;
  const right =
    (readLittleEndian(kr) + readLittleEndian(z.subarray(32, 64))) % UINT256_MODULUS;
  return concatenateBytes(
    writeLittleEndian(left, 32),
    writeLittleEndian(right, 32),
    nextChain.subarray(32, 64),
  );
}

function derivePublicBytes(parent: Uint8Array, index: number): Uint8Array {
  if (index >= HARDENED) throw new TypeError("A public key cannot perform hardened derivation");
  const publicKey = parent.subarray(0, 32);
  const chainCode = parent.subarray(32, 64);
  const indexBytes = writeLittleEndian(BigInt(index), 4);
  const z = hmacSha512(chainCode, concatenateBytes(Uint8Array.of(2), publicKey, indexBytes));
  const nextChain = hmacSha512(chainCode, concatenateBytes(Uint8Array.of(3), publicKey, indexBytes));
  const scalar = 8n * readLittleEndian(z.subarray(0, 28));
  return concatenateBytes(addPublicKeyScalar(publicKey, scalar), nextChain.subarray(32, 64));
}

export class Ed25519Signature {
  private constructor(bytes: Uint8Array) {
    assertByteLength("Ed25519Signature", bytes, 64);
    signatureStates.set(this, { bytes: copyBytes(bytes), disposed: false });
  }

  public static from_raw_bytes(bytes: Uint8Array): Ed25519Signature {
    return new Ed25519Signature(bytes);
  }
  public static from_hex(input: string): Ed25519Signature {
    return new Ed25519Signature(hexToBytes(input));
  }
  public static from_bech32(input: string): Ed25519Signature {
    return new Ed25519Signature(expectBech32(input, "ed25519_sig", 64, "Ed25519Signature"));
  }
  public to_raw_bytes(): Uint8Array {
    return copyBytes(active(signatureStates, this, "Ed25519Signature"));
  }
  public to_hex(): string {
    return bytesToHex(active(signatureStates, this, "Ed25519Signature"));
  }
  public to_bech32(): string {
    return encodeBech32("ed25519_sig", active(signatureStates, this, "Ed25519Signature"));
  }
  public dispose(): void {
    dispose(signatureStates, this);
  }
}

export class PrivateKey {
  private constructor(kind: PrivateKeyKind, bytes: Uint8Array) {
    assertByteLength("PrivateKey", bytes, kind === "normal" ? 32 : 64);
    privateKeyStates.set(this, { kind, bytes: copyBytes(bytes), disposed: false });
  }

  public static from_bech32(input: string): PrivateKey {
    const decoded = decodeBech32WithPrefix(input);
    if (decoded.prefix === "ed25519_sk") return PrivateKey.from_normal_bytes(decoded.bytes);
    if (decoded.prefix === "ed25519e_sk") return PrivateKey.from_extended_bytes(decoded.bytes);
    throw new TypeError("PrivateKey Bech32 prefix must be ed25519_sk or ed25519e_sk");
  }
  public static from_normal_bytes(bytes: Uint8Array): PrivateKey {
    return new PrivateKey("normal", bytes);
  }
  public static from_extended_bytes(bytes: Uint8Array): PrivateKey {
    return new PrivateKey("extended", bytes);
  }
  public static generate_ed25519(): PrivateKey {
    return generateEd25519PrivateKey();
  }
  public static generate_ed25519extended(): PrivateKey {
    return generateExtendedPrivateKey();
  }
  public to_public(): PublicKey {
    const state = privateState(this);
    const bytes = state.kind === "normal" ? normalPublicKey(state.bytes) : extendedPublicKey(state.bytes);
    return PublicKey.from_bytes(bytes);
  }
  public sign(message: Uint8Array): Ed25519Signature {
    const state = privateState(this);
    return Ed25519Signature.from_raw_bytes(
      state.kind === "normal" ? normalSign(state.bytes, message) : extendedSign(state.bytes, message),
    );
  }
  public to_raw_bytes(): Uint8Array {
    return copyBytes(privateState(this).bytes);
  }
  public to_bech32(): string {
    const state = privateState(this);
    return encodeBech32(state.kind === "normal" ? "ed25519_sk" : "ed25519e_sk", state.bytes);
  }
  public dispose(): void {
    disposePrivate(this);
  }
}

export class PublicKey {
  private constructor(bytes: Uint8Array) {
    assertByteLength("PublicKey", bytes, 32);
    publicKeyStates.set(this, { bytes: copyBytes(bytes), disposed: false });
  }

  public static from_bech32(input: string): PublicKey {
    return new PublicKey(expectBech32(input, "ed25519_pk", 32, "PublicKey"));
  }
  public static from_bytes(bytes: Uint8Array): PublicKey {
    return new PublicKey(bytes);
  }
  public to_raw_bytes(): Uint8Array {
    return copyBytes(active(publicKeyStates, this, "PublicKey"));
  }
  public to_bech32(): string {
    return encodeBech32("ed25519_pk", active(publicKeyStates, this, "PublicKey"));
  }
  public verify(data: Uint8Array, signature: Ed25519Signature): boolean {
    return verifyEd25519(
      active(publicKeyStates, this, "PublicKey"),
      signature.to_raw_bytes(),
      data,
    );
  }
  public hash(): Ed25519KeyHash {
    return Ed25519KeyHash.from_raw_bytes(blake2b224(active(publicKeyStates, this, "PublicKey")));
  }
  public dispose(): void {
    dispose(publicKeyStates, this);
  }
}

export class Bip32PrivateKey {
  private constructor(bytes: Uint8Array) {
    validateXprv(bytes);
    bip32PrivateStates.set(this, { bytes: copyBytes(bytes), disposed: false });
  }

  public static from_raw_bytes(bytes: Uint8Array): Bip32PrivateKey {
    return new Bip32PrivateKey(bytes);
  }
  public static from_128_xprv(bytes: Uint8Array): Bip32PrivateKey {
    assertByteLength("128-byte extended private key", bytes, 128);
    return new Bip32PrivateKey(concatenateBytes(bytes.subarray(0, 64), bytes.subarray(96, 128)));
  }
  public static from_bech32(input: string): Bip32PrivateKey {
    return new Bip32PrivateKey(expectBech32(input, "xprv", 96, "Bip32PrivateKey"));
  }
  public static from_bip39_entropy(entropy: Uint8Array, password: Uint8Array): Bip32PrivateKey {
    return new Bip32PrivateKey(normalizeXprv(pbkdf2Sha512(password, entropy, 4096, 96)));
  }
  public static generate_ed25519_bip32(): Bip32PrivateKey {
    return generateBip32PrivateKey();
  }
  public derive(index: number): Bip32PrivateKey {
    assertIndex(index);
    return new Bip32PrivateKey(
      derivePrivateBytes(active(bip32PrivateStates, this, "Bip32PrivateKey"), index),
    );
  }
  public to_raw_key(): PrivateKey {
    return PrivateKey.from_extended_bytes(
      active(bip32PrivateStates, this, "Bip32PrivateKey").subarray(0, 64),
    );
  }
  public to_public(): Bip32PublicKey {
    const bytes = active(bip32PrivateStates, this, "Bip32PrivateKey");
    return Bip32PublicKey.from_raw_bytes(
      concatenateBytes(extendedPublicKey(bytes.subarray(0, 64)), bytes.subarray(64, 96)),
    );
  }
  public chaincode(): Uint8Array {
    return active(bip32PrivateStates, this, "Bip32PrivateKey").slice(64, 96);
  }
  public to_raw_bytes(): Uint8Array {
    return copyBytes(active(bip32PrivateStates, this, "Bip32PrivateKey"));
  }
  public to_128_xprv(): Uint8Array {
    const bytes = active(bip32PrivateStates, this, "Bip32PrivateKey");
    return concatenateBytes(bytes.subarray(0, 64), this.to_public().to_raw_key().to_raw_bytes(), bytes.subarray(64));
  }
  public to_bech32(): string {
    return encodeBech32("xprv", active(bip32PrivateStates, this, "Bip32PrivateKey"));
  }
  public dispose(): void {
    dispose(bip32PrivateStates, this);
  }
}

export class Bip32PublicKey {
  private constructor(bytes: Uint8Array) {
    assertByteLength("Bip32PublicKey", bytes, 64);
    bip32PublicStates.set(this, { bytes: copyBytes(bytes), disposed: false });
  }

  public static from_raw_bytes(bytes: Uint8Array): Bip32PublicKey {
    return new Bip32PublicKey(bytes);
  }
  public static from_bech32(input: string): Bip32PublicKey {
    return new Bip32PublicKey(expectBech32(input, "xpub", 64, "Bip32PublicKey"));
  }
  public derive(index: number): Bip32PublicKey {
    assertIndex(index);
    return new Bip32PublicKey(
      derivePublicBytes(active(bip32PublicStates, this, "Bip32PublicKey"), index),
    );
  }
  public to_raw_key(): PublicKey {
    return PublicKey.from_bytes(active(bip32PublicStates, this, "Bip32PublicKey").subarray(0, 32));
  }
  public chaincode(): Uint8Array {
    return active(bip32PublicStates, this, "Bip32PublicKey").slice(32, 64);
  }
  public to_raw_bytes(): Uint8Array {
    return copyBytes(active(bip32PublicStates, this, "Bip32PublicKey"));
  }
  public to_bech32(): string {
    return encodeBech32("xpub", active(bip32PublicStates, this, "Bip32PublicKey"));
  }
  public dispose(): void {
    dispose(bip32PublicStates, this);
  }
}

export function generateEd25519PrivateKey(source?: SecureRandomSource): PrivateKey {
  return PrivateKey.from_normal_bytes(secureRandomBytes(32, source));
}

export function generateExtendedPrivateKey(source?: SecureRandomSource): PrivateKey {
  return PrivateKey.from_extended_bytes(normalizeXprv(secureRandomBytes(96, source)).subarray(0, 64));
}

export function generateBip32PrivateKey(source?: SecureRandomSource): Bip32PrivateKey {
  return Bip32PrivateKey.from_raw_bytes(normalizeXprv(secureRandomBytes(96, source)));
}
