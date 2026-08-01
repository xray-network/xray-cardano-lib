import { HistoricalData, HistoricalList } from "../shared/codec.js";
import type { CborValue } from "@xray-network/xray-cardano-lib-core";
import { validateBabbage } from "./validation.js";

abstract class BabbageData extends HistoricalData {
  public static override validateNode(node: CborValue): void { validateBabbage(this.name, node); }
}
abstract class BabbageList<T> extends HistoricalList<T> {
  public static override validateNode(node: CborValue): void { validateBabbage(this.name, node); }
}

export const BabbageAuxiliaryDataKind = Object.freeze({ Shelley: 0, ShelleyMA: 1, Babbage: 2 });
export const BabbageScriptKind = Object.freeze({ Native: 0, PlutusV1: 1, PlutusV2: 2 });
export const BabbageTransactionOutputKind = Object.freeze({ AlonzoFormatTxOut: 0, BabbageFormatTxOut: 1 });

export class BabbageAuxiliaryData extends BabbageData {}
export class BabbageBlock extends BabbageData {}
export class BabbageFormatAuxData extends BabbageData {}
export class BabbageFormatTxOut extends BabbageData {}
export class BabbageProposedProtocolParameterUpdates extends BabbageData {}
export class BabbageProtocolParamUpdate extends BabbageData {}
export class BabbageScript extends BabbageData {}
export class BabbageScriptRef extends BabbageData {}
export class BabbageTransaction extends BabbageData {}
export class BabbageTransactionBody extends BabbageData {}
export class BabbageTransactionBodyList extends BabbageList<HistoricalData> {}
export class BabbageTransactionOutput extends BabbageData {}
export class BabbageTransactionOutputList extends BabbageList<HistoricalData> {}
export class BabbageTransactionWitnessSet extends BabbageData {}
export class BabbageTransactionWitnessSetList extends BabbageList<HistoricalData> {}
export class BabbageUpdate extends BabbageData {}
export class MapTransactionIndexToBabbageAuxiliaryData extends BabbageData {}
