import type { CborValue } from "@xray-network/xray-cardano-lib-core";
import { embeddedCbor, integer } from "../shared/post-shelley-validation.js";
import {
  anyCbor,
  arrayOf,
  boolean,
  bytes,
  exactUnsigned,
  hash28,
  hash32,
  invalid,
  mapOf,
  oneOf,
  tagged,
  tuple,
  uint16,
  uint64,
  unsigned,
  type CborValidator,
} from "../shared/validation.js";

const attributes = mapOf(anyCbor, anyCbor);
const byronU32 = unsigned();
const nonzeroU8: CborValidator = (node, path = "nonzero_u8") => {
  if (node.kind !== "unsigned" || node.value === 0n || node.value > 255n) {
    invalid(path, "a non-zero uint8");
  }
};
const bytesText: CborValidator = (node, path = "text") => {
  if (node.kind !== "text") invalid(path, "CBOR text");
};
const slotId = tuple(uint64, uint64);
const addressAttributes = (node: CborValue, path = "addrattr"): void => {
  if (node.kind !== "map") invalid(path, "an address-attribute map");
  node.entries.forEach(([key, value], index) => {
    if (key.kind !== "unsigned" || (key.value !== 1n && key.value !== 2n)) {
      invalid(`${path}.key[${index}]`, "address attribute key 1 or 2");
    }
    bytes()(value, `${path}.value[${index}]`);
  });
};
const addressType = unsigned();
const address = tuple(
  embeddedCbor(tuple(hash28, addressAttributes, addressType)),
  uint64,
);
const txOut = tuple(address, uint64);
const txOutPointer = tuple(hash32, byronU32);
const encodedCbor = tagged(24n, bytes());

const txIn: CborValidator = (node, path = "txin") => {
  if (node.kind !== "array" || node.values.length !== 2 || node.values[0]?.kind !== "unsigned") {
    invalid(path, "a two-field transaction input");
  }
  if (node.values[0].value === 0n) embeddedCbor(txOutPointer)(node.values[1] as CborValue, `${path}[1]`);
  else {
    if (node.values[0].value > 255n) invalid(`${path}[0]`, "a non-zero uint8 discriminator");
    encodedCbor(node.values[1] as CborValue, `${path}[1]`);
  }
};
const tx = tuple(arrayOf(txIn, 1), arrayOf(txOut, 1), attributes);
const txProof = tuple(byronU32, hash32, hash32);

const validatorScript = tuple(uint16, bytes());
const transactionWitness: CborValidator = (node, path = "twit") => {
  if (node.kind !== "array" || node.values.length !== 2 || node.values[0]?.kind !== "unsigned") {
    invalid(path, "a two-field transaction witness");
  }
  const tag = node.values[0].value;
  if (tag === 0n || tag === 2n) {
    embeddedCbor(tuple(bytes(), bytes()))(node.values[1] as CborValue, `${path}[1]`);
  } else if (tag === 1n) {
    embeddedCbor(tuple(validatorScript, validatorScript))(node.values[1] as CborValue, `${path}[1]`);
  } else {
    if (tag > 255n) invalid(`${path}[0]`, "a uint8 discriminator");
    encodedCbor(node.values[1] as CborValue, `${path}[1]`);
  }
};

const vssEncrypted = tuple(bytes());
const vssProof = tuple(bytes(), bytes(), bytes(), arrayOf(bytes()));
const sscCommitment = tuple(
  bytes(),
  tuple(mapOf(bytes(), vssEncrypted), vssProof),
  bytes(),
);
const sscCommitments = tagged(258n, arrayOf(sscCommitment));
const sscOpens = mapOf(hash28, bytes());
const sscShares = oneOf(
  mapOf(hash28, tuple(hash28, arrayOf(bytes()))),
  mapOf(hash28, mapOf(hash28, arrayOf(bytes()))),
);
// Historical Byron blocks in the frozen compatibility corpus use the legacy
// certificate field order [vss key, epoch, signing key, signature]. The
// captured official CDDL documents [vss key, signing key, epoch, signature].
const sscCert = oneOf(
  tuple(bytes(), bytes(), uint64, bytes()),
  tuple(bytes(), uint64, bytes(), bytes()),
);
const sscCerts = tagged(258n, arrayOf(sscCert));
const ssc = oneOf(
  tuple(exactUnsigned(0n), sscCommitments, sscCerts),
  tuple(exactUnsigned(1n), sscOpens, sscCerts),
  tuple(exactUnsigned(2n), sscShares, sscCerts),
  tuple(exactUnsigned(3n), sscCerts),
);
const sscProof = oneOf(
  tuple(exactUnsigned(0n), hash32, hash32),
  tuple(exactUnsigned(1n), hash32, hash32),
  tuple(exactUnsigned(2n), hash32, hash32),
  tuple(exactUnsigned(3n), hash32),
);

const delegation = tuple(uint64, bytes(), bytes(), bytes());
const delegationSignature = tuple(delegation, bytes());
const epochRange = tuple(uint64, uint64);
const lightDelegation = tuple(epochRange, bytes(), bytes(), bytes());
const lightDelegationSignature = tuple(lightDelegation, bytes());

const blockVersion = tuple(uint16, uint16, unsigned(255n));
const standardFeePolicy = tuple(integer, integer);
const transactionFeePolicy: CborValidator = (node, path = "txfeepol") => {
  if (node.kind !== "array" || node.values.length !== 2 || node.values[0]?.kind !== "unsigned") {
    invalid(path, "a transaction fee policy");
  }
  if (node.values[0].value === 0n) {
    embeddedCbor(standardFeePolicy)(node.values[1] as CborValue, `${path}[1]`);
  } else {
    if (node.values[0].value > 255n) invalid(`${path}[0]`, "a uint8 discriminator");
    encodedCbor(node.values[1] as CborValue, `${path}[1]`);
  }
};
const optionalField = (validator: CborValidator): CborValidator => arrayOf(validator, 0, 1);
const softForkRule = tuple(uint64, uint64, uint64);
const blockVersionModifier = tuple(
  optionalField(uint16),
  optionalField(integer),
  optionalField(integer),
  optionalField(integer),
  optionalField(integer),
  optionalField(integer),
  optionalField(uint64),
  optionalField(uint64),
  optionalField(uint64),
  optionalField(uint64),
  optionalField(uint64),
  optionalField(softForkRule),
  optionalField(transactionFeePolicy),
  optionalField(uint64),
);
const updateData = tuple(hash32, hash32, hash32, hash32);
const softwareVersion = tuple(bytesText, byronU32);
const updateProposal = tuple(
  blockVersion,
  blockVersionModifier,
  softwareVersion,
  mapOf(bytesText, updateData),
  attributes,
  bytes(),
  bytes(),
);
const updateVote = tuple(bytes(), hash32, boolean, bytes());
const update = tuple(optionalField(updateProposal), arrayOf(updateVote));

const difficulty = tuple(uint64);
const blockSignature = oneOf(
  tuple(exactUnsigned(0n), bytes()),
  tuple(exactUnsigned(1n), lightDelegationSignature),
  tuple(exactUnsigned(2n), delegationSignature),
);
const blockConsensus = tuple(slotId, bytes(), difficulty, blockSignature);
const blockHeaderExtraData = tuple(blockVersion, softwareVersion, attributes, hash32);
const bodyProof = tuple(txProof, sscProof, hash32, hash32);
const blockHeader = tuple(byronU32, hash32, bodyProof, blockConsensus, blockHeaderExtraData);
const blockBody = tuple(
  arrayOf(tuple(tx, arrayOf(transactionWitness))),
  ssc,
  arrayOf(delegation),
  update,
);
const mainBlock = tuple(blockHeader, blockBody, tuple(attributes));
const ebbConsensus = tuple(uint64, difficulty);
const ebbHeader = tuple(byronU32, hash32, hash32, ebbConsensus, tuple(attributes));
const epochBoundaryBlock = tuple(ebbHeader, arrayOf(hash28), tuple(attributes));
const byronBlock = oneOf(mainBlock, epochBoundaryBlock);

const validators: Readonly<Record<string, CborValidator>> = {
  AddressIdList: arrayOf(hash28),
  BigIntegerList: arrayOf(integer),
  BlockHeaderExtraData: blockHeaderExtraData,
  Bvermod: blockVersionModifier,
  ByronAny: anyCbor,
  ByronAnyList: arrayOf(anyCbor),
  ByronAttributes: attributes,
  ByronAttributesList: arrayOf(attributes),
  ByronBlock: byronBlock,
  ByronBlockBody: blockBody,
  ByronBlockConsensusData: blockConsensus,
  ByronBlockHeader: blockHeader,
  ByronBlockSignature: blockSignature,
  ByronBlockSignatureNormal: tuple(exactUnsigned(0n), bytes()),
  ByronBlockSignatureProxyHeavy: tuple(exactUnsigned(2n), delegationSignature),
  ByronBlockSignatureProxyLight: tuple(exactUnsigned(1n), lightDelegationSignature),
  ByronBlockVersion: blockVersion,
  ByronBodyProof: bodyProof,
  ByronDelegation: delegation,
  ByronDelegationList: arrayOf(delegation),
  ByronDelegationSignature: delegationSignature,
  ByronDifficulty: difficulty,
  ByronEbBlock: epochBoundaryBlock,
  ByronMainBlock: mainBlock,
  ByronPkWitness: tuple(exactUnsigned(0n), embeddedCbor(tuple(bytes(), bytes()))),
  ByronPkWitnessEntry: tuple(bytes(), bytes()),
  ByronRedeemWitness: tuple(exactUnsigned(2n), embeddedCbor(tuple(bytes(), bytes()))),
  ByronRedeemerScript: validatorScript,
  ByronRedeemerWitnessEntry: tuple(bytes(), bytes()),
  ByronScriptWitness: tuple(exactUnsigned(1n), embeddedCbor(tuple(validatorScript, validatorScript))),
  ByronScriptWitnessEntry: tuple(validatorScript, validatorScript),
  ByronSlotId: slotId,
  ByronSoftwareVersion: softwareVersion,
  ByronTx: tx,
  ByronTxFeePolicy: transactionFeePolicy,
  ByronTxFeePolicyList: arrayOf(transactionFeePolicy),
  ByronTxIn: txIn,
  ByronTxInGenesis: tuple(nonzeroU8, encodedCbor),
  ByronTxInList: arrayOf(txIn),
  ByronTxInRegular: tuple(exactUnsigned(0n), embeddedCbor(txOutPointer)),
  ByronTxOutList: arrayOf(txOut),
  ByronTxOutPtr: txOutPointer,
  ByronTxProof: txProof,
  ByronTxWitness: transactionWitness,
  ByronTxWitnessList: arrayOf(transactionWitness),
  ByronUpdate: update,
  ByronUpdateData: updateData,
  ByronUpdateProposal: updateProposal,
  ByronUpdateProposalList: arrayOf(updateProposal),
  ByronUpdateVote: updateVote,
  ByronUpdateVoteList: arrayOf(updateVote),
  ByronValidatorScript: validatorScript,
  BytesList: arrayOf(bytes()),
  EbbConsensusData: ebbConsensus,
  EbbHead: ebbHeader,
  EpochRange: epochRange,
  LightWeightDelegationSignature: lightDelegationSignature,
  LightWeightDlg: lightDelegation,
  MapSystemTagToByronUpdateData: mapOf(bytesText, updateData),
  SoftForkRule: softForkRule,
  SoftForkRuleList: arrayOf(softForkRule),
  Ssc: ssc,
  SscCert: sscCert,
  SscCertificatesPayload: tuple(exactUnsigned(3n), sscCerts),
  SscCertificatesProof: tuple(exactUnsigned(3n), hash32),
  SscCerts: sscCerts,
  SscCommitment: sscCommitment,
  SscCommitmentsPayload: tuple(exactUnsigned(0n), sscCommitments, sscCerts),
  SscCommitmentsProof: tuple(exactUnsigned(0n), hash32, hash32),
  SscOpeningsPayload: tuple(exactUnsigned(1n), sscOpens, sscCerts),
  SscOpeningsProof: tuple(exactUnsigned(1n), hash32, hash32),
  SscOpens: sscOpens,
  SscProof: sscProof,
  SscShares: sscShares,
  SscSharesPayload: tuple(exactUnsigned(2n), sscShares, sscCerts),
  SscSharesProof: tuple(exactUnsigned(2n), hash32, hash32),
  SscSharesSubmap: tuple(hash28, arrayOf(bytes())),
  SscSignedCommitment: sscCommitment,
  SscSignedCommitments: sscCommitments,
  StakeholderIdList: arrayOf(hash28),
  StdFeePolicy: standardFeePolicy,
  SystemTagList: arrayOf(bytesText),
  TxAux: tuple(tx, arrayOf(transactionWitness)),
  TxPayload: arrayOf(tuple(tx, arrayOf(transactionWitness))),
  VssEncryptedShare: vssEncrypted,
  VssProof: vssProof,
  VssShares: sscShares,
};

export function validateByron(name: string, node: CborValue): void {
  const validator = validators[name];
  if (validator === undefined) throw new TypeError(`No Byron CDDL validator registered for ${name}`);
  validator(node, name);
}
