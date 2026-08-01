import { HistoricalData, HistoricalList } from "../shared/codec.js";
import type { CborValue } from "@xray-network/xray-cardano-lib-core";
import { validateShelley } from "./validation.js";

abstract class ShelleyData extends HistoricalData {
  public static override validateNode(node: CborValue): void { validateShelley(this.name, node); }
}

abstract class ShelleyList<T> extends HistoricalList<T> {
  public static override validateNode(node: CborValue): void { validateShelley(this.name, node); }
}

export const MIRActionKind = Object.freeze({ ToStakeCredentials: 0, ToOtherPot: 1 });
export const MIRPot = Object.freeze({ Reserve: 0, Treasury: 1 });
export const MultisigScriptKind = Object.freeze({ MultisigPubkey: 0, MultisigAll: 1, MultisigAny: 2, MultisigNOfK: 3 });
export const ShelleyCertificateKind = Object.freeze({ StakeRegistration: 0, StakeDeregistration: 1, StakeDelegation: 2, ShelleyPoolRegistration: 3, PoolRetirement: 4, GenesisKeyDelegation: 5, ShelleyMoveInstantaneousRewardsCert: 6 });
export const ShelleyRelayKind = Object.freeze({ SingleHostAddr: 0, ShelleySingleHostName: 1, ShelleyMultiHostName: 2 });

export class GenesisKeyDelegation extends ShelleyData {}
export class MIRAction extends ShelleyData {}
export class MoveInstantaneousReward extends ShelleyData {}
export class MoveInstantaneousRewardsCert extends ShelleyData {}
export class MultisigAll extends ShelleyData {}
export class MultisigAny extends ShelleyData {}
export class MultisigNOfK extends ShelleyData {}
export class MultisigPubkey extends ShelleyData {}
export class MultisigScript extends ShelleyData {}
export class MultisigScriptList extends ShelleyList<HistoricalData> {}
export class ProtocolVersionStruct extends ShelleyData {}
export class ShelleyBlock extends ShelleyData {}
export class ShelleyCertificate extends ShelleyData {}
export class ShelleyCertificateList extends ShelleyList<HistoricalData> {}
export class ShelleyDNSName extends ShelleyData {}
export class ShelleyHeader extends ShelleyData {}
export class ShelleyHeaderBody extends ShelleyData {}
export class ShelleyMoveInstantaneousReward extends ShelleyData {}
export class ShelleyMoveInstantaneousRewardsCert extends ShelleyData {}
export class ShelleyMultiHostName extends ShelleyData {}
export class ShelleyPoolParams extends ShelleyData {}
export class ShelleyPoolRegistration extends ShelleyData {}
export class ShelleyProposedProtocolParameterUpdates extends ShelleyData {}
export class ShelleyProtocolParamUpdate extends ShelleyData {}
export class ShelleyRelay extends ShelleyData {}
export class ShelleyRelayList extends ShelleyList<HistoricalData> {}
export class ShelleySingleHostName extends ShelleyData {}
export class ShelleyTransaction extends ShelleyData {}
export class ShelleyTransactionBody extends ShelleyData {}
export class ShelleyTransactionBodyList extends ShelleyList<HistoricalData> {}
export class ShelleyTransactionOutput extends ShelleyData {}
export class ShelleyTransactionOutputList extends ShelleyList<HistoricalData> {}
export class ShelleyTransactionWitnessSet extends ShelleyData {}
export class ShelleyTransactionWitnessSetList extends ShelleyList<HistoricalData> {}
export class ShelleyUpdate extends ShelleyData {}
