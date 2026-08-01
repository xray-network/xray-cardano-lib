import type { CborValue } from "@xray-network/xray-cardano-lib-core";
import { makePostShelleyValidators } from "../shared/post-shelley-validation.js";
import { arrayOf, mapOf, uint16, type CborValidator } from "../shared/validation.js";

const era = makePostShelleyValidators({ era: "allegra", maximumProtocolVersion: 4n });
const validators: Readonly<Record<string, CborValidator>> = {
  AllegraAuxiliaryData: era.auxiliaryData,
  AllegraBlock: era.block,
  AllegraCertificate: era.certificate,
  AllegraCertificateList: arrayOf(era.certificate),
  AllegraTransaction: era.transaction,
  AllegraTransactionBody: era.transactionBody,
  AllegraTransactionBodyList: arrayOf(era.transactionBody),
  AllegraTransactionWitnessSet: era.transactionWitnessSet,
  AllegraTransactionWitnessSetList: arrayOf(era.transactionWitnessSet),
  MapTransactionIndexToAllegraAuxiliaryData: mapOf(uint16, era.auxiliaryData),
};

export function validateAllegra(name: string, node: CborValue): void {
  const validator = validators[name];
  if (validator === undefined) throw new TypeError(`No Allegra CDDL validator registered for ${name}`);
  validator(node, name);
}
