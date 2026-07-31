import type {
  BootstrapWitnessJSON,
  MetadataJSON,
  NativeScriptJSON,
  PoolRetirementJSON,
  ShelleyMAFormatAuxDataJSON,
  StakeDelegationJSON,
  StakeDeregistrationJSON,
  StakeRegistrationJSON,
  TransactionInputJSON,
  VkeywitnessJSON,
} from "../shared/json-types.js";
import type {
  GenesisKeyDelegationJSON,
  MoveInstantaneousRewardsCertJSON,
  ShelleyHeaderJSON,
  ShelleyPoolRegistrationJSON,
  ShelleyTransactionOutputJSON,
  ShelleyUpdateJSON,
} from "../shelley/types.js";

export type AllegraAuxiliaryDataJSON = {
    Shelley: MetadataJSON;
} | {
    ShelleyMA: ShelleyMAFormatAuxDataJSON;
};
export interface AllegraBlockJSON {
    auxiliary_data_set: {
        [k: string]: AllegraAuxiliaryDataJSON;
    };
    header: ShelleyHeaderJSON;
    transaction_bodies: AllegraTransactionBodyJSON[];
    transaction_witness_sets: AllegraTransactionWitnessSetJSON[];
}
export type AllegraCertificateJSON = {
    StakeRegistration: StakeRegistrationJSON;
} | {
    StakeDeregistration: StakeDeregistrationJSON;
} | {
    StakeDelegation: StakeDelegationJSON;
} | {
    ShelleyPoolRegistration: ShelleyPoolRegistrationJSON;
} | {
    PoolRetirement: PoolRetirementJSON;
} | {
    GenesisKeyDelegation: GenesisKeyDelegationJSON;
} | {
    MoveInstantaneousRewardsCert: MoveInstantaneousRewardsCertJSON;
};
export interface AllegraTransactionJSON {
    auxiliary_data?: AllegraAuxiliaryDataJSON | null;
    body: AllegraTransactionBodyJSON;
    witness_set: AllegraTransactionWitnessSetJSON;
}
export interface AllegraTransactionBodyJSON {
    auxiliary_data_hash?: string | null;
    certs?: AllegraCertificateJSON[] | null;
    fee: number;
    inputs: TransactionInputJSON[];
    outputs: ShelleyTransactionOutputJSON[];
    ttl?: number | null;
    update?: ShelleyUpdateJSON | null;
    validity_interval_start?: number | null;
    withdrawals?: {
        [k: string]: number;
    } | null;
}
export interface AllegraTransactionWitnessSetJSON {
    bootstrap_witnesses?: BootstrapWitnessJSON[] | null;
    native_scripts?: NativeScriptJSON[] | null;
    vkeywitnesses?: VkeywitnessJSON[] | null;
}

export type {
  MIRActionJSON,
  MIRPotJSON,
  MoveInstantaneousRewardJSON,
  MoveInstantaneousRewardsCertJSON,
} from "../shelley/types.js";
