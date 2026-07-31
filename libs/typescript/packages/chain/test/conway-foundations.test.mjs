import assert from "node:assert/strict";
import test from "node:test";
import {
  Anchor,
  AnchorDocHash,
  AssetName,
  CostModels,
  Credential,
  Ed25519KeyHash,
  ExUnits,
  Int,
  MapAssetNameToCoin,
  MapU64ToArrI64,
  NetworkId,
  ProtocolVersion,
  Rational,
  TransactionMetadatum,
  UnitInterval,
  Url,
  Value,
  VRFCert,
} from "../../runtime/dist/esm/index.js";
import { decodeCbor } from "../../core/dist/esm/index.js";

const bytes = (hex) => Uint8Array.from(Buffer.from(hex, "hex"));

test("foundational scalar and bounded types round-trip their wire forms", () => {
  const cases = [
    [AssetName, AssetName.new(Uint8Array.of(65))],
    [NetworkId, NetworkId.new(1n)],
    [Rational, Rational.new(1n, 2n)],
    [UnitInterval, UnitInterval.new(1n, 2n)],
    [ExUnits, ExUnits.new(3n, 4n)],
  ];
  for (const [Type, value] of cases) {
    const decoded = Type.from_cbor_hex(value.to_cbor_hex());
    assert.equal(decoded.to_cbor_hex(), value.to_cbor_hex());
    assert.equal(decoded.to_canonical_cbor_hex(), value.to_canonical_cbor_hex());
    assert.equal(decoded.to_json(), value.to_json());
  }
  assert.throws(() => AssetName.new(new Uint8Array(33)));
  assert.throws(() => UnitInterval.new(2n, 1n));
  assert.equal(ExUnits.new(0x7fff_ffff_ffff_ffffn, 0n).checked_add(ExUnits.new(1n, 0n)), undefined);
  assert.throws(() => ExUnits.new(0x8000_0000_0000_0000n, 0n));
});

test("assets use structural keys and checked component-wise arithmetic", () => {
  const canonical = AssetName.new(Uint8Array.of(0xaa));
  const noncanonical = AssetName.from_cbor_hex("5801aa");
  const bundle = MapAssetNameToCoin.new();
  assert.equal(bundle.insert(canonical, 2n), undefined);
  assert.equal(bundle.insert(noncanonical, 3n), 2n);
  assert.equal(bundle.len(), 1);

  const value = Value.from_coin(5n);
  assert.equal(Value.from_cbor_hex(value.to_cbor_hex()).coin(), 5n);
  assert.equal(value.checked_sub(Value.from_coin(6n)), undefined);
  assert.equal(value.checked_add(Value.from_coin(7n))?.coin(), 12n);
});

test("certificate, governance, crypto, and block family representatives round-trip", () => {
  const keyHash = Ed25519KeyHash.from_hex("00".repeat(28));
  const credential = Credential.new_pub_key(keyHash);
  const docHash = AnchorDocHash.from_hex("00".repeat(32));
  const anchor = Anchor.new(Url.new("https://x"), docHash);
  const protocol = ProtocolVersion.new(1n, 2n);
  const vrf = VRFCert.new(Uint8Array.of(1), new Uint8Array(80));
  for (const [Type, value] of [[Credential, credential], [Anchor, anchor], [ProtocolVersion, protocol], [VRFCert, vrf]]) {
    const decoded = Type.from_cbor_hex(value.to_cbor_hex());
    assert.equal(decoded.to_canonical_cbor_hex(), value.to_canonical_cbor_hex());
    assert.equal(decoded.to_json(), value.to_json());
  }
});

test("metadata bounds and duplicate maps remain lossless", () => {
  const integer = TransactionMetadatum.new_int(Int.new(42n));
  assert.equal(integer.to_cbor_hex(), "182a");
  assert.equal(TransactionMetadatum.from_cbor_hex(integer.to_cbor_hex()).to_json(), integer.to_json());
  assert.throws(() => TransactionMetadatum.new_bytes(new Uint8Array(65)));
  assert.throws(() => TransactionMetadatum.new_text("x".repeat(65)));
});

test("Plutus V1 language views preserve the historical embedded indefinite-list encoding", () => {
  const map = MapU64ToArrI64.new();
  map.insert(0n, BigInt64Array.from([1n, -1n]));
  const models = CostModels.new(map);
  assert.equal(Buffer.from(models.language_views_encoding()).toString("hex"), "a14100449f0120ff");
  assert.deepEqual([...models.get().get(0n)], [1n, -1n]);
});

test("lossless records preserve wire form and canonicalize independently", () => {
  const wire = "9f781968747470733a2f2f780000000000000000000000005820" + "00".repeat(32) + "ff";
  assert.throws(() => Anchor.from_cbor_bytes(bytes(wire)));
  const decoded = Anchor.from_cbor_hex("9f6968747470733a2f2f785820" + "00".repeat(32) + "ff");
  assert.equal(decoded.to_cbor_hex(), "9f6968747470733a2f2f785820" + "00".repeat(32) + "ff");
  assert.equal(decoded.to_canonical_cbor_hex(), "826968747470733a2f2f785820" + "00".repeat(32));
  assert.equal(decodeCbor(decoded.to_cbor_bytes()).kind, "array");
});
