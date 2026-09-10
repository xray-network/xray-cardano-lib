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
  const delegations = NonEmptyCIP36DelegationList.new(CIP36Delegation.new(vote, 1));
  const registration = CIP36KeyRegistration.new(CIP36DelegationDistribution.new_weighted(delegations), signer, legacyAddress, 42n);
  const signature = privateKey.sign(registration.hash_to_sign(false));
  const view = CIP36RegistrationCbor.new(registration, CIP36RegistrationWitness.new(signature));
  view.verify(); assert.equal(signer.verify(registration.hash_to_sign(false), view.registration_witness().stake_witness()), true);

  const metadata = Metadata.new(); metadata.set(7n, TransactionMetadatum.new_text("unrelated")); view.add_to_metadata(metadata);
  assert.equal(metadata.len(), 3);
  const restored = CIP36RegistrationCbor.try_from_metadata(metadata);
  assert.equal(restored.key_registration().nonce(), 42n);
  assert.equal(CIP36RegistrationCbor.from_metadata_bytes(metadata.to_cbor_bytes()).registration_witness().stake_witness().to_hex(), signature.to_hex());

  const invalid = NonEmptyCIP36DelegationList.new(CIP36Delegation.new(vote, 0));
  const invalidView = CIP36RegistrationCbor.new(CIP36KeyRegistration.new(CIP36DelegationDistribution.new_weighted(invalid), signer, legacyAddress, 43n), CIP36RegistrationWitness.new(signature));
  assert.throws(() => invalidView.verify(), /Invalid delegation weights/);
});

test("CIP36 weighted metadata requires at least one positive weight before mutation", () => {
  const privateKey = PrivateKey.from_normal_bytes(Uint8Array.from({ length: 32 }, (_, index) => index + 1));
  for (const weights of [[0], [0, 0], [1], [0, 1], [1, 0], [0xffff_ffff]]) {
    const list = NonEmptyCIP36DelegationList.new(CIP36Delegation.new(vote, weights[0]));
    for (const weight of weights.slice(1)) list.add(CIP36Delegation.new(vote, weight));
    const registration = CIP36KeyRegistration.new(CIP36DelegationDistribution.new_weighted(list), privateKey.to_public(), legacyAddress, 42n);
    const witness = CIP36RegistrationWitness.new(privateKey.sign(registration.hash_to_sign(false)));
    const view = CIP36RegistrationCbor.new(registration, witness);
    const metadata = Metadata.new();
    metadata.set(7n, TransactionMetadatum.new_text("unrelated"));
    metadata.set(61284n, TransactionMetadatum.new_text("existing registration"));
    metadata.set(61285n, TransactionMetadatum.new_text("existing witness"));
    const before = metadata.to_cbor_bytes();
    const operations = [() => view.verify(), () => view.to_metadata_bytes(), () => view.try_into_metadata(), () => view.add_to_metadata(metadata)];
    for (const operation of operations) {
      if (weights.some((weight) => weight > 0)) assert.doesNotThrow(operation, `weights ${weights}`);
      else {
        assert.throws(operation, { name: "TypeError", message: "Invalid delegation weights" }, `weights ${weights}`);
        assert.deepEqual(metadata.to_cbor_bytes(), before);
      }
    }
    assert.equal(privateKey.to_public().verify(registration.hash_to_sign(false), witness.stake_witness()), true);
  }
  const legacy = CIP36KeyRegistration.new(CIP36DelegationDistribution.new_legacy(vote), privateKey.to_public(), legacyAddress, 42n);
  const legacyView = CIP36RegistrationCbor.new(legacy, CIP36RegistrationWitness.new(privateKey.sign(legacy.hash_to_sign(false))));
  assert.doesNotThrow(() => legacyView.verify());
  assert.doesNotThrow(() => legacyView.to_metadata_bytes());
  for (const weight of [-1, 0x1_0000_0000, 0.5, NaN]) assert.throws(() => CIP36Delegation.new(vote, weight), /uint32/);
  assert.throws(() => CIP36DelegationDistribution.from_cbor_hex("80"), /must not be empty/);
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

function metadataViewFixture(deregister) {
  const key = PrivateKey.from_normal_bytes(Uint8Array.from({ length: 32 }, (_, index) => index + 1));
  const Payload = deregister ? CIP36KeyDeregistration : CIP36KeyRegistration;
  const View = deregister ? CIP36DeregistrationCbor : CIP36RegistrationCbor;
  const Witness = deregister ? CIP36DeregistrationWitness : CIP36RegistrationWitness;
  const payload = deregister
    ? Payload.new(key.to_public(), 42n)
    : Payload.new(CIP36DelegationDistribution.new_legacy(vote), key.to_public(), legacyAddress, 42n);
  const node = decodeCbor(payload.to_cbor_bytes());
  node.encoding = { kind: "indefinite" };
  node.entries.find(([label]) => label.value === (deregister ? 2n : 4n))[1].encoding = { width: 8 };
  const preserved = Payload.from_cbor_bytes(encodeCbor(node));
  const view = View.new(preserved, Witness.new(key.sign(preserved.hash_to_sign(false))));
  return { View, view, payload: preserved, primaryLabel: deregister ? 61286n : 61284n };
}

for (const deregister of [false, true]) {
  const kind = deregister ? "deregistration" : "registration";
  test(`CIP36 ${kind} views round-trip complete metadata and own their buffers`, () => {
    const { View, view, payload } = metadataViewFixture(deregister);
    const signingHash = payload.hash_to_sign(false);
    for (const encoding of [{ kind: "indefinite" }, { kind: "definite", width: 2 }]) {
      const fields = decodeCbor(view.to_metadata_bytes()).entries;
      const text = { kind: "text", value: "unrelated", encoding: { kind: "indefinite", chunks: [
        { value: "un", width: 1 }, { value: "related", width: 2 },
      ] } };
      const original = encodeCbor({ kind: "map", encoding, entries: [
        [{ ...u(7n), encoding: { width: 8 } }, text],
        [fields[1][0], { ...fields[1][1], encoding: { kind: "indefinite" } }],
        [u(8n), { kind: "array", values: [b(Uint8Array.of(1, 2)), u(3n)], encoding: { kind: "indefinite" } }],
        fields[0],
      ] });
      const input = original.slice();
      const restored = View.from_metadata_bytes(input);
      input.fill(0);
      assert.deepEqual(restored.to_metadata_bytes(), original);
      const returned = restored.to_metadata_bytes(); returned.fill(0);
      assert.deepEqual(restored.to_metadata_bytes(), original);
      const metadata = restored.try_into_metadata();
      assert.ok(metadata instanceof Metadata);
      assert.deepEqual(metadata.to_cbor_bytes(), original);
      assert.deepEqual(metadata.to_canonical_cbor_bytes(), Metadata.from_cbor_bytes(original).to_canonical_cbor_bytes());
      assert.notDeepEqual(metadata.to_canonical_cbor_bytes(), original);
      const objectView = View.try_from_metadata(metadata);
      metadata.set(7n, TransactionMetadatum.new_text("changed"));
      assert.deepEqual(objectView.to_metadata_bytes(), original);
      assert.deepEqual(restored.to_metadata_bytes(), original);
      const roundTripPayload = deregister ? restored.key_deregistration() : restored.key_registration();
      assert.deepEqual(roundTripPayload.hash_to_sign(false), signingHash);
      assert.equal(restored.to_json(), view.to_json());
    }
  });

  test(`CIP36 ${kind} merge preserves caller labels without importing unrelated view labels`, () => {
    const { View, view, primaryLabel } = metadataViewFixture(deregister);
    const original = view.try_into_metadata();
    original.set(7n, TransactionMetadatum.new_text("view"));
    original.set(8n, TransactionMetadatum.new_text("view only"));
    const restored = View.try_from_metadata(original);
    const caller = Metadata.new();
    caller.set(7n, TransactionMetadatum.new_text("caller"));
    caller.set(9n, TransactionMetadatum.new_text("caller only"));
    restored.add_to_metadata(caller);
    assert.equal(caller.get(7n).as_text(), "caller");
    assert.equal(caller.get(8n), undefined);
    assert.equal(caller.get(9n).as_text(), "caller only");
    assert.deepEqual(caller.get(primaryLabel).to_cbor_bytes(), original.get(primaryLabel).to_cbor_bytes());
    assert.deepEqual(caller.get(61285n).to_cbor_bytes(), original.get(61285n).to_cbor_bytes());
    assert.equal(restored.try_into_metadata().get(7n).as_text(), "view");
    assert.equal(restored.try_into_metadata().get(8n).as_text(), "view only");
  });

  test(`CIP36 ${kind} views reject malformed reserved fields and duplicate labels`, () => {
    const { View, view, primaryLabel } = metadataViewFixture(deregister);
    const node = decodeCbor(view.to_metadata_bytes());
    for (const label of [primaryLabel, 61285n, 7n]) {
      const entries = [...node.entries, [u(7n), u(1n)], [u(label), u(2n)]];
      assert.throws(() => View.from_metadata_bytes(encodeCbor({ ...node, entries })), /duplicate metadata label/);
    }
    const primary = node.entries.find(([key]) => key.value === primaryLabel)[1];
    const invalid = { ...primary, entries: [...primary.entries, [u(99n), u(1n)]] };
    assert.throws(() => View.from_metadata_bytes(encodeCbor({ ...node, entries: node.entries.map(([key, item]) => [key, key.value === primaryLabel ? invalid : item]) })), /unknown CIP36 map key/);
    assert.throws(() => View.from_metadata_bytes(encodeCbor({ ...node, entries: node.entries.filter(([key]) => key.value !== 61285n) })), /missing CIP36 map key/);
    assert.throws(() => View.from_metadata_bytes(view.to_metadata_bytes().slice(0, -1)));
  });
}
