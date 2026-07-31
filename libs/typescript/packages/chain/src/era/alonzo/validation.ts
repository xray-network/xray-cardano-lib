import type { CborValue } from "@xray-network/cardano-core";
import { makePostShelleyValidators } from "../shared/post-shelley-validation.js";
import { arrayOf, mapOf, uint16, type CborValidator } from "../shared/validation.js";

const era = makePostShelleyValidators({ era: "alonzo", maximumProtocolVersion: 7n });
const validators: Readonly<Record<string, CborValidator>> = {
  AlonzoAuxiliaryData: era.auxiliaryData,
  AlonzoBlock: era.block,
  AlonzoFormatAuxData: era.auxiliaryDataMap as CborValidator,
  AlonzoProposedProtocolParameterUpdates: era.proposedProtocolParameterUpdates,
  AlonzoProtocolParamUpdate: era.protocolParamUpdate,
  AlonzoRedeemer: era.redeemer as CborValidator,
  AlonzoRedeemerList: arrayOf(era.redeemer as CborValidator),
  AlonzoTransaction: era.transaction,
  AlonzoTransactionBody: era.transactionBody,
  AlonzoTransactionBodyList: arrayOf(era.transactionBody),
  AlonzoTransactionWitnessSet: era.transactionWitnessSet,
  AlonzoTransactionWitnessSetList: arrayOf(era.transactionWitnessSet),
  AlonzoUpdate: era.update,
  MapTransactionIndexToAlonzoAuxiliaryData: mapOf(uint16, era.auxiliaryData),
};

export function validateAlonzo(name: string, node: CborValue): void {
  const validator = validators[name];
  if (validator === undefined) throw new TypeError(`No Alonzo CDDL validator registered for ${name}`);
  validator(node, name);
}
