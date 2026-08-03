import assert from "node:assert/strict";
import test from "node:test";
import { ScriptHash } from "@xray-network/xray-cardano-lib-crypto";
import { BigInteger } from "@xray-network/xray-cardano-lib-core";
import { ConstrPlutusData, PlutusData, PlutusDataList, PlutusMap } from "@xray-network/xray-cardano-lib-chain";
import { encode_asset_name_label, decode_asset_name_label, make_labeled_asset_name, split_labeled_asset_name } from "@xray-network/xray-cardano-lib-cip/cip67";
import { CIP68Datum, CIP68TokenClass, make_cip68_reference_asset_name, make_cip68_user_asset_name, validate_cip68_asset_pair } from "@xray-network/xray-cardano-lib-cip/cip68";

const vectors = [[0,"00000000"],[1,"00001070"],[23,"00017650"],[99,"000632e0"],[533,"00215410"],[2000,"007d0550"],[4567,"011d7690"],[11111,"02b670b0"],[49328,"0c0b0f40"],[65535,"0ffff240"],[222,"000de140"]];
test("CIP-67 official label vectors and strict splitting", () => {
  for (const [label, expected] of vectors) { const encoded = encode_asset_name_label(label); assert.equal(Buffer.from(encoded).toString("hex"), expected); assert.equal(decode_asset_name_label(encoded), label); }
  const value = make_labeled_asset_name(222, Uint8Array.of(1,2,3)), parts = split_labeled_asset_name(value); assert.equal(parts.label, 222); assert.deepEqual(parts.content, Uint8Array.of(1,2,3));
  parts.content[0] = 9; assert.deepEqual(split_labeled_asset_name(value).content, Uint8Array.of(1,2,3));
  assert.throws(() => decode_asset_name_label(Uint8Array.of(0,0,0,1))); assert.throws(() => make_labeled_asset_name(1, new Uint8Array(29)));
});
test("CIP-68 name relationships preserve the existing asset and policy owners", () => {
  const policy = ScriptHash.from_raw_bytes(new Uint8Array(28)), content = Uint8Array.of(4,5);
  const user = make_cip68_user_asset_name(CIP68TokenClass.NFT, content), reference = make_cip68_reference_asset_name(content);
  assert.equal(validate_cip68_asset_pair(policy, user, policy, reference), CIP68TokenClass.NFT);
  assert.throws(() => validate_cip68_asset_pair(policy, user, policy, make_cip68_reference_asset_name(Uint8Array.of(9))));
});

test("CIP-68 enforces the strict three-field datum and class metadata", () => {
  const bytes = (value) => PlutusData.new_bytes(new TextEncoder().encode(value));
  const metadataMap = PlutusMap.new();
  metadataMap.append(bytes("name"), bytes("Example"));
  metadataMap.append(bytes("image"), bytes("ipfs://example"));
  const metadata = PlutusData.new_map(metadataMap);
  const datum = CIP68Datum.new(metadata, 3);
  const policy = ScriptHash.from_raw_bytes(new Uint8Array(28));
  const user = make_cip68_user_asset_name(CIP68TokenClass.NFT, Uint8Array.of(1));
  assert.doesNotThrow(() => datum.validate_for(CIP68TokenClass.NFT, policy, user));
  assert.equal(datum.version(), 3);
  const twoFields = PlutusData.new_constr_plutus_data(ConstrPlutusData.new(0n, PlutusDataList.from([
    metadata, PlutusData.new_integer(BigInteger.from_str("1")),
  ])));
  assert.throws(() => CIP68Datum.from_data(twoFields));
  const direct721 = PlutusMap.new(); direct721.append(bytes("721"), metadata);
  assert.throws(() => CIP68Datum.new(PlutusData.new_map(direct721), 2).validate_for(CIP68TokenClass.NFT, policy, user));
});
