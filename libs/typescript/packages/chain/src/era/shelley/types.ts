import type {
  BootstrapWitnessJSON,
  MetadataJSON,
  NonceJSON,
  OperationalCertJSON,
  PoolMetadataJSON,
  PoolRetirementJSON,
  ProtocolVersionJSON,
  PublicKeyJSON,
  RationalJSON,
  SingleHostAddrJSON,
  StakeDelegationJSON,
  StakeDeregistrationJSON,
  StakeRegistrationJSON,
  TransactionInputJSON,
  UnitIntervalJSON,
  VRFCertJSON,
  VkeywitnessJSON,
} from "../shared/json-types.js";

export type MIRActionJSON = {
    ToStakeCredentials: {
        to_stake_credentials: {
            [k: string]: string;
        };
        [k: string]: unknown;
    };
} | {
    ToOtherPot: {
        to_other_pot: number;
        [k: string]: unknown;
    };
};
export type MIRPotJSON = "Reserve" | "Treasury";
export interface MoveInstantaneousRewardJSON {
    action: MIRActionJSON;
    pot: MIRPotJSON;
}
export interface MoveInstantaneousRewardsCertJSON {
    move_instantaneous_reward: MoveInstantaneousRewardJSON;
}

export interface GenesisKeyDelegationJSON {
    genesis_delegate_hash: string;
    genesis_hash: string;
    vrf_key_hash: string;
}
export interface MultisigAllJSON {
    multisig_scripts: MultisigScriptJSON[];
}
export interface MultisigAnyJSON {
    multisig_scripts: MultisigScriptJSON[];
}
export interface MultisigNOfKJSON {
    multisig_scripts: MultisigScriptJSON[];
    n: number;
}
export interface MultisigPubkeyJSON {
    ed25519_key_hash: string;
}
export type MultisigScriptJSON = {
    MultisigPubkey: MultisigPubkeyJSON;
} | {
    MultisigAll: MultisigAllJSON;
} | {
    MultisigAny: MultisigAnyJSON;
} | {
    MultisigNOfK: MultisigNOfKJSON;
};
export interface ProtocolVersionStructJSON {
    protocol_version: ProtocolVersionJSON;
}
export interface ShelleyBlockJSON {
    header: ShelleyHeaderJSON;
    transaction_bodies: ShelleyTransactionBodyJSON[];
    transaction_metadata_set: {
        [k: string]: MetadataJSON;
    };
    transaction_witness_sets: ShelleyTransactionWitnessSetJSON[];
}
export type ShelleyCertificateJSON = {
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
    ShelleyMoveInstantaneousRewardsCert: ShelleyMoveInstantaneousRewardsCertJSON;
};
export type ShelleyDNSNameJSON = string;
export interface ShelleyHeaderJSON {
    body: ShelleyHeaderBodyJSON;
    signature: string;
}
export interface ShelleyHeaderBodyJSON {
    block_body_hash: string;
    block_body_size: number;
    block_number: number;
    issuer_vkey: PublicKeyJSON;
    leader_vrf: VRFCertJSON;
    nonce_vrf: VRFCertJSON;
    operational_cert: OperationalCertJSON;
    prev_hash?: string | null;
    protocol_version: ProtocolVersionJSON;
    slot: number;
    vrf_vkey: string;
}
export interface ShelleyMoveInstantaneousRewardJSON {
    pot: MIRPotJSON;
    to_stake_credentials: {
        [k: string]: number;
    };
}
export interface ShelleyMoveInstantaneousRewardsCertJSON {
    shelley_move_instantaneous_reward: ShelleyMoveInstantaneousRewardJSON;
}
export interface ShelleyMultiHostNameJSON {
    /**
     * A SRV DNS record
     */
    shelley_dns_name: string;
}
export interface ShelleyPoolParamsJSON {
    cost: number;
    margin: UnitIntervalJSON;
    operator: string;
    pledge: number;
    pool_metadata?: PoolMetadataJSON | null;
    pool_owners: string[];
    relays: ShelleyRelayJSON[];
    reward_account: string;
    vrf_keyhash: string;
}
export interface ShelleyPoolRegistrationJSON {
    pool_params: ShelleyPoolParamsJSON;
}
export interface ShelleyProtocolParamUpdateJSON {
    decentralization_constant?: UnitIntervalJSON | null;
    expansion_rate?: UnitIntervalJSON | null;
    extra_entropy?: NonceJSON | null;
    key_deposit?: number | null;
    max_block_body_size?: number | null;
    max_block_header_size?: number | null;
    max_transaction_size?: number | null;
    maximum_epoch?: number | null;
    min_utxo_value?: number | null;
    minfee_a?: number | null;
    minfee_b?: number | null;
    n_opt?: number | null;
    pool_deposit?: number | null;
    pool_pledge_influence?: RationalJSON | null;
    protocol_version?: ProtocolVersionStructJSON | null;
    treasury_growth_rate?: UnitIntervalJSON | null;
}
export type ShelleyRelayJSON = {
    SingleHostAddr: SingleHostAddrJSON;
} | {
    ShelleySingleHostName: ShelleySingleHostNameJSON;
} | {
    ShelleyMultiHostName: ShelleyMultiHostNameJSON;
};
export interface ShelleySingleHostNameJSON {
    port?: number | null;
    /**
     * An A or AAAA DNS record
     */
    shelley_dns_name: string;
}
export interface ShelleyTransactionJSON {
    body: ShelleyTransactionBodyJSON;
    metadata?: MetadataJSON | null;
    witness_set: ShelleyTransactionWitnessSetJSON;
}
export interface ShelleyTransactionBodyJSON {
    auxiliary_data_hash?: string | null;
    certs?: ShelleyCertificateJSON[] | null;
    fee: number;
    inputs: TransactionInputJSON[];
    outputs: ShelleyTransactionOutputJSON[];
    ttl: number;
    update?: ShelleyUpdateJSON | null;
    withdrawals?: {
        [k: string]: number;
    } | null;
}
export interface ShelleyTransactionOutputJSON {
    address: string;
    amount: number;
}
export interface ShelleyTransactionWitnessSetJSON {
    bootstrap_witnesses?: BootstrapWitnessJSON[] | null;
    native_scripts?: MultisigScriptJSON[] | null;
    vkeywitnesses?: VkeywitnessJSON[] | null;
}
export interface ShelleyUpdateJSON {
    epoch: number;
    shelley_proposed_protocol_parameter_updates: {
        [k: string]: ShelleyProtocolParamUpdateJSON;
    };
}

export type { SoftForkRuleJSON } from "../byron/types.js";
