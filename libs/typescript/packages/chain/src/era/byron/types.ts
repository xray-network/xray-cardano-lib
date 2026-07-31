import type { ByronTxOutJSON } from "../shared/json-types.js";

export type Blake2B256JSON = string;
export interface ByronSlotIdJSON {
    epoch: number;
    slot: number;
}
export interface BlockHeaderExtraDataJSON {
    block_version: ByronBlockVersionJSON;
    byron_attributes: {
        [k: string]: string;
    };
    extra_proof: string;
    software_version: ByronSoftwareVersionJSON;
}
export type ByronBlockJSON = {
    EpochBoundary: ByronEbBlockJSON;
} | {
    Main: ByronMainBlockJSON;
};
export interface ByronBlockBodyJSON {
    dlg_payload: ByronDelegationJSON[];
    ssc_payload: SscJSON;
    tx_payload: TxAuxJSON[];
    upd_payload: ByronUpdateJSON;
}
export interface ByronBlockConsensusDataJSON {
    byron_block_signature: ByronBlockSignatureJSON;
    byron_difficulty: ByronDifficultyJSON;
    byron_pub_key: number[];
    byron_slot_id: ByronSlotIdJSON;
}
export interface ByronBlockHeaderJSON {
    body_proof: ByronBodyProofJSON;
    consensus_data: ByronBlockConsensusDataJSON;
    extra_data: BlockHeaderExtraDataJSON;
    prev_block: string;
    protocol_magic: number;
}
export type ByronBlockSignatureJSON = {
    Signature: ByronBlockSignatureNormalJSON;
} | {
    ProxyLight: ByronBlockSignatureProxyLightJSON;
} | {
    ProxyHeavy: ByronBlockSignatureProxyHeavyJSON;
};
export interface ByronBlockSignatureNormalJSON {
    signature: number[];
}
export interface ByronBlockSignatureProxyHeavyJSON {
    signature: ByronDelegationSignatureJSON;
}
export interface ByronBlockSignatureProxyLightJSON {
    signature: LightWeightDelegationSignatureJSON;
}
export interface ByronBodyProofJSON {
    dlg_proof: string;
    ssc_proof: SscProofJSON;
    tx_proof: ByronTxProofJSON;
    upd_proof: string;
}
export interface ByronDifficultyJSON {
    u64: number;
}
export interface ByronEbBlockJSON {
    body: string[];
    extra: {
        [k: string]: string;
    }[];
    header: EbbHeadJSON;
}
export interface ByronMainBlockJSON {
    body: ByronBlockBodyJSON;
    extra: {
        [k: string]: string;
    }[];
    header: ByronBlockHeaderJSON;
}
export interface EbbConsensusDataJSON {
    byron_difficulty: ByronDifficultyJSON;
    epoch_id: number;
}
export interface EbbHeadJSON {
    body_proof: string;
    consensus_data: EbbConsensusDataJSON;
    extra_data: {
        [k: string]: string;
    }[];
    prev_block: string;
    protocol_magic: number;
}
export interface TxAuxJSON {
    byron_tx: ByronTxJSON;
    byron_tx_witnesss: ByronTxWitnessJSON[];
}
export interface ByronDelegationJSON {
    certificate: number[];
    delegate: number[];
    epoch: number;
    issuer: number[];
}
export interface ByronDelegationSignatureJSON {
    byron_delegation: ByronDelegationJSON;
    byron_signature: number[];
}
export interface EpochRangeJSON {
    epoch_id: number;
    epoch_id2: number;
}
export interface LightWeightDelegationSignatureJSON {
    byron_signature: number[];
    light_weight_dlg: LightWeightDlgJSON;
}
export interface LightWeightDlgJSON {
    certificate: number[];
    delegate: number[];
    epoch_range: EpochRangeJSON;
    issuer: number[];
}
export type SscJSON = {
    SscCommitmentsPayload: SscCommitmentsPayloadJSON;
} | {
    SscOpeningsPayload: SscOpeningsPayloadJSON;
} | {
    SscSharesPayload: SscSharesPayloadJSON;
} | {
    SscCertificatesPayload: SscCertificatesPayloadJSON;
};
export interface SscCertJSON {
    byron_pub_key: number[];
    byron_signature: number[];
    epoch_id: number;
    vss_pub_key: number[];
}
export interface SscCertificatesPayloadJSON {
    ssc_certs: SscCertJSON[];
}
export interface SscCertificatesProofJSON {
    blake2b256: string;
}
export interface SscCommitmentJSON {
    vss_proof: VssProofJSON;
    vss_shares: {
        [k: string]: VssEncryptedShareJSON;
    };
}
export interface SscCommitmentsPayloadJSON {
    ssc_certs: SscCertJSON[];
    ssc_signed_commitments: SscSignedCommitmentJSON[];
}
export interface SscCommitmentsProofJSON {
    blake2b256: string;
    blake2b2562: string;
}
export interface SscOpeningsPayloadJSON {
    ssc_certs: SscCertJSON[];
    ssc_opens: {
        [k: string]: number[];
    };
}
export interface SscOpeningsProofJSON {
    blake2b256: string;
    blake2b2562: string;
}
export type SscProofJSON = {
    SscCommitmentsProof: SscCommitmentsProofJSON;
} | {
    SscOpeningsProof: SscOpeningsProofJSON;
} | {
    SscSharesProof: SscSharesProofJSON;
} | {
    SscCertificatesProof: SscCertificatesProofJSON;
};
export interface SscSharesPayloadJSON {
    ssc_certs: SscCertJSON[];
    ssc_shares: {
        [k: string]: {
            [k: string]: number[][];
        };
    };
}
export interface SscSharesProofJSON {
    blake2b256: string;
    blake2b2562: string;
}
export interface SscSignedCommitmentJSON {
    byron_pub_key: number[];
    byron_signature: number[];
    ssc_commitment: SscCommitmentJSON;
}
export interface VssEncryptedShareJSON {
    index_0: number[];
}
export interface VssProofJSON {
    bytess: number[][];
    extra_gen: number[];
    parallel_proofs: number[];
    proof: number[];
}
export interface ByronPkWitnessJSON {
    index_1: ByronPkWitnessEntryJSON;
}
export interface ByronPkWitnessEntryJSON {
    byron_pub_key: number[];
    byron_signature: number[];
}
export interface ByronRedeemWitnessJSON {
    index_1: ByronRedeemerWitnessEntryJSON;
}
export interface ByronRedeemerScriptJSON {
    index_1: number[];
    u16: number;
}
export interface ByronRedeemerWitnessEntryJSON {
    byron_pub_key: number[];
    byron_signature: number[];
}
export interface ByronScriptWitnessJSON {
    index_1: ByronScriptWitnessEntryJSON;
}
export interface ByronScriptWitnessEntryJSON {
    byron_redeemer_script: ByronRedeemerScriptJSON;
    byron_validator_script: ByronValidatorScriptJSON;
}
export interface ByronTxJSON {
    attrs: {
        [k: string]: string;
    };
    inputs: ByronTxInJSON[];
    outputs: ByronTxOutJSON[];
}
export type ByronTxInJSON = {
    ByronTxInRegular: ByronTxInRegularJSON;
} | {
    ByronTxInGenesis: ByronTxInGenesisJSON;
};
export interface ByronTxInGenesisJSON {
    index_1: number[];
    u8: number;
}
export interface ByronTxInRegularJSON {
    index_1: ByronTxOutPtrJSON;
}
export interface ByronTxOutPtrJSON {
    byron_tx_id: string;
    u32: number;
}
export interface ByronTxProofJSON {
    blake2b256: string;
    blake2b2562: string;
    u32: number;
}
export type ByronTxWitnessJSON = {
    ByronPkWitness: ByronPkWitnessJSON;
} | {
    ByronScriptWitness: ByronScriptWitnessJSON;
} | {
    ByronRedeemWitness: ByronRedeemWitnessJSON;
};
export interface ByronValidatorScriptJSON {
    index_1: number[];
    u16: number;
}
export interface BvermodJSON {
    heavy_del_thd: number[];
    max_block_size: string[];
    max_header_size: string[];
    max_proposal_size: string[];
    max_tx_size: string[];
    mpc_thd: number[];
    script_version: number[];
    slot_duration: string[];
    soft_fork_rule: SoftForkRuleJSON[];
    tx_fee_policy: ByronTxFeePolicyJSON[];
    unlock_stake_epoch: number[];
    update_implicit: number[];
    update_proposal_thd: number[];
    update_vote_thd: number[];
}
export interface ByronBlockVersionJSON {
    u16: number;
    u162: number;
    u8: number;
}
export interface ByronSoftwareVersionJSON {
    application_name: string;
    u32: number;
}
export interface ByronTxFeePolicyJSON {
    index_1: StdFeePolicyJSON;
}
export interface ByronUpdateJSON {
    proposal: ByronUpdateProposalJSON[];
    votes: ByronUpdateVoteJSON[];
}
export interface ByronUpdateDataJSON {
    blake2b256: string;
    blake2b2562: string;
    blake2b2563: string;
    blake2b2564: string;
}
export interface ByronUpdateProposalJSON {
    block_version: ByronBlockVersionJSON;
    block_version_mod: BvermodJSON;
    byron_attributes: {
        [k: string]: string;
    };
    data: {
        [k: string]: ByronUpdateDataJSON;
    };
    from: number[];
    signature: number[];
    software_version: ByronSoftwareVersionJSON;
}
export interface ByronUpdateVoteJSON {
    proposal_id: string;
    signature: number[];
    vote: boolean;
    voter: number[];
}
export interface SoftForkRuleJSON {
    coin_portion: number;
    coin_portion2: number;
    coin_portion3: number;
}
export interface StdFeePolicyJSON {
    big_integer: string;
    big_integer2: string;
}
export type AnyJSON = string;
