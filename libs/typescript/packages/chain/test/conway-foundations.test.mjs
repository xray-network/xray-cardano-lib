import assert from "node:assert/strict";
import test from "node:test";
import {
  Anchor,
  AnchorDocHash,
  AuthCommitteeHotCert,
  AssetName,
  AuxiliaryData,
  CostModels,
  Certificate,
  CertificateKind,
  Constitution,
  Credential,
  DRep,
  DRepKind,
  Ed25519KeyHash,
  ExUnits,
  GovAction,
  GovActionId,
  GovActionKind,
  Int,
  MapAssetNameToCoin,
  MapCommitteeColdCredentialToEpoch,
  MapU64ToArrI64,
  NetworkId,
  PoolParams,
  PoolRegistration,
  PoolRetirement,
  ProposalProcedure,
  ProtocolParamUpdate,
  ProtocolVersion,
  Rational,
  RegCert,
  RegDrepCert,
  ResignCommitteeColdCert,
  RewardAddress,
  ScriptHash,
  StakeDelegation,
  StakeDeregistration,
  StakeRegistration,
  StakeRegDelegCert,
  StakeVoteDelegCert,
  StakeVoteRegDelegCert,
  TransactionHash,
  Transaction,
  TransactionBody,
  TransactionWitnessSet,
  TransactionMetadatum,
  UnitInterval,
  UnregCert,
  UnregDrepCert,
  UpdateDrepCert,
  Url,
  Value,
  Vote,
  VoteDelegCert,
  VoteRegDelegCert,
  Voter,
  VoterKind,
  VotingProcedure,
  VotingProcedures,
  Withdrawals,
  VRFCert,
} from "../../runtime/dist/esm/index.js";
import { decodeCbor, encodeCbor } from "../../core/dist/esm/index.js";

const bytes = (hex) => Uint8Array.from(Buffer.from(hex, "hex"));
const uint = (value) => ({ kind: "unsigned", value: BigInt(value), encoding: { width: 0 } });
const byteNode = (value) => ({ kind: "bytes", value, encoding: { kind: "definite", width: 0 } });
const array = (values) => ({ kind: "array", values, encoding: { kind: "definite", width: 0 } });

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

test("typed Conway certificates cover every official discriminator and reject gaps", () => {
  const keyHash = Ed25519KeyHash.from_hex("11".repeat(28));
  const scriptHash = ScriptHash.from_hex("22".repeat(28));
  const keyCredential = Credential.new_pub_key(keyHash);
  const scriptCredential = Credential.new_script(scriptHash);
  const drep = DRep.new_key(keyHash);
  const anchor = Anchor.new(Url.new("https://example.test/governance"), AnchorDocHash.from_hex("33".repeat(32)));
  const reward = RewardAddress.new(1, keyCredential);
  const poolParams = PoolParams.from_cbor_bytes(encodeCbor(array([
    byteNode(keyHash.to_raw_bytes()),
    byteNode(new Uint8Array(32)),
    uint(1n),
    uint(2n),
    { kind: "tag", tag: 30n, value: array([uint(1n), uint(2n)]), encoding: { width: 0 } },
    byteNode(reward.to_address().to_raw_bytes()),
    array([byteNode(keyHash.to_raw_bytes())]),
    array([]),
    { kind: "null" },
  ])));

  const variants = [
    [CertificateKind.StakeRegistration, StakeRegistration.new(keyCredential), Certificate.new_stake_registration, "as_stake_registration"],
    [CertificateKind.StakeDeregistration, StakeDeregistration.new(scriptCredential), Certificate.new_stake_deregistration, "as_stake_deregistration"],
    [CertificateKind.StakeDelegation, StakeDelegation.new(keyCredential, keyHash), Certificate.new_stake_delegation, "as_stake_delegation"],
    [CertificateKind.PoolRegistration, PoolRegistration.new(poolParams), Certificate.new_pool_registration, "as_pool_registration"],
    [CertificateKind.PoolRetirement, PoolRetirement.new(keyHash, 44n), Certificate.new_pool_retirement, "as_pool_retirement"],
    [CertificateKind.RegCert, RegCert.new(keyCredential, 45n), Certificate.new_reg_cert, "as_reg_cert"],
    [CertificateKind.UnregCert, UnregCert.new(scriptCredential, 46n), Certificate.new_unreg_cert, "as_unreg_cert"],
    [CertificateKind.VoteDelegCert, VoteDelegCert.new(keyCredential, drep), Certificate.new_vote_deleg_cert, "as_vote_deleg_cert"],
    [CertificateKind.StakeVoteDelegCert, StakeVoteDelegCert.new(keyCredential, keyHash, drep), Certificate.new_stake_vote_deleg_cert, "as_stake_vote_deleg_cert"],
    [CertificateKind.StakeRegDelegCert, StakeRegDelegCert.new(keyCredential, keyHash, 47n), Certificate.new_stake_reg_deleg_cert, "as_stake_reg_deleg_cert"],
    [CertificateKind.VoteRegDelegCert, VoteRegDelegCert.new(keyCredential, drep, 48n), Certificate.new_vote_reg_deleg_cert, "as_vote_reg_deleg_cert"],
    [CertificateKind.StakeVoteRegDelegCert, StakeVoteRegDelegCert.new(keyCredential, keyHash, drep, 49n), Certificate.new_stake_vote_reg_deleg_cert, "as_stake_vote_reg_deleg_cert"],
    [CertificateKind.AuthCommitteeHotCert, AuthCommitteeHotCert.new(keyCredential, scriptCredential), Certificate.new_auth_committee_hot_cert, "as_auth_committee_hot_cert"],
    [CertificateKind.ResignCommitteeColdCert, ResignCommitteeColdCert.new(keyCredential, anchor), Certificate.new_resign_committee_cold_cert, "as_resign_committee_cold_cert"],
    [CertificateKind.RegDrepCert, RegDrepCert.new(keyCredential, 50n, anchor), Certificate.new_reg_drep_cert, "as_reg_drep_cert"],
    [CertificateKind.UnregDrepCert, UnregDrepCert.new(scriptCredential, 51n), Certificate.new_unreg_drep_cert, "as_unreg_drep_cert"],
    [CertificateKind.UpdateDrepCert, UpdateDrepCert.new(keyCredential, null), Certificate.new_update_drep_cert, "as_update_drep_cert"],
  ];
  for (const [kind, variant, wrap, accessor] of variants) {
    const certificate = wrap.call(Certificate, variant);
    assert.equal(certificate.kind(), kind);
    assert.equal(certificate[accessor]().to_canonical_cbor_hex(), variant.to_canonical_cbor_hex());
    assert.equal(decodeCbor(certificate.to_canonical_cbor_bytes()).values[0].value, BigInt(kind));
  }
  assert.equal(PoolRegistration.new(poolParams).pool_params().operator().to_hex(), keyHash.to_hex());
  assert.deepEqual(PoolRegistration.new(poolParams).pool_params().pool_owners().map((owner) => owner.to_hex()), [keyHash.to_hex()]);
  assert.equal(PoolRetirement.new(keyHash, 0xffff_ffff_ffff_ffffn).epoch(), 0xffff_ffff_ffff_ffffn);
  assert.throws(() => PoolRetirement.new(keyHash, 0x1_0000_0000_0000_0000n));
  for (const tag of [5n, 6n, 19n]) assert.throws(() => Certificate.from_cbor_bytes(encodeCbor(array([uint(tag)]))));

  const canonical = Certificate.new_stake_registration(StakeRegistration.new(keyCredential)).to_cbor_hex();
  const noncanonical = Certificate.from_cbor_hex(`9f${canonical.slice(2)}ff`);
  assert.equal(noncanonical.to_cbor_hex(), `9f${canonical.slice(2)}ff`);
  assert.equal(noncanonical.to_canonical_cbor_hex(), canonical);
});

test("typed Conway governance factories preserve kinds, fields, maps, and public identity", async () => {
  const keyHash = Ed25519KeyHash.from_hex("44".repeat(28));
  const scriptHash = ScriptHash.from_hex("55".repeat(28));
  const credential = Credential.new_pub_key(keyHash);
  const actionId = GovActionId.new(TransactionHash.from_hex("66".repeat(32)), 0xffff);
  assert.equal(actionId.transaction_id().to_hex(), "66".repeat(32));
  assert.equal(actionId.index(), 0xffff);
  assert.throws(() => GovActionId.new(TransactionHash.from_hex("66".repeat(32)), 0x1_0000));

  const dreps = [DRep.new_key(keyHash), DRep.new_script(scriptHash), DRep.new_always_abstain(), DRep.new_always_no_confidence()];
  assert.deepEqual(dreps.map((value) => value.kind()), [DRepKind.Key, DRepKind.Script, DRepKind.AlwaysAbstain, DRepKind.AlwaysNoConfidence]);
  assert.equal(dreps[0].as_key().to_hex(), keyHash.to_hex());
  assert.equal(dreps[1].as_script().to_hex(), scriptHash.to_hex());

  const voters = [
    Voter.new_constitutional_committee_hot_key(keyHash),
    Voter.new_constitutional_committee_hot_script(scriptHash),
    Voter.new_drep_key(keyHash),
    Voter.new_drep_script(scriptHash),
    Voter.new_stake_pool_key(keyHash),
  ];
  assert.deepEqual(voters.map((value) => value.kind()), [0, 1, 2, 3, 4]);
  assert.equal(voters[4].as_stake_pool_key().to_hex(), keyHash.to_hex());
  assert.equal(VoterKind.DRepScriptHash, 3);

  const anchor = Anchor.new(Url.new("https://example.test/action"), AnchorDocHash.from_hex("77".repeat(32)));
  const constitution = Constitution.new(anchor, scriptHash);
  assert.equal(constitution.anchor().anchor_url().get(), "https://example.test/action");
  assert.equal(constitution.guardrails_script_hash().to_hex(), scriptHash.to_hex());
  const withdrawals = Withdrawals.from_cbor_bytes(encodeCbor({ kind: "map", entries: [[byteNode(RewardAddress.new(1, credential).to_address().to_raw_bytes()), uint(9n)]], encoding: { kind: "definite", width: 0 } }));
  const committee = MapCommitteeColdCredentialToEpoch.new();
  committee.insert(credential, 10n);
  const actions = [
    GovAction.new_parameter_change(actionId, ProtocolParamUpdate.new(), scriptHash),
    GovAction.new_hard_fork(actionId, ProtocolVersion.new(10n, 2n)),
    GovAction.new_treasury_withdrawals(withdrawals, null),
    GovAction.new_no_confidence(actionId),
    GovAction.new_update_committee(actionId, [credential], committee, UnitInterval.new(1n, 2n)),
    GovAction.new_constitution(actionId, constitution),
    GovAction.new_info_action(),
  ];
  assert.deepEqual(actions.map((value) => value.kind()), [0, 1, 2, 3, 4, 5, 6]);
  for (const [index, accessor] of ["as_parameter_change", "as_hard_fork", "as_treasury_withdrawals", "as_no_confidence", "as_update_committee", "as_new_constitution", "as_info_action"].entries()) {
    assert.equal(actions[index][accessor]().kind?.() ?? index, index);
  }
  assert.equal(GovActionKind.InfoAction, 6);

  const reward = RewardAddress.new(1, credential);
  const proposal = ProposalProcedure.new(20n, reward, actions[6], anchor);
  assert.equal(proposal.deposit(), 20n);
  assert.equal(proposal.reward_account().to_address().to_hex(), reward.to_address().to_hex());
  assert.equal(proposal.gov_action().kind(), GovActionKind.InfoAction);
  assert.equal(proposal.anchor().anchor_doc_hash().to_hex(), "77".repeat(32));

  const procedures = VotingProcedures.new();
  assert.equal(procedures.get(voters[0], actionId), undefined);
  for (const vote of [Vote.No, Vote.Yes, Vote.Abstain]) {
    const procedure = VotingProcedure.new(vote, vote === Vote.Yes ? anchor : null);
    assert.equal(procedure.vote(), vote);
    assert.equal(procedures.insert(voters[0], actionId, procedure)?.vote(), vote === Vote.No ? undefined : vote - 1);
  }
  assert.equal(procedures.get(voters[0], actionId).vote(), Vote.Abstain);

  const focused = await import("../dist/esm/era/conway/index.js");
  const root = await import("../dist/esm/index.js");
  const aggregate = await import("../../runtime/dist/esm/index.js");
  for (const binding of ["Certificate", "DRep", "GovAction", "GovActionId", "ProposalProcedure", "Voter", "VotingProcedure", "VotingProcedures"]) {
    assert.equal(focused[binding], root[binding]);
    assert.equal(root[binding], aggregate[binding]);
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

test("transactions expose typed defensive parts and preserve noncanonical child wire bytes", async () => {
  const body = TransactionBody.from_cbor_hex("a300d901028001800200");
  const witnesses = TransactionWitnessSet.from_cbor_hex("a0");
  const withoutAuxiliary = Transaction.new(body, witnesses, false);
  assert.equal(withoutAuxiliary.body().to_cbor_hex(), body.to_cbor_hex());
  assert.equal(withoutAuxiliary.witness_set().to_cbor_hex(), witnesses.to_cbor_hex());
  assert.equal(withoutAuxiliary.is_valid(), false);
  assert.equal(withoutAuxiliary.auxiliary_data(), undefined);

  const auxiliary = AuxiliaryData.from_cbor_hex("a0");
  const withAuxiliary = Transaction.new(body, witnesses, true, auxiliary);
  assert.equal(withAuxiliary.is_valid(), true);
  assert.equal(withAuxiliary.auxiliary_data().to_cbor_hex(), "a0");
  const returned = withAuxiliary.body().to_cbor_bytes(); returned.fill(0);
  assert.equal(withAuxiliary.body().to_cbor_hex(), body.to_cbor_hex());

  const preserved = Transaction.from_cbor_hex("9fbf00d90102800180021800ffbffff5f6ff");
  assert.equal(preserved.to_cbor_hex(), "9fbf00d90102800180021800ffbffff5f6ff");
  assert.equal(preserved.body().to_cbor_hex(), "bf00d90102800180021800ff");
  assert.equal(preserved.witness_set().to_cbor_hex(), "bfff");
  assert.equal(preserved.body().to_canonical_cbor_hex(), body.to_canonical_cbor_hex());
  assert.throws(() => Transaction.from_cbor_hex("8300a0f5"));

  const focused = await import("../dist/esm/era/conway/index.js");
  const root = await import("../dist/esm/index.js");
  const aggregate = await import("../../runtime/dist/esm/index.js");
  assert.equal(focused.Transaction, root.Transaction);
  assert.equal(root.Transaction, aggregate.Transaction);
});
