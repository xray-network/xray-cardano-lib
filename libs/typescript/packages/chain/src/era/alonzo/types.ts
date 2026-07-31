import type {
  AlonzoFormatTxOutJSON,
  BootstrapWitnessJSON,
  ExUnitPricesJSON,
  ExUnitsJSON,
  MetadataJSON,
  NativeScriptJSON,
  NonceJSON,
  PlutusDataJSON,
  RationalJSON,
  ShelleyMAFormatAuxDataJSON,
  TransactionInputJSON,
  UnitIntervalJSON,
  VkeywitnessJSON,
} from "../shared/json-types.js";
import type { AllegraCertificateJSON } from "../allegra/types.js";
import type { ProtocolVersionStructJSON, ShelleyHeaderJSON } from "../shelley/types.js";

type AssetBundle2 = Record<string, Record<string, number>>;

export type AlonzoAuxiliaryDataJSON = {
    Shelley: MetadataJSON;
} | {
    ShelleyMA: ShelleyMAFormatAuxDataJSON;
} | {
    Alonzo: AlonzoFormatAuxDataJSON;
};
export interface AlonzoBlockJSON {
    auxiliary_data_set: {
        [k: string]: AlonzoAuxiliaryDataJSON;
    };
    header: ShelleyHeaderJSON;
    invalid_transactions: number[];
    transaction_bodies: AlonzoTransactionBodyJSON[];
    transaction_witness_sets: AlonzoTransactionWitnessSetJSON[];
}
export interface AlonzoFormatAuxDataJSON {
    metadata?: MetadataJSON | null;
    native_scripts?: NativeScriptJSON[] | null;
    plutus_v1_scripts?: string[] | null;
}
export interface AlonzoProtocolParamUpdateJSON {
    ada_per_utxo_byte?: number | null;
    collateral_percentage?: number | null;
    cost_models_for_script_languages?: {
        /**
         * This interface was referenced by `undefined`'s JSON-Schema definition
         * via the `patternProperty` "^\d+$".
         */
        [k: string]: number[];
    } | null;
    decentralization_constant?: UnitIntervalJSON | null;
    execution_costs?: ExUnitPricesJSON | null;
    expansion_rate?: UnitIntervalJSON | null;
    extra_entropy?: NonceJSON | null;
    key_deposit?: number | null;
    max_block_body_size?: number | null;
    max_block_ex_units?: ExUnitsJSON | null;
    max_block_header_size?: number | null;
    max_collateral_inputs?: number | null;
    max_transaction_size?: number | null;
    max_tx_ex_units?: ExUnitsJSON | null;
    max_value_size?: number | null;
    maximum_epoch?: number | null;
    min_pool_cost?: number | null;
    minfee_a?: number | null;
    minfee_b?: number | null;
    n_opt?: number | null;
    pool_deposit?: number | null;
    pool_pledge_influence?: RationalJSON | null;
    protocol_version?: ProtocolVersionStructJSON | null;
    treasury_growth_rate?: UnitIntervalJSON | null;
}
export interface AlonzoRedeemerJSON {
    data: PlutusDataJSON;
    ex_units: ExUnitsJSON;
    index: number;
    tag: AlonzoRedeemerTagJSON;
}
export type AlonzoRedeemerTagJSON = "Spend" | "Mint" | "Cert" | "Reward";
export interface AlonzoTransactionJSON {
    auxiliary_data?: AlonzoAuxiliaryDataJSON | null;
    body: AlonzoTransactionBodyJSON;
    is_valid: boolean;
    witness_set: AlonzoTransactionWitnessSetJSON;
}
export interface AlonzoTransactionBodyJSON {
    auxiliary_data_hash?: string | null;
    certs?: AllegraCertificateJSON[] | null;
    collateral_inputs?: TransactionInputJSON[] | null;
    fee: number;
    inputs: TransactionInputJSON[];
    mint?: AssetBundle2 | null;
    network_id?: number | null;
    outputs: AlonzoFormatTxOutJSON[];
    required_signers?: string[] | null;
    script_data_hash?: string | null;
    ttl?: number | null;
    update?: AlonzoUpdateJSON | null;
    validity_interval_start?: number | null;
    withdrawals?: {
        [k: string]: number;
    } | null;
}
export interface AlonzoTransactionWitnessSetJSON {
    bootstrap_witnesses?: BootstrapWitnessJSON[] | null;
    native_scripts?: NativeScriptJSON[] | null;
    plutus_datums?: PlutusDataJSON[] | null;
    plutus_v1_scripts?: string[] | null;
    redeemers?: AlonzoRedeemerJSON[] | null;
    vkeywitnesses?: VkeywitnessJSON[] | null;
}
export interface AlonzoUpdateJSON {
    epoch: number;
    proposed_protocol_parameter_updates: {
        [k: string]: AlonzoProtocolParamUpdateJSON;
    };
}
