import { HistoricalData, HistoricalList } from "../shared/codec.js";
import type { CborValue } from "@xray-network/xray-cardano-lib-core";
import { validateMary } from "./validation.js";

abstract class MaryData extends HistoricalData {
  public static override validateNode(node: CborValue): void { validateMary(this.name, node); }
}
abstract class MaryList<T> extends HistoricalList<T> {
  public static override validateNode(node: CborValue): void { validateMary(this.name, node); }
}

export class MaryBlock extends MaryData {}
export class MaryTransaction extends MaryData {}
export class MaryTransactionBody extends MaryData {}
export class MaryTransactionBodyList extends MaryList<HistoricalData> {}
export class MaryTransactionOutput extends MaryData {}
export class MaryTransactionOutputList extends MaryList<HistoricalData> {}
