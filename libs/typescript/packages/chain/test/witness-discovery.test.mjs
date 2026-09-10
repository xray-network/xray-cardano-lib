import assert from "node:assert/strict";
import test from "node:test";
import { decodeCbor, encodeCbor } from "../../core/dist/esm/index.js";
import { DatumHash, Ed25519KeyHash, ScriptHash, TransactionHash } from "../../crypto/dist/esm/index.js";
import {
  Address,
  ByronAddress,
  Certificate,
  Credential,
  DatumOption,
  EnterpriseAddress,
  ExUnits,
  LegacyRedeemer,
  NativeScript,
  PlutusData,
  PlutusScript,
  PlutusV1Script,
  RedeemerTag,
  RequiredSigners,
  RewardAddress,
  ScriptRef,
  StakeDeregistration,
  Transaction,
  TransactionBody,
  TransactionInput,
  TransactionOutput,
  TransactionUnspentOutput,
  TransactionWitnessSet,
  Value,
  discover_required_witnesses,
  hash_plutus_data,
} from "../dist/esm/index.js";

const uint = (value) => ({ kind: "unsigned", value, encoding: { width: 0 } });
const bytes = (value) => ({ kind: "bytes", value, encoding: { kind: "definite", width: 0 } });
const array = (values) => ({ kind: "array", values, encoding: { kind: "definite", width: 0 } });
const map = (entries) => ({ kind: "map", entries, encoding: { kind: "definite", width: 0 } });
const tagged = (values) => ({ kind: "tag", tag: 258n, value: array(values), encoding: { width: 2 } });
const node = (value) => decodeCbor(value.to_cbor_bytes());

function hash28(fill) { return Ed25519KeyHash.from_raw_bytes(new Uint8Array(28).fill(fill)); }
function input(fill, index = 0n) { return TransactionInput.new(TransactionHash.from_raw_bytes(new Uint8Array(32).fill(fill)), index); }
function output(address, datum, reference) { return TransactionOutput.new(address, Value.from_coin(2_000_000n), datum, reference); }
function transaction(bodyEntries, witnessEntries = []) {
  const supplied = new Set(bodyEntries.map(([key]) => key.value));
  const body = TransactionBody.from_cbor_bytes(encodeCbor(map([
    [uint(0n), tagged([])],
    [uint(1n), array([])],
    [uint(2n), uint(0n)],
  ].filter(([key]) => !supplied.has(key.value)).concat(bodyEntries))));
  const witnesses = TransactionWitnessSet.from_cbor_bytes(encodeCbor(map(witnessEntries)));
  return Transaction.new(body, witnesses, true);
}

test("required witness discovery covers resolved inputs, collateral, certs, withdrawals, signers, native scripts, and bootstrap witnesses", () => {
  const payment = hash28(1), collateral = hash28(2), nativeSigner = hash28(3), withdrawal = hash28(4), explicit = hash28(5);
  const native = NativeScript.new_script_pubkey(nativeSigner);
  const certificate = Certificate.new_stake_deregistration(StakeDeregistration.new(Credential.new_script(native.hash())));
  const reward = RewardAddress.new(0, Credential.new_pub_key(withdrawal));
  const paymentInput = input(10), byronInput = input(11), collateralInput = input(12);
  const byron = ByronAddress.from_base58("Ae2tdPwUPEZGUEsuMAhvDcy94LKsZxDjCbgaiBBMgYpR8sKf96xJmit7Eho");
  const tx = transaction([
    [uint(0n), tagged([node(paymentInput), node(byronInput)])],
    [uint(4n), tagged([node(certificate)])],
    [uint(5n), map([[bytes(reward.to_address().to_raw_bytes()), uint(7n)]])],
    [uint(13n), tagged([node(collateralInput)])],
    [uint(14n), tagged([bytes(explicit.to_raw_bytes())])],
  ], [[uint(1n), tagged([node(native)])]]);
  const discovered = discover_required_witnesses(tx, [
    TransactionUnspentOutput.new(paymentInput, output(EnterpriseAddress.new(0, Credential.new_pub_key(payment)).to_address())),
    TransactionUnspentOutput.new(byronInput, output(byron.to_address())),
    TransactionUnspentOutput.new(collateralInput, output(EnterpriseAddress.new(0, Credential.new_pub_key(collateral)).to_address())),
  ]);

  for (const hash of [payment, collateral, nativeSigner, withdrawal, explicit]) assert.ok(discovered.vkeys.has(hash.to_hex()));
  assert.equal(discovered.bootstraps.size, 1);
  assert.ok(discovered.scripts.has(native.hash().to_hex()));
  assert.equal(discovered.redeemers.size, 0);
});

test("required witness discovery identifies datum, Plutus redeemer, and matching reference script", () => {
  const plutus = PlutusScript.from_v1(PlutusV1Script.new(Uint8Array.of(1, 2, 3)));
  const datum = PlutusData.from_cbor_bytes(Uint8Array.of(0));
  const datumHash = hash_plutus_data(datum);
  const spendInput = input(20), referenceInput = input(21);
  const scriptAddress = EnterpriseAddress.new(0, Credential.new_script(plutus.hash())).to_address();
  const reference = ScriptRef.from_cbor_bytes(encodeCbor({
    kind: "tag", tag: 24n, value: bytes(plutus.to_script().to_cbor_bytes()), encoding: { width: 0 },
  }));
  const extraRedeemer = LegacyRedeemer.new(RedeemerTag.Mint, 4n, datum, ExUnits.new(1n, 2n));
  const tx = transaction([
    [uint(0n), tagged([node(spendInput)])],
    [uint(18n), tagged([node(referenceInput)])],
  ], [[uint(5n), array([node(extraRedeemer)])]]);
  const discovered = discover_required_witnesses(tx, [
    TransactionUnspentOutput.new(spendInput, output(scriptAddress, DatumOption.new(0n, datumHash))),
    TransactionUnspentOutput.new(referenceInput, output(EnterpriseAddress.new(0, Credential.new_pub_key(hash28(6))).to_address(), null, reference)),
  ]);

  assert.ok(discovered.plutusData.has(datumHash.to_hex()));
  assert.ok(discovered.scriptRefs.has(plutus.hash().to_hex()));
  assert.equal(discovered.scripts.size, 0);
  assert.ok(discovered.redeemers.has(`${RedeemerTag.Spend}:0`));
  assert.ok(discovered.redeemers.has(`${RedeemerTag.Mint}:4`));
});

test("required witness discovery rejects missing and conflicting resolutions", () => {
  const source = input(30), address = EnterpriseAddress.new(0, Credential.new_pub_key(hash28(7))).to_address();
  const tx = transaction([[uint(0n), tagged([node(source)])]]);
  assert.throws(() => discover_required_witnesses(tx, []), /missing resolved transaction input/);
  assert.throws(() => discover_required_witnesses(tx, [
    TransactionUnspentOutput.new(source, output(address)),
    TransactionUnspentOutput.new(source, TransactionOutput.new(address, Value.from_coin(3_000_000n))),
  ]), /conflicting resolved transaction input/);
  assert.throws(() => TransactionBody.from_cbor_bytes(encodeCbor(map([
    [uint(0n), tagged([uint(0n)])], [uint(1n), array([])], [uint(2n), uint(0n)],
  ]))), /TransactionBody/);
});

test("RequiredSigners.new creates the canonical empty tagged set", () => {
  assert.equal(RequiredSigners.new().to_canonical_cbor_hex(), "d9010280");
});

test("discovery hash owners remain nominal", () => {
  assert.ok(DatumHash.from_raw_bytes(new Uint8Array(32)) instanceof DatumHash);
  assert.ok(ScriptHash.from_raw_bytes(new Uint8Array(28)) instanceof ScriptHash);
  assert.ok(Address.from_raw_bytes(EnterpriseAddress.new(0, Credential.new_pub_key(hash28(8))).to_address().to_raw_bytes()) instanceof Address);
});
