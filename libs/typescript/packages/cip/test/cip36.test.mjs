import assert from "node:assert/strict";
import test from "node:test";

import {
  CIP36Delegation,
  CIP36DelegationDistribution,
  CIP36DelegationDistributionKind,
  CIP36DelegationList,
  CIP36DeregistrationCbor,
  CIP36DeregistrationWitness,
  CIP36KeyDeregistration,
  CIP36KeyRegistration,
  CIP36RegistrationCbor,
  CIP36RegistrationWitness,
  NonEmptyCIP36DelegationList,
} from "@xray-network/xray-cardano-lib-cip/cip36";
import {
  Address,
  Credential,
  EnterpriseAddress,
  Metadata,
  TransactionMetadatum,
} from "@xray-network/xray-cardano-lib-chain";
import { bytesToHex, decodeCbor, encodeCbor } from "@xray-network/xray-cardano-lib-core";
import { Ed25519KeyHash, PrivateKey, PublicKey } from "@xray-network/xray-cardano-lib-crypto";

const stakeBytes = Uint8Array.from([227, 205, 36, 4, 200, 77, 230, 95, 150, 145, 143, 24, 213, 180, 69, 188, 185, 51, 167, 205, 161, 142, 237, 237, 121, 69, 221, 25, 30, 67, 35, 105]);
const voteBytes = Uint8Array.from([0, 54, 239, 62, 31, 13, 63, 89, 137, 226, 209, 85, 234, 84, 189, 178, 167, 44, 76, 69, 108, 203, 149, 154, 244, 201, 72, 104, 244, 115, 245, 160]);
const stake = PublicKey.from_bytes(stakeBytes), vote = PublicKey.from_bytes(voteBytes);
const legacyAddress = EnterpriseAddress.new(0, Credential.new_pub_key(Ed25519KeyHash.from_raw_bytes(Uint8Array.from({ length: 28 }, (_, index) => index + 3)))).to_address();

test("sign_data matches the committed CIP36 legacy and weighted hashes", () => {
  const sourceLegacyAddress = Address.from_raw_bytes(Uint8Array.from([224, 114, 182, 23, 101, 120, 129, 227, 10, 209, 124, 70, 228, 1, 12, 156, 179, 235, 178, 68, 6, 83, 163, 77, 50, 33, 156, 131, 233]));
  const legacy = CIP36KeyRegistration.new(CIP36DelegationDistribution.new_legacy(vote), stake, sourceLegacyAddress, 1234n);
  assert.equal(bytesToHex(legacy.hash_to_sign(false)), "9946e71b5f6c16150cf431910a0f7dbb8084a992577847802e60d32becb3d6be");
  assert.equal(legacy.voting_purpose(), 0n);
  assert.equal(decodeCbor(legacy.to_cbor_bytes()).entries.some(([key]) => key.kind === "unsigned" && key.value === 5n), false);

  const weightedAddress = Address.from_raw_bytes(Uint8Array.from([0, 71, 119, 86, 30, 125, 158, 193, 18, 236, 48, 117, 114, 250, 236, 26, 255, 97, 255, 12, 254, 214, 141, 244, 205, 92, 132, 127, 24, 114, 182, 23, 101, 120, 129, 227, 10, 209, 124, 70, 228, 1, 12, 156, 179, 235, 178, 68, 6, 83, 163, 77, 50, 33, 156, 131, 233]));
  const delegations = NonEmptyCIP36DelegationList.new(CIP36Delegation.new(vote, 1));
  const weighted = CIP36KeyRegistration.new(CIP36DelegationDistribution.new_weighted(delegations), stake, weightedAddress, 1234n);
  assert.equal(bytesToHex(weighted.hash_to_sign(false)), "3110fbad72589a80de7fc174310e92dac35bbfece1690c2dce53c2235a9776fa");
  assert.equal(decodeCbor(weighted.to_cbor_bytes()).entries.some(([key]) => key.kind === "unsigned" && key.value === 5n), true);
});

test("CIP36 delegation variants, non-empty bounds, and preserved/canonical CBOR", () => {
  const list = CIP36DelegationList.new(); assert.throws(() => NonEmptyCIP36DelegationList.try_from(list), /must not be empty/);
  list.add(CIP36Delegation.new(vote, 0));
  const weighted = CIP36DelegationDistribution.new_weighted(NonEmptyCIP36DelegationList.try_from(list));
  assert.equal(weighted.kind(), CIP36DelegationDistributionKind.Weighted); assert.equal(weighted.as_weighted().len(), 1);
  const legacy = CIP36DelegationDistribution.new_legacy(vote); assert.equal(legacy.kind(), CIP36DelegationDistributionKind.Legacy); assert.deepEqual(legacy.as_legacy().to_raw_bytes(), voteBytes);

  const noncanonical = Uint8Array.from([0x9f, 0x58, 0x20, ...voteBytes, 0x1a, 0, 0, 0, 1, 0xff]);
  const delegation = CIP36Delegation.from_cbor_bytes(noncanonical);
  assert.deepEqual(delegation.to_cbor_bytes(), noncanonical);
  assert.deepEqual(delegation.to_canonical_cbor_bytes(), Uint8Array.from([0x82, 0x58, 0x20, ...voteBytes, 1]));
});

test("CIP36 registration metadata views sign, verify, and preserve unrelated labels", () => {
  const privateKey = PrivateKey.from_normal_bytes(Uint8Array.from({ length: 32 }, (_, index) => index + 1));
  const signer = privateKey.to_public();
  const delegations = NonEmptyCIP36DelegationList.new(CIP36Delegation.new(vote, 0));
  const registration = CIP36KeyRegistration.new(CIP36DelegationDistribution.new_weighted(delegations), signer, legacyAddress, 42n);
  const signature = privateKey.sign(registration.hash_to_sign(false));
  const view = CIP36RegistrationCbor.new(registration, CIP36RegistrationWitness.new(signature));
  view.verify(); assert.equal(signer.verify(registration.hash_to_sign(false), view.registration_witness().stake_witness()), true);

  const metadata = Metadata.new(); metadata.set(7n, TransactionMetadatum.new_text("unrelated")); view.add_to_metadata(metadata);
  assert.equal(metadata.len(), 3);
  const restored = CIP36RegistrationCbor.try_from_metadata(metadata);
  assert.equal(restored.key_registration().nonce(), 42n);
  assert.equal(CIP36RegistrationCbor.from_metadata_bytes(metadata.to_cbor_bytes()).registration_witness().stake_witness().to_hex(), signature.to_hex());

  const invalid = NonEmptyCIP36DelegationList.new(CIP36Delegation.new(vote, 1));
  const invalidView = CIP36RegistrationCbor.new(CIP36KeyRegistration.new(CIP36DelegationDistribution.new_weighted(invalid), signer, legacyAddress, 43n), CIP36RegistrationWitness.new(signature));
  assert.throws(() => invalidView.verify(), /Invalid delegation weights/);
});

test("CIP36 deregistration metadata views retain explicit default-purpose presence", () => {
  const privateKey = PrivateKey.from_normal_bytes(Uint8Array.from({ length: 32 }, (_, index) => 32 - index));
  const deregistration = CIP36KeyDeregistration.new(privateKey.to_public(), 99n);
  const absent = deregistration.to_cbor_bytes(); assert.equal(decodeCbor(absent).entries.length, 2);
  const explicit = CIP36KeyDeregistration.from_cbor_bytes(encodeCbor({ kind: "map", entries: [[u(1n), b(privateKey.to_public().to_raw_bytes())], [u(2n), u(99n)], [u(3n), u(0n)]], encoding: { kind: "indefinite" } }));
  assert.equal(explicit.voting_purpose(), 0n); assert.equal(decodeCbor(explicit.to_cbor_bytes()).entries.length, 3);

  const signature = privateKey.sign(deregistration.hash_to_sign(false));
  const view = CIP36DeregistrationCbor.new(deregistration, CIP36DeregistrationWitness.new(signature));
  const metadata = view.try_into_metadata(); assert.equal(metadata.len(), 2);
  assert.equal(CIP36DeregistrationCbor.from_metadata_bytes(view.to_metadata_bytes()).key_deregistration().nonce(), 99n);
  assert.equal(privateKey.to_public().verify(deregistration.hash_to_sign(false), view.deregistration_witness().stake_witness()), true);
});

function u(value) { return { kind: "unsigned", value, encoding: { width: 0 } }; }
function b(value) { return { kind: "bytes", value, encoding: { kind: "definite", width: 0 } }; }
