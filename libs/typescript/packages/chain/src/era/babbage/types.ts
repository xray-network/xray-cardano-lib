import type {
  AlonzoFormatTxOutJSON,
  BootstrapWitnessJSON,
  DatumOptionJSON,
  ExUnitPricesJSON,
  ExUnitsJSON,
  HeaderJSON,
  MetadataJSON,
  NativeScriptJSON,
  PlutusDataJSON,
  RationalJSON,
  ShelleyMAFormatAuxDataJSON,
  TransactionInputJSON,
  UnitIntervalJSON,
  ValueJSON,
  VkeywitnessJSON,
} from "../shared/json-types.js";
import type { AllegraCertificateJSON } from "../allegra/types.js";
import type { AlonzoRedeemerJSON } from "../alonzo/types.js";
import type { ProtocolVersionStructJSON } from "../shelley/types.js";

type BabbageScriptRefJSON = BabbageScriptJSON;

export type BabbageAuxiliaryDataJSON = {
    Shelley: MetadataJSON;
} | {
    ShelleyMA: ShelleyMAFormatAuxDataJSON;
} | {
    Babbage: BabbageFormatAuxDataJSON;
};
export interface BabbageBlockJSON {
    auxiliary_data_set: {
        [k: string]: BabbageAuxiliaryDataJSON;
    };
    header: HeaderJSON;
    invalid_transactions: number[];
    transaction_bodies: BabbageTransactionBodyJSON[];
    transaction_witness_sets: BabbageTransactionWitnessSetJSON[];
}
export interface BabbageFormatAuxDataJSON {
    metadata?: MetadataJSON | null;
    native_scripts?: NativeScriptJSON[] | null;
    plutus_v1_scripts?: string[] | null;
    plutus_v2_scripts?: string[] | null;
}
export interface BabbageFormatTxOutJSON {
    address: string;
    amount: ValueJSON;
    datum_option?: DatumOptionJSON | null;
    script_reference?: BabbageScriptRefJSON | null;
}
export interface BabbageProtocolParamUpdateJSON {
    ada_per_utxo_byte?: number | null;
    collateral_percentage?: number | null;
    cost_models_for_script_languages?: {
        /**
         * This interface was referenced by `undefined`'s JSON-Schema definition
         * via the `patternProperty` "^\d+$".
         */
        [k: string]: number[];
    } | null;
    execution_costs?: ExUnitPricesJSON | null;
    expansion_rate?: UnitIntervalJSON | null;
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
export type BabbageScriptJSON = {
    Native: {
        script: NativeScriptJSON;
        [k: string]: unknown;
    };
} | {
    PlutusV1: {
        script: string;
        [k: string]: unknown;
    };
} | {
    PlutusV2: {
        script: string;
        [k: string]: unknown;
    };
};
export interface BabbageTransactionJSON {
    auxiliary_data?: BabbageAuxiliaryDataJSON | null;
    body: BabbageTransactionBodyJSON;
    is_valid: boolean;
    witness_set: BabbageTransactionWitnessSetJSON;
}
export interface BabbageTransactionBodyJSON {
    auxiliary_data_hash?: string | null;
    certs?: AllegraCertificateJSON[] | null;
    collateral_inputs?: TransactionInputJSON[] | null;
    collateral_return?: BabbageTransactionOutputJSON | null;
    fee: number;
    inputs: TransactionInputJSON[];
    mint?: BabbageMintJSON | null;
    network_id?: number | null;
    outputs: BabbageTransactionOutputJSON[];
    reference_inputs?: TransactionInputJSON[] | null;
    required_signers?: string[] | null;
    script_data_hash?: string | null;
    total_collateral?: number | null;
    ttl?: number | null;
    update?: BabbageUpdateJSON | null;
    validity_interval_start?: number | null;
    withdrawals?: {
        [k: string]: number;
    } | null;
}
export type BabbageTransactionOutputJSON = {
    AlonzoFormatTxOut: AlonzoFormatTxOutJSON;
} | {
    BabbageFormatTxOut: BabbageFormatTxOutJSON;
};
export interface BabbageTransactionWitnessSetJSON {
    bootstrap_witnesses?: BootstrapWitnessJSON[] | null;
    native_scripts?: NativeScriptJSON[] | null;
    plutus_datums?: PlutusDataJSON[] | null;
    plutus_v1_scripts?: string[] | null;
    plutus_v2_scripts?: string[] | null;
    redeemers?: AlonzoRedeemerJSON[] | null;
    vkeywitnesses?: VkeywitnessJSON[] | null;
}
export interface BabbageUpdateJSON {
    epoch: number;
    updates: {
        [k: string]: BabbageProtocolParamUpdateJSON;
    };
}
/**
 * Babbage mints can have multiple maps resulting in different encodings so this works around it
 */
export interface BabbageMintJSON {
    assets: [
        unknown,
        unknown
    ][];
}
