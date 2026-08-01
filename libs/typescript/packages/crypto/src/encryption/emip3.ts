import { bytesToHex, hexToBytes } from "@xray-network/xray-cardano-lib-core";
import {
  chacha20Poly1305Decrypt,
  chacha20Poly1305Encrypt,
  concatenateBytes,
  pbkdf2Sha512,
} from "../primitives/crypto.js";

const ITERATIONS = 19_162;
const SALT_SIZE = 32;
const NONCE_SIZE = 12;
const TAG_SIZE = 16;
const METADATA_SIZE = SALT_SIZE + NONCE_SIZE + TAG_SIZE;

function deriveKey(password: Uint8Array, salt: Uint8Array): Uint8Array {
  return pbkdf2Sha512(password, salt, ITERATIONS, 32);
}

export function emip3_encrypt_with_password(
  passwordHex: string,
  saltHex: string,
  nonceHex: string,
  dataHex: string,
): string {
  const password = hexToBytes(passwordHex);
  const salt = hexToBytes(saltHex);
  const nonce = hexToBytes(nonceHex);
  const data = hexToBytes(dataHex);
  if (salt.length !== SALT_SIZE) throw new TypeError(`Salt length must be ${SALT_SIZE} bytes`);
  if (nonce.length !== NONCE_SIZE) throw new TypeError(`Nonce length must be ${NONCE_SIZE} bytes`);
  if (password.length === 0) throw new TypeError("Password must not be empty");

  const key = deriveKey(password, salt);
  try {
    const encrypted = chacha20Poly1305Encrypt(key, nonce, data);
    return bytesToHex(concatenateBytes(salt, nonce, encrypted.tag, encrypted.ciphertext));
  } finally {
    key.fill(0);
    password.fill(0);
  }
}

export function emip3_decrypt_with_password(passwordHex: string, dataHex: string): string {
  const password = hexToBytes(passwordHex);
  const data = hexToBytes(dataHex);
  if (data.length <= METADATA_SIZE) {
    throw new TypeError(`Encrypted input must contain more than ${METADATA_SIZE} bytes`);
  }
  const salt = data.subarray(0, SALT_SIZE);
  const nonce = data.subarray(SALT_SIZE, SALT_SIZE + NONCE_SIZE);
  const tag = data.subarray(SALT_SIZE + NONCE_SIZE, METADATA_SIZE);
  const ciphertext = data.subarray(METADATA_SIZE);
  const key = deriveKey(password, salt);
  try {
    return bytesToHex(chacha20Poly1305Decrypt(key, nonce, ciphertext, tag));
  } catch (error) {
    throw new TypeError("EMIP-3 decryption failed", { cause: error });
  } finally {
    key.fill(0);
    password.fill(0);
  }
}
