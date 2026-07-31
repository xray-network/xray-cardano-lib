import type { CborValue } from "@xray-network/cardano-core";
import { makePostShelleyValidators } from "../shared/post-shelley-validation.js";
import { arrayOf, mapOf, uint16, type CborValidator } from "../shared/validation.js";

const era = makePostShelleyValidators({ era: "babbage", maximumProtocolVersion: 9n });
const validators: Readonly<Record<string, CborValidator>> = {
  BabbageAuxiliaryData: era.auxiliaryData,
  BabbageBlock: era.block,
  BabbageFormatAuxData: era.auxiliaryDataMap as CborValidator,
  BabbageFormatTxOut: era.transactionOutput,
  BabbageProposedProtocolParameterUpdates: era.proposedProtocolParameterUpdates,
  BabbageProtocolParamUpdate: era.protocolParamUpdate,
  BabbageScript: era.script as CborValidator,
  BabbageScriptRef: era.scriptRef as CborValidator,
  BabbageTransaction: era.transaction,
  BabbageTransactionBody: era.transactionBody,
  BabbageTransactionBodyList: arrayOf(era.transactionBody),
  BabbageTransactionOutput: era.transactionOutput,
  BabbageTransactionOutputList: arrayOf(era.transactionOutput),
  BabbageTransactionWitnessSet: era.transactionWitnessSet,
  BabbageTransactionWitnessSetList: arrayOf(era.transactionWitnessSet),
  BabbageUpdate: era.update,
  MapTransactionIndexToBabbageAuxiliaryData: mapOf(uint16, era.auxiliaryData),
};

export function validateBabbage(name: string, node: CborValue): void {
  const validator = validators[name];
  if (validator === undefined) throw new TypeError(`No Babbage CDDL validator registered for ${name}`);
  validator(node, name);
}
