import assert from "node:assert/strict";
import test from "node:test";

import {
  AssetName,
  CIP25ChunkableString,
  CIP25ChunkableStringKind,
  CIP25FilesDetails,
  CIP25FilesDetailsList,
  CIP25LabelMetadata,
  CIP25Metadata,
  CIP25MetadataDetails,
  CIP25MiniMetadataDetails,
  CIP25String64,
  CIP25Version,
  PolicyId,
} from "@xray-network/cardano-cip/cip25";
import { TransactionMetadatum } from "@xray-network/cardano-chain";
import { decodeCbor } from "@xray-network/cardano-core";

const policy = PolicyId.from_raw_bytes(Uint8Array.from({ length: 28 }, (_, index) => index + 1));
const asset = AssetName.from_raw_bytes(Uint8Array.of(0xca, 0xfe, 0xd0, 0x0d));

test("CIP25String64 and CIP25ChunkableString enforce metadata byte boundaries", () => {
  assert.equal(CIP25String64.new("x".repeat(64)).get(), "x".repeat(64));
  assert.throws(() => CIP25String64.new("é".repeat(33)), /64 UTF-8 bytes/);
  const chunked = CIP25ChunkableString.from_string("x".repeat(65));
  assert.equal(chunked.kind(), CIP25ChunkableStringKind.Chunked);
  assert.equal(chunked.as_chunked().len(), 2);
  assert.equal(chunked.to_string(), "x".repeat(65));
  assert.equal(CIP25ChunkableString.from_cbor_bytes(chunked.to_cbor_bytes()).to_string(), "x".repeat(65));
  assert.throws(() => CIP25ChunkableString.from_string(`${"x".repeat(63)}é`), /encoded data was not valid/);
});

test("CIP25 V1 text keys and V2 byte keys round-trip through label 721 metadata", () => {
  const details = CIP25MetadataDetails.new(CIP25String64.new("Metadata Name"), CIP25ChunkableString.from_string("ipfs://image"));
  details.set_media_type(CIP25String64.new("image/*"));
  details.set_description(CIP25ChunkableString.from_string("description"));
  const files = CIP25FilesDetailsList.new();
  files.add(CIP25FilesDetails.new(CIP25String64.new("preview"), CIP25String64.new("image/png"), CIP25ChunkableString.from_string("ipfs://preview")));
  details.set_files(files);

  const v1 = CIP25LabelMetadata.new(CIP25Version.V1);
  v1.set(policy, AssetName.from_raw_bytes(new TextEncoder().encode("Token")), details);
  const v1Node = decodeCbor(v1.to_cbor_bytes()); assert.equal(v1Node.kind, "map"); assert.equal(v1Node.entries[0][0].kind, "text");
  assert.throws(() => v1.set(policy, AssetName.from_raw_bytes(Uint8Array.of(0xff)), details), /encoded data was not valid/);

  const v2 = CIP25LabelMetadata.new(CIP25Version.V2); v2.set(policy, asset, details);
  const metadata = CIP25Metadata.new(v2), chainMetadata = metadata.to_metadata();
  assert.equal(chainMetadata.len(), 1);
  assert.equal(CIP25Metadata.from_metadata(chainMetadata).key_721().get(policy, asset).description().to_string(), "description");
  const v2Node = decodeCbor(v2.to_cbor_bytes()); assert.equal(v2Node.kind, "map");
  const data = v2Node.entries.find(([key]) => key.kind === "text" && key.value === "data")[1]; assert.equal(data.kind, "map"); assert.equal(data.entries[0][0].kind, "bytes");
});

test("CIP25 loose parsing and permissive noisy metadata retain historical behavior", () => {
  const uppercase = TransactionMetadatum.from_cbor_hex("a2644e616d656955707065726361736565496d616765646e6f7065");
  assert.equal(CIP25MiniMetadataDetails.loose_parse(uppercase).name().get(), "Uppercase");
  const fallback = TransactionMetadatum.from_cbor_hex("a262696462303065696d6167657835697066733a2f2f516d5366595446384234756136684664723655526452445a425a39466a43514e556444634c723266375038786e33");
  const mini = CIP25MiniMetadataDetails.loose_parse(fallback);
  assert.equal(mini.name().get(), "00"); assert.match(mini.image().to_string(), /^ipfs:/u);

  const noisy = "bf1902d1a36464617461a2581cbaadf00dbaadf00dbaadf00dbaadf00dbaadf00dbaadf00dbaadf00da344cafed00da6646e616d656d4d65746164617461204e616d656566696c657382a4637372636473726331646e616d656966696c656e616d6531696d65646961547970656966696c657479706531816864736b6a66616b7381a1403864a3637372636473726332646e616d656966696c656e616d6532696d65646961547970656966696c65747970653265696d6167657821687474733a2f2f736f6d652e776562736974652e636f6d2f696d6167652e706e67696d656469615479706567696d6167652f2a6b6465736372697074696f6e776465736372697074696f6e206f662074686973204e4654a14038641832a1403864a140386481a1403864816864736b6a66616b73a1403864a14038646776657273696f6e02a1403864a14038641905398144baadf00dff";
  const parsed = CIP25Metadata.from_cbor_hex(noisy);
  const noisyPolicy = PolicyId.from_hex("baadf00d".repeat(7));
  assert.equal(parsed.key_721().version(), CIP25Version.V2);
  assert.notEqual(parsed.to_cbor_hex(), noisy);
  assert.equal(parsed.key_721().get(noisyPolicy, asset).name().get(), "Metadata Name");
});
