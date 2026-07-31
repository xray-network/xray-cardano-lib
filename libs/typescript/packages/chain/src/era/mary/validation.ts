import type { CborValue } from "@xray-network/cardano-core";
import { makePostShelleyValidators } from "../shared/post-shelley-validation.js";
import { arrayOf, type CborValidator } from "../shared/validation.js";

const era = makePostShelleyValidators({ era: "mary", maximumProtocolVersion: 5n });
const validators: Readonly<Record<string, CborValidator>> = {
  MaryBlock: era.block,
  MaryTransaction: era.transaction,
  MaryTransactionBody: era.transactionBody,
  MaryTransactionBodyList: arrayOf(era.transactionBody),
  MaryTransactionOutput: era.transactionOutput,
  MaryTransactionOutputList: arrayOf(era.transactionOutput),
};

export function validateMary(name: string, node: CborValue): void {
  const validator = validators[name];
  if (validator === undefined) throw new TypeError(`No Mary CDDL validator registered for ${name}`);
  validator(node, name);
}
