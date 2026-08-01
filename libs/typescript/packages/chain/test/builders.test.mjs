import assert from "node:assert/strict";
import test from "node:test";
import { decodeCbor, encodeCbor } from "../../core/dist/esm/index.js";
import { __setCoinSelectionRandomSourceForTests } from "../dist/esm/builder/transaction.js";
import { evaluatePhaseTwoRaw } from "../../plutus/dist/esm/api.js";
import {
  Bip32PrivateKey,
  PrivateKey,
  ScriptHash,
  TransactionHash,
} from "../../crypto/dist/esm/index.js";
import {
  Address,
  AssetName,
  AuxiliaryData,
  ByronAddress,
  Certificate,
  ChangeSelectionAlgo,
  CoinSelectionStrategyCIP2,
  CostModels,
  Credential,
  EnterpriseAddress,
  ExUnitPrices,
  ExUnits,
  GovActionId,
  LinearFee,
  MapAssetNameToNonZeroInt64,
  MultiAsset,
  NativeScript,
  NativeScriptWitnessInfo,
  PartialPlutusWitness,
  PlutusData,
  PlutusScript,
  PlutusScriptWitness,
  PlutusV1Script,
  ProposalBuilder,
  ProposalProcedure,
  Rational,
  RedeemerSetBuilder,
  RedeemerTag,
  RedeemerWitnessKey,
  RequiredSigners,
  RequiredWitnessSet,
  RewardAddress,
  Script,
  ScriptRef,
  SingleCertificateBuilder,
  SingleInputBuilder,
  SingleMintBuilder,
  SingleWithdrawalBuilder,
  TransactionBuilder,
  TransactionBuilderConfigBuilder,
  TransactionInput,
  TransactionOutputBuilder,
  TransactionUnspentOutput,
  TransactionWitnessSetBuilder,
  Value,
  VoteBuilder,
  Voter,
  VotingProcedure,
  hash_transaction,
  make_icarus_bootstrap_witness,
  make_vkey_witness,
  min_ada_required,
} from "../dist/esm/index.js";

const key = PrivateKey.from_normal_bytes(Uint8Array.from({ length: 32 }, (_, index) => index + 1));
const paymentHash = key.to_public().hash();
const address = EnterpriseAddress.new(0, Credential.new_pub_key(paymentHash)).to_address();
const reward = RewardAddress.new(0, Credential.new_pub_key(paymentHash));

function config(overrides = {}) {
  const builder = TransactionBuilderConfigBuilder.new()
    .fee_algo(overrides.fee ?? LinearFee.new(1n, 10n, 0n))
    .pool_deposit(500n)
    .key_deposit(100n)
    .max_value_size(overrides.maxValueSize ?? 5_000)
    .max_tx_size(overrides.maxTxSize ?? 16_384)
    .coins_per_utxo_byte(1n)
    .ex_unit_prices(ExUnitPrices.new(Rational.new(1n, 10n), Rational.new(1n, 10n)))
    .collateral_percentage(150)
    .max_collateral_inputs(3);
  if (overrides.pureChange) builder.prefer_pure_change(true);
  return builder.build();
}

function input(index, coin, outputAddress = address, assets) {
  const hash = new Uint8Array(32); hash[31] = index;
  const txInput = TransactionInput.new(TransactionHash.from_raw_bytes(hash), BigInt(index));
  const output = TransactionOutputBuilder.new().with_address(outputAddress).next().with_value(Value.new(BigInt(coin), assets)).build().output();
  const result = outputAddress.payment_cred()?.as_pub_key() === undefined ? undefined : SingleInputBuilder.new(txInput, output).payment_key();
  return { txInput, output, result };
}

function output(coin, outputAddress = address, assets) {
  return TransactionOutputBuilder.new().with_address(outputAddress).next().with_value(Value.new(BigInt(coin), assets)).build();
}

function bodyField(body, field) {
  const value = decodeCbor(body.to_cbor_bytes());
  assert.equal(value.kind, "map");
  return value.entries.find(([keyNode]) => keyNode.kind === "unsigned" && keyNode.value === BigInt(field))?.[1];
}

test("add_output_amount, add_output_coin, and too-big output gates", () => {
  assert.throws(() => TransactionBuilderConfigBuilder.new().build(), /uninitialized/);
  const built = TransactionOutputBuilder.new().with_address(address).next().with_asset_and_min_required_coin(MultiAsset.new(), 2n).build();
  assert.ok(built.output().to_cbor_bytes().length > 0);
  assert.ok(built.output().to_js_value());
  assert.ok(min_ada_required(built.output(), 2n) <= Value.from_cbor_bytes(bodyFieldFromOutput(built.output(), 1)).coin());
  assert.throws(() => TransactionBuilder.new(config({ maxValueSize: 1 })).add_output(output(2_000_000n)), /maximum size/);
});

test("vkey_test, bootstrap_test, native_script_test, and witness requirements", () => {
  const requirement = RequiredWitnessSet.new(); requirement.add_vkey_key_hash(paymentHash);
  const witnesses = TransactionWitnessSetBuilder.new(); witnesses.add_required_wits(requirement);
  assert.throws(() => witnesses.try_build(), /missing/);
  const signature = make_vkey_witness(TransactionHash.from_raw_bytes(new Uint8Array(32)), key);
  witnesses.add_vkey(signature); witnesses.add_vkey(signature);
  assert.equal(witnesses.remaining_wits().len(), 0);
  const native = NativeScript.new_script_pubkey(paymentHash);
  witnesses.add_script(Script.new(0n, native)); witnesses.add_script(Script.new(0n, native));
  const built = decodeCbor(witnesses.try_build().to_cbor_bytes());
  assert.equal(witnessFieldLength(built, 0), 1);
  assert.equal(witnessFieldLength(built, 1), 1);

  const root = Bip32PrivateKey.from_bip39_entropy(new Uint8Array(32), new Uint8Array());
  const byron = ByronAddress.from_base58("Ae2tdPwUPEZGUEsuMAhvDcy94LKsZxDjCbgaiBBMgYpR8sKf96xJmit7Eho");
  const bootstrap = make_icarus_bootstrap_witness(TransactionHash.from_raw_bytes(new Uint8Array(32)), byron, root);
  witnesses.add_bootstrap(bootstrap); witnesses.add_bootstrap(bootstrap);
  assert.equal(witnessFieldLength(decodeCbor(witnesses.build().to_cbor_bytes()), 2), 1);
});

test("multisig input requirements and aggregate signer estimates are assembled", () => {
  const native = NativeScript.new_script_pubkey(paymentHash);
  const scriptAddress = EnterpriseAddress.new(0, Credential.new_script(native.hash())).to_address();
  const source = input(2, 4_000_000n, scriptAddress);
  assert.throws(() => SingleInputBuilder.new(source.txInput, source.output).payment_key(), /not a payment key/);
  const result = SingleInputBuilder.new(source.txInput, source.output).native_script(native, NativeScriptWitnessInfo.assume_signature_count());
  const builder = TransactionBuilder.new(config()); builder.add_input(result); builder.add_output(output(2_000_000n));
  const signed = builder.build(ChangeSelectionAlgo.Default, address);
  signed.add_vkey(make_vkey_witness(hash_transaction(signed.body()), key));
  assert.ok(signed.build_checked().to_cbor_bytes().length > 0);
});

test("test_redeemer_set_builder assigns deterministic indices and requires execution units", () => {
  const datum = PlutusData.from_cbor_bytes(Uint8Array.of(0));
  const plutus = PlutusScript.from_v1(PlutusV1Script.new(Uint8Array.of(1, 2, 3)));
  const signerList = requiredSigners(paymentHash);
  const partial = PartialPlutusWitness.new(PlutusScriptWitness.new_script(plutus), datum);
  const scriptAddress = EnterpriseAddress.new(0, Credential.new_script(plutus.hash())).to_address();
  const source = input(3, 4_000_000n, scriptAddress);
  const result = SingleInputBuilder.new(source.txInput, source.output).plutus_script_inline_datum(partial, signerList);
  const redeemers = RedeemerSetBuilder.new(); redeemers.add_spend(result);
  assert.throws(() => redeemers.build(false), /missing execution units/);
  redeemers.update_ex_units(RedeemerWitnessKey.new(RedeemerTag.Spend, 0n), ExUnits.new(7n, 11n));
  const built = redeemers.build(false);
  assert.equal(built.len(), 1);
  assert.deepEqual(built.get(0).to_js_value().slice(0, 2), [0, 0]);
});

test("build_tx_with_change, build_tx_without_change, and build_tx_exact_amount", () => {
  const builder = TransactionBuilder.new(config()); builder.add_input(input(4, 5_000_000n).result); builder.add_output(output(2_000_000n));
  const signed = builder.build(ChangeSelectionAlgo.Default, address);
  assert.ok((builder.get_fee_if_set() ?? 0n) > 0n);
  const outputs = bodyField(signed.body(), 1); assert.equal(outputs?.kind, "array"); assert.equal(outputs.values.length, 2);
  signed.add_vkey(make_vkey_witness(hash_transaction(signed.body()), key));
  const transaction = signed.build_checked();
  assert.equal(decodeCbor(transaction.to_cbor_bytes()).kind, "array");
  assert.ok(builder.full_size() <= 16_384);
  assert.equal(builder.output_sizes().length, 2);

  const exact = TransactionBuilder.new(config({ fee: LinearFee.new(0n, 200n, 0n) })); exact.add_input(input(5, 2_000_200n).result); exact.add_output(output(2_000_000n));
  assert.equal(exact.add_change_if_needed(address, true), false); assert.equal(exact.get_fee_if_set(), 200n);
});

test("tx_builder_cip2 largest-first and seeded random-improve strategies", (context) => {
  context.after(() => __setCoinSelectionRandomSourceForTests());
  const selected = new Map();
  for (const strategy of Object.values(CoinSelectionStrategyCIP2).filter((value) => typeof value === "number")) {
    if (strategy === CoinSelectionStrategyCIP2.RandomImprove || strategy === CoinSelectionStrategyCIP2.RandomImproveMultiAsset) {
      __setCoinSelectionRandomSourceForTests(recordedCoinSelectionSeed(0x5eed_c1f2));
    }
    const builder = TransactionBuilder.new(config()); builder.add_output(output(2_500_000n));
    for (let index = 10; index < 14; index += 1) builder.add_utxo(input(index, 1_000_000n).result);
    builder.select_utxos(strategy);
    assert.ok(builder.get_explicit_input().coin() >= 3_000_000n);
    selected.set(strategy, builder.get_explicit_input().to_cbor_hex());
  }
  __setCoinSelectionRandomSourceForTests(recordedCoinSelectionSeed(0x5eed_c1f2));
  const repeat = TransactionBuilder.new(config()); repeat.add_output(output(2_500_000n));
  for (let index = 10; index < 14; index += 1) repeat.add_utxo(input(index, 1_000_000n).result);
  repeat.select_utxos(CoinSelectionStrategyCIP2.RandomImprove);
  assert.equal(repeat.get_explicit_input().to_cbor_hex(), selected.get(CoinSelectionStrategyCIP2.RandomImprove));

  const policy = NativeScript.new_script_invalid_before(0n).hash();
  const asset = AssetName.new(Uint8Array.of(1));
  const targetAssets = MultiAsset.new(); targetAssets.insert(policy, asset, 5n);
  const highAsset = MultiAsset.new(); highAsset.insert(policy, asset, 10n);
  const highCoin = MultiAsset.new(); highCoin.insert(policy, asset, 1n);
  const assetFirst = TransactionBuilder.new(config()); assetFirst.add_output(output(2_500_000n, address, targetAssets));
  assetFirst.add_utxo(input(14, 4_000_000n, address, highAsset).result); assetFirst.add_utxo(input(15, 10_000_000n, address, highCoin).result);
  assetFirst.select_utxos(CoinSelectionStrategyCIP2.LargestFirstMultiAsset);
  assert.equal(assetFirst.get_explicit_input().coin(), 4_000_000n);

  const coinFirst = TransactionBuilder.new(config()); coinFirst.add_output(output(2_500_000n, address, targetAssets));
  coinFirst.add_utxo(input(14, 4_000_000n, address, highAsset).result); coinFirst.add_utxo(input(15, 10_000_000n, address, highCoin).result);
  coinFirst.select_utxos(CoinSelectionStrategyCIP2.LargestFirst);
  assert.equal(coinFirst.get_explicit_input().coin(), 14_000_000n);
});

test("mint, burn, native-asset change, and purification retain exact balances", () => {
  const native = NativeScript.new_script_invalid_before(0n);
  const policy = native.hash();
  const asset = AssetName.new(Uint8Array.of(65));
  const sourceAssets = MultiAsset.new(); sourceAssets.insert(policy, asset, 5n);
  const builder = TransactionBuilder.new(config()); builder.add_input(input(20, 8_000_000n, address, sourceAssets).result);
  const mintAssets = MapAssetNameToNonZeroInt64.new(); mintAssets.insert(asset, -2n);
  builder.add_mint(SingleMintBuilder.new(mintAssets).native_script(native, NativeScriptWitnessInfo.num_signatures(0)));
  builder.add_output(output(2_000_000n)); builder.build(ChangeSelectionAlgo.Default, address);
  assert.equal(builder.get_total_input().multi_asset()?.get_value(policy, asset), 5n);
  assert.equal(builder.get_total_output().multi_asset()?.get_value(policy, asset), 5n);
});

test("build_tx_with_certs, withdrawals, proposals, and votes", () => {
  const builder = TransactionBuilder.new(config()); builder.add_input(input(30, 8_000_000n).result); builder.add_output(output(2_000_000n));
  builder.add_cert(SingleCertificateBuilder.new(Certificate.new(1n, Credential.new_pub_key(paymentHash))).payment_key());
  builder.add_withdrawal(SingleWithdrawalBuilder.new(reward, 1_000n).payment_key());
  const proposal = ProposalProcedure.from_cbor_bytes(encodeCbor({
    kind: "array",
    values: [
      uintNode(200n),
      { kind: "bytes", value: reward.to_address().to_raw_bytes(), encoding: { kind: "definite", width: 0 } },
      { kind: "array", values: [uintNode(6n)], encoding: { kind: "definite", width: 0 } },
      {
        kind: "array",
        values: [
          { kind: "text", value: "", encoding: { kind: "definite", width: 0 } },
          { kind: "bytes", value: new Uint8Array(32), encoding: { kind: "definite", width: 0 } },
        ],
        encoding: { kind: "definite", width: 0 },
      },
    ],
    encoding: { kind: "definite", width: 0 },
  })); builder.add_proposal(ProposalBuilder.new().with_proposal(proposal).build());
  const voter = Voter.new(0n, paymentHash.to_raw_bytes());
  const action = GovActionId.from_cbor_bytes(encodeCbor({ kind: "array", values: [{ kind: "bytes", value: new Uint8Array(32), encoding: { kind: "definite", width: 0 } }, uintNode(0n)], encoding: { kind: "definite", width: 0 } }));
  const procedure = VotingProcedure.new(1n, null);
  builder.add_vote(VoteBuilder.new().with_vote(voter, action, procedure).build());
  const signed = builder.build(ChangeSelectionAlgo.Default, address);
  for (const field of [4, 5, 19, 20]) assert.ok(bodyField(signed.body(), field));
  assert.equal(builder.get_deposit(), 200n);
  assert.equal(builder.get_implicit_input().coin(), 1_100n);
});

test("test_collateral and build_tx_with_ref_input avoid double counting", () => {
  const builder = TransactionBuilder.new(config()); builder.add_input(input(40, 6_000_000n).result); builder.add_output(output(2_000_000n));
  const collateral = input(41, 2_000_000n); builder.add_collateral(collateral.result); builder.set_collateral_return(output(1_500_000n).output());
  const reference = input(42, 1_000_000n); builder.add_reference_input(TransactionUnspentOutput.new(reference.txInput, reference.output));
  const body = builder.build(ChangeSelectionAlgo.Default, address).body();
  assert.ok(bodyField(body, 13)); assert.ok(bodyField(body, 16)); assert.equal(bodyField(body, 17)?.value, 500_000n); assert.ok(bodyField(body, 18));
});

test("test_contract and UPLC-valued execution-unit flow", () => {
  const datum = PlutusData.from_cbor_bytes(Uint8Array.of(0));
  const plutus = PlutusScript.from_v1(PlutusV1Script.new(Uint8Array.of(
    0x4d, 0x01, 0x00, 0x00, 0x33, 0x22, 0x22, 0x20,
    0x05, 0x12, 0x00, 0x12, 0x00, 0x11,
  )));
  const partial = PartialPlutusWitness.new(PlutusScriptWitness.new_script(plutus), datum);
  const scriptAddress = EnterpriseAddress.new(0, Credential.new_script(plutus.hash())).to_address();
  const sourceInput = TransactionInput.new(TransactionHash.from_raw_bytes(Uint8Array.from({ length: 32 }, (_, index) => index === 31 ? 50 : 0)), 50n);
  const sourceOutput = TransactionOutputBuilder.new()
    .with_address(scriptAddress)
    .with_communication_data(datum)
    .next()
    .with_value(Value.from_coin(7_000_000n))
    .build()
    .output();
  const builder = TransactionBuilder.new(config());
  builder.add_input(SingleInputBuilder.new(sourceInput, sourceOutput).plutus_script(partial, requiredSigners(paymentHash), datum));
  builder.add_output(output(2_000_000n));
  const evaluation = builder.build_for_evaluation(ChangeSelectionAlgo.Default, address);
  assert.ok(bodyField(evaluation.draft_body(), 11)); assert.ok(evaluation.draft_tx().to_cbor_bytes().length > 0);
  const draft = decodeCbor(evaluation.draft_tx().to_cbor_bytes()); assert.equal(draft.kind, "array");
  assert.equal(witnessFieldLength(draft.values[1], 4), 1);
  const parameters = Array.from({ length: 332 }, () => uintNode(1n));
  const costModels = encodeCbor({
    kind: "map",
    entries: [[uintNode(0n), { kind: "array", values: parameters, encoding: { kind: "definite", width: 0 } }]],
    encoding: { kind: "definite", width: 0 },
  });
  const [[redeemerBytes, result]] = evaluatePhaseTwoRaw(
    evaluation.draft_tx().to_cbor_bytes(),
    [[sourceInput.to_cbor_bytes(), sourceOutput.to_cbor_bytes()]],
    costModels,
    [10_000_000n, 10_000_000n],
    [0n, 0n, 1_000n],
    5,
    true,
  );
  const valued = decodeCbor(redeemerBytes);
  assert.equal(valued.kind, "array");
  assert.equal(valued.values[0].value, BigInt(RedeemerTag.Spend));
  assert.equal(valued.values[1].value, 0n);
  evaluation.set_exunits(
    RedeemerWitnessKey.new(RedeemerTag.Spend, 0n),
    ExUnits.new(result.cost.memory, result.cost.cpu),
  );
  const rebuilt = evaluation.build();
  assert.equal(rebuilt.len(), 1);
  const rebuiltRedeemer = decodeCbor(rebuilt.get(0).to_cbor_bytes());
  assert.deepEqual(rebuiltRedeemer.values[3].values.map((item) => item.value), [
    result.cost.memory,
    result.cost.cpu,
  ]);
});

test("set_metadata and add_metadata merge auxiliary maps and body hashes", () => {
  const first = AuxiliaryData.from_cbor_bytes(encodeCbor({ kind: "map", entries: [[uintNode(1n), uintNode(2n)]], encoding: { kind: "definite", width: 0 } }));
  const second = AuxiliaryData.from_cbor_bytes(encodeCbor({ kind: "map", entries: [[uintNode(3n), uintNode(4n)]], encoding: { kind: "definite", width: 0 } }));
  const builder = TransactionBuilder.new(config()); builder.add_input(input(60, 5_000_000n).result); builder.add_output(output(2_000_000n)); builder.set_auxiliary_data(first); builder.add_auxiliary_data(second);
  const body = builder.build(ChangeSelectionAlgo.Default, address).body(); assert.ok(bodyField(body, 7));
  const merged = decodeCbor(builder.get_auxiliary_data().to_cbor_bytes()); assert.equal(merged.kind, "map"); assert.equal(merged.entries.length, 2);
});

function bodyFieldFromOutput(value, field) {
  const decoded = decodeCbor(value.to_cbor_bytes()); assert.equal(decoded.kind, "map");
  return encodeCbor(decoded.entries.find(([key]) => key.kind === "unsigned" && key.value === BigInt(field))[1]);
}

function witnessFieldLength(decoded, field) {
  assert.equal(decoded.kind, "map"); const value = decoded.entries.find(([key]) => key.kind === "unsigned" && key.value === BigInt(field))?.[1];
  const inner = value?.kind === "tag" ? value.value : value; assert.equal(inner?.kind, "array"); return inner.values.length;
}

function uintNode(value) { return { kind: "unsigned", value, encoding: { width: 0 } }; }

function requiredSigners(...hashes) {
  return RequiredSigners.from_cbor_bytes(encodeCbor({
    kind: "tag",
    tag: 258n,
    value: { kind: "array", values: hashes.map((hash) => ({ kind: "bytes", value: hash.to_raw_bytes(), encoding: { kind: "definite", width: 0 } })), encoding: { kind: "definite", width: 0 } },
    encoding: { width: 2 },
  }));
}

function recordedCoinSelectionSeed(seed) {
  let state = seed >>> 0;
  return () => {
    state ^= state << 13; state ^= state >>> 17; state ^= state << 5;
    return state >>> 0;
  };
}
