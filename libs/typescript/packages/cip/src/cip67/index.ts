import { AssetName } from "@xray-network/xray-cardano-lib-chain/conway";

function checkedLabel(label: number): number {
  if (!Number.isInteger(label) || label < 0 || label > 0xffff) throw new RangeError("CIP-67 label must be an integer from 0 through 65535");
  return label;
}

function crc8(first: number, second: number): number {
  let crc = 0;
  for (const byte of [first, second]) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit += 1) crc = (crc & 0x80) === 0 ? (crc << 1) & 0xff : ((crc << 1) ^ 0x07) & 0xff;
  }
  return crc;
}

/** Proposed CIP-67 four-byte label codec. */
export function encode_asset_name_label(label: number): Uint8Array {
  const value = checkedLabel(label), high = value >>> 8, low = value & 0xff;
  return Uint8Array.of(high >>> 4, ((high & 0x0f) << 4) | (low >>> 4), ((low & 0x0f) << 4) | (crc8(high, low) >>> 4), (crc8(high, low) << 4) & 0xf0);
}

export function decode_asset_name_label(bytes: Uint8Array): number {
  if (bytes.length !== 4) throw new RangeError("CIP-67 label prefix must contain exactly four bytes");
  if (((bytes[0] ?? 0) & 0xf0) !== 0 || ((bytes[3] ?? 0) & 0x0f) !== 0) throw new TypeError("CIP-67 label bracket nibbles must be zero");
  const high = (((bytes[0] ?? 0) & 0x0f) << 4) | ((bytes[1] ?? 0) >>> 4);
  const low = (((bytes[1] ?? 0) & 0x0f) << 4) | ((bytes[2] ?? 0) >>> 4);
  const checksum = (((bytes[2] ?? 0) & 0x0f) << 4) | ((bytes[3] ?? 0) >>> 4);
  if (checksum !== crc8(high, low)) throw new TypeError("CIP-67 label checksum mismatch");
  return (high << 8) | low;
}

export function make_labeled_asset_name(label: number, content: Uint8Array): AssetName {
  if (content.length > 28) throw new RangeError("CIP-67 labeled asset content is limited to 28 bytes");
  const bytes = new Uint8Array(4 + content.length); bytes.set(encode_asset_name_label(label)); bytes.set(content, 4);
  return AssetName.new(bytes);
}

export function split_labeled_asset_name(assetName: AssetName): Readonly<{ label: number; content: Uint8Array }> {
  const bytes = assetName.to_raw_bytes();
  if (bytes.length < 4) throw new RangeError("CIP-67 labeled asset name is shorter than four bytes");
  return Object.freeze({ label: decode_asset_name_label(bytes.slice(0, 4)), content: bytes.slice(4) });
}
