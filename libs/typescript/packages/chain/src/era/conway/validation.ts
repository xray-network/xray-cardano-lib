import type { CborValue } from "@xray-network/xray-cardano-lib-core";
import {
  embeddedCbor,
  integer,
  plutusData,
} from "../shared/post-shelley-validation.js";
import {
  arrayOf,
  boolean,
  bytes,
  discriminated,
  fixedMap,
  hash28,
  hash32,
  int64,
  invalid,
  mapOf,
  oneOf,
  optional,
  rational,
  signature64,
  tagged,
  taggedSet,
  text,
  tuple,
  tupleRange,
  uint16,
  uint32,
  uint64,
  unsigned,
  type CborValidator,
  type MapField,
} from "../shared/validation.js";

const coin = unsigned();
const positiveCoin = unsigned(0xffff_ffff_ffff_ffffn);
const strictlyPositiveCoin: CborValidator = (node, path = "positive_coin") => {
  positiveCoin(node, path);
  if (node.kind !== "unsigned" || node.value === 0n) invalid(path, "a positive coin");
};
const credential = discriminated({ 0: [hash28], 1: [hash28] });
const drep = discriminated({ 0: [hash28], 1: [hash28], 2: [], 3: [] });
const transactionInput = tuple(hash32, uint16);
const unitInterval = rational(true);
const nonnegativeInterval = rational(false);
const exUnits = tuple(
  unsigned(0x7fff_ffff_ffff_ffffn),
  unsigned(0x7fff_ffff_ffff_ffffn),
);
const protocolVersion = tuple(unsigned(12n), uint32);
const anchor = tuple(text(128), hash32);

let nativeScript: CborValidator;
nativeScript = (node, path) => discriminated({
  0: [hash28],
  1: [arrayOf(nativeScript)],
  2: [arrayOf(nativeScript)],
  3: [int64, arrayOf(nativeScript)],
  4: [uint64],
  5: [uint64],
})(node, path);

const script = discriminated({
  0: [nativeScript],
  1: [bytes()],
  2: [bytes()],
  3: [bytes()],
});
const scriptRef = embeddedCbor(script);
const datumOption = discriminated({
  0: [hash32],
  1: [embeddedCbor(plutusData)],
});

function multiasset(amount: CborValidator, nonempty = false): CborValidator {
  return mapOf(hash28, mapOf(bytes([0, 32]), amount, 1), nonempty ? 1 : 0);
}

const value = oneOf(coin, tuple(coin, multiasset(strictlyPositiveCoin)));
const mintAmount: CborValidator = (node, path = "nonzero_int64") => {
  int64(node, path);
  if ((node.kind === "unsigned" || node.kind === "negative") && node.value === 0n) {
    invalid(path, "a non-zero int64");
  }
};
const mint = multiasset(mintAmount, true);
const alonzoOutput = tupleRange([bytes(), value, hash32], 2, 3);
const conwayOutput = fixedMap({
  0: { required: true, validate: bytes() },
  1: { required: true, validate: value },
  2: { validate: datumOption },
  3: { validate: scriptRef },
});
const transactionOutput = oneOf(alonzoOutput, conwayOutput);

const relay = discriminated({
  0: [optional(uint16), optional(bytes(4)), optional(bytes(16))],
  1: [optional(uint16), text(128)],
  2: [text(128)],
});
const poolMetadata = tuple(text(128), bytes());
const poolFields: readonly CborValidator[] = [
  hash28,
  hash32,
  coin,
  coin,
  unitInterval,
  bytes(),
  taggedSet(hash28),
  arrayOf(relay),
  optional(poolMetadata),
];

const certificate = discriminated({
  0: [credential],
  1: [credential],
  2: [credential, hash28],
  3: poolFields,
  4: [hash28, uint64],
  7: [credential, coin],
  8: [credential, coin],
  9: [credential, drep],
  10: [credential, hash28, drep],
  11: [credential, hash28, coin],
  12: [credential, drep, coin],
  13: [credential, hash28, drep, coin],
  14: [credential, credential],
  15: [credential, optional(anchor)],
  16: [credential, coin, optional(anchor)],
  17: [credential, coin],
  18: [credential, optional(anchor)],
});

const govActionId = tuple(hash32, uint16);
const voter = discriminated({
  0: [hash28],
  1: [hash28],
  2: [hash28],
  3: [hash28],
  4: [hash28],
});
const votingProcedure = tuple(unsigned(2n), optional(anchor));
const govActionVotes = mapOf(govActionId, votingProcedure, 1);
const votingProcedures = mapOf(voter, govActionVotes, 1);

const costModels: CborValidator = (node, path = "cost_models") => {
  if (node.kind !== "map") invalid(path, "a cost-model map");
  node.entries.forEach(([key, parameters], index) => {
    if (key.kind !== "unsigned" || key.value > 255n) {
      invalid(`${path}.key[${index}]`, "a language id in 0..255");
    }
    arrayOf(int64)(parameters, `${path}.${key.kind === "unsigned" ? key.value : index}`);
  });
};
const poolVotingThresholds = tuple(
  unitInterval,
  unitInterval,
  unitInterval,
  unitInterval,
  unitInterval,
);
const drepVotingThresholds = tuple(
  unitInterval,
  unitInterval,
  unitInterval,
  unitInterval,
  unitInterval,
  unitInterval,
  unitInterval,
  unitInterval,
  unitInterval,
  unitInterval,
);
const protocolParamFields: Record<number, MapField> = {
  0: { validate: coin },
  1: { validate: coin },
  2: { validate: uint32 },
  3: { validate: uint32 },
  4: { validate: uint16 },
  5: { validate: coin },
  6: { validate: coin },
  7: { validate: uint32 },
  8: { validate: uint16 },
  9: { validate: nonnegativeInterval },
  10: { validate: unitInterval },
  11: { validate: unitInterval },
  16: { validate: coin },
  17: { validate: coin },
  18: { validate: costModels },
  19: { validate: tuple(nonnegativeInterval, nonnegativeInterval) },
  20: { validate: exUnits },
  21: { validate: exUnits },
  22: { validate: uint32 },
  23: { validate: uint16 },
  24: { validate: uint16 },
  25: { validate: poolVotingThresholds },
  26: { validate: drepVotingThresholds },
  27: { validate: uint16 },
  28: { validate: uint32 },
  29: { validate: uint32 },
  30: { validate: coin },
  31: { validate: coin },
  32: { validate: uint32 },
  33: { validate: nonnegativeInterval },
};
const protocolParamUpdate = fixedMap(protocolParamFields);
const historicalProtocolParamUpdate = fixedMap({
  ...protocolParamFields,
  // Frozen CML vectors encoded these fields through uint64 owners. Keep this
  // read-only compatibility path for historical block ingestion while the
  // public ProtocolParamUpdate codec enforces the narrower official bounds.
  8: { validate: uint64 },
  22: { validate: uint64 },
  23: { validate: uint64 },
  24: { validate: uint64 },
  27: { validate: uint64 },
});

const constitution = tuple(anchor, optional(hash28));
const govAction = discriminated({
  0: [optional(govActionId), protocolParamUpdate, optional(hash28)],
  1: [optional(govActionId), protocolVersion],
  2: [mapOf(bytes(), coin), optional(hash28)],
  3: [optional(govActionId)],
  4: [
    optional(govActionId),
    taggedSet(credential),
    mapOf(credential, uint64),
    unitInterval,
  ],
  5: [optional(govActionId), constitution],
  6: [],
});
const proposalProcedure = tuple(coin, bytes(), govAction, anchor);
const proposalProcedures = taggedSet(proposalProcedure, true);
const historicalGovAction = discriminated({
  0: [optional(govActionId), historicalProtocolParamUpdate, optional(hash28)],
  1: [optional(govActionId), protocolVersion],
  2: [mapOf(bytes(), coin), optional(hash28)],
  3: [optional(govActionId)],
  4: [optional(govActionId), taggedSet(credential), mapOf(credential, uint64), unitInterval],
  5: [optional(govActionId), constitution],
  6: [],
});
const historicalProposalProcedures = taggedSet(
  tuple(coin, bytes(), historicalGovAction, anchor),
  true,
);

const redeemer = tuple(unsigned(5n), uint32, plutusData, exUnits);
const redeemerKey = tuple(unsigned(5n), uint32);
const redeemerVal = tuple(plutusData, exUnits);
const redeemers = oneOf(
  arrayOf(redeemer, 1),
  mapOf(redeemerKey, redeemerVal, 1),
);

const transactionWitnessSet = fixedMap({
  0: { validate: taggedSet(tuple(bytes(32), signature64), true) },
  1: { validate: taggedSet(nativeScript, true) },
  2: { validate: taggedSet(tuple(bytes(32), signature64, bytes(), bytes()), true) },
  3: { validate: taggedSet(bytes(), true) },
  4: { validate: taggedSet(plutusData, true) },
  5: { validate: redeemers },
  6: { validate: taggedSet(bytes(), true) },
  7: { validate: taggedSet(bytes(), true) },
});

let metadatum: CborValidator;
metadatum = (node, path) => oneOf(
  mapOf(metadatum, metadatum),
  arrayOf(metadatum),
  integer,
  bytes([0, 64]),
  text(64),
)(node, path);
const metadata = mapOf(uint64, metadatum);
const shelleyMaAux = tuple(metadata, arrayOf(nativeScript));
const conwayAux = tagged(259n, fixedMap({
  0: { validate: metadata },
  1: { validate: arrayOf(nativeScript) },
  2: { validate: arrayOf(bytes()) },
  3: { validate: arrayOf(bytes()) },
  4: { validate: arrayOf(bytes()) },
}));
const auxiliaryData = oneOf(metadata, shelleyMaAux, conwayAux);

const transactionBodyFields: Record<number, MapField> = {
  0: { required: true, validate: taggedSet(transactionInput) },
  1: { required: true, validate: arrayOf(transactionOutput) },
  2: { required: true, validate: coin },
  3: { validate: uint64 },
  4: { validate: taggedSet(certificate, true) },
  5: { validate: mapOf(bytes(), coin, 1) },
  7: { validate: hash32 },
  8: { validate: uint64 },
  9: { validate: mint },
  11: { validate: hash32 },
  13: { validate: taggedSet(transactionInput, true) },
  14: { validate: taggedSet(hash28, true) },
  15: { validate: unsigned(1n) },
  16: { validate: transactionOutput },
  17: { validate: coin },
  18: { validate: taggedSet(transactionInput, true) },
  19: { validate: votingProcedures },
  20: { validate: proposalProcedures },
  21: { validate: coin },
  22: { validate: strictlyPositiveCoin },
};
const transactionBody = fixedMap(transactionBodyFields);
const historicalBlockTransactionBody = fixedMap({
  ...transactionBodyFields,
  20: { validate: historicalProposalProcedures },
});
const transaction = tuple(transactionBody, transactionWitnessSet, boolean, optional(auxiliaryData));

const operationalCert = tuple(bytes(32), uint64, uint64, signature64);
const headerBody = tuple(
  uint64,
  uint64,
  optional(hash32),
  bytes(32),
  bytes(32),
  tuple(bytes(), bytes(80)),
  uint32,
  hash32,
  operationalCert,
  protocolVersion,
);
const header = tuple(headerBody, bytes(448));
const blockShape = tuple(
  header,
  arrayOf(historicalBlockTransactionBody),
  arrayOf(transactionWitnessSet),
  mapOf(uint16, auxiliaryData),
  arrayOf(uint16),
);
const block: CborValidator = (node, path = "Block") => {
  blockShape(node, path);
  if (node.kind !== "array") return;
  const bodies = node.values[1];
  const witnesses = node.values[2];
  const auxiliary = node.values[3];
  const invalidTransactions = node.values[4];
  if (bodies?.kind !== "array" || witnesses?.kind !== "array") return;
  if (bodies.values.length !== witnesses.values.length) {
    invalid(path, "equal transaction-body and witness-set counts");
  }
  if (auxiliary?.kind === "map") {
    for (const [key] of auxiliary.entries) {
      if (key.kind !== "unsigned" || key.value >= BigInt(bodies.values.length)) {
        invalid(path, "auxiliary-data indexes within the transaction-body list");
      }
    }
  }
  if (invalidTransactions?.kind === "array") {
    for (const index of invalidTransactions.values) {
      if (index.kind !== "unsigned" || index.value >= BigInt(bodies.values.length)) {
        invalid(path, "invalid-transaction indexes within the transaction-body list");
      }
    }
  }
};

const classValidators: Readonly<Record<string, CborValidator>> = {
  AlonzoFormatTxOut: alonzoOutput,
  AuthCommitteeHotCert: discriminated({ 14: [credential, credential] }),
  AuxiliaryData: auxiliaryData,
  Block: block,
  Certificate: certificate,
  Constitution: constitution,
  ConwayFormatAuxData: conwayAux,
  ConwayFormatTxOut: conwayOutput,
  CostModels: costModels,
  DRep: drep,
  DRepVotingThresholds: drepVotingThresholds,
  DatumOption: datumOption,
  GovAction: govAction,
  GovActionId: govActionId,
  HardForkInitiationAction: discriminated({ 1: [optional(govActionId), protocolVersion] }),
  Header: header,
  HeaderBody: headerBody,
  LegacyRedeemer: redeemer,
  Metadata: metadata,
  MultiHostName: discriminated({ 2: [text(128)] }),
  NewConstitution: discriminated({ 5: [optional(govActionId), constitution] }),
  NoConfidence: discriminated({ 3: [optional(govActionId)] }),
  Nonce: discriminated({ 0: [], 1: [hash32] }),
  OperationalCert: operationalCert,
  ParameterChangeAction: discriminated({
    0: [optional(govActionId), protocolParamUpdate, optional(hash28)],
  }),
  PoolMetadata: poolMetadata,
  PoolParams: tuple(...poolFields),
  PoolRegistration: discriminated({ 3: poolFields }),
  PoolRetirement: discriminated({ 4: [hash28, uint64] }),
  PoolVotingThresholds: poolVotingThresholds,
  ProposalProcedure: proposalProcedure,
  ProtocolParamUpdate: protocolParamUpdate,
  RedeemerKey: redeemerKey,
  RedeemerVal: redeemerVal,
  Redeemers: redeemers,
  RegCert: discriminated({ 7: [credential, coin] }),
  RegDrepCert: discriminated({ 16: [credential, coin, optional(anchor)] }),
  Relay: relay,
  RequiredSigners: taggedSet(hash28, true),
  ResignCommitteeColdCert: discriminated({ 15: [credential, optional(anchor)] }),
  Script: script,
  ScriptRef: scriptRef,
  ShelleyMAFormatAuxData: shelleyMaAux,
  SingleHostAddr: discriminated({
    0: [optional(uint16), optional(bytes(4)), optional(bytes(16))],
  }),
  SingleHostName: discriminated({ 1: [optional(uint16), text(128)] }),
  StakeDelegation: discriminated({ 2: [credential, hash28] }),
  StakeDeregistration: discriminated({ 1: [credential] }),
  StakeRegDelegCert: discriminated({ 11: [credential, hash28, coin] }),
  StakeRegistration: discriminated({ 0: [credential] }),
  StakeVoteDelegCert: discriminated({ 10: [credential, hash28, drep] }),
  StakeVoteRegDelegCert: discriminated({ 13: [credential, hash28, drep, coin] }),
  Transaction: transaction,
  TransactionBody: transactionBody,
  TransactionMetadatum: metadatum,
  TransactionOutput: transactionOutput,
  TransactionWitnessSet: transactionWitnessSet,
  TreasuryWithdrawalsAction: discriminated({ 2: [mapOf(bytes(), coin), optional(hash28)] }),
  UnregCert: discriminated({ 8: [credential, coin] }),
  UnregDrepCert: discriminated({ 17: [credential, coin] }),
  UpdateCommittee: discriminated({
    4: [optional(govActionId), taggedSet(credential), mapOf(credential, uint64), unitInterval],
  }),
  UpdateDrepCert: discriminated({ 18: [credential, optional(anchor)] }),
  Value: value,
  Vkeywitness: tuple(bytes(32), signature64),
  VoteDelegCert: discriminated({ 9: [credential, drep] }),
  VoteRegDelegCert: discriminated({ 12: [credential, drep, coin] }),
  Voter: voter,
  VotingProcedure: votingProcedure,
  VotingProcedures: votingProcedures,
  Withdrawals: mapOf(bytes(), coin, 1),
};

export function validateConwayModel(name: string, node: CborValue): boolean {
  const validator = classValidators[name];
  if (validator === undefined) return false;
  validator(node, name);
  return true;
}
