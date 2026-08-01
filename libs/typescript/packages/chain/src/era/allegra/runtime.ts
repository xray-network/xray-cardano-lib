import { HistoricalData, HistoricalList } from "../shared/codec.js";
import type { CborValue } from "@xray-network/xray-cardano-lib-core";
import { validateAllegra } from "./validation.js";

abstract class AllegraData extends HistoricalData {
  public static override validateNode(node: CborValue): void { validateAllegra(this.name, node); }
}
abstract class AllegraList<T> extends HistoricalList<T> {
  public static override validateNode(node: CborValue): void { validateAllegra(this.name, node); }
}

export const AllegraAuxiliaryDataKind = Object.freeze({ Shelley: 0, ShelleyMA: 1 });
export const AllegraCertificateKind = Object.freeze({ StakeRegistration: 0, StakeDeregistration: 1, StakeDelegation: 2, ShelleyPoolRegistration: 3, PoolRetirement: 4, GenesisKeyDelegation: 5, MoveInstantaneousRewardsCert: 6 });

export class AllegraAuxiliaryData extends AllegraData {}
export class AllegraBlock extends AllegraData {}
export class AllegraCertificate extends AllegraData {}
export class AllegraCertificateList extends AllegraList<HistoricalData> {}
export class AllegraTransaction extends AllegraData {}
export class AllegraTransactionBody extends AllegraData {}
export class AllegraTransactionBodyList extends AllegraList<HistoricalData> {}
export class AllegraTransactionWitnessSet extends AllegraData {}
export class AllegraTransactionWitnessSetList extends AllegraList<HistoricalData> {}
export class MapTransactionIndexToAllegraAuxiliaryData extends AllegraData {}
