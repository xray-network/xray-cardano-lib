import { assertByteLength, copyBytes } from "@xray-network/xray-cardano-lib-core";
import { Bip32PrivateKey, Bip32PublicKey, Ed25519Signature } from "../keys/ed25519.js";
import { concatenateBytes } from "../primitives/crypto.js";

export enum ByronSigningTag {
  Tx = 0x01,
  RedeemTx = 0x02,
  VssCert = 0x03,
  UpdateProposal = 0x04,
  Commitment = 0x05,
  UpdateVote = 0x06,
  MainBlock = 0x07,
  MainBlockLight = 0x08,
  MainBlockHeavy = 0x09,
  ProxySecretKey = 0x0a,
}

function encodeUnsigned(value: bigint): Uint8Array {
  if (value < 0n || value > 0xffffffffffffffffn) throw new RangeError("CBOR integer is out of range");
  if (value < 24n) return Uint8Array.of(Number(value));
  if (value <= 0xffn) return Uint8Array.of(0x18, Number(value));
  if (value <= 0xffffn) return Uint8Array.of(0x19, Number(value >> 8n), Number(value & 0xffn));
  if (value <= 0xffffffffn) {
    return Uint8Array.of(
      0x1a,
      Number((value >> 24n) & 0xffn),
      Number((value >> 16n) & 0xffn),
      Number((value >> 8n) & 0xffn),
      Number(value & 0xffn),
    );
  }
  const bytes = new Uint8Array(9);
  bytes[0] = 0x1b;
  for (let index = 8; index >= 1; index -= 1) {
    bytes[index] = Number(value & 0xffn);
    value >>= 8n;
  }
  return bytes;
}

function encodeBytes(bytes: Uint8Array): Uint8Array {
  const length = encodeUnsigned(BigInt(bytes.length));
  const header = copyBytes(length);
  header[0] = (header[0] ?? 0) | 0x40;
  return concatenateBytes(header, bytes);
}

export function byronProxySigningData(
  delegate: Bip32PublicKey,
  omega: bigint,
  protocolMagic: number,
): Uint8Array {
  if (!Number.isInteger(protocolMagic) || protocolMagic < 0 || protocolMagic > 0xffffffff) {
    throw new RangeError("Protocol magic must be a uint32");
  }
  const delegateBytes = delegate.to_raw_bytes();
  assertByteLength("Byron delegate public key", delegateBytes, 64);
  const inner = concatenateBytes(Uint8Array.of(0x30, 0x30), delegateBytes, encodeUnsigned(omega));
  return concatenateBytes(
    Uint8Array.of(ByronSigningTag.ProxySecretKey),
    encodeUnsigned(BigInt(protocolMagic)),
    encodeBytes(inner),
  );
}

export function signByronProxyCertificate(
  issuer: Bip32PrivateKey,
  delegate: Bip32PublicKey,
  omega: bigint,
  protocolMagic: number,
): Ed25519Signature {
  return issuer.to_raw_key().sign(byronProxySigningData(delegate, omega, protocolMagic));
}

export function verifyByronProxyCertificate(
  issuer: Bip32PublicKey,
  delegate: Bip32PublicKey,
  omega: bigint,
  protocolMagic: number,
  certificate: Ed25519Signature,
): boolean {
  return issuer
    .to_raw_key()
    .verify(byronProxySigningData(delegate, omega, protocolMagic), certificate);
}
