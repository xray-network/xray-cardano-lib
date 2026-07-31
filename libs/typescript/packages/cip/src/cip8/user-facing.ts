const PREFIX = "cms_";

function base64UrlEncode(bytes: Uint8Array): string {
  let binary = "";
  const chunkSize = 0x8000;
  for (let offset = 0; offset < bytes.length; offset += chunkSize) {
    const chunk = bytes.subarray(offset, offset + chunkSize);
    binary += String.fromCharCode(...chunk);
  }
  return btoa(binary)
    .replaceAll("+", "-")
    .replaceAll("/", "_")
    .replace(/=+$/u, "");
}

function base64UrlDecode(value: string, name: string): Uint8Array {
  if (value.length === 0 || !/^[A-Za-z0-9_-]+={0,2}$/u.test(value)) {
    throw new TypeError(`${name} is not valid base64url`);
  }
  const unpadded = value.replace(/=+$/u, "");
  if (unpadded.length % 4 === 1) throw new TypeError(`${name} is not valid base64url`);
  const normalized = unpadded.replaceAll("-", "+").replaceAll("_", "/");
  const padded = normalized.padEnd(normalized.length + (4 - normalized.length % 4) % 4, "=");
  let binary: string;
  try {
    binary = atob(padded);
  } catch (cause) {
    throw new TypeError(`${name} is not valid base64url`, { cause });
  }
  return Uint8Array.from(binary, (character) => character.charCodeAt(0));
}

function fnv1a32(bytes: Uint8Array): number {
  let hash = 0x811c9dc5;
  for (const byte of bytes) hash = Math.imul(hash ^ byte, 0x01000193) >>> 0;
  return hash;
}

function checksumBytes(bytes: Uint8Array): Uint8Array {
  const checksum = fnv1a32(bytes);
  return Uint8Array.of(
    checksum >>> 24,
    checksum >>> 16 & 0xff,
    checksum >>> 8 & 0xff,
    checksum & 0xff,
  );
}

export function encodeUserFacingMessage(bytes: Uint8Array): string {
  return `${PREFIX}${base64UrlEncode(bytes)}${base64UrlEncode(checksumBytes(bytes))}`;
}

export function decodeUserFacingMessage(value: string): Uint8Array {
  if (!value.startsWith(PREFIX)) {
    throw new TypeError('SignedMessage user-facing encoding must start with "cms_"');
  }
  const encoded = value.slice(PREFIX.length).replace(/=+$/u, "");
  if (encoded.length < 8) throw new TypeError("SignedMessage user-facing encoding is missing its checksum");
  const bodyText = encoded.slice(0, -6);
  const checksumText = encoded.slice(-6);
  const body = base64UrlDecode(bodyText, "SignedMessage body");
  const checksum = base64UrlDecode(checksumText, "SignedMessage checksum");
  if (checksum.length !== 4) throw new TypeError("SignedMessage checksum must contain four bytes");
  const expected = checksumBytes(body);
  for (let index = 0; index < expected.length; index += 1) {
    if (checksum[index] !== expected[index]) throw new TypeError("SignedMessage checksum does not match its body");
  }
  return body;
}
