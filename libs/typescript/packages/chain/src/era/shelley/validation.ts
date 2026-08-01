import type { CborValue } from "@xray-network/xray-cardano-lib-core";
import {
  anyCbor,
  arrayOf,
  bytes,
  discriminated,
  fixedMap,
  hash28,
  hash32,
  int32,
  mapOf,
  oneOf,
  optional,
  rational,
  signature64,
  taggedSet,
  text,
  tuple,
  tupleRange,
  uint16,
  uint32,
  uint64,
  unsigned,
  type CborValidator,
} from "../shared/validation.js";

const coin = unsigned();
const credential = discriminated({ 0: [hash28], 1: [hash28] });
const transactionInput = tuple(hash32, uint16);
const transactionOutput = tuple(bytes(), coin);
const vrfCert = tuple(bytes(), bytes(80));
const operationalCert = tuple(bytes(32), uint64, uint64, signature64);
const protocolVersion = tuple(unsigned(3n), uint32);
const unitInterval = rational(true);
const nonnegativeInterval = rational(false);

let nativeScript: CborValidator;
nativeScript = (node, path) => discriminated({
  0: [hash28],
  1: [arrayOf(nativeScript)],
  2: [arrayOf(nativeScript)],
  3: [int32, arrayOf(nativeScript)],
})(node, path);

const relay = discriminated({
  0: [optional(uint16), optional(bytes(4)), optional(bytes(16))],
  1: [optional(uint16), text(64)],
  2: [text(64)],
});

const poolMetadata = tuple(text(64), bytes());
const poolParams = tuple(
  hash28,
  hash32,
  coin,
  coin,
  unitInterval,
  bytes(),
  taggedSet(hash28),
  arrayOf(relay),
  optional(poolMetadata),
);

const moveInstantaneousReward = tuple(
  unsigned(1n),
  oneOf(mapOf(credential, (node, path) => {
    if (node.kind !== "unsigned" && node.kind !== "negative") {
      throw new TypeError(`${path ?? "delta_coin"} requires an integer`);
    }
  }), coin),
);

const certificate = discriminated({
  0: [credential],
  1: [credential],
  2: [credential, hash28],
  3: [
    hash28,
    hash32,
    coin,
    coin,
    unitInterval,
    bytes(),
    taggedSet(hash28),
    arrayOf(relay),
    optional(poolMetadata),
  ],
  4: [hash28, uint64],
  5: [hash28, hash28, hash32],
  6: [moveInstantaneousReward],
});

const nonce = discriminated({ 0: [], 1: [hash32] });
const protocolParamUpdate = fixedMap({
  0: { validate: unsigned() },
  1: { validate: unsigned() },
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
  12: { validate: unitInterval },
  13: { validate: nonce },
  14: { validate: protocolVersion },
  15: { validate: coin },
  16: { validate: coin },
});
const proposedUpdates = mapOf(hash28, protocolParamUpdate);
const update = tuple(proposedUpdates, uint64);
const withdrawals = mapOf(bytes(), coin);
const transactionBody = fixedMap({
  0: { required: true, validate: taggedSet(transactionInput) },
  1: { required: true, validate: arrayOf(transactionOutput) },
  2: { required: true, validate: coin },
  3: { required: true, validate: uint64 },
  4: { validate: arrayOf(certificate) },
  5: { validate: withdrawals },
  6: { validate: update },
  7: { validate: hash32 },
});
const vkeyWitness = tuple(bytes(32), signature64);
const bootstrapWitness = tuple(bytes(32), signature64, bytes(), bytes());
const transactionWitnessSet = fixedMap({
  0: { validate: arrayOf(vkeyWitness) },
  1: { validate: arrayOf(nativeScript) },
  2: { validate: arrayOf(bootstrapWitness) },
});

let metadataValue: CborValidator;
metadataValue = (node, path) => oneOf(
  mapOf(metadataValue, metadataValue),
  arrayOf(metadataValue),
  (value, itemPath = "metadatum") => {
    if (
      value.kind !== "unsigned" &&
      value.kind !== "negative" &&
      value.kind !== "bytes" &&
      value.kind !== "text"
    ) {
      throw new TypeError(`${itemPath} requires metadata CBOR`);
    }
  },
)(node, path);
const metadata = mapOf(uint64, metadataValue);
const transaction = tuple(transactionBody, transactionWitnessSet, optional(metadata));
const headerBody = tuple(
  uint64,
  uint64,
  optional(hash32),
  bytes(32),
  bytes(32),
  vrfCert,
  vrfCert,
  uint32,
  hash32,
  bytes(32),
  uint64,
  uint64,
  signature64,
  unsigned(3n),
  uint32,
);
const header = tuple(headerBody, bytes(448));
const blockShape = tuple(
  header,
  arrayOf(transactionBody),
  arrayOf(transactionWitnessSet),
  mapOf(uint16, metadata),
);
const block: CborValidator = (node, path = "ShelleyBlock") => {
  blockShape(node, path);
  if (node.kind !== "array") return;
  const bodies = node.values[1];
  const witnesses = node.values[2];
  const metadataSet = node.values[3];
  if (bodies?.kind !== "array" || witnesses?.kind !== "array") return;
  if (bodies.values.length !== witnesses.values.length) {
    throw new TypeError(`${path} requires equal transaction-body and witness-set counts`);
  }
  if (metadataSet?.kind === "map") {
    for (const [key] of metadataSet.entries) {
      if (key.kind !== "unsigned" || key.value >= BigInt(bodies.values.length)) {
        throw new TypeError(`${path} metadata index is outside the transaction-body list`);
      }
    }
  }
};

const validators: Readonly<Record<string, CborValidator>> = {
  GenesisKeyDelegation: discriminated({ 5: [hash28, hash28, hash32] }),
  MIRAction: oneOf(mapOf(credential, anyCbor), coin),
  MoveInstantaneousReward: moveInstantaneousReward,
  MoveInstantaneousRewardsCert: discriminated({ 6: [moveInstantaneousReward] }),
  MultisigAll: discriminated({ 1: [arrayOf(nativeScript)] }),
  MultisigAny: discriminated({ 2: [arrayOf(nativeScript)] }),
  MultisigNOfK: discriminated({ 3: [int32, arrayOf(nativeScript)] }),
  MultisigPubkey: discriminated({ 0: [hash28] }),
  MultisigScript: nativeScript,
  MultisigScriptList: arrayOf(nativeScript),
  ProtocolVersionStruct: tuple(protocolVersion),
  ShelleyBlock: block,
  ShelleyCertificate: certificate,
  ShelleyCertificateList: arrayOf(certificate),
  ShelleyDNSName: text(64),
  ShelleyHeader: header,
  ShelleyHeaderBody: headerBody,
  ShelleyMoveInstantaneousReward: tuple(unsigned(1n), mapOf(credential, coin)),
  ShelleyMoveInstantaneousRewardsCert: discriminated({ 6: [moveInstantaneousReward] }),
  ShelleyMultiHostName: discriminated({ 2: [text(64)] }),
  ShelleyPoolParams: poolParams,
  ShelleyPoolRegistration: discriminated({
    3: [
      hash28,
      hash32,
      coin,
      coin,
      unitInterval,
      bytes(),
      taggedSet(hash28),
      arrayOf(relay),
      optional(poolMetadata),
    ],
  }),
  ShelleyProposedProtocolParameterUpdates: proposedUpdates,
  ShelleyProtocolParamUpdate: protocolParamUpdate,
  ShelleyRelay: relay,
  ShelleyRelayList: arrayOf(relay),
  ShelleySingleHostName: discriminated({ 1: [optional(uint16), text(64)] }),
  ShelleyTransaction: transaction,
  ShelleyTransactionBody: transactionBody,
  ShelleyTransactionBodyList: arrayOf(transactionBody),
  ShelleyTransactionOutput: transactionOutput,
  ShelleyTransactionOutputList: arrayOf(transactionOutput),
  ShelleyTransactionWitnessSet: transactionWitnessSet,
  ShelleyTransactionWitnessSetList: arrayOf(transactionWitnessSet),
  ShelleyUpdate: update,
};

export function validateShelley(name: string, node: CborValue): void {
  const validator = validators[name];
  if (validator === undefined) throw new TypeError(`No Shelley CDDL validator registered for ${name}`);
  validator(node, name);
}
