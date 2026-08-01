import { HistoricalData, HistoricalList } from "../shared/codec.js";
import type { CborValue } from "@xray-network/xray-cardano-lib-core";
import { validateAlonzo } from "./validation.js";

abstract class AlonzoData extends HistoricalData {
  public static override validateNode(node: CborValue): void { validateAlonzo(this.name, node); }
}
abstract class AlonzoList<T> extends HistoricalList<T> {
  public static override validateNode(node: CborValue): void { validateAlonzo(this.name, node); }
}

export const AlonzoAuxiliaryDataKind = Object.freeze({ Shelley: 0, ShelleyMA: 1, Alonzo: 2 });
export const AlonzoRedeemerTag = Object.freeze({ Spend: 0, Mint: 1, Cert: 2, Reward: 3 });

export class AlonzoAuxiliaryData extends AlonzoData {}
export class AlonzoBlock extends AlonzoData {}
export class AlonzoFormatAuxData extends AlonzoData {}
export class AlonzoProposedProtocolParameterUpdates extends AlonzoData {}
export class AlonzoProtocolParamUpdate extends AlonzoData {}
export class AlonzoRedeemer extends AlonzoData {}
export class AlonzoRedeemerList extends AlonzoList<HistoricalData> {}
export class AlonzoTransaction extends AlonzoData {}
export class AlonzoTransactionBody extends AlonzoData {}
export class AlonzoTransactionBodyList extends AlonzoList<HistoricalData> {}
export class AlonzoTransactionWitnessSet extends AlonzoData {}
export class AlonzoTransactionWitnessSetList extends AlonzoList<HistoricalData> {}
export class AlonzoUpdate extends AlonzoData {}
export class MapTransactionIndexToAlonzoAuxiliaryData extends AlonzoData {}
